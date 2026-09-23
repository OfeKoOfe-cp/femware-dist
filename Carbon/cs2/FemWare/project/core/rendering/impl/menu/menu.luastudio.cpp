#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>
#include <core/resources/workspace.hpp>

#include "../../rendering.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <shellapi.h>

namespace rendering {

	namespace detail {
		xdraw::color console_tag_color( const std::string& tag )
		{
			if ( tag == "SUCCESS" || tag == "LOAD" || tag == "SAVE" || tag == "EXEC" )
			{
				return xdraw::color{ 74, 222, 128, 255 };
			}
			if ( tag == "RUNTIME ERROR" || tag == "SYNTAX ERROR" || tag == "ERROR" || tag == "DELETE" )
			{
				return xdraw::color{ 248, 113, 113, 255 };
			}
			if ( tag == "SCRIPT" )
			{
				return xdraw::color{ 96, 165, 250, 255 };
			}
			if ( tag == "DOC" )
			{
				return xdraw::color{ 167, 139, 250, 255 };
			}
			return tokens::col_text_dim;
		}
	}

	void menu::draw_lua_studio( float sw, float sh )
	{
		( void )sw;
		( void )sh;
		if ( !this->m_lua_initialized )
		{
			this->init_lua_studio( );
		}

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_lua", this->m_lua_x, this->m_lua_y, this->m_lua_w, this->m_lua_h, false, 500.0f, 400.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_lua_x;
		const auto wy = this->m_lua_y;
		const auto ww = this->m_lua_w;
		const auto wh = this->m_lua_h;
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

		// ── Title Bar: Minimalist title + Close button ─────────
		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "LUA EDITOR" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

			dl.text( wx + 12.0f, cy, "LUA EDITOR", tokens::col_accent );

			// Close button [X]
			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_lua = false;
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

		// Entire body height is now dedicated to editor and sidebar!
		const float upper_h = body_h;

		const float sidebar_w = 195.0f;
		const float editor_w  = body_w - sidebar_w - pad;
		const float editor_x  = body_x + sidebar_w + pad;

		const char* lua_active_tip = nullptr;
		float lua_active_tip_x = 0.0f;
		float lua_active_tip_y = 0.0f;

		auto draw_icon_btn = [&]( ID3D11ShaderResourceView* icon, float bx, float by, float bw, float bh, const char* tooltip, bool is_primary = false, bool is_danger = false ) -> bool {
			const xui::rect r{ bx, by, bw, bh };
			const bool hov = inp.in_rect( r );
			const bool clicked = hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( );

			auto& bdl = xui::draw::current( );
			if ( is_primary )
			{
				// Primary action uses the theme gradient (was off-theme green).
				const auto c1 = hov ? tokens::col_accent : tokens::col_accent.alpha( 225 );
				const auto c2 = hov ? tokens::col_accent_2 : tokens::col_accent_2.alpha( 225 );
				bdl.rect_filled_gradient( bx, by, bw, bh, c1, c2, c2, c1, xdraw::corner_radius{ tokens::round_md } );
				bdl.rect( bx, by, bw, bh, tokens::col_accent_2.alpha( 200 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
			}
			else if ( is_danger && hov )
			{
				bdl.rect_filled( bx, by, bw, bh, xdraw::color{ 239, 68, 68, 80 }, xdraw::corner_radius{ tokens::round_md } );
				bdl.rect( bx, by, bw, bh, xdraw::color{ 248, 113, 113, 255 }, xdraw::corner_radius{ tokens::round_md }, 1.0f );
			}
			else if ( hov )
			{
				bdl.rect_filled( bx, by, bw, bh, tokens::col_elevated, xdraw::corner_radius{ tokens::round_md } );
				bdl.rect( bx, by, bw, bh, tokens::col_accent.alpha( 220 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
			}
			else
			{
				bdl.rect_filled( bx, by, bw, bh, tokens::col_card, xdraw::corner_radius{ tokens::round_md } );
				bdl.rect( bx, by, bw, bh, tokens::col_border, xdraw::corner_radius{ tokens::round_md }, 1.0f );
			}

			if ( icon )
			{
				constexpr float ico_sz = 12.0f;
				const float ix = std::floor( bx + ( bw - ico_sz ) * 0.5f );
				const float iy = std::floor( by + ( bh - ico_sz ) * 0.5f );
				const auto content_col = is_primary ? xdraw::color{ 255, 255, 255, 255 } : ( is_danger && hov ? xdraw::color{ 248, 113, 113, 255 } : ( hov ? tokens::col_accent : tokens::col_text_dim ) );
				bdl.image( ix, iy, ico_sz, ico_sz, icon, content_col );
			}

			if ( hov && tooltip )
			{
				lua_active_tip = tooltip;
				lua_active_tip_x = bx + bw * 0.5f;
				lua_active_tip_y = by - 24.0f;
			}

			return clicked;
		};

		// ── Left Sidebar: Saved In-Folder Scripts ───
		xui::layout::set_cursor( body_x - wx, body_y - wy );
		if ( xui::begin_child( "##lua_sidebar_col", sidebar_w, upper_h, false ) )
		{
			rendering::group_header( "SCRIPTS" );

			const auto avail_w = xui::layout::avail( ).first;

			// Minimalist icon-only action bar: [+] and [reload]
			constexpr float act_btn_sz = 22.0f;
			constexpr float act_gap = 4.0f;
			const auto act_bar = xui::layout::item( avail_w, act_btn_sz );

			if ( draw_icon_btn( this->m_textures.cfg_plus.resource.Get( ), act_bar.x, act_bar.y, act_btn_sz, act_btn_sz, "New Script" ) )
			{
				const std::string new_fname = "script_" + std::to_string( this->m_lua_scripts.size( ) + 1 ) + ".lua";
				const auto full_p = workspace::scripts( ) / new_fname;
				this->m_lua_editor_lines = {
					"--     ______             _       __                 ",
					"--    / ____/__  ____ ___| |     / /___ _________ ",
					"--   / /_  / _ \\/ __ `__ \\ | /| / / __ `/ ___/ _ \\",
					"--  / __/ /  __/ / / / / / |/ |/ / /_/ / /  /  __/",
					"-- /_/    \\___/_/ /_/ /_/|__/|__/\\__,_/_/   \\___/ "
				};
				this->save_lua_script( full_p.string( ) );
				this->refresh_lua_scripts( );
				for ( size_t idx = 0; idx < this->m_lua_scripts.size( ); ++idx )
				{
					if ( this->m_lua_scripts[ idx ].name == new_fname )
					{
						this->m_lua_selected_script = static_cast< int >( idx );
						break;
					}
				}
			}

			if ( draw_icon_btn( this->m_textures.lua_reload.resource.Get( ), act_bar.x + act_btn_sz + act_gap, act_bar.y, act_btn_sz, act_btn_sz, "Reload Scripts Folder" ) )
			{
				this->refresh_lua_scripts( );
				this->append_lua_log( "SYSTEM", "Reloaded scripts from the FemWare workspace.", tokens::col_accent );
			}

			xui::layout::new_line( );
			xui::layout::spacing( 4.0f );

			// Scrollable list of in-folder scripts
			const float list_h = upper_h - 60.0f;
			if ( xui::begin_child( "##lua_file_list", avail_w, list_h, true ) )
			{
				for ( size_t i = 0; i < this->m_lua_scripts.size( ); ++i )
				{
					const auto& item = this->m_lua_scripts[ i ];
					const bool is_sel = ( this->m_lua_selected_script == static_cast< int >( i ) );
					const auto row_w = xui::layout::avail( ).first;
					const auto row = xui::layout::item( row_w, 22.0f );
					const bool hov = inp.in_rect( row );

					auto& cdl = xui::draw::current( );
					if ( is_sel )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 45 ), xdraw::corner_radius{ tokens::round_md } );
						cdl.rect( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 180 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
					}
					else if ( hov )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated, xdraw::corner_radius{ tokens::round_md } );
					}

					// Lua icon next to script name
					constexpr float ico_sz = 13.0f;
					const float ico_y = std::floor( row.y + ( row.h - ico_sz ) * 0.5f );
					const auto lua_ico_col = is_sel ? tokens::col_accent : ( hov ? tokens::col_text : tokens::col_text_dim );
					if ( this->m_textures.lua_icon.resource )
					{
						cdl.image( row.x + 5.0f, ico_y, ico_sz, ico_sz, this->m_textures.lua_icon.resource.Get( ), lua_ico_col );
					}

					// Delete icon button on far right when hovered
					constexpr float del_sz = 18.0f;
					const float del_x = row.x + row.w - del_sz - 2.0f;
					const float del_y = std::floor( row.y + ( row.h - del_sz ) * 0.5f );
					bool del_clicked = false;
					if ( hov )
					{
						del_clicked = draw_icon_btn( this->m_textures.lua_clear.resource.Get( ), del_x, del_y, del_sz, del_sz, "Delete Script", false, true );
					}

					// Script name
					const float max_text_w = std::max( 10.0f, ( hov ? del_x : ( row.x + row.w ) ) - ( row.x + 23.0f ) );
					cdl.push_clip( row.x + 23.0f, row.y, max_text_w, row.h );
					cdl.text( row.x + 23.0f, row.y + 3.0f, item.name, is_sel ? tokens::col_accent : ( hov ? tokens::col_text : tokens::col_text_dim ) );
					cdl.pop_clip( );

					if ( del_clicked )
					{
						std::error_code ec{};
						std::filesystem::remove( item.path, ec );
						this->refresh_lua_scripts( );
						if ( this->m_lua_selected_script >= static_cast< int >( this->m_lua_scripts.size( ) ) )
						{
							this->m_lua_selected_script = static_cast< int >( this->m_lua_scripts.size( ) ) - 1;
						}
						this->append_lua_log( "DELETE", "Deleted script: " + item.name, xdraw::color{ 239, 68, 68, 255 } );
						break;
					}
					else if ( hov && !del_clicked && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						this->m_lua_selected_script = static_cast< int >( i );
						this->load_lua_script( item.path );
					}
				}
				xui::end_child( );
			}

			xui::end_child( );
		}

		// ── Right Canvas: Minimalist Lua Editor ─────────────────────────────
		xui::layout::set_cursor( editor_x - wx, body_y - wy );
		if ( xui::begin_child( "##lua_editor_col", editor_w, upper_h, false ) )
		{
			const auto avail_w = xui::layout::avail( ).first;

			// 1. Top toolbar: filename + line count left, grouped action pill right.
			const float top_bar_h = 28.0f;
			const auto top_bar = xui::layout::item( avail_w, top_bar_h );
			auto& cdl_top = xui::draw::current( );

			cdl_top.rect_filled( top_bar.x, top_bar.y, top_bar.w, top_bar.h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_md } );
			cdl_top.rect( top_bar.x, top_bar.y, top_bar.w, top_bar.h, tokens::col_border, xdraw::corner_radius{ tokens::round_md }, 1.0f );

			const std::string active_name = ( this->m_lua_selected_script >= 0 && this->m_lua_selected_script < static_cast< int >( this->m_lua_scripts.size( ) ) )
				? this->m_lua_scripts[ this->m_lua_selected_script ].name : "buffer.lua";
			const auto [nw, nh] = xdraw::measure_text( active_name );
			const auto text_y = std::floor( top_bar.y + ( top_bar.h - nh ) * 0.5f );
			cdl_top.text( top_bar.x + 10.0f, text_y, active_name, tokens::col_text );
			const std::string lines_tag = "  (" + std::to_string( this->m_lua_editor_lines.size( ) ) + " lines)";
			cdl_top.text( top_bar.x + 10.0f + nw, text_y, lines_tag, tokens::col_text_dim );

			xui::layout::new_line( );
			xui::layout::spacing( 4.0f );

			// 2. Code scroll viewport (Takes all available vertical space)
			// Leave room for the inline line-edit bar and the bottom action bar.
			const float code_view_h = std::max( 70.0f, upper_h - top_bar_h - 38.0f - 64.0f );
			if ( xui::begin_child( "##lua_code_scroll", avail_w, code_view_h, true ) )
			{
				auto& cdl = xui::draw::current( );
				constexpr float line_h = 17.0f;
				constexpr float gutter_w = 34.0f;

				float art_x = 0.0f, art_y = 0.0f;
				bool have_art_pos = false;
				for ( size_t line_idx = 0; line_idx < this->m_lua_editor_lines.size( ); ++line_idx )
				{
					const auto& line_str = this->m_lua_editor_lines[ line_idx ];
					const auto row = xui::layout::item( avail_w, line_h );
					if ( !have_art_pos ) { art_x = row.x; art_y = row.y; have_art_pos = true; }
					const bool is_active_line = ( this->m_lua_cursor_line == static_cast< int >( line_idx ) );
					const bool hov = inp.in_rect( row );

					const int sel_a = ( this->m_lua_sel_start < this->m_lua_sel_end ) ? this->m_lua_sel_start : this->m_lua_sel_end;
					const int sel_b = ( this->m_lua_sel_start < this->m_lua_sel_end ) ? this->m_lua_sel_end : this->m_lua_sel_start;
					const bool in_sel = ( this->m_lua_sel_start >= 0 && this->m_lua_sel_end >= 0
						&& static_cast< int >( line_idx ) >= sel_a && static_cast< int >( line_idx ) <= sel_b );
					if ( in_sel )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 60 ) );
					}

