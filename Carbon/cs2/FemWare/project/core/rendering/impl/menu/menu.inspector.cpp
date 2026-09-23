#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>
#include <core/resources/workspace.hpp>
#include <core/features/features.hpp>
#include <protection/game_addresses.hpp>

#include "../../rendering.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cctype>
#include <unordered_set>
#include "menu.workspaces.detail.hpp"

namespace rendering {

	void menu::draw_players_inspector( float sw, float sh )
	{
		// Bots read steam_id == 0, which the players:: flag store treats as the
		// "no selection" sentinel (get() returns empty flags, set() is a no-op),
		// so every toggle on a bot row silently died. Derive a stable non-zero
		// key from the nickname and flag it with the top bit so it can never
		// collide with a real SteamID64 value.
		const auto player_flag_key = []( const player_match_entry& e ) -> std::uint64_t
			{
				if ( e.steam_id != 0 )
				{
					return e.steam_id;
				}

				const auto h = std::hash<std::string>{}( e.nickname ) & 0x3FFFFFFFFFFFFFFFull;
				return h | 0x8000000000000000ull;
			};

		const float dt = xdraw::delta_time( );
		this->m_players_last_refresh += dt;
		if ( this->m_players_last_refresh > 2.0f || this->m_match_players.empty( ) )
		{
			this->m_players_last_refresh = 0.0f;
			this->refresh_match_players( );
		}

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_players", this->m_players_x, this->m_players_y, this->m_players_w, this->m_players_h, false, 500.0f, 400.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_players_x;
		const auto wy = this->m_players_y;
		const auto ww = this->m_players_w;
		const auto wh = this->m_players_h;
		const auto& inp = xui::ctx( ).input;

		for ( int s_i = 5; s_i >= 1; --s_i )
		{
			const float spread = static_cast< float >( s_i ) * 3.0f;
			const std::uint8_t a = static_cast< std::uint8_t >( 6.0f * ( 6 - s_i ) * reveal );
			dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
				xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
		}
		dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast<std::uint8_t>( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );

		dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "PLAYERS" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

			dl.text( wx + 12.0f, cy, "PLAYERS", tokens::col_accent );

			// Close button [X]
			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_players = false;
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

			size_t ct_count = 0, t_count = 0;
			for ( const auto& p : this->m_match_players )
			{
				if ( p.team == 3 ) ++ct_count;
				else if ( p.team == 2 ) ++t_count;
			}

			char stats_buf[ 64 ];
			std::snprintf( stats_buf, sizeof( stats_buf ), "TOTAL: %zu  |  CT: %zu  |  T: %zu", this->m_match_players.size( ), ct_count, t_count );
			const auto [stw, sth] = xdraw::measure_text( stats_buf );
		dl.text( cbtn_x - stw - 14.0f, cy, stats_buf, tokens::col_text_dim );
	}

		// Inset accent top-edge line (follows the rounded frame)
		dl.rect_filled( wx + tokens::round_lg, wy + 1.0f, ww - tokens::round_lg * 2.0f, 2.0f,
			tokens::col_accent.alpha( static_cast< std::uint8_t >( tokens::col_accent.a * reveal ) ),
			xdraw::corner_radius{ 1.0f } );

		const float pad = 8.0f;
		const float body_x = wx + pad;
		const float body_y = wy + tokens::title_bar_h + pad;
		const float body_w = ww - pad * 2.0f;
		const float body_h = wh - tokens::title_bar_h - pad * 2.0f;

xui::layout::set_cursor( body_x - wx, body_y - wy );

