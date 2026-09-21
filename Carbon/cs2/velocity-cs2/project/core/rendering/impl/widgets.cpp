#include <pch/pch.hpp>
#include <utilities/math/math.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/systems/systems.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <core/features/misc/impl/lyrics.hpp>
#include <core/features/misc/impl/audio_spectrum.hpp>
#include <thread>
#include <mutex>

#include <robuffer.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#pragma comment(lib, "windowsapp.lib")

#include "../rendering.hpp"
#include <utilities/security/security.hpp>
#include <updater/updater.hpp>

namespace rendering {

	void widgets::draw( )
	{
		update_fw_logo_animation( xdraw::delta_time( ) );

		auto& dl = xdraw::get( );

		if ( settings::g_misc.m_watermark.enabled.value )
		{
			this->watermark( dl );
		}

		this->keybinds( dl );
		this->media_player( dl );
		this->audio_visualizer_widget( dl );
		this->fw_logo_widget( dl );
	}

	void widgets::watermark( xdraw::draw_list& draw_list )
	{
		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto& s  = xui::ctx( ).style;
		const auto& wm = settings::g_misc.m_watermark;
		const auto framerate = xdraw::framerate( );
		const auto local = systems::g_local.get( );

		constexpr auto h{ 24.0f };
		constexpr auto margin{ 10.0f };
		constexpr auto r{ 10.0f };
		constexpr auto inner_r{ 7.0f };
		constexpr auto inner_pad{ 2.0f };
		constexpr auto text_pad_x{ 8.0f };
		constexpr auto text_nudge{ 0.5f };
		constexpr auto section_spacing{ 2.0f };

		// Ã¢â€â‚¬Ã¢â€â‚¬ time Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		SYSTEMTIME st{};
		GetLocalTime( &st );
		char time_buf[ 8 ]{};
		std::snprintf( time_buf, sizeof( time_buf ), "%02d:%02d", st.wHour, st.wMinute );

		// Ã¢â€â‚¬Ã¢â€â‚¬ fps Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		static auto smoothed_fps{ 0.0f };
		if ( smoothed_fps == 0.0f ) smoothed_fps = framerate;
		smoothed_fps += ( framerate - smoothed_fps ) * std::min( 2.0f * xdraw::delta_time( ), 1.0f );
		char fps_val[ 8 ]{};
		std::snprintf( fps_val, sizeof( fps_val ), "%.0f", smoothed_fps );

		// Ã¢â€â‚¬Ã¢â€â‚¬ ping Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		auto ping{ 0 };
		if ( local.is_alive && local.controller && systems::g_entities.exists( local.controller ) )
			ping = memory::read<std::uint32_t>( local.controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
		char ping_val[ 8 ]{};
		std::snprintf( ping_val, sizeof( ping_val ), "%d", ping );

		// Ã¢â€â‚¬Ã¢â€â‚¬ map name (stored reliably from level_initialization hook) Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const bool has_map = wm.show_map.value && !s_map_name.empty( );

		// Ã¢â€â‚¬Ã¢â€â‚¬ tick rate (measured from server_tick delta over ~2 s of real time) Ã¢â€â‚¬
		static auto last_server_tick{ 0 };
		static auto last_curtime{ 0.0f };
		static auto measured_tickrate{ 0 };

		if ( local.controller )
		{
			const auto net_for_tick = addresses::globals::network_client_service;
			const auto tick_state   = net_for_tick ? memory::call_vfunc<std::uintptr_t>( net_for_tick, 23 ) : 0;
			const auto server_tick  = tick_state   ? memory::read<int>( tick_state + 892 ) : 0;
			const auto gv           = memory::read<std::uintptr_t>( addresses::globals::global_vars );
			const auto curtime      = gv ? memory::read<float>( gv + 0x30 ) : 0.0f;

			if ( server_tick > 0 && last_server_tick > 0 && curtime - last_curtime >= 2.0f )
			{
				const auto tick_delta = server_tick - last_server_tick;
				const auto time_delta = curtime - last_curtime;
				if ( tick_delta > 0 && time_delta > 0.5f )
				{
					const auto rate = static_cast<int>( std::round( tick_delta / time_delta ) );
					if ( rate >= 16 && rate <= 256 ) measured_tickrate = rate;
				}
				last_server_tick = server_tick;
				last_curtime     = curtime;
			}
			else if ( last_server_tick == 0 && server_tick > 0 )
			{
				last_server_tick = server_tick;
				last_curtime     = curtime;
			}
		}
		else
		{
			last_server_tick = 0;
			last_curtime     = 0.0f;
			measured_tickrate = 0;
		}

		const bool has_tick = wm.show_tick.value && local.controller && measured_tickrate > 0;
		char tick_val[ 8 ]{};
		if ( has_tick ) std::snprintf( tick_val, sizeof( tick_val ), "%d", measured_tickrate );

		// Ã¢â€â‚¬Ã¢â€â‚¬ velocity Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const bool has_velocity = wm.show_velocity.value && local.is_alive && local.pawn;
		static auto smoothed_velocity{ 0.0f };
		char vel_val[ 8 ]{};
		if ( has_velocity )
		{
			const auto velocity = memory::read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto speed = velocity.length_2d( );
			smoothed_velocity += ( speed - smoothed_velocity ) * std::min( 8.0f * xdraw::delta_time( ), 1.0f );
			std::snprintf( vel_val, sizeof( vel_val ), "%.0f", smoothed_velocity );
		}
		else
		{
			smoothed_velocity = 0.0f;
		}


		// Ã¢â€â‚¬Ã¢â€â‚¬ logo Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const auto inner_h     = h - inner_pad * 2.0f;

		// Ã¢â€â‚¬Ã¢â€â‚¬ measure text Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const auto [name_tw, name_th] = xdraw::measure_text( "femware" );
		const auto [user_tw, user_th] = xdraw::measure_text( "developer" );
		const auto [ping_vw, ping_vh] = xdraw::measure_text( ping_val );
		const auto [ping_uw, ping_uh] = xdraw::measure_text( " ms" );
		const auto [fps_vw,  fps_vh]  = xdraw::measure_text( fps_val );
		const auto [fps_uw,  fps_uh]  = xdraw::measure_text( " fps" );
		const auto [time_tw, time_th] = xdraw::measure_text( time_buf );

		float map_tw{}, map_th{};
		if ( has_map ) std::tie( map_tw, map_th ) = xdraw::measure_text( s_map_name.c_str( ) );

		float tick_vw{}, tick_vh{}, tick_uw{}, tick_uh{};
		if ( has_tick )
		{
			std::tie( tick_vw, tick_vh ) = xdraw::measure_text( tick_val );
			std::tie( tick_uw, tick_uh ) = xdraw::measure_text( " tick" );
		}

		float vel_vw{}, vel_vh{}, vel_uw{}, vel_uh{};
		if ( has_velocity )
		{
			std::tie( vel_vw, vel_vh ) = xdraw::measure_text( vel_val );
			std::tie( vel_uw, vel_uh ) = xdraw::measure_text( " u/s" );
		}

		// Ã¢â€â‚¬Ã¢â€â‚¬ build number Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const bool has_build = wm.show_build.value && carbon::offsets::dwBuildNumber > 0;
		char build_val[ 16 ]{};
		if ( has_build ) std::snprintf( build_val, sizeof( build_val ), "%u", memory::read<std::uint32_t>( carbon::offsets::dwBuildNumber ) );

		float build_vw{}, build_vh{}, build_uw{}, build_uh{};
		if ( has_build )
		{
			std::tie( build_vw, build_vh ) = xdraw::measure_text( build_val );
			std::tie( build_uw, build_uh ) = xdraw::measure_text( " build" );
		}


		// Ã¢â€â‚¬Ã¢â€â‚¬ pill widths Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const auto logo_pill_w = name_tw + text_pad_x * 2.0f;
		const auto user_pill_w = user_tw + text_pad_x * 2.0f;
		const auto ping_pill_w = ping_vw + ping_uw + text_pad_x * 2.0f;
		const auto fps_pill_w  = fps_vw  + fps_uw  + text_pad_x * 2.0f;
		const auto time_pill_w = time_tw + text_pad_x * 2.0f;
		const auto map_pill_w  = map_tw  + text_pad_x * 2.0f;
		const auto tick_pill_w = tick_vw + tick_uw + text_pad_x * 2.0f;
		const auto vel_pill_w  = vel_vw + vel_uw + text_pad_x * 2.0f;
		const auto build_pill_w = build_vw + build_uw + text_pad_x * 2.0f;

		// Ã¢â€â‚¬Ã¢â€â‚¬ dynamic total width Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		float target_w = inner_pad + logo_pill_w + section_spacing;
		if ( wm.show_user.value ) target_w += user_pill_w + section_spacing;
		if ( has_map )            target_w += map_pill_w  + section_spacing;
		if ( wm.show_ping.value ) target_w += ping_pill_w + section_spacing;
		if ( has_velocity )       target_w += vel_pill_w  + section_spacing;
		if ( wm.show_fps.value )  target_w += fps_pill_w  + section_spacing;
		if ( has_tick )           target_w += tick_pill_w + section_spacing;
		if ( has_build )          target_w += build_pill_w + section_spacing;
		if ( wm.show_time.value ) target_w += time_pill_w + section_spacing;
		target_w = target_w - section_spacing + inner_pad;

		static auto smoothed_w{ 0.0f };
		if ( smoothed_w == 0.0f ) smoothed_w = target_w;
		smoothed_w += ( target_w - smoothed_w ) * std::min( 8.0f * xdraw::delta_time( ), 1.0f );

		const auto w = smoothed_w;
		const auto x = static_cast<float>( screen_w ) - w - margin;
		const auto y = margin;

		draw_list.rect_filled_blurred( x, y, w, h, xdraw::corner_radius{ r } );
		draw_list.rect_filled( x, y, w, h, s.window_bg, xdraw::corner_radius{ r } );

		auto cx = x + inner_pad;

		auto draw_split_pill = [ & ]( const char* value, float vw, float vh, const char* unit, float uw, float uh, float pill_w )
			{
				draw_list.rect_filled( cx, y + inner_pad, pill_w, inner_h, s.child_bg, xdraw::corner_radius{ inner_r } );
				draw_list.rect( cx, y + inner_pad, pill_w, inner_h, xdraw::color{ 255, 255, 255, 16 }, xdraw::corner_radius{ inner_r }, 1.0f );
				draw_list.text( cx + text_pad_x, y + ( h - vh ) * 0.5f + text_nudge, value, s.accent );
				draw_list.text( cx + text_pad_x + vw, y + ( h - uh ) * 0.5f + text_nudge, unit, s.text_dim );
				cx += pill_w + section_spacing;
			};

		auto draw_pill = [ & ]( const char* text, float tw, float th, float pill_w )
			{
				draw_list.rect_filled( cx, y + inner_pad, pill_w, inner_h, s.child_bg, xdraw::corner_radius{ inner_r } );
				draw_list.rect( cx, y + inner_pad, pill_w, inner_h, xdraw::color{ 255, 255, 255, 16 }, xdraw::corner_radius{ inner_r }, 1.0f );
				draw_list.text( cx + text_pad_x, y + ( h - th ) * 0.5f + text_nudge, text, s.accent );
				cx += pill_w + section_spacing;
			};

		// brand pill (text only Ã¢â‚¬â€ one logo max, in the menu title)
		{
			const auto brand_top = s.accent;
			const auto brand_bot = xdraw::color
			{
				static_cast< std::uint8_t >( s.accent.r * 0.72f ),
				static_cast< std::uint8_t >( s.accent.g * 0.72f ),
				static_cast< std::uint8_t >( s.accent.b * 0.72f ),
				s.accent.a
			};
			draw_list.rect_filled_gradient( cx, y + inner_pad, logo_pill_w, inner_h,
				brand_top, brand_top, brand_bot, brand_bot, xdraw::corner_radius{ inner_r }, true );
			draw_list.rect( cx, y + inner_pad, logo_pill_w, inner_h, xdraw::color{ 255, 255, 255, 40 }, xdraw::corner_radius{ inner_r }, 1.0f );
			draw_list.text( cx + text_pad_x,
				y + ( h - name_th ) * 0.5f + text_nudge, "femware", xdraw::color{ 255, 255, 255, 255 } );
		}
		cx += logo_pill_w + section_spacing;

		if ( wm.show_user.value ) draw_pill( "developer", user_tw, user_th, user_pill_w );
		if ( has_map )            draw_pill( s_map_name.c_str( ), map_tw, map_th, map_pill_w );
		if ( wm.show_ping.value ) draw_split_pill( ping_val, ping_vw, ping_vh, " ms",   ping_uw, ping_uh, ping_pill_w );
		if ( has_velocity )       draw_split_pill( vel_val,  vel_vw,  vel_vh,  " u/s",  vel_uw,  vel_uh,  vel_pill_w );
		if ( wm.show_fps.value )  draw_split_pill( fps_val,  fps_vw,  fps_vh,  " fps",  fps_uw,  fps_uh,  fps_pill_w );
		if ( has_tick )           draw_split_pill( tick_val, tick_vw, tick_vh, " tick", tick_uw, tick_uh, tick_pill_w );
		if ( has_build )          draw_split_pill( build_val, build_vw, build_vh, " build", build_uw, build_uh, build_pill_w );
		if ( wm.show_time.value ) draw_pill( time_buf, time_tw, time_th, time_pill_w );
	}

	void widgets::keybinds( xdraw::draw_list& draw_list )
	{
		if ( !settings::g_misc.m_keybind_list.enabled.value )
		{
			return;
		}

		static animation::fade container_alpha;

		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		constexpr auto row_h{ 18.0f };
		constexpr auto header_h{ 22.0f };

		struct bind_entry
		{
			const char* name;
			char value[ 32 ];
			bool has_value_pill;
			xui::bind_mode mode;
		};

		bind_entry entries[ 32 ]{};
		auto count{ 0 };

		const auto& ctx = features::combat::g_shared.ctx( );
		const auto has_weapon = ctx.valid && ctx.weapon_type >= cstypes::weapon_type::pistol && ctx.weapon_type <= cstypes::weapon_type::lmg;

		for ( const auto setting : xui::binds::all( ) )
		{
			if ( !setting || setting->bind.key == 0 || !setting->bind.active || count >= 32 )
			{
				continue;
			}

			auto is_rage_group{ false };
			for ( auto i = 0u; i < settings::combat::ragebot::k_group_count; ++i )
			{
				const auto& g = settings::g_combat.m_ragebot.groups[ i ];
				if ( setting == &g.min_damage_override || setting == &g.hitchance_override || setting == &g.force_shot || setting == &g.force_shot_air || setting == &g.body_aim || setting == &g.silent || setting == &g.no_spread )
				{
					is_rage_group = true;
					break;
				}
			}

			if ( is_rage_group )
			{
				if ( !settings::g_combat.m_ragebot.enabled || !has_weapon )
				{
					continue;
				}

				const auto active_group = &settings::g_combat.m_ragebot.get_group( ctx.weapon_type );
				auto is_active{ false };

				for ( auto i = 0u; i < settings::combat::ragebot::k_group_count; ++i )
				{
					const auto& g = settings::g_combat.m_ragebot.groups[ i ];
					if ( &g == active_group )
					{
						if ( setting == &g.min_damage_override || setting == &g.hitchance_override || setting == &g.force_shot || setting == &g.force_shot_air || setting == &g.body_aim )
						{
							is_active = true;
						}
						break;
					}
				}

				if ( !is_active )
				{
					continue;
				}

				auto& e = entries[ count++ ];
				e.name = setting->name.c_str( );
				e.mode = setting->bind.mode;

				if ( setting == &active_group->min_damage_override )
				{
					std::snprintf( e.value, sizeof( e.value ), "%d", active_group->min_damage_override_value.value );
					e.has_value_pill = true;
				}
				else if ( setting == &active_group->hitchance_override )
				{
					std::snprintf( e.value, sizeof( e.value ), "%d%%", active_group->hitchance_override_value.value );
					e.has_value_pill = true;
				}
				else
				{
					e.value[ 0 ] = '\0';
					e.has_value_pill = false;
				}
				continue;
			}

			auto is_legit_group{ false };
			for ( auto i = 0u; i < settings::combat::legitbot::k_group_count; ++i )
			{
				const auto& g = settings::g_combat.m_legitbot.groups[ i ];
				if ( setting == &g.aimbot || setting == &g.rcs || setting == &g.standalone_rcs || setting == &g.triggerbot || setting == &g.autowall || setting == &g.visualize_fov || setting == &g.trigger_head_only || setting == &g.give_me_your_seed )
				{
					is_legit_group = true;
					break;
				}
			}

			if ( is_legit_group )
			{
				if ( !settings::g_combat.m_legitbot.enabled.value || !has_weapon )
				{
					continue;
				}

				const auto* active_group = &settings::g_combat.m_legitbot.get_group( ctx.weapon_type );
				auto is_active{ false };

				for ( auto i = 0u; i < settings::combat::legitbot::k_group_count; ++i )
				{
					if ( &settings::g_combat.m_legitbot.groups[ i ] == active_group )
					{
						const auto& g = settings::g_combat.m_legitbot.groups[ i ];
						if ( setting == &g.aimbot || setting == &g.rcs || setting == &g.standalone_rcs || setting == &g.triggerbot || setting == &g.autowall || setting == &g.visualize_fov || setting == &g.trigger_head_only || setting == &g.give_me_your_seed )
						{
							is_active = true;
						}

						if ( is_active && setting == &active_group->give_me_your_seed && !active_group->triggerbot.value )
						{
							is_active = false;
						}
						break;
					}
				}

				if ( !is_active )
				{
					continue;
				}

				auto& e = entries[ count++ ];
				e.name = setting->name.c_str( );
				e.mode = setting->bind.mode;
				e.value[ 0 ] = '\0';
				e.has_value_pill = false;
				continue;
			}

			if ( setting == &settings::g_combat.m_antiaim.enabled || setting == &settings::g_combat.m_antiaim.manual_left || setting == &settings::g_combat.m_antiaim.manual_right || setting == &settings::g_combat.m_antiaim.hide_shots || setting == &settings::g_combat.m_antiaim.avoid_backstab || setting == &settings::g_combat.m_antiaim.direction_indicator )
			{
				if ( !settings::g_combat.m_antiaim.enabled.value )
				{
					continue;
				}
			}

			auto& e = entries[ count++ ];
			e.name = setting->name.c_str( );
			e.mode = setting->bind.mode;
			e.value[ 0 ] = '\0';
			e.has_value_pill = false;
		}

		static float s_kb_x = -1.0f;
		static float s_kb_y = -1.0f;
		static bool s_kb_dragging = false;
		static float s_kb_drag_offset_x = 0.0f;
		static float s_kb_drag_offset_y = 0.0f;

		if ( s_kb_x < 0.0f )
		{
			s_kb_x = 20.0f;
			s_kb_y = 70.0f;
		}

		const bool menu_open = g_menu.is_open( );

		if ( count > 0 || menu_open )
			container_alpha.fade_in( 0.2f );
		else
			container_alpha.fade_out( 0.2f );

		container_alpha.update( );
		if ( !container_alpha.visible( ) )
			return;

		constexpr float hud_w = 185.0f;
		constexpr float pad_x = 8.0f;

		const auto active_rows = ( count > 0 ) ? count : ( menu_open ? 1 : 0 );
		const auto content_h = static_cast< float >( active_rows ) * row_h + 6.0f;
		const auto total_h = header_h + content_h;

		// Mouse dragging when menu is open
		if ( menu_open )
		{
			const auto& inp = xui::ctx( ).input;
			const xui::rect header_rect{ s_kb_x, s_kb_y, hud_w, header_h };

			if ( inp.mouse_clicked && inp.in_rect( header_rect ) )
			{
				s_kb_dragging = true;
				s_kb_drag_offset_x = inp.mouse_x - s_kb_x;
				s_kb_drag_offset_y = inp.mouse_y - s_kb_y;
			}
			else if ( !inp.mouse_down )
			{
				s_kb_dragging = false;
			}

			if ( s_kb_dragging )
			{
				s_kb_x = std::clamp( inp.mouse_x - s_kb_drag_offset_x, 0.0f, static_cast< float >( screen_w ) - hud_w );
				s_kb_y = std::clamp( inp.mouse_y - s_kb_drag_offset_y, 0.0f, static_cast< float >( screen_h ) - total_h );
			}
		}
		else
		{
			s_kb_dragging = false;
		}

		const auto master_alpha = container_alpha.alpha( );
		const auto master_u8 = static_cast< std::uint8_t >( 255.0f * master_alpha );

		// LinoriaLib authentic frame styling:
		// Outer border #0e0e0e
		draw_list.rect_filled( s_kb_x, s_kb_y, hud_w, total_h, xdraw::color{ 14, 14, 14, master_u8 } );
		// Inner border #2c2c2c
		draw_list.rect( s_kb_x + 1.0f, s_kb_y + 1.0f, hud_w - 2.0f, total_h - 2.0f, xdraw::color{ 44, 44, 44, master_u8 }, 1.0f );
		// Background fill #171717
		draw_list.rect_filled( s_kb_x + 2.0f, s_kb_y + 2.0f, hud_w - 4.0f, total_h - 4.0f, xdraw::color{ 23, 23, 23, master_u8 } );
		// Top accent line (2px)
		draw_list.rect_filled( s_kb_x + 2.0f, s_kb_y + 2.0f, hud_w - 4.0f, 2.0f, tokens::col_accent.alpha( master_u8 ) );

		// Header title: "Keybinds"
		const auto [htw, hth] = xdraw::measure_text( "Keybinds" );
		draw_list.text( s_kb_x + pad_x, s_kb_y + ( header_h - hth ) * 0.5f + 1.0f, "Keybinds", tokens::col_text.alpha( master_u8 ) );

		// Header separator line
		draw_list.line( s_kb_x + 2.0f, s_kb_y + header_h, s_kb_x + hud_w - 2.0f, s_kb_y + header_h, xdraw::color{ 36, 36, 36, master_u8 }, 1.0f );

		// Entries
		float curr_y = s_kb_y + header_h + 3.0f;

		if ( count == 0 && menu_open )
		{
			const auto [pw, ph] = xdraw::measure_text( "[no active binds]" );
			draw_list.text( s_kb_x + pad_x, curr_y + ( row_h - ph ) * 0.5f, "[no active binds]", tokens::col_text_dim.alpha( master_u8 ) );
		}
		else
		{
			for ( auto i = 0; i < count; ++i )
			{
				const auto& e = entries[ i ];
				const auto [nw, nh] = xdraw::measure_text( e.name );
				draw_list.text( s_kb_x + pad_x, curr_y + ( row_h - nh ) * 0.5f, e.name, tokens::col_text.alpha( master_u8 ) );

				// Mode tag on right
				char mode_buf[ 32 ];
				if ( e.has_value_pill )
				{
					std::snprintf( mode_buf, sizeof( mode_buf ), "[%s]", e.value );
				}
				else
				{
					switch ( e.mode )
					{
					case xui::bind_mode::hold_on:
					case xui::bind_mode::hold_off:
						std::snprintf( mode_buf, sizeof( mode_buf ), "[hold]" );
						break;
					case xui::bind_mode::toggle:
						std::snprintf( mode_buf, sizeof( mode_buf ), "[toggle]" );
						break;
					default:
						std::snprintf( mode_buf, sizeof( mode_buf ), "[on]" );
						break;
					}
				}

				const auto [mw, mh] = xdraw::measure_text( mode_buf );
				draw_list.text( s_kb_x + hud_w - pad_x - mw, curr_y + ( row_h - mh ) * 0.5f, mode_buf, tokens::col_accent.alpha( master_u8 ) );

				curr_y += row_h;
			}
		}
	}

	void widgets::media_player( xdraw::draw_list& draw_list )
	{
		auto& mp_cfg = settings::g_misc.m_media_player;
		if ( !mp_cfg.enabled.value )
		{
			return;
		}

		struct media_scan_result
		{
			char title[ 128 ]{ "No media playing" };
			char artist[ 128 ]{ "Spotify / System" };
			bool is_playing{ false };
			bool is_spotify{ false };
			float position{ 0.0f };
			float duration{ 0.0f };
		};

		static media_scan_result s_scanned_media{};
		static std::vector<std::byte> s_scanned_art{};
		static std::mutex s_art_mutex{};
		static std::atomic<std::uint32_t> s_scan_rev{ 1 };
		static std::atomic<std::uint32_t> s_art_rev{ 0 };
		static std::atomic<bool> s_scanner_started{ false };
		static std::atomic<bool> s_scanner_running{ true };

		// Detached background worker thread queries universal Windows GSMTC sessions and falls back to window enumeration
		if ( !s_scanner_started.exchange( true ) )
		{
			std::thread( []( )
			{
				try
				{
					winrt::init_apartment( winrt::apartment_type::multi_threaded );
				}
				catch ( ... ) {}

				while ( s_scanner_running.load( ) )
				{
					media_scan_result updated{};
					std::vector<std::byte> art_bytes{};
					bool detected = false;

					// Priority 1: Windows 10/11 GSMTC (Spotify Desktop, Web Spotify, Chrome, Edge, YouTube, Apple Music)
					try
					{
						auto manager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
						if ( manager )
						{
							auto session = manager.GetCurrentSession();
							if ( !session )
							{
								auto sessions = manager.GetSessions();
								for ( uint32_t i = 0; i < sessions.Size(); ++i )
								{
									auto s = sessions.GetAt( i );
									auto pb = s.GetPlaybackInfo();
									if ( pb && pb.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing )
									{
										session = s;
										break;
									}
								}
								if ( !session && sessions.Size() > 0 )
								{
									session = sessions.GetAt( 0 );
								}
							}

							if ( session )
							{
								auto pb = session.GetPlaybackInfo();
								const bool is_playing = pb && ( pb.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing );

								auto props = session.TryGetMediaPropertiesAsync().get();
								if ( props )
								{
									std::string title = winrt::to_string( props.Title() );
									std::string artist = winrt::to_string( props.Artist() );
									if ( artist.empty() )
									{
										artist = winrt::to_string( props.AlbumArtist() );
									}

									auto app_id = winrt::to_string( session.SourceAppUserModelId() );
									bool is_spotify = ( app_id.find( "Spotify" ) != std::string::npos || app_id.find( "spotify" ) != std::string::npos );

									if ( !title.empty() )
									{
										detected = true;
										updated.is_playing = is_playing;
										updated.is_spotify = is_spotify;
										strncpy_s( updated.title, sizeof( updated.title ), title.c_str(), _TRUNCATE );
										if ( !artist.empty() )
										{
											strncpy_s( updated.artist, sizeof( updated.artist ), artist.c_str(), _TRUNCATE );
										}
										else
										{
											strncpy_s( updated.artist, sizeof( updated.artist ), is_spotify ? "Spotify" : "Media Player", _TRUNCATE );
										}

										try
										{
											auto tl = session.GetTimelineProperties();
											if ( tl )
											{
												const float pos_t = static_cast< float >( tl.Position().count() ) / 10000000.0f;
												const float span_t = static_cast< float >( tl.EndTime().count() - tl.StartTime().count() ) / 10000000.0f;
												float live_pos = pos_t;
												if ( is_playing )
												{
													const auto updated_time = tl.LastUpdatedTime();
													const auto now = winrt::clock::now();
													if ( now > updated_time )
													{
														const float elapsed = std::chrono::duration<float>( now - updated_time ).count();
														if ( elapsed >= 0.0f && elapsed < 30.0f )
														{
															live_pos += elapsed;
														}
													}
												}
												updated.position = live_pos < 0.0f ? 0.0f : live_pos;
												updated.duration = span_t > 0.0f ? span_t : 0.0f;
											}
										}
										catch ( ... ) {}

										// Extract raw album cover art
										auto thumb = props.Thumbnail();
										if ( thumb )
										{
											try
											{
												auto stream = thumb.OpenReadAsync().get();
												const auto sz = static_cast<uint32_t>( stream.Size() );
												if ( sz > 0 && sz < 10 * 1024 * 1024 )
												{
													winrt::Windows::Storage::Streams::Buffer buf( sz );
													stream.ReadAsync( buf, sz, winrt::Windows::Storage::Streams::InputStreamOptions::None ).get();
													auto byte_access = buf.as<::Windows::Storage::Streams::IBufferByteAccess>();
													byte* raw_bytes = nullptr;
													if ( SUCCEEDED( byte_access->Buffer( &raw_bytes ) ) && raw_bytes )
													{
														art_bytes.assign( reinterpret_cast<const std::byte*>( raw_bytes ), reinterpret_cast<const std::byte*>( raw_bytes ) + buf.Length() );
													}
												}
											}
											catch ( ... ) {}
										}
									}
								}
							}
						}
					}
					catch ( ... ) {}

					// Priority 2: Fallback to EnumWindows if GSMTC found no media
					if ( !detected )
					{
						struct enum_ctx
						{
							std::string title{};
							bool is_spotify{};
							bool is_playing{};
						} scan_res{};

						EnumWindows( []( HWND hwnd, LPARAM lp ) -> BOOL
						{
							if ( !IsWindowVisible( hwnd ) )
								return TRUE;

							wchar_t class_buf[ 128 ]{};
							GetClassNameW( hwnd, class_buf, 128 );

							wchar_t text_buf[ 512 ]{};
							GetWindowTextW( hwnd, text_buf, 512 );
							if ( text_buf[ 0 ] == L'\0' )
								return TRUE;

							char utf8[ 512 ]{};
							WideCharToMultiByte( CP_UTF8, 0, text_buf, -1, utf8, sizeof( utf8 ), nullptr, nullptr );
							std::string s( utf8 );

							auto* out = reinterpret_cast< enum_ctx* >( lp );

							// Spotify Desktop Client
							if ( wcscmp( class_buf, L"Chrome_WidgetWin_0" ) == 0 )
							{
								if ( s.find( " - " ) != std::string::npos && s.find( "Chrome" ) == std::string::npos )
								{
									out->title = s;
									out->is_spotify = true;
									out->is_playing = true;
									return FALSE;
								}
								else if ( s == "Spotify" || s == "Spotify Free" || s == "Spotify Premium" )
								{
									if ( out->title.empty( ) )
									{
										out->title = s;
										out->is_spotify = true;
										out->is_playing = false;
									}
								}
							}

							// Web Browsers (YouTube / SoundCloud / Web Spotify)
							if ( out->title.empty( ) )
							{
								if ( s.find( "- YouTube" ) != std::string::npos || s.find( "SoundCloud" ) != std::string::npos || s.find( "Spotify -" ) != std::string::npos )
								{
									out->title = s;
									out->is_spotify = false;
									out->is_playing = true;
								}
							}

							return TRUE;
						}, reinterpret_cast< LPARAM >( &scan_res ) );

						if ( !scan_res.title.empty( ) )
						{
							detected = true;
							if ( scan_res.is_spotify )
							{
								updated.is_spotify = true;
								updated.is_playing = scan_res.is_playing;

								if ( scan_res.is_playing )
								{
									const auto sep = scan_res.title.find( " - " );
									if ( sep != std::string::npos )
									{
										const auto art = scan_res.title.substr( 0, sep );
										const auto tit = scan_res.title.substr( sep + 3 );
										strncpy_s( updated.artist, sizeof( updated.artist ), art.c_str( ), _TRUNCATE );
										strncpy_s( updated.title, sizeof( updated.title ), tit.c_str( ), _TRUNCATE );
									}
									else
									{
										strncpy_s( updated.title, sizeof( updated.title ), scan_res.title.c_str( ), _TRUNCATE );
										strncpy_s( updated.artist, sizeof( updated.artist ), "Spotify", _TRUNCATE );
									}
								}
								else
								{
									strncpy_s( updated.title, sizeof( updated.title ), "Paused", _TRUNCATE );
									strncpy_s( updated.artist, sizeof( updated.artist ), "Spotify", _TRUNCATE );
								}
							}
							else
							{
								updated.is_spotify = false;
								updated.is_playing = scan_res.is_playing;

								auto raw = scan_res.title;
								const auto yt_pos = raw.find( " - YouTube" );
								if ( yt_pos != std::string::npos )
								{
									raw = raw.substr( 0, yt_pos );
									const auto sep = raw.find( " - " );
									if ( sep != std::string::npos )
									{
										const auto art = raw.substr( 0, sep );
										const auto tit = raw.substr( sep + 3 );
										strncpy_s( updated.artist, sizeof( updated.artist ), art.c_str( ), _TRUNCATE );
										strncpy_s( updated.title, sizeof( updated.title ), tit.c_str( ), _TRUNCATE );
									}
									else
									{
										strncpy_s( updated.title, sizeof( updated.title ), raw.c_str( ), _TRUNCATE );
										strncpy_s( updated.artist, sizeof( updated.artist ), "YouTube", _TRUNCATE );
									}
								}
								else
								{
									strncpy_s( updated.title, sizeof( updated.title ), raw.c_str( ), _TRUNCATE );
									strncpy_s( updated.artist, sizeof( updated.artist ), "Media", _TRUNCATE );
								}
							}
						}
					}

					// Update lock-free shared packet and increment revision
			if ( strcmp( updated.title, s_scanned_media.title ) != 0 ||
				strcmp( updated.artist, s_scanned_media.artist ) != 0 ||
				updated.is_playing != s_scanned_media.is_playing )
			{
				s_scanned_media = updated;
				s_scan_rev.fetch_add( 1, std::memory_order_release );
			}

			// Position/duration change every second, so they can't ride the
			// title-change swap above Ã¢â‚¬â€ publish them every scan.
			s_scanned_media.position = updated.position;
			s_scanned_media.duration = updated.duration;
			s_scanned_media.is_playing = updated.is_playing;

					// Update album art if changed
					bool art_changed = false;
					{
						std::lock_guard lock( s_art_mutex );
						if ( s_scanned_art != art_bytes )
						{
							s_scanned_art = std::move( art_bytes );
							art_changed = true;
						}
					}
					if ( art_changed )
					{
						s_art_rev.fetch_add( 1, std::memory_order_release );
					}

					std::this_thread::sleep_for( std::chrono::milliseconds( 400 ) );
				}
			} ).detach( );
		}

		struct media_state
		{
			std::string title{ "No media playing" };
			std::string artist{ "Spotify / System" };
			bool is_playing{ false };
			bool is_spotify{ false };
			float track_time{ 0.0f };
			float track_start_time{ 0.0f };
			float estimated_duration{ 180.0f };
			bool prev_play_key_down{ false };
			bool prev_next_key_down{ false };
			bool prev_prev_key_down{ false };
		};
		static media_state state;
		static animation::fade container_alpha;

		const float cur_time_sec = static_cast< float >( GetTickCount64( ) ) * 0.001f;

		// Lock-free check: only update strings if revision changed
		static std::uint32_t s_local_rev = 0;
		const auto cur_rev = s_scan_rev.load( std::memory_order_acquire );
		if ( cur_rev != s_local_rev )
		{
			s_local_rev = cur_rev;
			if ( state.title != s_scanned_media.title )
			{
				state.track_time = 0.0f;
				state.track_start_time = cur_time_sec;
			}
			state.title = s_scanned_media.title;
			state.artist = s_scanned_media.artist;
			state.is_playing = s_scanned_media.is_playing;
			state.is_spotify = s_scanned_media.is_spotify;
		}

		// Update album art texture on render thread when art revision changes
		static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> s_album_art_srv{ nullptr };
		static std::uint32_t s_local_art_rev = 0xFFFFFFFF;

		const auto cur_art_rev = s_art_rev.load( std::memory_order_acquire );
		if ( cur_art_rev != s_local_art_rev )
		{
			s_local_art_rev = cur_art_rev;
			std::vector<std::byte> art_copy;
			{
				std::lock_guard lock( s_art_mutex );
				art_copy = s_scanned_art;
			}
			if ( !art_copy.empty( ) )
			{
				s_album_art_srv = xdraw::load_texture( std::span<const std::byte>{ art_copy.data( ), art_copy.size( ) } );
			}
			else
			{
				s_album_art_srv.Reset( );
			}
		}

		const auto dispatch_media_key = [ ]( BYTE vk )
		{
			using keybd_event_t = void( WINAPI* )( BYTE, BYTE, DWORD, ULONG_PTR );
			static const auto p_keybd_event = reinterpret_cast< keybd_event_t >( GetProcAddress( GetModuleHandleA( "user32.dll" ), "keybd_event" ) );
			if ( p_keybd_event )
			{
				p_keybd_event( vk, 0, 0, 0 );
				p_keybd_event( vk, 0, 2 /* KEYEVENTF_KEYUP */, 0 );
			}
		};

		constexpr BYTE k_vk_play_pause = 0xB3;
		constexpr BYTE k_vk_next_track = 0xB0;
		constexpr BYTE k_vk_prev_track = 0xB1;

		const bool menu_open = g_menu.is_open( );

		// Ã¢â€â‚¬Ã¢â€â‚¬ Hotkey Processing Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		if ( !menu_open )
		{
			if ( mp_cfg.key_play_pause.value > 0 )
			{
				const bool down = ( GetAsyncKeyState( mp_cfg.key_play_pause.value ) & 0x8000 ) != 0;
				if ( down && !state.prev_play_key_down )
				{
					dispatch_media_key( k_vk_play_pause );
				}
				state.prev_play_key_down = down;
			}

			if ( mp_cfg.key_next.value > 0 )
			{
				const bool down = ( GetAsyncKeyState( mp_cfg.key_next.value ) & 0x8000 ) != 0;
				if ( down && !state.prev_next_key_down )
				{
					dispatch_media_key( k_vk_next_track );
				}
				state.prev_next_key_down = down;
			}

			if ( mp_cfg.key_prev.value > 0 )
			{
				const bool down = ( GetAsyncKeyState( mp_cfg.key_prev.value ) & 0x8000 ) != 0;
				if ( down && !state.prev_prev_key_down )
				{
					dispatch_media_key( k_vk_prev_track );
				}
				state.prev_prev_key_down = down;
			}
		}

		// Ã¢â€â‚¬Ã¢â€â‚¬ Progress: use the real media position, resynced from the scanner Ã¢â€â‚¬Ã¢â€â‚¬
		static auto last_frame_tick = GetTickCount64( );
		const auto frame_now = GetTickCount64( );
		const float dt = static_cast< float >( frame_now - last_frame_tick ) * 0.001f;
		last_frame_tick = frame_now;

		const auto scanned_pos = s_scanned_media.position;
		const auto scanned_dur = s_scanned_media.duration;
		if ( scanned_dur > 1.0f )
		{
			state.estimated_duration = scanned_dur;
		}

		if ( state.is_playing )
		{
			if ( scanned_pos > 0.01f )
			{
				const auto delta = scanned_pos - state.track_time;
				// Snap immediately on large user seek (>2.0s forward, or >4.0s backward)
				if ( delta > 2.0f || delta < -4.0f )
				{
					state.track_time = scanned_pos;
				}
				else
				{
					// Smooth drift convergence without jitter or snapping back
					state.track_time += dt + delta * 0.05f;
				}
			}
			else
			{
				state.track_time += dt;
			}

			if ( state.estimated_duration > 1.0f && state.track_time > state.estimated_duration )
			{
				state.track_time = state.estimated_duration;
			}
		}
		else if ( scanned_pos > 0.0f )
		{
			state.track_time = scanned_pos;
		}

		// Ã¢â€â‚¬Ã¢â€â‚¬ Visibility & Fade Animation Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		if ( !mp_cfg.auto_hide.value || state.is_playing || menu_open )
			container_alpha.fade_in( 0.25f );
		else
			container_alpha.fade_out( 0.25f );

		container_alpha.update( );
		if ( !container_alpha.visible( ) )
			return;

		const auto master_alpha = container_alpha.alpha( );
		const auto master_u8 = static_cast< std::uint8_t >( 255.0f * master_alpha );

		// Ã¢â€â‚¬Ã¢â€â‚¬ Dimensions & Screen Placement Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto sw = static_cast< float >( screen_w );
		const auto sh = static_cast< float >( screen_h );

		constexpr float hud_w = 286.0f;
		constexpr float hud_h = 68.0f;

		static float s_mp_x = -1.0f;
		static float s_mp_y = -1.0f;
		static bool s_mp_dragging = false;
		static float s_mp_drag_offset_x = 0.0f;
		static float s_mp_drag_offset_y = 0.0f;

		if ( s_mp_x < 0.0f || mp_cfg.position.value != settings::misc::media_player_cfg::position_preset::custom )
		{
			switch ( mp_cfg.position.value )
			{
			case settings::misc::media_player_cfg::position_preset::top_center:
				s_mp_x = ( sw - hud_w ) * 0.5f;
				s_mp_y = mp_cfg.pos_y.value;
				break;
			case settings::misc::media_player_cfg::position_preset::top_right:
				s_mp_x = sw - hud_w - 20.0f;
				s_mp_y = 70.0f;
				break;
			case settings::misc::media_player_cfg::position_preset::bottom_left:
				s_mp_x = 20.0f;
				s_mp_y = sh - hud_h - 40.0f;
				break;
			case settings::misc::media_player_cfg::position_preset::bottom_right:
				s_mp_x = sw - hud_w - 20.0f;
				s_mp_y = sh - hud_h - 40.0f;
				break;
			case settings::misc::media_player_cfg::position_preset::custom:
				if ( s_mp_x < 0.0f )
				{
					s_mp_x = ( sw - hud_w ) * 0.5f;
					s_mp_y = 40.0f;
				}
				else
				{
					s_mp_x = mp_cfg.pos_x.value;
					s_mp_y = mp_cfg.pos_y.value;
				}
				break;
			}
		}

		s_mp_x = std::clamp( s_mp_x, 0.0f, sw - hud_w );
		s_mp_y = std::clamp( s_mp_y, 0.0f, sh - hud_h );

		// Progress bar geometry Ã¢â‚¬â€ computed early so seek hit-testing works
		const float bar_x     = s_mp_x + 8.0f;
		const float bar_w     = hud_w - 16.0f;
		const float bar_y     = s_mp_y + hud_h - 6.0f;
		constexpr float bar_h = 3.0f;
		const xui::rect seek_hit_rect{ bar_x, bar_y - 6.0f, bar_w, 14.0f };
		static bool s_seeking = false;

		// Ã¢â€â‚¬Ã¢â€â‚¬ Interactive Buttons & Dragging Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		auto& inp = xui::ctx( ).input;
		const xui::rect card_rect{ s_mp_x, s_mp_y, hud_w, hud_h };

		const float btn_prev_x = s_mp_x + hud_w - 74.0f;
		const float btn_play_x = s_mp_x + hud_w - 48.0f;
		const float btn_next_x = s_mp_x + hud_w - 22.0f;
		const float btn_cy = s_mp_y + 24.0f;

		const xui::rect prev_rect{ btn_prev_x - 11.0f, btn_cy - 11.0f, 22.0f, 22.0f };
		const xui::rect play_rect{ btn_play_x - 14.0f, btn_cy - 14.0f, 28.0f, 28.0f };
		const xui::rect next_rect{ btn_next_x - 11.0f, btn_cy - 11.0f, 22.0f, 22.0f };

		const bool prev_hover = menu_open && inp.in_rect( prev_rect );
		const bool play_hover = menu_open && inp.in_rect( play_rect );
		const bool next_hover = menu_open && inp.in_rect( next_rect );

		if ( menu_open )
		{
			if ( inp.mouse_clicked )
			{
				if ( inp.in_rect( seek_hit_rect ) )
				{
					s_seeking = true;
					const float t = std::clamp( ( inp.mouse_x - bar_x ) / bar_w, 0.0f, 1.0f );
					state.track_time = t * state.estimated_duration;
				}
				else if ( play_hover )
				{
					dispatch_media_key( k_vk_play_pause );
				}
				else if ( prev_hover )
				{
					dispatch_media_key( k_vk_prev_track );
				}
				else if ( next_hover )
				{
					dispatch_media_key( k_vk_next_track );
				}
				else if ( inp.in_rect( card_rect ) )
				{
					s_mp_dragging = true;
					s_mp_drag_offset_x = inp.mouse_x - s_mp_x;
					s_mp_drag_offset_y = inp.mouse_y - s_mp_y;
					mp_cfg.position.value = settings::misc::media_player_cfg::position_preset::custom;
				}
			}
			else if ( !inp.mouse_down )
			{
				s_mp_dragging = false;
				s_seeking = false;
			}

			if ( s_seeking && inp.mouse_down )
			{
				const float t = std::clamp( ( inp.mouse_x - bar_x ) / bar_w, 0.0f, 1.0f );
				state.track_time = t * state.estimated_duration;
			}

			if ( s_mp_dragging )
			{
				s_mp_x = std::clamp( inp.mouse_x - s_mp_drag_offset_x, 0.0f, sw - hud_w );
				s_mp_y = std::clamp( inp.mouse_y - s_mp_drag_offset_y, 0.0f, sh - hud_h );
				mp_cfg.pos_x.value = s_mp_x;
				mp_cfg.pos_y.value = s_mp_y;
			}
		}
		else
		{
			s_mp_dragging = false;
			s_seeking = false;
		}

		// Ã¢â€â‚¬Ã¢â€â‚¬ Render Card Frame Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		// One-time migration: the pre-1.0.0.8 media accent was hot pink
		// {255,95,175} while the rest of the UI uses the soft magenta theme
		// accent. Fold any leftover saved pink into the theme token.
		static bool s_color_migrated = false;
		if ( !s_color_migrated )
		{
			s_color_migrated = true;
			const xdraw::color old_pink{ 255, 95, 175, 255 };
			auto& mcfg = settings::g_misc.m_media_player;
			if ( mcfg.accent_color.value == old_pink ) mcfg.accent_color.value = tokens::col_accent;
			if ( mcfg.lyrics_highlight.value == old_pink ) mcfg.lyrics_highlight.value = tokens::col_accent;
		}

		const auto& acc = mp_cfg.accent_color.value;

		// Rounded glass card (matches the menu chrome) Ã¢â‚¬â€ no hard white outline.
		const auto card_r = xdraw::corner_radius{ tokens::round_md };
		draw_list.rect_filled( s_mp_x, s_mp_y, hud_w, hud_h, tokens::col_dark.alpha( master_u8 ), card_r );
		// Inset rounded accent top edge.
		draw_list.rect_filled( s_mp_x + tokens::round_md, s_mp_y + 1.0f, hud_w - tokens::round_md * 2.0f, 2.0f,
			acc.alpha( master_u8 ), xdraw::corner_radius{ 1.0f } );

		// Ã¢â€â‚¬Ã¢â€â‚¬ Left: Album Art Thumbnail Badge Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const float badge_x = s_mp_x + 8.0f;
		const float badge_y = s_mp_y + 8.0f;
		constexpr float badge_size = 36.0f;
		const float badge_cx = badge_x + badge_size * 0.5f;
		const float badge_cy = badge_y + badge_size * 0.5f;

		if ( s_album_art_srv )
		{
			draw_list.image( badge_x, badge_y, badge_size, badge_size, s_album_art_srv.Get( ), xdraw::corner_radius{ 4.0f }, xdraw::color{ 255, 255, 255, master_u8 } );
		}
		else
		{
			// Fallback badge plate with styled vinyl when no cover image
			draw_list.rect_filled( badge_x, badge_y, badge_size, badge_size, tokens::col_card.alpha( master_u8 ), xdraw::corner_radius{ 4.0f } );

			const float vinyl_r = 13.5f;
			draw_list.circle_filled( badge_cx, badge_cy, vinyl_r, xdraw::color{ 16, 10, 15, master_u8 }, 16 );
			draw_list.circle( badge_cx, badge_cy, vinyl_r * 0.78f, tokens::col_border.alpha( static_cast< std::uint8_t >( 140.0f * master_alpha ) ), 14, 1.0f );
			draw_list.circle( badge_cx, badge_cy, vinyl_r * 0.55f, tokens::col_border.alpha( static_cast< std::uint8_t >( 140.0f * master_alpha ) ), 12, 1.0f );
			draw_list.circle_filled( badge_cx, badge_cy, 4.0f, acc.alpha( master_u8 ), 10 );
			draw_list.circle_filled( badge_cx, badge_cy, 1.5f, tokens::col_dark.alpha( master_u8 ), 6 );
		}

		// Ã¢â€â‚¬Ã¢â€â‚¬ Typography & Smooth Marquee Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const float text_left = s_mp_x + 50.0f;
		constexpr float text_max_w = 140.0f;

		const auto [title_w, title_h] = xdraw::measure_text( state.title );
		float title_offset = 0.0f;
		if ( title_w > text_max_w )
		{
			const float max_scroll = title_w - text_max_w + 16.0f;
			const float scroll_speed = 28.0f; // px/s
			const float pause_start = 1.8f;
			const float pause_end = 1.2f;
			const float scroll_dur = max_scroll / scroll_speed;
			const float loop_total = pause_start + scroll_dur + pause_end;
			const float t_in_loop = std::fmod( cur_time_sec - state.track_start_time, loop_total );

			if ( t_in_loop < pause_start )
				title_offset = 0.0f;
			else if ( t_in_loop < pause_start + scroll_dur )
				title_offset = -( t_in_loop - pause_start ) * scroll_speed;
			else
				title_offset = -max_scroll;
		}

		draw_list.push_clip( text_left, s_mp_y + 2.0f, text_max_w, hud_h - 7.0f );
		// Track Title: Soft pearl rose white
		draw_list.text( text_left + title_offset, s_mp_y + 8.5f, state.title, tokens::col_text.alpha( master_u8 ) );
		// Artist: Soft rose-tinted dim
		draw_list.text( text_left, s_mp_y + 24.5f, state.artist, tokens::col_text_dim.alpha( static_cast< std::uint8_t >( 215.0f * master_alpha ) ) );
		draw_list.pop_clip( );

		// Ã¢â€â‚¬Ã¢â€â‚¬ Right: Modern Transport Buttons Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const auto icon_col = tokens::col_text_dim.alpha( static_cast< std::uint8_t >( 200.0f * master_alpha ) );

		// 1. Previous Track [|<<]
		if ( menu_open && prev_hover )
		{
			draw_list.circle_filled( btn_prev_x, btn_cy, 11.5f, xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 20.0f * master_alpha ) }, 12 );
		}
		const auto p_col = ( menu_open && prev_hover ) ? acc.alpha( master_u8 ) : icon_col;
		draw_list.line( btn_prev_x - 4.5f, btn_cy - 4.5f, btn_prev_x - 4.5f, btn_cy + 4.5f, p_col, 1.4f );
		draw_list.triangle_filled( btn_prev_x - 4.5f, btn_cy, btn_prev_x + 3.5f, btn_cy - 4.5f, btn_prev_x + 3.5f, btn_cy + 4.5f, p_col );

