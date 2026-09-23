#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/random/random.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>

#include "../../rendering.hpp"
#include <core/features/changer/skin_preview_bridge.hpp>
#include <core/resources/cat_overlay.hpp>
#include <core/resources/esp_models.hpp>
#include <core/resources/fw_logo_resources.hpp>

namespace rendering {

	void menu::draw( )
	{
		if ( this->m_last_open != this->m_open )
		{
			if ( this->m_open )
			{
				this->m_saved_relative_mouse = memory::read<std::uint8_t>( addresses::globals::input_system + 84 );
				memory::call_vfunc<void>( addresses::globals::input_system, 76, false );
				this->apply_saved_cursor( );
			}
			else
			{
				POINT pt{};
				if ( GetCursorPos( &pt ) )
				{
					this->m_saved_cursor_x = pt.x;
					this->m_saved_cursor_y = pt.y;
					this->m_has_saved_cursor = true;
				}
				memory::call_vfunc<void>( addresses::globals::input_system, 76, this->m_saved_relative_mouse != 0 );
			}
			this->m_last_open = this->m_open;
		}

		if ( !g_context.ui_assets_ready( ) )
		{
			this->draw_intro( );
			return;
		}

		if ( this->draw_intro( ) )
			return;

		xui::begin( );
		this->sync_theme_style( );
		this->try_load_user_avatar( );
		{
			const auto dt         = xdraw::delta_time( );
			const auto anim_speed = this->m_open ? 14.0f : 16.0f;
			const auto anim_tgt   = this->m_open ? 1.0f : 0.0f;
			this->m_open_anim    += ( anim_tgt - this->m_open_anim ) * std::min( anim_speed * dt, 1.0f );

			if ( this->m_open_anim < 0.01f && !this->m_open )
			{
				features::changer::skin_preview_bridge::hide_panel( );
				xui::end( );
				return;
			}

			const auto reveal = xui::ease::out_cubic( this->m_open_anim );

			const auto [screen_w, screen_h] = xdraw::viewport_size( );
			const float sw = static_cast< float >( screen_w );
			const float sh = static_cast< float >( screen_h );

			// ── Background ambience (blur + snowfall) ──────────────────────
			this->draw_ambience( sw, sh );

			// ── Top Master Bar ──────────────────────────────────────────────
			this->draw_top_master_bar( sw, sh );

			if ( this->m_show_main )
			{
				if ( xui::begin_window( "##menu", this->m_x, this->m_y, this->m_w, this->m_h, false, 200.0f, 200.0f, reveal ) )
				{

				auto&      dl   = xui::draw::current( );
			const auto wx   = this->m_x;
			const auto wy   = this->m_y;
			const auto ww   = this->m_w;
			const auto wh   = this->m_h;

			// ── 0. Ambient drop shadow & glow ──────────────────────────────
			for ( int s_i = 5; s_i >= 1; --s_i )
			{
				const float spread = static_cast<float>( s_i ) * 3.0f;
				const std::uint8_t a = static_cast<std::uint8_t>( 6.0f * ( 6 - s_i ) * reveal );
				dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
								xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
			}
			dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast<std::uint8_t>( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );

			// ── 1. Window background — rounded glass panel ──────────────────
			dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

			// ── 2. Title bar "FEMWARE  |  CS2" + Avatar & Cat Overlay ───────
			{
				const auto ty = wy;
				// Rounded header (top corners match the frame; bottom squared off).
				dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
				dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
				// 1px bottom separator
				dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_line );

				// FW glowing animated logo in top left (no box, no cat overlay, crisp large display)
				constexpr float logo_w = 34.0f;
				constexpr float logo_h = 22.0f;
				const auto logo_x = wx + 8.0f;
				const auto logo_y = ty + std::floor( ( tokens::title_bar_h - logo_h ) * 0.5f );

				draw_fw_logo( dl, logo_x + logo_w * 0.5f, logo_y + logo_h * 0.5f, logo_w, logo_h, fw_logo_mode::glowing, reveal );

				// Persona name
				const auto* persona_name = steam::friends::get_persona_name( );
				const std::string_view display_name = ( persona_name && *persona_name ) ? persona_name : "User";
				const auto [nw, nh] = xdraw::measure_text( display_name );
				const auto name_x = logo_x + logo_w + 8.0f;
				const auto name_y = std::floor( ty + ( tokens::title_bar_h - nh ) * 0.5f );
				dl.text( name_x, name_y, display_name, tokens::col_text );
				dl.text( name_x + nw + 4.0f, name_y, "<3", tokens::col_accent );

				const auto [bw, bh] = xdraw::measure_text( "FEMWARE" );
				const auto [sw, sh] = xdraw::measure_text( "  |  CS2" );
				const auto total_w  = bw + sw;
				const auto tx       = std::floor( wx + ( ww - total_w ) * 0.5f );
				const auto cy       = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

				dl.text( tx,      cy, "FEMWARE", tokens::col_accent );
				dl.text( tx + bw, cy, "  |  CS2", tokens::col_text_dim );

				// Top-right close button
				constexpr float cbtn_sz = 16.0f;
				const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
				const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
				const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
				const bool cbtn_hov = xui::ctx( ).input.in_rect( cbtn_rect );
				if ( cbtn_hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
				{
					this->m_show_main = false;
				}
				if ( cbtn_hov )
				{
					dl.rect_filled( cbtn_rect.x, cbtn_rect.y, cbtn_rect.w, cbtn_rect.h, xdraw::color{ 239, 68, 68, 70 }, xdraw::corner_radius{ tokens::round_md } );
				}
				const float ccx = cbtn_x + cbtn_sz * 0.5f;
				const float ccy = cbtn_y + cbtn_sz * 0.5f;
				const auto cross_col = cbtn_hov ? xdraw::color{ 248, 113, 113, 255 } : tokens::col_text_dim;
				dl.line( ccx - 4.0f, ccy - 4.0f, ccx + 4.0f, ccy + 4.0f, cross_col, 1.4f );
				dl.line( ccx - 4.0f, ccy + 4.0f, ccx + 4.0f, ccy - 4.0f, cross_col, 1.4f );

				// Top-right search button
				constexpr float sbtn_sz = 16.0f;
				const float sbtn_x = cbtn_x - sbtn_sz - 10.0f;
				const float sbtn_y = ty + std::floor( ( tokens::title_bar_h - sbtn_sz ) * 0.5f );
				const auto sbtn_rect = xui::rect{ sbtn_x - 3.0f, sbtn_y - 2.0f, sbtn_sz + 6.0f, sbtn_sz + 4.0f };
				const bool sbtn_hov = xui::ctx( ).input.in_rect( sbtn_rect );
				if ( sbtn_hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
				{
					this->m_search_open = !this->m_search_open;
					if ( this->m_search_open )
						this->rebuild_search_index( );
					else
						this->close_search( );
				}
				if ( sbtn_hov )
				{
					dl.rect_filled( sbtn_rect.x, sbtn_rect.y, sbtn_rect.w, sbtn_rect.h, tokens::col_elevated.alpha( 120 ), xdraw::corner_radius{ tokens::round_md } );
				}
				if ( this->m_textures.search.resource )
				{
					const auto icon_col = this->m_search_open ? tokens::col_accent : ( sbtn_hov ? tokens::col_text : tokens::col_text_dim );
					dl.image( sbtn_x, sbtn_y, sbtn_sz, sbtn_sz, this->m_textures.search.resource.Get( ), icon_col );
				}
			}

			// ── 3. Main tab bar ─────────────────────────────────────────────
			const auto tab_y = wy + tokens::title_bar_h;
			this->draw_linoria_tab_bar( wx, tab_y, ww );

			// ── 4. Sub-tab bar (if current tab has subtabs) ─────────────────
			const auto stab_y = tab_y + tokens::tab_bar_h;
			const auto& def   = k_subtab_defs[ this->m_tab ];
			if ( def.count > 1 )
				this->draw_linoria_subtab_bar( wx, stab_y, ww );

			// ── 5. accent top-edge line (inset so it follows the rounded frame)
			dl.rect_filled( wx + tokens::round_lg, wy + 1.0f, ww - tokens::round_lg * 2.0f, 2.0f,
				tokens::col_accent.alpha( static_cast<std::uint8_t>( tokens::col_accent.a * reveal ) ),
				xdraw::corner_radius{ 1.0f } );

			// ── 6. Content area ─────────────────────────────────────────────
			constexpr float status_bar_h = 24.0f;
			const auto stabs_used = ( def.count > 1 ) ? tokens::subtab_bar_h : 0.0f;
			const auto body_y     = stab_y + stabs_used + tokens::gap;
			const auto body_h     = wh - tokens::title_bar_h - tokens::tab_bar_h
			                        - stabs_used - status_bar_h - tokens::gap * 2.0f;
			const auto body_x     = wx + tokens::gap;
			const auto body_w     = ww - tokens::gap * 2.0f;
			const auto col_w      = ( body_w - tokens::col_gap ) * 0.5f;

			this->m_body_x = body_x;
			this->m_body_y = body_y;
			this->m_body_w = body_w;
			this->m_body_h = body_h;

			xui::layout::set_cursor( body_x - wx, body_y - wy );

			if ( this->m_search_open )
			{
				xui::ctx( ).inside_overlay = xui::fnv1a( "menu_search_mode" );
				this->draw_search_results( body_x, body_y, body_w, body_h );
				xui::ctx( ).inside_overlay = xui::null_id;
				xui::end_window( );
				xui::end( );
				return;
			}

			switch ( this->m_tab )
			{
			case 0: this->draw_ragebot  ( col_w ); break;
			case 1: this->draw_legitbot ( col_w ); break;
			case 2: this->draw_player   ( col_w ); break;
			case 3: this->draw_world    ( col_w ); break;
			case 4: this->draw_misc     ( col_w ); break;
			case 5: this->draw_config   ( col_w ); break;
			}

			// ── 7. Bottom Status Bar ─────────────────────────────────────────
			this->draw_status_bar( wx, wy + wh - status_bar_h, ww, status_bar_h );

				xui::end_window( );
			}
		}

		if ( this->m_show_lua )
		{
			this->draw_lua_studio( sw, sh );
		}

		if ( this->m_show_lua_console )
		{
			this->draw_lua_console( sw, sh );
		}

		if ( this->m_show_players )
		{
			this->draw_players_inspector( sw, sh );
		}

		if ( this->m_show_skins )
		{
			this->draw_skin_changer( sw, sh );
		}
		else
		{
			features::changer::skin_preview_bridge::hide_panel( );
		}

		if ( this->m_show_models )
		{
			this->draw_models_studio( sw, sh );
		}
	}
	xui::end( );
	}


	// Renders the fullscreen scene-blur backdrop and the ambient snowfall
	// behind every window. Called right after xui::begin() so the backdrop is
	// the first draw command of the frame - the blur chain captures the game
	// frame as it stands before anything UI-shaped touches the back buffer.
	void menu::draw_ambience( float sw, float sh )
	{
		auto& amb = settings::g_misc.m_ambience;
		const float reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( reveal <= 0.01f )
		{
			return;
		}

		auto& dl = xui::draw::current( );

		// Reset the shared blur strength back to full when the backdrop is off
		// so dropdowns / glass widgets keep using the complete pyramid chain.
		xdraw::set_backdrop_blur_strength( amb.blur.value ? amb.blur_intensity.value : 1.0f );

		if ( amb.blur.value && !this->m_draw_on_overlay )
		{
			// A streamproof overlay is a transparent layer over the game, so
			// its capture source is an unpopulated texture - drawing a blurred
			// frame there would flatten everything to black. Keep the backdrop
			// to the direct back-buffer path.
			dl.rect_filled_blurred( 0.0f, 0.0f, sw, sh,
				xdraw::color{ 8, 10, 16, static_cast< std::uint8_t >( 235.0f * reveal ) } );
		}

		if ( amb.snow.value && amb.snow_count.value > 0 )
		{
			const float dt = std::min( xdraw::delta_time( ), 0.05f );
			this->m_snow_time += dt;

			const int want = std::max( 0, amb.snow_count.value );
			if ( static_cast< int >( this->m_snow.size( ) ) != want )
			{
				this->m_snow.resize( static_cast< std::size_t >( want ) );
			}

			const float base_speed = std::max( 1.0f, amb.snow_speed.value );
			const float drift = amb.snow_drift.value;
			const float base_size = std::max( 0.5f, amb.snow_size.value );
			const float opacity = std::clamp( amb.snow_opacity.value, 0.0f, 1.0f );

			const auto seed_flake = [ & ]( snow_flake& f )
				{
					f.x = random::floating( -12.0f, sw + 12.0f );
					f.y = random::floating( 0.0f, sh );
					f.vx = random::floating( -drift, drift );
					f.vy = random::floating( base_speed * 0.55f, base_speed * 1.35f );
					f.size = random::floating( base_size * 0.5f, base_size * 1.6f );
					f.alpha = static_cast< std::uint8_t >(
						std::clamp( 255.0f * opacity * random::floating( 0.55f, 1.0f ), 0.0f, 255.0f ) );
				};

			for ( auto& flake : this->m_snow )
			{
				if ( flake.size <= 0.0f )
				{
					seed_flake( flake );
				}

				flake.x += flake.vx * dt;
				flake.y += flake.vy * dt;

				// Gentle horizontal sway that varies per flake.
				flake.x += std::sin( this->m_snow_time * 1.5f + flake.y * 0.02f + flake.size ) * 0.30f * dt;

				if ( flake.y > sh + 8.0f )
				{
					seed_flake( flake );
					flake.x = random::floating( -12.0f, sw + 12.0f );
					flake.y = random::floating( -12.0f, -4.0f );
				}

				const auto a = static_cast< std::uint8_t >( static_cast< float >( flake.alpha ) * reveal );
				dl.circle_filled( flake.x, flake.y, flake.size, xdraw::color{ 255, 255, 255, a } );
			}
		}
	}

	void menu::draw_linoria_tab_bar( float x, float y, float w )
	{
		static constexpr const char* k_tab_names[] = {
			"Rage", "Legit", "Player", "World", "Misc", "Config"
		};
		static constexpr int k_tab_count = 6;

		auto& dl              = xui::draw::current( );
		const auto& inp       = xui::ctx( ).input;
		const auto bar_h      = tokens::tab_bar_h;

		// Background
		dl.rect_filled( x, y, w, bar_h, tokens::col_title_bar );
		// Bottom separator
		dl.rect_filled( x, y + bar_h - 1.0f, w, 1.0f, tokens::col_border );

		const auto btn_w = w / static_cast<float>( k_tab_count );

		// Smoothly lerp sliding indicator to active tab
		const float target_ind_x = x + btn_w * static_cast<float>( this->m_tab );
		const float target_ind_w = btn_w;
		const float dt = xdraw::delta_time( );
		if ( this->m_tab_indicator_x < 0.0f )
		{
			this->m_tab_indicator_x = target_ind_x;
			this->m_tab_indicator_w = target_ind_w;
		}
		else
		{
			this->m_tab_indicator_x += ( target_ind_x - this->m_tab_indicator_x ) * std::min( 22.0f * dt, 1.0f );
			this->m_tab_indicator_w += ( target_ind_w - this->m_tab_indicator_w ) * std::min( 22.0f * dt, 1.0f );
		}

		// Active tab: soft light pill (reference style, not a neon accent fill)
		dl.rect_filled( this->m_tab_indicator_x + 2.0f, y + 3.0f, this->m_tab_indicator_w - 4.0f, bar_h - 6.0f,
			xdraw::color{ 255, 255, 255, 34 }, xdraw::corner_radius{ 7.0f } );

		for ( auto i = 0; i < k_tab_count; ++i )
		{
			const auto bx      = x + btn_w * static_cast<float>( i );
			const auto btn     = xui::rect{ bx, y, btn_w, bar_h };
			const auto hovered = inp.in_rect( btn );
			const auto active  = ( this->m_tab == i );

			// Click
			if ( hovered && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_tab    = i;
				this->m_subtab = 0;
			}

			// Hover fill
			const auto hover_a = xui::anim::lerp( xui::fnv1a( "ltab_hov" ) + i,
				( hovered && !active ) ? 1.0f : 0.0f, 14.0f );
			if ( hover_a > 0.01f )
				dl.rect_filled( bx, y, btn_w, bar_h,
					tokens::col_elevated.alpha( static_cast<std::uint8_t>( 80.0f * hover_a ) ) );

			// Label & Icon
			const auto [tw, th] = xdraw::measure_text( k_tab_names[ i ] );
			const auto act_a = xui::anim::lerp( xui::fnv1a( "ltab_act" ) + i, active ? 1.0f : 0.0f, 14.0f );
			const auto text_col = xui::lerp( tokens::col_text_dim, tokens::col_accent, act_a );
			constexpr float icon_sz = 14.0f;
			constexpr float icon_gap = 5.0f;
			const bool has_icon = ( this->m_textures.tabs[ i ].resource != nullptr );
			const float total_content_w = has_icon ? ( icon_sz + icon_gap + tw ) : tw;
			const float start_x = std::floor( bx + ( btn_w - total_content_w ) * 0.5f );
			const float label_y = std::floor( y + ( bar_h - th ) * 0.5f );

			if ( has_icon )
			{
				const float icon_y = std::floor( y + ( bar_h - icon_sz ) * 0.5f );
				dl.image( start_x, icon_y, icon_sz, icon_sz, this->m_textures.tabs[ i ].resource.Get( ), text_col );
				dl.text( start_x + icon_sz + icon_gap, label_y, k_tab_names[ i ], text_col );
			}
			else
			{
				dl.text( start_x, label_y, k_tab_names[ i ], text_col );
			}

			// Divider between tabs (skip first)
			if ( i > 0 )
				dl.rect_filled( bx, y + 4.0f, 1.0f, bar_h - 8.0f, tokens::col_line );
		}
	}

	// ─────────────────────────────────────────────────────────────────────────
	//  LinoriaLib-style subtab bar (second row, same pattern but smaller)
	// ─────────────────────────────────────────────────────────────────────────
	void menu::draw_linoria_subtab_bar( float x, float y, float w )
	{
		auto& dl         = xui::draw::current( );
		const auto& inp  = xui::ctx( ).input;
		const auto bar_h = tokens::subtab_bar_h;
		const auto& def  = k_subtab_defs[ this->m_tab ];
		const auto count = def.count;

		if ( count <= 1 )
			return;

		// Background — slightly lighter than title bar
		dl.rect_filled( x, y, w, bar_h, tokens::col_card );
		// Bottom separator
		dl.rect_filled( x, y + bar_h - 1.0f, w, 1.0f, tokens::col_border );

		const auto btn_w = w / static_cast<float>( count );

		// Smoothly lerp sliding indicator to active subtab
		const float target_sub_x = x + btn_w * static_cast<float>( this->m_subtab );
		const float target_sub_w = btn_w;
		const float dt = xdraw::delta_time( );
		if ( this->m_subtab_indicator_x < 0.0f )
		{
			this->m_subtab_indicator_x = target_sub_x;
			this->m_subtab_indicator_w = target_sub_w;
		}
		else
		{
			this->m_subtab_indicator_x += ( target_sub_x - this->m_subtab_indicator_x ) * std::min( 22.0f * dt, 1.0f );
			this->m_subtab_indicator_w += ( target_sub_w - this->m_subtab_indicator_w ) * std::min( 22.0f * dt, 1.0f );
		}

		// Active sub-tab: faint light pill
		dl.rect_filled( this->m_subtab_indicator_x + 2.0f, y + 3.0f, this->m_subtab_indicator_w - 4.0f, bar_h - 6.0f,
			xdraw::color{ 255, 255, 255, 18 }, xdraw::corner_radius{ 6.0f } );

		char upper[ 32 ]{};
		for ( auto i = 0; i < count; ++i )
		{
			const auto bx      = x + btn_w * static_cast<float>( i );
			const auto btn     = xui::rect{ bx, y, btn_w, bar_h };
			const auto hovered = inp.in_rect( btn );
			const auto active  = ( this->m_subtab == i );

			if ( hovered && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
				this->m_subtab = i;

			const auto act_a = xui::anim::lerp( xui::fnv1a( "lstab_act" ) + i + this->m_tab * 16,
				active ? 1.0f : 0.0f, 14.0f );
			const auto hov_a = xui::anim::lerp( xui::fnv1a( "lstab_hov" ) + i + this->m_tab * 16,
				( hovered && !active ) ? 1.0f : 0.0f, 14.0f );

			if ( hov_a > 0.01f )
				dl.rect_filled( bx, y, btn_w, bar_h,
					tokens::col_elevated.alpha( static_cast<std::uint8_t>( 60.0f * hov_a ) ) );

			// Uppercase name
			auto len = 0;
			while ( def.names[ i ][ len ] && len < 31 )
			{
				const auto c = def.names[ i ][ len ];
				upper[ len ] = ( c >= 'a' && c <= 'z' ) ? static_cast<char>( c - 32 ) : c;
				++len;
			}
			upper[ len ] = '\0';

			const auto [tw, th] = xdraw::measure_text( upper );
			const auto tx = std::floor( bx + ( btn_w - tw ) * 0.5f );
			const auto ty = std::floor( y  + ( bar_h - th ) * 0.5f );
			dl.text( tx, ty, upper, xui::lerp( tokens::col_text_dim, tokens::col_accent, act_a ) );

			if ( i > 0 )
				dl.rect_filled( bx, y + 3.0f, 1.0f, bar_h - 6.0f, tokens::col_line );
		}
	}

	void menu::draw_status_bar( float x, float y, float w, float h )
	{
		auto& dl = xui::draw::current( );

		// 1. Background (rounded bottom so it doesn't square off the window's corners)
		dl.rect_filled( x, y, w, h, tokens::col_title_bar, xdraw::corner_radius{ 0.0f, 0.0f, tokens::round_lg, tokens::round_lg } );
		// Top border
		dl.rect_filled( x, y, w, 1.0f, tokens::col_border );

		// Status information
		const auto local = systems::g_local.get( );
		const bool in_game = local.is_valid( );

		// Status pill in bottom-left
		constexpr float dot_sz = 6.0f;
		const float dot_x = x + 10.0f;
		const float dot_y = y + ( h - dot_sz ) * 0.5f;
		const auto status_col = in_game ? xdraw::color{ 74, 222, 128, 255 } : tokens::col_accent;
		dl.rect_filled( dot_x, dot_y, dot_sz, dot_sz, status_col, xdraw::corner_radius{ tokens::round_md } );

		const char* status_text = in_game ? "IN-GAME" : "READY";
		const auto [stw, sth] = xdraw::measure_text( status_text );
		dl.text( dot_x + dot_sz + 6.0f, y + ( h - sth ) * 0.5f, status_text, tokens::col_text_dim );

		// Realtime FPS
		const auto fps = xdraw::framerate( );
		char fps_buf[ 32 ]{};
		std::snprintf( fps_buf, sizeof( fps_buf ), "%.0f FPS", fps );
		const auto [fw, fh] = xdraw::measure_text( fps_buf );
		const float fps_x = dot_x + dot_sz + 6.0f + stw + 16.0f;
		dl.text( fps_x, y + ( h - fh ) * 0.5f, fps_buf, tokens::col_text );

		// Ping if in game
		if ( in_game && local.is_alive && local.controller && systems::g_entities.exists( local.controller ) )
		{
			const auto ping = memory::read<std::uint32_t>( local.controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
			char ping_buf[ 32 ]{};
			std::snprintf( ping_buf, sizeof( ping_buf ), "%u ms", ping );
			const auto [pw, ph] = xdraw::measure_text( ping_buf );
			dl.text( fps_x + fw + 14.0f, y + ( h - ph ) * 0.5f, ping_buf, tokens::col_text_dim );
		}

		// Center: current tab & subtab path
		static constexpr const char* k_tab_names[] = {
			"Rage", "Legit", "Player", "World", "Skins", "Misc", "Config"
		};
		char path_buf[ 64 ]{};
		const auto& def = k_subtab_defs[ this->m_tab ];
		if ( def.count > 1 && this->m_subtab < def.count )
		{
			std::snprintf( path_buf, sizeof( path_buf ), "%s  /  %s", k_tab_names[ this->m_tab ], def.names[ this->m_subtab ] );
		}
		else
		{
			std::snprintf( path_buf, sizeof( path_buf ), "%s", k_tab_names[ this->m_tab ] );
		}
		const auto [path_w, path_h] = xdraw::measure_text( path_buf );
		dl.text( std::floor( x + ( w - path_w ) * 0.5f ), y + ( h - path_h ) * 0.5f, path_buf, tokens::col_accent );
	}


} // namespace rendering
