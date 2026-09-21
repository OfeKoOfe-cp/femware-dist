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

		// Bars-only card: no captions. The full height goes to the spectrum so
		// even short cards read clearly.
		features::misc::spectrum::draw( draw_list, s_viz_x + 8.0f, s_viz_y + 4.0f,
			std::max( 0.0f, s_viz_w - 16.0f ), std::max( 0.0f, s_viz_h - 8.0f ),
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
