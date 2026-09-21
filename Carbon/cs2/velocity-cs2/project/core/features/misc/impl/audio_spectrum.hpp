#pragma once

#include <pch/pch.hpp>

#include <core/settings.hpp>
#include <core/rendering/rendering.hpp>
#include <external/xdraw/xdraw.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ksmedia.h>
#pragma comment( lib, "ole32.lib" )

// WASAPI loopback capture -> FFT -> smoothed bands, rendered as a spectrum
// under the media player. Ported concept from Johnwikix/SpectrumVisualization.
namespace features::misc::spectrum
{
	inline constexpr int k_bands = 64;
	inline constexpr int k_fft = 1024;

	inline std::array<float, k_bands>& bands( ) { static std::array<float, k_bands> b{}; return b; }
	inline std::atomic<bool>& started( ) { static std::atomic<bool> s{ false }; return s; }

	// Millis of the last capture packet that actually yielded samples. 0 = never.
	// Lets callers show a "no audio signal" state instead of silent dead bars.
	inline std::atomic<long long>& last_packet_ms( ) { static std::atomic<long long> t{ 0 }; return t; }

	// Millis of the last packet that carried audible energy. Distinct from
	// last_packet_ms so the UI can tell "stream connected but silent" apart from
	// "capture never started".
	inline std::atomic<long long>& last_audio_ms( ) { static std::atomic<long long> t{ 0 }; return t; }

	enum class sample_format : std::uint8_t
	{
		ieee_float,
		pcm_i16,
		pcm_i32,
		pcm_i8,
		unknown
	};