		// 2. Play / Pause Button: Prominent circular pill
		const float pl_radius = 12.0f;
		if ( state.is_playing )
		{
			// Glowing accent circle
			draw_list.circle_filled( btn_play_x, btn_cy, pl_radius + 2.0f, acc.alpha( static_cast< std::uint8_t >( 40.0f * master_alpha ) ), 16 );
			draw_list.circle_filled( btn_play_x, btn_cy, pl_radius, acc.alpha( master_u8 ), 16 );
			// Dark pause bars
			const auto bar_col = tokens::col_dark.alpha( master_u8 );
			const auto pause_r = xdraw::corner_radius{ 1.25f };
			draw_list.rect_filled( btn_play_x - 4.5f, btn_cy - 5.5f, 3.5f, 11.0f, bar_col, pause_r );
			draw_list.rect_filled( btn_play_x + 1.0f, btn_cy - 5.5f, 3.5f, 11.0f, bar_col, pause_r );
		}
		else
		{
			// Paused: card circle with play triangle
			draw_list.circle_filled( btn_play_x, btn_cy, pl_radius, tokens::col_card.alpha( master_u8 ), 16 );
			draw_list.circle( btn_play_x, btn_cy, pl_radius, ( menu_open && play_hover ) ? acc.alpha( master_u8 ) : tokens::col_border.alpha( master_u8 ), 16, 1.0f );
			const auto tri_col = ( menu_open && play_hover ) ? acc.alpha( master_u8 ) : tokens::col_text.alpha( master_u8 );
			draw_list.triangle_filled( btn_play_x + 4.0f, btn_cy, btn_play_x - 3.0f, btn_cy - 4.5f, btn_play_x - 3.0f, btn_cy + 4.5f, tri_col );
		}