					if ( is_active_line )
					{
						const auto t = static_cast< float >( GetTickCount64( ) ) * 0.001f;
						const auto pulse = 0.5f + 0.5f * std::sinf( t * 3.2f );

						// Soft pulsing row wash.
						cdl.rect_filled( row.x, row.y, row.w, row.h,
							tokens::col_accent.alpha( static_cast< std::uint8_t >( 18.0f + 28.0f * pulse ) ) );

						// Bloom behind the caret bar.
						cdl.rect_filled( row.x, row.y, 5.0f, row.h,
							tokens::col_accent.alpha( static_cast< std::uint8_t >( 40.0f + 45.0f * pulse ) ) );

						// Animated magenta bar — width and brightness breathe.
						cdl.rect_filled( row.x, row.y, 2.2f + 1.2f * pulse, row.h, tokens::col_accent );
					}
					else if ( hov )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated.alpha( 50 ) );
					}

					char num_buf[ 16 ];
					std::snprintf( num_buf, sizeof( num_buf ), "%02zu", line_idx + 1 );
					cdl.text( row.x + 4.0f, row.y, num_buf, is_active_line ? tokens::col_accent : tokens::col_text_dim.alpha( 120 ) );
					cdl.rect_filled( row.x + gutter_w - 2.0f, row.y, 1.0f, line_h, tokens::col_border );

										// Monaco (VS Code Dark+) style per-token Lua highlighting.
					{
						const auto keyword_col = xdraw::color{ 197, 134, 192, 255 };
						const auto ident_col   = xdraw::color{ 156, 220, 254, 255 };
						const auto func_col    = xdraw::color{ 220, 220, 170, 255 };
						const auto string_col  = xdraw::color{ 206, 145, 120, 255 };
						const auto number_col  = xdraw::color{ 181, 206, 168, 255 };
						const auto comment_col = xdraw::color{ 106, 153,  85, 255 };
						const auto plain_col   = xdraw::color{ 212, 212, 212, 255 };

						const auto is_digit = []( char ch ) { return ch >= '0' && ch <= '9'; };
						const auto is_alpha = []( char ch ) { return ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ch == '_'; };
						const auto is_word  = [ & ]( char ch ) { return is_alpha( ch ) || is_digit( ch ); };
						const auto is_space = []( char ch ) { return ch == ' ' || ch == '\t'; };
						const auto is_keyword = []( const std::string& w )
							{
								static const char* kws[] = { "local","function","end","if","then","else","elseif","for","while","do","return","break","repeat","until","and","or","not","in","nil","true","false" };
								for ( const auto* k : kws ) { if ( w == k ) return true; }
								return false;
							};

						float px = row.x + gutter_w + 6.0f;
						const auto emit = [ & ]( const std::string& t, const xdraw::color& col )
							{
								if ( !t.empty( ) ) { cdl.text( px, row.y, t, col ); }
								px += xdraw::measure_text( t ).first;
							};

						for ( std::size_t i = 0; i < line_str.size( ); )
						{
							const char ch = line_str[ i ];

							if ( ch == '-' && i + 1 < line_str.size( ) && line_str[ i + 1 ] == '-' )
							{
								emit( line_str.substr( i ), comment_col );
								break;
							}
							if ( ch == '"' || ch == '\'' )
							{
								std::size_t j = i + 1;
								while ( j < line_str.size( ) && line_str[ j ] != ch )
								{
									if ( line_str[ j ] == '\\' && j + 1 < line_str.size( ) ) { ++j; }
									++j;
								}
								if ( j < line_str.size( ) ) { ++j; }
								emit( line_str.substr( i, j - i ), string_col );
								i = j;
								continue;
							}
							if ( is_digit( ch ) )
							{
								std::size_t j = i;
								while ( j < line_str.size( ) && ( is_digit( line_str[ j ] ) || line_str[ j ] == '.' ) ) { ++j; }
								emit( line_str.substr( i, j - i ), number_col );
								i = j;
								continue;
							}
							if ( is_alpha( ch ) )
							{
								std::size_t j = i;
								while ( j < line_str.size( ) && ( is_word( line_str[ j ] ) || line_str[ j ] == '.' ) ) { ++j; }
								const auto w = line_str.substr( i, j - i );
								auto col = ident_col;
								if ( is_keyword( w ) ) { col = keyword_col; }
								else if ( w.find( '.' ) != std::string::npos || ( j < line_str.size( ) && line_str[ j ] == '(' ) ) { col = func_col; }
								emit( w, col );
								i = j;
								continue;
							}
							if ( is_space( ch ) )
							{
								std::size_t j = i;
								while ( j < line_str.size( ) && is_space( line_str[ j ] ) ) { ++j; }
								emit( line_str.substr( i, j - i ), plain_col );
								i = j;
								continue;
							}

							emit( std::string( 1, ch ), plain_col );
							++i;
						}
					}

					if ( this->m_lua_sel_dragging && inp.mouse_down && hov )
					{
						this->m_lua_sel_end = static_cast< int >( line_idx );
					}
					if ( hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						const int li = static_cast< int >( line_idx );
						if ( inp.shift_held( ) && this->m_lua_sel_start >= 0 )
						{
							this->m_lua_sel_end = li;
						}
						else
						{
							this->m_lua_sel_start = li;
							this->m_lua_sel_end = li;
							this->m_lua_sel_dragging = true;
						}
						this->m_lua_cursor_line = li;
						this->m_lua_line_edit_buf = line_str;
					}
				}

				// Empty-buffer art: shown when the script has no real code
				// (blank lines / comment-only count as empty).
				const bool lua_empty = std::all_of(
					this->m_lua_editor_lines.begin( ), this->m_lua_editor_lines.end( ),
					[]( const std::string& l )
					{
						const auto first = l.find_first_not_of( " \t" );
						if ( first == std::string::npos ) return true;
						return l.compare( first, 2, "--" ) == 0;
					} );

				if ( lua_empty && have_art_pos )
				{
					static const char* fw_art[ 5 ] = {
						" _____           __        __",
						"|  ___|__ _ __ __\\ \\      / /_ _ _ __ ___",
						"| |_ / _ \\ '_ ` _ \\ \\ /\\ / / _` | '__/ _ \\",
						"|  _|  __/ | | | | \\ V  V / (_| | | |  __/",
						"|_|  \\___|_| |_| |_|\\_/\\_/ \\__,_|_|  \\___|"
					};

					xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );

					float art_max_w = 0.0f, art_lh = 0.0f;
					for ( const auto* a : fw_art )
					{
						const auto [aw, ah] = xdraw::measure_text( a );
						art_max_w = ( std::max )( art_max_w, aw );
						art_lh = ah;
					}

					const float ax = art_x + ( std::max )( 0.0f, ( avail_w - art_max_w ) * 0.5f );
					const float ay = art_y + 30.0f;
					for ( int i = 0; i < 5; ++i )
					{
						const auto [lw, lh] = xdraw::measure_text( fw_art[ i ] );
						cdl.text( ax + ( art_max_w - lw ) * 0.5f, ay + static_cast< float >( i ) * ( art_lh + 2.0f ),
							fw_art[ i ], tokens::col_text_dim.alpha( 90 ) );
					}

					xdraw::pop_font( );
				}

				xui::end_child( );

				// ── Standard text editing: line selection + clipboard ──
				auto lua_set_clipboard = []( const std::string& s )
				{
					if ( OpenClipboard( nullptr ) )
					{
						EmptyClipboard( );
						const auto len = s.size( ) + 1;
						const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
						if ( hMem ) { memcpy( GlobalLock( hMem ), s.c_str( ), len ); GlobalUnlock( hMem ); SetClipboardData( CF_TEXT, hMem ); }
						CloseClipboard( );
					}
				};
				auto lua_get_clipboard = []( ) -> std::string
				{
					std::string out;
					if ( OpenClipboard( nullptr ) )
					{
						if ( const auto h = GetClipboardData( CF_TEXT ) )
						{
							if ( const auto p = static_cast< const char* >( GlobalLock( h ) ) )
							{
								out = p;
								GlobalUnlock( h );
							}
						}
						CloseClipboard( );
					}
					return out;
				};

				if ( inp.mouse_released )
				{
					this->m_lua_sel_dragging = false;
				}

				const bool editor_hovered = inp.mouse_x >= editor_x && inp.mouse_x <= editor_x + editor_w
					&& inp.mouse_y >= body_y && inp.mouse_y <= body_y + upper_h;
				if ( editor_hovered )
				{
					const bool ctrl = inp.ctrl_held( );
					for ( int vk : inp.key_presses( ) )
					{
						int a = this->m_lua_cursor_line;
						int b = this->m_lua_cursor_line;
						const bool has_sel = ( this->m_lua_sel_start >= 0 && this->m_lua_sel_end >= 0 );
						if ( has_sel )
						{
							a = std::min( this->m_lua_sel_start, this->m_lua_sel_end );
							b = std::max( this->m_lua_sel_start, this->m_lua_sel_end );
						}

						if ( ctrl && vk == 'C' )
						{
							std::string txt;
							for ( int i = a; i <= b && i < static_cast< int >( this->m_lua_editor_lines.size( ) ); ++i )
							{
								txt += this->m_lua_editor_lines[ i ] + "\r\n";
							}
							lua_set_clipboard( txt );
						}
						else if ( ctrl && vk == 'V' )
						{
							const auto raw = lua_get_clipboard( );
							std::vector<std::string> ins;
							std::string cur;
							for ( char c : raw )
							{
								if ( c == '\n' ) { if ( !cur.empty( ) && cur.back( ) == '\r' ) { cur.pop_back( ); } ins.push_back( cur ); cur.clear( ); }
								else if ( c != '\r' ) { cur.push_back( c ); }
							}
							if ( !cur.empty( ) ) { ins.push_back( cur ); }
							if ( ins.empty( ) ) { continue; }

							const int cnt = static_cast< int >( this->m_lua_editor_lines.size( ) );
							if ( a < 0 ) { a = 0; }
							if ( a > cnt ) { a = cnt; }
							int eb = has_sel ? b : a - 1;
							if ( eb >= cnt ) { eb = cnt - 1; }
							if ( eb >= a && a < cnt )
							{
								this->m_lua_editor_lines.erase( this->m_lua_editor_lines.begin( ) + a, this->m_lua_editor_lines.begin( ) + eb + 1 );
							}
							this->m_lua_editor_lines.insert( this->m_lua_editor_lines.begin( ) + a, ins.begin( ), ins.end( ) );
							this->m_lua_cursor_line = a;
							this->m_lua_sel_start = a;
							this->m_lua_sel_end = a + static_cast< int >( ins.size( ) ) - 1;
						}
						else if ( ctrl && vk == 'A' )
						{
							if ( !this->m_lua_editor_lines.empty( ) )
							{
								this->m_lua_sel_start = 0;
								this->m_lua_sel_end = static_cast< int >( this->m_lua_editor_lines.size( ) ) - 1;
							}
						}
						else if ( ( vk == VK_DELETE || vk == VK_BACK ) && has_sel )
						{
							const int cnt = static_cast< int >( this->m_lua_editor_lines.size( ) );
							int ea = std::min( this->m_lua_sel_start, this->m_lua_sel_end );
							int eb2 = std::max( this->m_lua_sel_start, this->m_lua_sel_end );
							if ( ea < 0 ) { ea = 0; }
							if ( eb2 >= cnt ) { eb2 = cnt - 1; }
							if ( eb2 >= ea && ea < cnt )
							{
								this->m_lua_editor_lines.erase( this->m_lua_editor_lines.begin( ) + ea, this->m_lua_editor_lines.begin( ) + eb2 + 1 );
								if ( this->m_lua_editor_lines.empty( ) ) { this->m_lua_editor_lines.push_back( "" ); }
								this->m_lua_cursor_line = std::min( ea, static_cast< int >( this->m_lua_editor_lines.size( ) ) - 1 );
								this->m_lua_sel_start = -1;
								this->m_lua_sel_end = -1;
							}
						}
					}
				}

				// Scrollbar
				const auto code_id = xui::make_id( "##lua_code_scroll" );
				auto& c_ctx = xui::ctx( );
				const auto it_ch = c_ctx.child_height_cache.find( code_id );
				const auto it_sc = c_ctx.child_scroll_cache.find( code_id );
				if ( it_ch != c_ctx.child_height_cache.end( ) && it_sc != c_ctx.child_scroll_cache.end( ) )
				{
					const float true_h = it_ch->second;
					if ( true_h > code_view_h )
					{
						auto& cdl_sc = xui::draw::current( );
						const float track_w = 4.0f;
						const float track_pad_y = 6.0f;
						const float track_x = editor_x + editor_w - track_w - 7.0f;
						const float track_y = body_y + top_bar_h + 8.0f + track_pad_y;
						const float track_h = code_view_h - track_pad_y * 2.0f;
						const float max_s = true_h - code_view_h;
						const float thumb_h = std::clamp( track_h * ( code_view_h / true_h ), 20.0f, track_h );
						const float thumb_y = track_y + ( it_sc->second.scroll / max_s ) * ( track_h - thumb_h );

						cdl_sc.rect_filled( track_x, track_y, track_w, track_h, tokens::col_dark.alpha( 140 ), xdraw::corner_radius{ tokens::round_md } );
						cdl_sc.rect_filled( track_x, thumb_y, track_w, thumb_h, tokens::col_accent.alpha( 220 ), xdraw::corner_radius{ tokens::round_md } );
						cdl_sc.rect( track_x - 1.0f, thumb_y - 1.0f, track_w + 2.0f, thumb_h + 2.0f, tokens::col_accent.alpha( 90 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
					}
				}
			}

			xui::layout::new_line( );
			xui::layout::spacing( 3.0f );


			// 4. Bottom action bar (text buttons)
			xui::layout::new_line( );
			xui::layout::spacing( 5.0f );
			{
				const auto avail = xui::layout::avail( ).first;
				constexpr float bar_gap = 4.0f;
				const float bwidth = std::max( 40.0f, ( avail - bar_gap * 4.0f ) / 5.0f );

				if ( xui::button( "Run##lua_bottom", bwidth, 22.0f ) )
				{
					this->execute_lua_buffer( );
				}
				xui::layout::same_line( );
				if ( xui::button( "Save##lua_bottom", bwidth, 22.0f ) )
				{
					if ( this->m_lua_selected_script >= 0 && this->m_lua_selected_script < static_cast< int >( this->m_lua_scripts.size( ) ) )
					{
						this->save_lua_script( this->m_lua_scripts[ this->m_lua_selected_script ].path );
					}
				}
				xui::layout::same_line( );
				if ( xui::button( "Copy##lua_bottom", bwidth, 22.0f ) )
				{
					std::string full_code;
					for ( const auto& ln : this->m_lua_editor_lines )
					{
						full_code += ln + "\r\n";
					}
					if ( OpenClipboard( nullptr ) )
					{
						EmptyClipboard( );
						const auto len = full_code.size( ) + 1;
						const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
						if ( hMem )
						{
							memcpy( GlobalLock( hMem ), full_code.c_str( ), len );
							GlobalUnlock( hMem );
							SetClipboardData( CF_TEXT, hMem );
						}
						CloseClipboard( );
					}
				}
				xui::layout::same_line( );
				if ( xui::button( "Clear##lua_bottom", bwidth, 22.0f ) )
				{
					this->m_lua_editor_lines = { "" };
					this->m_lua_cursor_line = 0;
					this->m_lua_line_edit_buf.clear( );
				}
				xui::layout::same_line( );
				if ( xui::button( "Docs##lua_bottom", bwidth, 22.0f ) )
				{
					ShellExecuteA( nullptr, "open", "https://github.com/OfeKoOfe-cp/femware-dist", nullptr, nullptr, SW_SHOWNORMAL );
				}
			}

			xui::end_child( );
		}

		// Tooltip overlay for icon buttons in Lua Studio
		if ( lua_active_tip )
		{
			auto& tdl = xui::draw::current( );
			const auto [tw, th] = xdraw::measure_text( lua_active_tip );
			const float pad_x = 8.0f;
			const float pad_y = 3.0f;
			const float tw_box = tw + pad_x * 2.0f;
			const float th_box = th + pad_y * 2.0f;
			const float tx = std::clamp( lua_active_tip_x - tw_box * 0.5f, wx + 4.0f, wx + ww - tw_box - 4.0f );
			const float ty = std::max( wy + 4.0f, lua_active_tip_y );

			tdl.rect_filled( tx, ty, tw_box, th_box, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_md } );
			tdl.rect( tx, ty, tw_box, th_box, tokens::col_accent.alpha( 180 ), 1.0f );
			tdl.text( tx + pad_x, ty + pad_y, lua_active_tip, tokens::col_accent );
		}

		xui::end_window( );
	}

	void menu::draw_lua_console( float sw, float sh )
	{
		( void )sw;
		( void )sh;

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_lua_console", this->m_console_x, this->m_console_y, this->m_console_w, this->m_console_h, false, 420.0f, 220.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_console_x;
		const auto wy = this->m_console_y;
		const auto ww = this->m_console_w;
		const auto wh = this->m_console_h;
		const auto& inp = xui::ctx( ).input;

		// Ambient drop shadow & glow
		for ( int s_i = 5; s_i >= 1; --s_i )
		{
			const float spread = static_cast< float >( s_i ) * 3.0f;
			const std::uint8_t a = static_cast< std::uint8_t >( 6.0f * ( 6 - s_i ) * reveal );
			dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
				xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
		}
		dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast<std::uint8_t>( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );
		dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

		// Title Bar
		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "CONSOLE" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

		dl.text( wx + 12.0f, cy, "CONSOLE", tokens::col_accent );

			// Close button [X]
			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_lua_console = false;
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

			// Title bar icons: Copy Logs & Clear
			constexpr float act_btn_sz = 16.0f;
			const float copy_x = cbtn_x - act_btn_sz - 8.0f;
			const float copy_y = cbtn_y;
			const xui::rect copy_r{ copy_x, copy_y, act_btn_sz, act_btn_sz };
			const bool copy_hov = inp.in_rect( copy_r );
			if ( copy_hov )
			{
				dl.rect_filled( copy_x, copy_y, act_btn_sz, act_btn_sz, tokens::col_elevated, xdraw::corner_radius{ tokens::round_md } );
			}
			if ( this->m_textures.lua_copy.resource )
			{
				dl.image( copy_x + 2.0f, copy_y + 2.0f, 12.0f, 12.0f, this->m_textures.lua_copy.resource.Get( ), copy_hov ? tokens::col_accent : tokens::col_text_dim );
			}
			if ( copy_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				std::string all_logs;
				for ( const auto& log : this->m_lua_logs )
				{
					all_logs += "[" + log.time + "] " + log.text + "\r\n";
				}
				if ( OpenClipboard( nullptr ) )
				{
					EmptyClipboard( );
					const auto len = all_logs.size( ) + 1;
					const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
					if ( hMem )
					{
						memcpy( GlobalLock( hMem ), all_logs.c_str( ), len );
						GlobalUnlock( hMem );
						SetClipboardData( CF_TEXT, hMem );
					}
					CloseClipboard( );
					this->append_lua_log( "SYSTEM", "Copied logs to clipboard.", tokens::col_accent );
				}
			}

			const float clr_x = copy_x - act_btn_sz - 6.0f;
			const float clr_y = cbtn_y;
			const xui::rect clr_r{ clr_x, clr_y, act_btn_sz, act_btn_sz };
			const bool clr_hov = inp.in_rect( clr_r );
			if ( clr_hov )
			{
				dl.rect_filled( clr_x, clr_y, act_btn_sz, act_btn_sz, xdraw::color{ 239, 68, 68, 70 }, xdraw::corner_radius{ tokens::round_md } );
			}
			if ( this->m_textures.lua_clear.resource )
			{
				dl.image( clr_x + 2.0f, clr_y + 2.0f, 12.0f, 12.0f, this->m_textures.lua_clear.resource.Get( ), clr_hov ? xdraw::color{ 248, 113, 113, 255 } : tokens::col_text_dim );
			}
			if ( clr_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_lua_logs.clear( );
				this->append_lua_log( "SYSTEM", "Console cleared.", tokens::col_text_dim );
			}
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

		// No command input anymore — the log list fills the body above a status footer.
		const float log_view_h = body_h - 26.0f;

		// 1. Scrollable Logs List
		xui::layout::set_cursor( body_x - wx, body_y - wy );
		if ( xui::begin_child( "##console_logs_view", body_w, log_view_h, true ) )
		{
			const auto avail_w = xui::layout::avail( ).first;
			auto& cdl = xui::draw::current( );
			constexpr float row_h = 20.0f;

			for ( const auto& log : this->m_lua_logs )
			{
				const auto row = xui::layout::item( avail_w, row_h );
				const bool hov = inp.in_rect( row );
				if ( hov )
				{
					cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated.alpha( 40 ), xdraw::corner_radius{ tokens::round_md } );
				}

				const auto tag_col = detail::console_tag_color( log.tag );
				const auto [tag_w, tag_h] = xdraw::measure_text( log.tag.c_str( ) );
				constexpr float chip_h = 15.0f;
				const float chip_w = tag_w + 10.0f;
				const float chip_y = std::floor( row.y + ( row.h - chip_h ) * 0.5f );
				cdl.rect_filled( row.x + 4.0f, chip_y, chip_w, chip_h, tag_col.alpha( 24 ), xdraw::corner_radius{ 4.0f } );
				cdl.rect( row.x + 4.0f, chip_y, chip_w, chip_h, tag_col.alpha( 75 ), xdraw::corner_radius{ 4.0f }, 1.0f );
				cdl.text( std::floor( row.x + 4.0f + ( chip_w - tag_w ) * 0.5f ), std::floor( chip_y + 2.0f ), log.tag.c_str( ), tag_col );

				const auto [time_w, time_h] = xdraw::measure_text( log.time.c_str( ) );
				const float msg_max_w = row.w - chip_w - time_w - 28.0f;

				std::string msg = log.text;
				if ( msg_max_w > 0.0f )
				{
					const auto [m0, h0] = xdraw::measure_text( msg.c_str( ) );
					if ( m0 > msg_max_w )
					{
						while ( !msg.empty( ) && msg.size( ) > 1 )
						{
							msg.pop_back( );
							const auto [wl, hl] = xdraw::measure_text( ( msg + "…" ).c_str( ) );
							if ( wl <= msg_max_w )
							{
								msg += "…";
								break;
							}
						}
					}
				}

				cdl.text( row.x + 10.0f + chip_w, row.y + 3.0f, msg.c_str( ), log.col );
				cdl.text( row.x + row.w - time_w - 6.0f, row.y + 3.0f, log.time.c_str( ), tokens::col_text_dim.alpha( 150 ) );
			}

			if ( this->m_lua_logs.empty( ) )
			{
				cdl.text( row_h * 0.25f, row_h, "console idle — run a script to see output", tokens::col_text_dim.alpha( 130 ) );
			}

			xui::end_child( );
		}

		// Auto-scroll to the newest entry whenever fresh logs arrive.
		{
			const auto cid = xui::make_id( "##console_logs_view" );
			auto& c_ctx = xui::ctx( );
			const auto it_h = c_ctx.child_height_cache.find( cid );
			const auto it_s = c_ctx.child_scroll_cache.find( cid );
			if ( it_h != c_ctx.child_height_cache.end( ) && it_s != c_ctx.child_scroll_cache.end( ) )
			{
				const float max_s = std::max( 0.0f, it_h->second - log_view_h );
				if ( this->m_lua_logs.size( ) > this->m_console_last_count )
				{
					it_s->second.scroll_target = max_s + 8192.0f;
				}
				this->m_console_last_count = this->m_lua_logs.size( );
			}
		}

		// Status footer
		{
			const float foot_y = body_y + log_view_h + 9.0f;
			dl.rect_filled( body_x, foot_y - 4.0f, body_w, 1.0f, tokens::col_line.alpha( 130 ) );
			const auto foot_text = " " + std::to_string( this->m_lua_logs.size( ) ) + " entries · scroll to read older";
			const auto [foot_w, foot_h] = xdraw::measure_text( foot_text.c_str( ) );
			dl.text( body_x + 2.0f, foot_y, foot_text.c_str( ), tokens::col_text_dim.alpha( 190 ) );
			dl.text( body_x + body_w - foot_w - 2.0f, foot_y, foot_text.c_str( ), tokens::col_accent.alpha( 170 ) );
		}

		xui::end_window( );
	}


} // namespace rendering