		// ── Outer body: filter row on top, then list + mini profile side by side ──
		if ( xui::begin_child( "##players_table_view", body_w, body_h, false ) )
		{
			auto* view = xui::layout::current_window( );
			const float view_w = view->bounds.w;
			const float view_h = view->bounds.h;

			static constexpr const char* k_team_filters[] = { "All Players", "Counter-Terrorists", "Terrorists" };
			rendering::group_subtabs( k_team_filters, 3, this->m_players_team_filter );

			xui::layout::new_line( );
			xui::layout::spacing( 4.0f );
			xui::text_input( "##players_search", this->m_players_filter, 128, "Search nickname or Steam ID..." );
			xui::layout::same_line( );
			if ( xui::button( "REFRESH", 84.0f, 22.0f ) )
			{
				this->refresh_match_players( );
			}

			xui::layout::new_line( );
			xui::layout::spacing( 6.0f );

			const float split_top = view->cursor_y;
			const float list_w = std::floor( ( view_w - tokens::col_gap ) * 0.58f );
			const float info_w = view_w - list_w - tokens::col_gap;
			const float split_h = std::max( 60.0f, view_h - split_top );

			const auto filter_lower = detail::to_lower_workspaces( this->m_players_filter );

			// ── Left: compact player list ──
			xui::layout::set_cursor( 0.0f, split_top );
			if ( xui::begin_child( "##players_list", list_w, split_h, true ) )
			{
				auto* lwin = xui::layout::current_window( );
				const float lwi = lwin->bounds.w - 8.0f;
				auto& cdl = xui::draw::current( );

				if ( this->m_match_players.empty( ) )
				{
					const char* e1 = "NO ACTIVE MATCH";
					const char* e2 = "join a match and press REFRESH";
					const auto [e1w, e1h] = xdraw::measure_text( e1 );
					const auto [e2w, e2h] = xdraw::measure_text( e2 );
					cdl.text( std::floor( lwin->bounds.x + ( lwin->bounds.w - e1w ) * 0.5f ), lwin->bounds.y + 34.0f, e1, tokens::col_text_dim );
					cdl.text( std::floor( lwin->bounds.x + ( lwin->bounds.w - e2w ) * 0.5f ), lwin->bounds.y + 54.0f, e2, tokens::col_text_dim );
				}
				else
				{
					// Compact column header (HP + flag icons share fixed columns with rows)
					const auto hrow = xui::layout::item( lwi, 18.0f );
					auto& hdl = xui::draw::current( );
					const float hy = hrow.y + 2.0f;
					hdl.text( hrow.x + 4.0f, hy, "#", tokens::col_text_dim );
					hdl.text( hrow.x + 28.0f, hy, "PLAYER", tokens::col_text_dim );
					const float right = hrow.x + hrow.w - 6.0f;
					const float flags_x = right - 68.0f;
					const float hp_align = flags_x - 8.0f;
					const auto [hpw0, hph0] = xdraw::measure_text( "HP" );
					hdl.text( hp_align - hpw0, hy, "HP", tokens::col_text_dim );
					if ( this->m_textures.flag_shield.resource )
					{
						hdl.image( flags_x + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_shield.resource.Get( ), tokens::col_text_dim );
					}
					if ( this->m_textures.flag_eye.resource )
					{
						hdl.image( flags_x + 24.0f + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_eye.resource.Get( ), tokens::col_text_dim );
					}
					if ( this->m_textures.flag_eye_off.resource )
					{
						hdl.image( flags_x + 48.0f + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_eye_off.resource.Get( ), tokens::col_text_dim );
					}

					constexpr float row_h = 26.0f;
					size_t display_idx = 0;
					for ( std::size_t i = 0; i < this->m_match_players.size( ); ++i )
					{
						const auto& p = this->m_match_players[ i ];

						if ( this->m_players_team_filter == 1 && p.team != 3 ) continue;
						if ( this->m_players_team_filter == 2 && p.team != 2 ) continue;

						if ( !filter_lower.empty( ) )
						{
							const auto name_low = detail::to_lower_workspaces( p.nickname );
							const auto id_str = std::to_string( p.steam_id );
							if ( name_low.find( filter_lower ) == std::string::npos && id_str.find( filter_lower ) == std::string::npos )
							{
								continue;
							}
						}

						++display_idx;
						const auto row = xui::layout::item( lwi, row_h );
						const bool hov = xui::ctx( ).input.in_rect( row );
						if ( hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
						{
							this->m_players_selected = static_cast< int >( i );
						}

						const auto is_sel = this->m_players_selected == static_cast< int >( i );
						const auto hover_anim = xui::anim::lerp( xui::fnv1a( "plyrow" ) + i, hov ? 1.0f : 0.0f, 14.0f );
						const auto sel_anim = xui::anim::lerp( xui::fnv1a( "plysel" ) + i, is_sel ? 1.0f : 0.0f, 10.0f );

						const auto row_bg = ( display_idx % 2 == 0 ) ? tokens::col_dark : tokens::col_card;
						cdl.rect_filled( row.x, row.y, row.w, row.h, row_bg );
						if ( sel_anim > 0.01f )
						{
							// Soft light pill, matches the dock-tab selection style.
							cdl.rect_filled( row.x, row.y, row.w, row.h, xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 34.0f * sel_anim ) } );
						}
						else if ( hover_anim > 0.01f )
						{
							cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated.alpha( static_cast< std::uint8_t >( 255.0f * hover_anim * 0.4f ) ) );
						}

						float rx = row.x + 4.0f;
						const float ry = row.y + std::floor( ( row_h - 14.0f ) * 0.5f );

						// Index
						cdl.text( rx, ry, std::to_string( display_idx ), tokens::col_text_dim );
						rx += 20.0f;

						// Tiny avatar (only when loaded; no placeholder box)
						constexpr float av_sz = 16.0f;
						const float av_y = row.y + std::floor( ( row_h - av_sz ) * 0.5f );
						if ( p.avatar_srv )
						{
							cdl.image( rx, av_y, av_sz, av_sz, p.avatar_srv, xdraw::corner_radius{ 4.0f }, xdraw::color{ 255, 255, 255, 255 } );
						}
						rx += av_sz + 4.0f;

						// Team dot
						const auto team_col = ( p.team == 3 ) ? xdraw::color{ 96, 165, 250, 255 } : ( p.team == 2 ? xdraw::color{ 251, 146, 60, 255 } : tokens::col_text_dim );
						cdl.rect_filled( rx, row.y + std::floor( ( row_h - 6.0f ) * 0.5f ), 6.0f, 6.0f, team_col, xdraw::corner_radius{ 3.0f } );
						rx += 12.0f;

						// Nickname
						std::string name_display = p.nickname;
						if ( p.is_local ) name_display += " (YOU)";
						else if ( p.is_bot ) name_display += " (BOT)";

						const auto name_col = p.is_local ? tokens::col_accent : ( p.is_alive ? tokens::col_text : tokens::col_text_dim );

						const float right = row.x + row.w - 6.0f;
						const float flags_x = right - 68.0f;
						const float hp_align = flags_x - 8.0f;
						const float name_space = hp_align - 44.0f - rx;
						while ( name_space > 12.0f && !name_display.empty( ) && xdraw::measure_text( name_display ).first > name_space )
						{
							name_display.pop_back( );
						}
						if ( name_display.size( ) < p.nickname.size( ) + 5 ) name_display += "...";
						cdl.text( rx, ry - 2.0f, name_display, xui::lerp( name_col, tokens::col_accent, sel_anim ) );

						// HP (right-aligned to its column, clear of flags)
						{
							std::string hp_str = ( !p.is_alive || p.health <= 0 ) ? "DEAD" : std::to_string( p.health );
							const auto hp_col = ( !p.is_alive || p.health <= 0 ) ? xdraw::color{ 248, 113, 113, 255 }
								: ( p.health > 50 ? xdraw::color{ 74, 222, 128, 255 } : ( p.health > 20 ? xdraw::color{ 251, 191, 36, 255 } : xdraw::color{ 248, 113, 113, 255 } ) );
							const auto [hw0, hh0] = xdraw::measure_text( hp_str );
							cdl.text( hp_align - hw0, ry, hp_str, hp_col );
						}

						// Flag toggles: lucide icons (shield = whitelist, eye = priority)
						auto p_flags = features::players::get( player_flag_key( p ) );
						const auto flag_icon = [ & ]( const textures::entry& ico, float bx, float by, bool on, const xdraw::color& on_col, std::uint32_t key ) -> bool
							{
								const xui::rect fr{ bx, by, 20.0f, 20.0f };
								const bool hov = xui::ctx( ).input.in_rect( fr );
								xui::anim::lerp( key, ( on || hov ) ? 1.0f : 0.0f, 12.0f );
								if ( ico.resource )
								{
									const auto col = on ? on_col : ( hov ? tokens::col_text : tokens::col_text_dim.alpha( 150 ) );
									cdl.image( bx + 2.0f, by + 3.0f, 16.0f, 16.0f, ico.resource.Get( ), col );
								}
								return hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( );
							};

						{
							const bool on = p_flags.whitelist( );
							if ( flag_icon( this->m_textures.flag_shield, flags_x, row.y + 3.0f, on, xdraw::color{ 74, 222, 128, 255 }, xui::fnv1a( "ply_wl" ) + i ) )
							{
								p_flags.set_whitelist( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
						{
							const bool on = p_flags.priority( );
							if ( flag_icon( this->m_textures.flag_eye, flags_x + 24.0f, row.y + 3.0f, on, xdraw::color{ 251, 191, 36, 255 }, xui::fnv1a( "ply_pr" ) + i ) )
							{
								p_flags.set_priority( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
						{
							const bool on = p_flags.no_visuals( );
							if ( flag_icon( this->m_textures.flag_eye_off, flags_x + 48.0f, row.y + 3.0f, on, xdraw::color{ 248, 113, 113, 255 }, xui::fnv1a( "ply_ne" ) + i ) )
							{
								p_flags.set_no_visuals( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
					}
				}

				xui::end_child( );
			}

			// ── Right: mini profile ──
			xui::layout::set_cursor( list_w + tokens::col_gap, split_top );
			if ( xui::begin_child( "##players_info", info_w, split_h, false ) )
			{
				auto* iwin = xui::layout::current_window( );
				auto& pdl = xui::draw::current( );
				const float px = iwin->bounds.x;
				const float py = iwin->bounds.y;
				const float pw = iwin->bounds.w;

				if ( this->m_players_selected >= 0 && this->m_players_selected < static_cast< int >( this->m_match_players.size( ) ) )
				{
					const auto& sel = this->m_match_players[ static_cast< std::size_t >( this->m_players_selected ) ];
					auto sel_flags = features::players::get( player_flag_key( sel ) );

					// Mini avatar (no placeholder border box; dim silhouette fallback)
					constexpr float av = 40.0f;
					const float avx = px + 8.0f;
					const float avy = py + 8.0f;
					if ( sel.avatar_srv )
					{
						pdl.image( avx, avy, av, av, sel.avatar_srv, xdraw::corner_radius{ tokens::round_md }, xdraw::color{ 255, 255, 255, 255 } );
					}
					else if ( this->m_textures.model_mannequin.resource )
					{
						pdl.image( avx + 6.0f, avy + 6.0f, av - 12.0f, av - 12.0f, this->m_textures.model_mannequin.resource.Get( ), tokens::col_text_dim );
					}

					// Name + team
					const float nx = avx + av + 8.0f;
					std::string nname = sel.nickname;
					if ( sel.is_local ) nname += "  (YOU)";
					pdl.text( nx, avy - 1.0f, xui::truncate( nname, pw - ( nx - px ) - 8.0f ), sel.is_local ? tokens::col_accent : tokens::col_text );
					const auto tcol = ( sel.team == 3 ) ? xdraw::color{ 96, 165, 250, 255 } : ( sel.team == 2 ? xdraw::color{ 251, 146, 60, 255 } : tokens::col_text_dim );
					pdl.text( nx, avy + 16.0f, ( sel.team == 3 ) ? "CT" : ( sel.team == 2 ? "T" : "SPEC" ), tcol );

					// Health bar under the header block
					const float hbar_y = py + 8.0f + av + 8.0f;
					const float hbw = pw - 16.0f;
					const auto hp_col = ( sel.is_alive && sel.health > 0 ) ? xdraw::color{ 74, 222, 128, 255 } : xdraw::color{ 248, 113, 113, 255 };
					pdl.rect_filled( px + 8.0f, hbar_y, hbw, 3.0f, tokens::col_card, xdraw::corner_radius{ 1.5f } );
					const float hp_ratio = ( sel.is_alive && sel.health > 0 ) ? std::clamp( sel.health / 100.0f, 0.0f, 1.0f ) : 0.0f;
					pdl.rect_filled( px + 8.0f, hbar_y, hbw * hp_ratio, 3.0f, hp_col, xdraw::corner_radius{ 1.5f } );

					// Divider
					const float div_y = hbar_y + 8.0f;
					pdl.rect_filled( px + 8.0f, div_y, pw - 16.0f, 1.0f, tokens::col_border );

					// Minimal info lines
					float row_y = div_y + 12.0f;
					const float lx = px + 8.0f;
					const float vx = lx + 78.0f;
					const auto line = [ & ]( const char* label, const char* value, const xdraw::color& vcol )
						{
							pdl.text( lx, row_y, label, tokens::col_text_dim );
							pdl.text( vx, row_y, value, vcol );
							row_y += 18.0f;
						};

					{
						char b[ 32 ];
						std::snprintf( b, sizeof( b ), "%d", sel.health );
						const auto hcol = ( sel.is_alive && sel.health > 0 ) ? xdraw::color{ 74, 222, 128, 255 } : xdraw::color{ 248, 113, 113, 255 };
						line( "HEALTH", b, hcol );
						std::snprintf( b, sizeof( b ), "%d AP%s%s", sel.armor, sel.has_helmet ? " [H]" : "", sel.has_defuser ? " [DEF]" : "" );
						line( "ARMOR", b, xdraw::color{ 147, 197, 253, 255 } );
						std::string wep = sel.weapon.empty( ) ? "knife" : sel.weapon;
						if ( !wep.empty( ) && wep[ 0 ] >= 'a' && wep[ 0 ] <= 'z' )
						{
							wep[ 0 ] = static_cast< char >( std::toupper( static_cast< unsigned char >( wep[ 0 ] ) ) );
						}
						line( "WEAPON", wep.c_str( ), tokens::col_text );
						std::snprintf( b, sizeof( b ), "$%d", sel.money );
						line( "MONEY", b, xdraw::color{ 74, 222, 128, 255 } );
						std::snprintf( b, sizeof( b ), "%d ms", sel.ping );
						line( "PING", b, tokens::col_text );
					}

					// Steam ID + ghost copy link (no box)
					if ( sel.steam_id != 0 )
					{
						char sb[ 32 ];
						std::snprintf( sb, sizeof( sb ), "%llu", sel.steam_id );
						pdl.text( lx, row_y, "STEAM", tokens::col_text_dim );
						const auto sid_str = std::string( sb );
						pdl.text( vx, row_y, xui::truncate( sid_str, ( px + pw - 4.0f ) - vx - 40.0f ), tokens::col_text_dim );
						const auto [cw0, ch0] = xdraw::measure_text( "COPY" );
						const xui::rect cpb{ px + pw - 6.0f - cw0 - 2.0f, row_y - 1.0f, cw0 + 2.0f, 16.0f };
						const bool cph = inp.in_rect( cpb );
						pdl.text( cpb.x, cpb.y - 1.0f, "COPY", cph ? tokens::col_accent : tokens::col_text_dim );
						if ( cph && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
						{
							if ( OpenClipboard( nullptr ) )
							{
								EmptyClipboard( );
								const auto len = strlen( sb ) + 1;
								const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
								if ( hMem )
								{
									memcpy( GlobalLock( hMem ), sb, len );
									GlobalUnlock( hMem );
									SetClipboardData( CF_TEXT, hMem );
								}
								CloseClipboard( );
							}
						}
						row_y += 18.0f;
					}

					// Divider
					pdl.rect_filled( px + 8.0f, row_y + 2.0f, pw - 16.0f, 1.0f, tokens::col_border );
					row_y += 10.0f;

					// Flag toggles: ghost icon rows (no filled boxes)
					const auto flag_row = [ & ]( const textures::entry& ico, const char* label, bool on, const xdraw::color& on_col, std::uint32_t key ) -> bool
						{
							const xui::rect fr{ px + 8.0f, row_y, pw - 16.0f, 22.0f };
							const bool hov = inp.in_rect( fr );
							xui::anim::lerp( key, ( on || hov ) ? 1.0f : 0.0f, 12.0f );
							if ( ico.resource )
							{
								const auto icol = on ? on_col : ( hov ? tokens::col_text : tokens::col_text_dim );
								pdl.image( fr.x + 4.0f, row_y + 3.0f, 16.0f, 16.0f, ico.resource.Get( ), icol );
							}
							pdl.text( fr.x + 26.0f, row_y + 3.0f, label, on ? tokens::col_text : ( hov ? tokens::col_text : tokens::col_text_dim ) );
							return hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( );
						};

					{
						const bool on = sel_flags.whitelist( );
						if ( flag_row( this->m_textures.flag_shield, "Whitelist (never target)", on, xdraw::color{ 74, 222, 128, 255 }, xui::fnv1a( "ply_sel_wl" ) ) )
						{
							sel_flags.set_whitelist( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}
					{
						const bool on = sel_flags.priority( );
						if ( flag_row( this->m_textures.flag_eye, "Priority target", on, xdraw::color{ 251, 191, 36, 255 }, xui::fnv1a( "ply_sel_pr" ) ) )
						{
							sel_flags.set_priority( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}
					{
						const bool on = sel_flags.no_visuals( );
						if ( flag_row( this->m_textures.flag_eye_off, "No ESP (hide from visuals)", on, xdraw::color{ 248, 113, 113, 255 }, xui::fnv1a( "ply_sel_ne" ) ) )
						{
							sel_flags.set_no_visuals( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}

					// Actions: ghost buttons (accent glow on hover, no heavy boxes)
					row_y += 4.0f;
					pdl.rect_filled( px + 8.0f, row_y + 2.0f, pw - 16.0f, 1.0f, tokens::col_border );
					row_y += 10.0f;
					const float abw = ( pw - 16.0f - 8.0f ) / 3.0f;
					const auto act = [ & ]( float bx, float bw2, const char* label, const xdraw::color& c, bool enabled ) -> bool
						{
							const xui::rect br{ bx, row_y, bw2, 24.0f };
							const bool bh = enabled && inp.in_rect( br );
							const auto [lw0, lh0] = xdraw::measure_text( label );
							const float ltx = std::floor( br.x + ( br.w - lw0 ) * 0.5f );
							const float lty = std::floor( br.y + ( br.h - lh0 ) * 0.5f );
							pdl.text( ltx, lty, label, !enabled ? tokens::col_text_dim.alpha( 90 ) : ( bh ? c : tokens::col_text_dim ) );
							if ( bh )
							{
								pdl.rect_filled( ltx, br.y + br.h - 3.0f, static_cast< float >( lw0 ), 1.0f, c.alpha( 200 ) );
							}
							return bh && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( );
						};

					const float bx0 = px + 8.0f;
					const bool can_profile = ( sel.steam_id != 0 );
					const bool can_spectate = !sel.is_local;
					const bool can_vote = !sel.is_local && !sel.is_bot;
					if ( act( bx0, abw, "PROFILE", tokens::col_accent, can_profile ) && can_profile )
					{
						steam::friends::open_profile( sel.steam_id );
					}
					if ( act( bx0 + abw + 4.0f, abw, "SPECTATE", tokens::col_accent, can_spectate ) && can_spectate )
					{
						const auto lcl = systems::g_local.get( );
						if ( lcl.is_valid( ) && !lcl.is_alive && lcl.controller )
						{
							const auto observer_pawn_handle = memory::read<std::uint32_t>( lcl.controller + SCHEMA( "CCSPlayerController", "m_hObserverPawn"_hash ) );
							if ( observer_pawn_handle )
							{
								const auto observer_pawn = systems::g_entities.lookup( observer_pawn_handle );
								if ( observer_pawn )
								{
									const auto observer_services = memory::read<std::uintptr_t>( observer_pawn + SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash ) );
									if ( observer_services )
									{
										for ( const auto& cached : systems::g_entities.get_by_type( systems::entities::type::player ) )
										{
											// Real players match on SteamID; bots (steam id 0)
											// match on the sanitized name, same source the
											// players list uses.
											const auto sid = memory::read<std::uint64_t>( cached.ptr + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
											if ( sel.steam_id != 0 )
											{
												if ( sid != sel.steam_id ) continue;
											}
											else
											{
												const auto ctrl_name_ptr = memory::read<std::uintptr_t>( cached.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
												const auto ctrl_name = ctrl_name_ptr ? memory::read_string( ctrl_name_ptr, 128 ) : std::string{};
												if ( ctrl_name != sel.nickname ) continue;
											}

											const auto target_handle = memory::read<std::uint32_t>( cached.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
											if ( target_handle )
											{
												memory::write<std::uint32_t>( observer_services + SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash ), target_handle );
											}
											break;
										}
									}
								}
							}
						}
					}
					if ( act( bx0 + ( abw + 4.0f ) * 2.0f, abw, "CALL VOTE", tokens::col_accent, can_vote ) && can_vote )
					{
						std::string kick_cmd = "callvote kick \"" + sel.nickname + "\"";
						memory::call<void>( PATTERN( patterns::engine_client_cmd ), addresses::globals::source2engine_to_client, 0, kick_cmd.c_str( ), 0x7ffef001 );
					}
				}
				else
				{
					const char* t2 = "click a row to inspect";
					const auto [t2w, t2h] = xdraw::measure_text( t2 );
					pdl.text( std::floor( px + ( pw - t2w ) * 0.5f ), py + 34.0f, t2, tokens::col_text_dim );
				}

				xui::end_child( );
			}

			xui::end_child( );
		}

		xui::end_window( );
	}


} // namespace rendering