		// 3. Next Track [>>|]
		if ( menu_open && next_hover )
		{
			draw_list.circle_filled( btn_next_x, btn_cy, 11.5f, xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 20.0f * master_alpha ) }, 12 );
		}
		const auto n_col = ( menu_open && next_hover ) ? acc.alpha( master_u8 ) : icon_col;
		draw_list.triangle_filled( btn_next_x + 4.5f, btn_cy, btn_next_x - 3.5f, btn_cy - 4.5f, btn_next_x - 3.5f, btn_cy + 4.5f, n_col );
		draw_list.line( btn_next_x + 4.5f, btn_cy - 4.5f, btn_next_x + 4.5f, btn_cy + 4.5f, n_col, 1.4f );

		// Ã¢â€â‚¬Ã¢â€â‚¬ Bottom: Timeline Seek Slider + Time Labels Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
		const float prog_frac = std::clamp( state.track_time / state.estimated_duration, 0.0f, 1.0f );

		// Elapsed / remaining time labels
		{
			const int elapsed_s = static_cast<int>( state.track_time );
			const int remain_s  = static_cast<int>( std::max( state.estimated_duration - state.track_time, 0.0f ) );
			char elapsed_buf[ 12 ];
			char remain_buf[ 12 ];
			std::snprintf( elapsed_buf, sizeof( elapsed_buf ), "%d:%02d", elapsed_s / 60, elapsed_s % 60 );
			std::snprintf( remain_buf,  sizeof( remain_buf ),  "-%d:%02d", remain_s / 60, remain_s % 60 );
			const auto [ew, eh] = xdraw::measure_text( elapsed_buf );
			const auto [rw, rh] = xdraw::measure_text( remain_buf );
			const auto dim_u8   = static_cast<std::uint8_t>( 155.0f * master_alpha );
			draw_list.text( bar_x, bar_y - eh - 4.0f, elapsed_buf, tokens::col_text_dim.alpha( dim_u8 ) );
			draw_list.text( bar_x + bar_w - rw, bar_y - eh - 4.0f, remain_buf, tokens::col_text_dim.alpha( dim_u8 ) );
		}

		// Track base
		draw_list.rect_filled( bar_x, bar_y, bar_w, bar_h, tokens::col_line.alpha( static_cast<std::uint8_t>( 200.0f * master_alpha ) ), xdraw::corner_radius{ bar_h * 0.5f } );

		// Active fill + interactive thumb
		if ( prog_frac > 0.005f )
		{
			const float filled_w = bar_w * prog_frac;
			draw_list.rect_filled( bar_x, bar_y, filled_w, bar_h, acc.alpha( master_u8 ), xdraw::corner_radius{ bar_h * 0.5f } );
			const float thumb_r = s_seeking ? 3.5f : 2.5f;
			draw_list.circle_filled( bar_x + filled_w, bar_y + bar_h * 0.5f, thumb_r, acc.alpha( master_u8 ), 8 );
		}

		// Live synced lyrics (fetched from LRCLIB) under the card.
		features::misc::lyrics::update( state.title.c_str( ), state.artist.c_str( ), static_cast< int >( state.estimated_duration ) );
		features::misc::lyrics::draw( draw_list, s_mp_x + hud_w * 0.5f, s_mp_y + hud_h + 14.0f, hud_w,
			static_cast< double >( state.track_time ) );
	}

	void widgets::fw_logo_widget( xdraw::draw_list& draw_list )
	{
		const auto& cfg = settings::g_misc.m_hud.m_fw_logo;
		if ( !cfg.enabled.value )
		{
			return;
		}

		const auto [sw, sh] = xdraw::viewport_size( );
		const bool menu_open = rendering::g_menu.is_open( );

		static float s_fw_x = -1.0f;
		static float s_fw_y = -1.0f;
		static float s_fw_size = 64.0f;

		static bool s_initialized = false;
		if ( !s_initialized )
		{
			s_fw_x = ( cfg.pos_x.value >= 0.0f ) ? cfg.pos_x.value : 100.0f;
			s_fw_y = ( cfg.pos_y.value >= 0.0f ) ? cfg.pos_y.value : 100.0f;
			s_fw_size = std::clamp( cfg.size.value, 24.0f, 256.0f );
			s_initialized = true;
		}

		if ( std::abs( s_fw_size - cfg.size.value ) > 0.5f )
		{
			s_fw_size = std::clamp( cfg.size.value, 24.0f, 256.0f );
		}

		static bool s_dragging = false;
		static bool s_resizing = false;
		static float s_drag_offset_x = 0.0f;
		static float s_drag_offset_y = 0.0f;

		const float cur_h = s_fw_size;
		const float cur_w = std::round( s_fw_size * 1.58f );

		if ( menu_open )
		{
			auto& inp = xui::ctx( ).input;
			const xui::rect logo_rect{ s_fw_x, s_fw_y, cur_w, cur_h };
			constexpr float handle_sz = 14.0f;
			const xui::rect resize_rect{ s_fw_x + cur_w - handle_sz, s_fw_y + cur_h - handle_sz, handle_sz, handle_sz };

			if ( inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				if ( inp.in_rect( resize_rect ) )
				{
					s_resizing = true;
				}
				else if ( inp.in_rect( logo_rect ) )
				{
					s_dragging = true;
					s_drag_offset_x = inp.mouse_x - s_fw_x;
					s_drag_offset_y = inp.mouse_y - s_fw_y;
				}
			}

			if ( !inp.mouse_down )
			{
				s_dragging = false;
				s_resizing = false;
			}

			if ( s_resizing )
			{
				const float new_sz = std::clamp( ( inp.mouse_x - s_fw_x ) / 1.58f, 24.0f, 256.0f );
				s_fw_size = new_sz;
				const_cast<config::val<float>&>( cfg.size ).value = new_sz;
			}
			else if ( s_dragging )
			{
				s_fw_x = std::clamp( inp.mouse_x - s_drag_offset_x, 0.0f, static_cast<float>( sw ) - cur_w );
				s_fw_y = std::clamp( inp.mouse_y - s_drag_offset_y, 0.0f, static_cast<float>( sh ) - cur_h );
				const_cast<config::val<float>&>( cfg.pos_x ).value = s_fw_x;
				const_cast<config::val<float>&>( cfg.pos_y ).value = s_fw_y;
			}

			const bool hovered = inp.in_rect( logo_rect );
			const auto border_col = ( s_dragging || s_resizing ) ? tokens::col_accent : ( hovered ? tokens::col_accent.alpha( 120 ) : tokens::col_border.alpha( 60 ) );
			draw_list.rect( s_fw_x - 1.0f, s_fw_y - 1.0f, cur_w + 2.0f, cur_h + 2.0f, border_col, 1.0f );
			draw_list.rect_filled( s_fw_x + cur_w - 6.0f, s_fw_y + cur_h - 6.0f, 6.0f, 6.0f, border_col );
		}
		else
		{
			s_dragging = false;
			s_resizing = false;
		}

		s_fw_x = std::clamp( s_fw_x, 0.0f, static_cast<float>( sw ) - cur_w );
		s_fw_y = std::clamp( s_fw_y, 0.0f, static_cast<float>( sh ) - cur_h );

		const auto mode = static_cast<fw_logo_mode>( std::clamp( cfg.mode.value, 0, 3 ) );
		draw_fw_logo( draw_list, s_fw_x + cur_w * 0.5f, s_fw_y + cur_h * 0.5f, cur_w, cur_h, mode, 1.0f );
	}

	void widgets::audio_visualizer_widget( xdraw::draw_list& draw_list )
	{
		const auto& cfg = settings::g_misc.m_spectrum;
		if ( !cfg.enabled.value )
		{
			return;
		}

		const auto [sw, sh] = xdraw::viewport_size( );
		const bool menu_open = rendering::g_menu.is_open( );

		static float s_viz_x = -1.0f;
		static float s_viz_y = -1.0f;
		static float s_viz_w = 240.0f;
		static float s_viz_h = 48.0f;

		static bool s_initialized = false;
		if ( !s_initialized )
		{
			s_viz_x = ( cfg.pos_x.value >= 0.0f ) ? cfg.pos_x.value : 40.0f;
			s_viz_y = ( cfg.pos_y.value >= 0.0f ) ? cfg.pos_y.value : 120.0f;
			s_initialized = true;
		}

		if ( std::abs( s_viz_w - cfg.width.value ) > 1.0f ) s_viz_w = std::clamp( cfg.width.value, 80.0f, 800.0f );
		if ( std::abs( s_viz_h - cfg.height.value ) > 1.0f ) s_viz_h = std::clamp( cfg.height.value, 16.0f, 160.0f );

		static bool s_dragging = false;
		static float s_drag_off_x = 0.0f;
		static float s_drag_off_y = 0.0f;

		if ( menu_open )
		{
			auto& inp = xui::ctx( ).input;
			const xui::rect viz_rect{ s_viz_x, s_viz_y, s_viz_w, s_viz_h };

			if ( inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) && inp.in_rect( viz_rect ) )
			{
				s_dragging = true;
				s_drag_off_x = inp.mouse_x - s_viz_x;
				s_drag_off_y = inp.mouse_y - s_viz_y;
			}

			if ( !inp.mouse_down )
			{
				s_dragging = false;
			}

			if ( s_dragging )
			{
				const float max_x = std::max( 0.0f, static_cast< float >( sw ) - s_viz_w );
				const float max_y = std::max( 0.0f, static_cast< float >( sh ) - s_viz_h );
				s_viz_x = std::clamp( inp.mouse_x - s_drag_off_x, 0.0f, max_x );
				s_viz_y = std::clamp( inp.mouse_y - s_drag_off_y, 0.0f, max_y );
				const_cast<config::val<float>&>( cfg.pos_x ).value = s_viz_x;
				const_cast<config::val<float>&>( cfg.pos_y ).value = s_viz_y;
			}

			const bool hovered = inp.in_rect( viz_rect );
			const auto border_col = s_dragging ? tokens::col_accent : ( hovered ? tokens::col_accent.alpha( 120 ) : tokens::col_border.alpha( 60 ) );
			draw_list.rect( s_viz_x - 1.0f, s_viz_y - 1.0f, s_viz_w + 2.0f, s_viz_h + 2.0f, border_col, 1.0f );
		}
		else
		{
			s_dragging = false;
		}

		const float max_vx = std::max( 0.0f, static_cast< float >( sw ) - s_viz_w );
		const float max_vy = std::max( 0.0f, static_cast< float >( sh ) - s_viz_h );
		s_viz_x = std::clamp( s_viz_x, 0.0f, max_vx );
		s_viz_y = std::clamp( s_viz_y, 0.0f, max_vy );

		// Card glass.
		draw_list.rect_filled( s_viz_x, s_viz_y, s_viz_w, s_viz_h, tokens::col_dark.alpha( 225 ), xdraw::corner_radius{ 8.0f } );
		draw_list.rect( s_viz_x, s_viz_y, s_viz_w, s_viz_h, tokens::col_accent.alpha( 140 ), xdraw::corner_radius{ 8.0f }, 1.0f );

		// Clip bars AND diagnostics to the card so the hint text can never
		// spill outside the widget at small sizes.
		draw_list.push_clip( s_viz_x, s_viz_y, s_viz_w, s_viz_h );

		const float header_h = ( s_viz_h >= 30.0f ) ? 18.0f : 0.0f;
		if ( header_h > 0.0f )
		{
			const auto [tw, th] = xdraw::measure_text( "AUDIO VISUALIZER" );
			const float hy = s_viz_y + 5.0f;
			draw_list.text( s_viz_x + 8.0f, hy, "AUDIO VISUALIZER", tokens::col_accent.alpha( 200 ) );
			draw_list.rect_filled( s_viz_x + 8.0f + tw + 6.0f, hy + th * 0.62f, std::max( 0.0f, s_viz_w - 16.0f - tw - 6.0f ), 1.0f, tokens::col_accent.alpha( 60 ) );
		}

		features::misc::spectrum::draw( draw_list, s_viz_x + 8.0f, s_viz_y + header_h + 4.0f,
			std::max( 0.0f, s_viz_w - 16.0f ), std::max( 0.0f, s_viz_h - header_h - 8.0f ),
			cfg.color.value, cfg.sensitivity.value );

		// "No audio signal" diagnostic: capture thread alive but loopback is
		// silent (wrong default device, exclusive-mode endpoint, muted source).
		const auto now_ms = GetTickCount64( );
		const auto last = features::misc::spectrum::last_packet_ms( ).load( );
		const auto last_audio = features::misc::spectrum::last_audio_ms( ).load( );

		const char* msg = ( last == 0 ) ? "connecting to audio device..."
			: ( now_ms - last_audio > 2500 ) ? "no audio signal"
			: nullptr;

		if ( msg )
		{
			const float inner_w = std::max( 0.0f, s_viz_w - 16.0f );
			const float inner_h = std::max( 0.0f, s_viz_h - 16.0f );

			const auto [dot_w, dot_h] = xdraw::measure_text( "..." );
			if ( inner_w >= dot_w + 8.0f && inner_h >= dot_h )
			{
				const std::string_view full = msg;
				std::string_view text = full;
				auto [tw, th] = xdraw::measure_text( text );
				while ( text.size( ) > 1 && tw > inner_w - dot_w )
				{
					text.remove_suffix( 1 );
					tw = xdraw::measure_text( text ).first;
				}

				const bool truncated = text.size( ) < full.size( );
				const float txt_w = tw + ( truncated ? dot_w : 0.0f );
				const float base_x = s_viz_x + ( s_viz_w - txt_w ) * 0.5f;

				draw_list.text( base_x, s_viz_y + ( s_viz_h - th ) * 0.5f, text, tokens::col_text_dim.alpha( 170 ) );
				if ( truncated )
				{
					draw_list.text( base_x + tw + 1.0f, s_viz_y + ( s_viz_h - dot_h ) * 0.5f, "...", tokens::col_text_dim.alpha( 170 ) );
				}
			}
		}

		draw_list.pop_clip( );
	}

} // namespace rendering