	// Detect the real sample format instead of guessing from wBitsPerSample:
	// GetMixFormat commonly returns a WAVEFORMATEXTENSIBLE where 24-bit PCM is
	// packed in a 32-bit container (valid bits < container bits), and treating
	// that integer data as floats poisoned the whole band pipeline.
	inline sample_format detect_sample_format( const WAVEFORMATEX* wfx )
	{
		if ( !wfx )
		{
			return sample_format::unknown;
		}

		if ( wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT )
		{
			return sample_format::ieee_float;
		}

		if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
		{
			const auto* ext = reinterpret_cast< const WAVEFORMATEXTENSIBLE* >( wfx );
			if ( ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
			{
				return sample_format::ieee_float;
			}
			if ( ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
			{
				if ( ext->Samples.wValidBitsPerSample <= 8 ) return sample_format::pcm_i8;
				if ( ext->Samples.wValidBitsPerSample <= 16 ) return sample_format::pcm_i16;
				return sample_format::pcm_i32;
			}
			return sample_format::unknown;
		}

		if ( wfx->wBitsPerSample <= 8 ) return sample_format::pcm_i8;
		if ( wfx->wBitsPerSample <= 16 ) return sample_format::pcm_i16;
		return sample_format::pcm_i32;
	}

	inline int container_valid_bits( const WAVEFORMATEX* wfx )
	{
		if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
		{
			const auto* ext = reinterpret_cast< const WAVEFORMATEXTENSIBLE* >( wfx );
			const auto valid = static_cast< int >( ext->Samples.wValidBitsPerSample );
			if ( valid > 0 && valid <= 32 )
			{
				return valid;
			}
		}
		return wfx->wBitsPerSample;
	}

	inline void fft_radix2( std::vector<float>& re, std::vector<float>& im )
	{
		const int n = static_cast< int >( re.size( ) );
		for ( int i = 1, j = 0; i < n; ++i )
		{
			int bit = n >> 1;
			for ( ; j & bit; bit >>= 1 ) j ^= bit;
			j ^= bit;
			if ( i < j ) { std::swap( re[ i ], re[ j ] ); std::swap( im[ i ], im[ j ] ); }
		}

		for ( int len = 2; len <= n; len <<= 1 )
		{
			const float ang = -6.2831853f / static_cast< float >( len );
			const float wr = std::cosf( ang ), wi = std::sinf( ang );
			for ( int i = 0; i < n; i += len )
			{
				float cwr = 1.0f, cwi = 0.0f;
				for ( int k = 0; k < len / 2; ++k )
				{
					const float ur = re[ i + k ], ui = im[ i + k ];
					const float xr = re[ i + k + len / 2 ], xi = im[ i + k + len / 2 ];
					const float vr = xr * cwr - xi * cwi;
					const float vi = xr * cwi + xi * cwr;
					re[ i + k ] = ur + vr; im[ i + k ] = ui + vi;
					re[ i + k + len / 2 ] = ur - vr; im[ i + k + len / 2 ] = ui - vi;
					const float nwr = cwr * wr - cwi * wi, nwi = cwr * wi + cwi * wr;
					cwr = nwr; cwi = nwi;
				}
			}
		}
	}

	inline void capture_loop( )
	{
		CoInitializeEx( nullptr, COINIT_MULTITHREADED );

		IMMDeviceEnumerator* enumer = nullptr;
		if ( FAILED( CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL, IID_PPV_ARGS( &enumer ) ) ) || !enumer )
		{
			CoUninitialize( );
			return;
		}

		IMMDevice* dev = nullptr;
		if ( FAILED( enumer->GetDefaultAudioEndpoint( eRender, eConsole, &dev ) ) || !dev )
		{
			enumer->Release( ); CoUninitialize( ); return;
		}

		IAudioClient* client = nullptr;
		if ( FAILED( dev->Activate( __uuidof( IAudioClient ), CLSCTX_ALL, nullptr, reinterpret_cast< void** >( &client ) ) ) || !client )
		{
			dev->Release( ); enumer->Release( ); CoUninitialize( ); return;
		}

		WAVEFORMATEX* wfx = nullptr;
		if ( FAILED( client->GetMixFormat( &wfx ) ) || !wfx )
		{
			client->Release( ); dev->Release( ); enumer->Release( ); CoUninitialize( ); return;
		}

		if ( FAILED( client->Initialize( AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
			10000000, 0, wfx, nullptr ) ) )
		{
			CoTaskMemFree( wfx ); client->Release( ); dev->Release( ); enumer->Release( ); CoUninitialize( ); return;
		}

		IAudioCaptureClient* cap = nullptr;
		if ( FAILED( client->GetService( __uuidof( IAudioCaptureClient ), reinterpret_cast< void** >( &cap ) ) ) || !cap )
		{
			CoTaskMemFree( wfx ); client->Release( ); dev->Release( ); enumer->Release( ); CoUninitialize( ); return;
		}

		client->Start( );

		const auto fmt = detect_sample_format( wfx );
		if ( fmt == sample_format::unknown )
		{
			CoTaskMemFree( wfx ); cap->Release( ); client->Release( ); dev->Release( ); enumer->Release( ); CoUninitialize( ); return;
		}

		const int channels = std::max< int >( 1, wfx->nChannels );
		const int valid_bits = container_valid_bits( wfx );
		const int i32_shift = std::clamp( 32 - valid_bits, 0, 31 );
		const float i32_scale = ( valid_bits >= 2 )
			? 1.0f / static_cast< float >( std::int64_t{ 1 } << ( valid_bits - 1 ) )
			: 1.0f / 2147483648.0f;
		const float sample_rate = std::max( 1.0f, static_cast< float >( wfx->nSamplesPerSec ) );

		std::vector<float> samples;
		samples.reserve( k_fft * 4 );

		std::vector<float> re( k_fft ), im( k_fft );
		std::vector<float> win( k_fft );
		for ( int i = 0; i < k_fft; ++i )
		{
			win[ i ] = 0.5f * ( 1.0f - std::cosf( 6.2831853f * static_cast< float >( i ) / static_cast< float >( k_fft - 1 ) ) );
		}

		auto& band = bands( );

		while ( true )
		{
			Sleep( 10 );

			UINT32 packet = 0;
			cap->GetNextPacketSize( &packet );
			while ( packet != 0 )
			{
				BYTE* data = nullptr;
				UINT32 frames = 0;
				DWORD flags = 0;
				if ( FAILED( cap->GetBuffer( &data, &frames, &flags, nullptr, nullptr ) ) )
				{
					break;
				}

				const bool silent = ( flags & AUDCLNT_BUFFERFLAGS_SILENT ) != 0;

				if ( frames == 0 || !data )
				{
					cap->ReleaseBuffer( frames );
					cap->GetNextPacketSize( &packet );
					continue;
				}

				if ( silent )
				{
					// Keep the FFT timeline flowing with silence so bands decay
					// smoothly instead of the pipeline stalling forever.
					for ( UINT32 i = 0; i < frames; ++i )
					{
						samples.push_back( 0.0f );
					}
				}
				else
				{
					for ( UINT32 f = 0; f < frames; ++f )
					{
						float s = 0.0f;
						const auto* raw = data + static_cast< std::size_t >( f ) * wfx->nBlockAlign;

						switch ( fmt )
						{
						case sample_format::ieee_float:
						{
							const auto* p = reinterpret_cast< const float* >( raw );
							for ( int c = 0; c < channels; ++c )
							{
								const float v = p[ c ];
								s += std::isfinite( v ) ? v : 0.0f;
							}
							break;
						}
						case sample_format::pcm_i16:
						{
							const auto* p = reinterpret_cast< const std::int16_t* >( raw );
							for ( int c = 0; c < channels; ++c ) s += static_cast< float >( p[ c ] ) / 32768.0f;
							break;
						}
						case sample_format::pcm_i32:
						{
							const auto* p = reinterpret_cast< const std::int32_t* >( raw );
							for ( int c = 0; c < channels; ++c )
							{
								const auto v = p[ c ] >> i32_shift;
								s += static_cast< float >( v ) * i32_scale;
							}
							break;
						}
						case sample_format::pcm_i8:
						{
							const auto* p = reinterpret_cast< const std::int8_t* >( raw );
							for ( int c = 0; c < channels; ++c ) s += static_cast< float >( p[ c ] ) / 128.0f;
							break;
						}
						default:
							break;
						}

						samples.push_back( s / static_cast< float >( channels ) );
					}

					last_audio_ms( ).store( GetTickCount64( ) );
				}

				cap->ReleaseBuffer( frames );
				last_packet_ms( ).store( GetTickCount64( ) );
				cap->GetNextPacketSize( &packet );
			}

			if ( samples.size( ) < static_cast< std::size_t >( k_fft ) )
			{
				continue;
			}

			// Keep only the newest k_fft samples.
			const auto start = samples.size( ) - k_fft;
			for ( int i = 0; i < k_fft; ++i )
			{
				re[ i ] = samples[ start + i ] * win[ i ];
				im[ i ] = 0.0f;
			}
			samples.clear( );

			fft_radix2( re, im );

			const auto& cfg = settings::g_misc.m_spectrum;
			const float sens = std::clamp( cfg.sensitivity.value, 0.2f, 6.0f );

			const float bin_hz = sample_rate / static_cast< float >( k_fft );

			for ( int b = 0; b < k_bands; ++b )
			{
				// Log-spaced bins from 40 Hz to 16 kHz.
				const float lo = 40.0f * std::powf( 400.0f, static_cast< float >( b ) / static_cast< float >( k_bands ) );
				const float hi = 40.0f * std::powf( 400.0f, static_cast< float >( b + 1 ) / static_cast< float >( k_bands ) );

				int k0 = static_cast< int >( lo / bin_hz );
				int k1 = static_cast< int >( hi / bin_hz );
				k0 = std::clamp( k0, 1, k_fft / 2 - 1 );
				k1 = std::clamp( k1, k0 + 1, k_fft / 2 );

				float sum = 0.0f;
				for ( int k = k0; k < k1; ++k )
				{
					sum += std::sqrtf( re[ k ] * re[ k ] + im[ k ] * im[ k ] );
				}
				sum /= static_cast< float >( k1 - k0 );

				// Perceptual dB mapping. Linearly scaling the raw FFT magnitude
				// against k_fft left ordinary music around a couple percent of
				// the chart, which read as "the visualizer does nothing" even
				// while capture worked. A full-scale tone sits near -6 dBFS, so
				// a [-70, -12] sweep keeps quiet passages subtle and loud peaks
				// full-height.
				constexpr float k_db_floor = -70.0f;
				constexpr float k_db_ceil = -12.0f;

				const float mag = sum / static_cast< float >( k_fft );
				const float db = 20.0f * std::log10f( mag + 1.0e-9f );
				const float frac = std::clamp( ( db - k_db_floor ) / ( k_db_ceil - k_db_floor ), 0.0f, 1.0f );
				const float norm = std::clamp( std::powf( frac, 1.5f ) * sens, 0.0f, 1.0f );

				// Fast attack, slow decay.
				band[ b ] = norm > band[ b ] ? norm : band[ b ] * 0.82f + norm * 0.18f;
			}
		}

		// unreachable
	}

	inline void ensure_started( )
	{
		if ( !started( ).exchange( true ) )
		{
			// Persistent capture thread: capture_loop only returns on an init
			// failure (device busy, endpoint unavailable, CoCreateInstance race
			// at startup, ...), so keep retrying with a delay instead of going
			// dead for the session - a transient failure was leaving the
			// visualizer permanently absent.
			std::thread( []( )
				{
					for ( ;; )
					{
						capture_loop( );
						Sleep( 1500 );
					}
				} ).detach( );
		}
	}

	inline void draw( xdraw::draw_list& dl, float x, float y, float w, float h, const xdraw::color& accent, float sensitivity )
	{
		ensure_started( );

		const auto& band = bands( );
		const float sens = std::clamp( sensitivity, 0.2f, 6.0f );

		const float gap = 2.0f;
		const float bw = ( w - gap * ( k_bands - 1 ) ) / static_cast< float >( k_bands );
		if ( bw <= 0.0f ) return;

		for ( int i = 0; i < k_bands; ++i )
		{
			const float v = std::clamp( band[ i ], 0.0f, 1.0f );
			const float bh = std::max( 1.5f, v * h );
			const float bx = x + static_cast< float >( i ) * ( bw + gap );

			dl.rect_filled( bx, y + h - bh, bw, bh,
				accent.alpha( static_cast< std::uint8_t >( 90.0f + 165.0f * v ) ),
				xdraw::corner_radius{ bw * 0.5f } );
		}
	}
}
