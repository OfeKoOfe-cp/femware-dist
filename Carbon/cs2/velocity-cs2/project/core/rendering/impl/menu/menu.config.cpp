#include <pch/pch.hpp>
#include <utilities/math/math.hpp>
#include <core/settings.hpp>
#include <shellapi.h>

#include "../../rendering.hpp"

namespace rendering {

	namespace detail {

		std::string search_buf{};
		std::string new_profile_buf{};
		std::vector<std::wstring> config_list{};
		std::wstring pending_select{};
		std::wstring active{};
		auto selected{ -1 };
		auto needs_refresh{ true };
		auto confirm_delete{ false };
		auto confirm_reset{ false };
		auto confirm_timer{ 0.0f };
		std::string status_msg{};
		auto status_timer{ 0.0f };

		static inline bool copy_to_clipboard( const std::string& text )
		{
			if ( !OpenClipboard( nullptr ) )
			{
				return false;
			}

			EmptyClipboard( );

			const auto size = ( text.size( ) + 1 ) * sizeof( char );

			auto mem = GlobalAlloc( GMEM_MOVEABLE, size );
			if ( !mem )
			{
				CloseClipboard( );
				return false;
			}

			auto dest = GlobalLock( mem );
			if ( dest )
			{
				std::memcpy( dest, text.c_str( ), size );
				GlobalUnlock( mem );
			}

			SetClipboardData( CF_TEXT, mem );
			CloseClipboard( );
			return true;
		}

		static inline std::string paste_from_clipboard( )
		{
			if ( !OpenClipboard( nullptr ) )
			{
				return {};
			}

			std::string result{};

			auto mem = GetClipboardData( CF_TEXT );
			if ( mem )
			{
				auto data = static_cast< const char* >( GlobalLock( mem ) );
				if ( data )
				{
					result = data;
					GlobalUnlock( mem );
				}
			}

			CloseClipboard( );
			return result;
		}

		static inline void wide_to_utf8( const std::wstring& wide, char* out, int out_size )
		{
			WideCharToMultiByte( CP_UTF8, 0, wide.c_str( ), -1, out, out_size, nullptr, nullptr );
		}

		static inline std::wstring utf8_to_wide( const std::string& utf8 )
		{
			wchar_t buf[ 128 ]{};
			MultiByteToWideChar( CP_UTF8, 0, utf8.c_str( ), -1, buf, 128 );
			return buf;
		}

		static inline bool config_matches_search( const std::wstring& wname )
		{
			if ( detail::search_buf.empty( ) )
			{
				return true;
			}

			char narrow[ 128 ]{};
			wide_to_utf8( wname, narrow, sizeof( narrow ) );

			std::string lower_name{ narrow };
			std::string lower_search{ detail::search_buf };

			for ( auto& c : lower_name )
			{
				c = static_cast< char >( std::tolower( c ) );
			}

			for ( auto& c : lower_search )
			{
				c = static_cast< char >( std::tolower( c ) );
			}

			return lower_name.find( lower_search ) != std::string::npos;
		}

		static inline std::string selected_name( )
		{
			if ( detail::selected < 0 || detail::selected >= static_cast< int >( detail::config_list.size( ) ) )
			{
				return {};
			}

			char narrow[ 128 ]{};
			wide_to_utf8( detail::config_list[ detail::selected ], narrow, sizeof( narrow ) );
			return narrow;
		}

		static inline void reset_defaults( )
		{
			config::files::reset_to_defaults( );
			settings::finalize_binds( );
		}

	} // namespace detail

	void menu::draw_config( float group_w )
	{
		( void )group_w;

		if ( detail::needs_refresh )
		{
			detail::config_list = config::files::list( );
			if ( detail::config_list.empty( ) )
			{
				config::files::save( L"default" );
				config::files::set_active( L"default" );
				detail::config_list = config::files::list( );
			}
			detail::needs_refresh = false;

			if ( const auto loaded_active = config::files::get_active( ); loaded_active )
			{
				detail::active = *loaded_active;
			}
			else
			{
				detail::active.clear( );
			}

			if ( detail::selected >= static_cast< int >( detail::config_list.size( ) ) )
			{
				detail::selected = -1;
			}

			if ( !detail::pending_select.empty( ) )
			{
				for ( auto i = 0; i < static_cast< int >( detail::config_list.size( ) ); ++i )
				{
					if ( detail::config_list[ i ] == detail::pending_select )
					{
						detail::selected = i;
						break;
					}
				}

				detail::pending_select.clear( );
			}
		}

		const auto dt = xdraw::delta_time( );

		if ( detail::status_timer > 0.0f )
		{
			detail::status_timer = std::max( 0.0f, detail::status_timer - dt );
			if ( detail::status_timer <= 0.0f )
			{
				detail::status_msg.clear( );
			}
		}

		if ( detail::confirm_delete || detail::confirm_reset )
		{
			detail::confirm_timer += dt;

			if ( detail::confirm_timer > 3.0f )
			{
				detail::confirm_delete = false;
				detail::confirm_reset = false;
			}
		}

		auto& dl = xui::draw::current( );
		const auto& s = xui::ctx( ).style;
		const auto& input = xui::ctx( ).input;

		const auto body_x = this->m_body_x;
		const auto body_y = this->m_body_y;
		const auto body_w = this->m_body_w;
		const auto col_w  = ( body_w - tokens::col_gap ) * 0.5f;
		const auto right_x = body_x + col_w + tokens::col_gap;

		// ── Subtab 0: Configurations ─────────────────────────────────────────
		if ( this->m_subtab == 0 )
		{
			// Left Column: Config Profiles List & Primary Actions
			xui::layout::set_cursor( body_x - this->m_x, body_y - this->m_y );
			if ( xui::begin_child( "##cfg_list_panel", col_w, this->m_body_h, true ) )
			{
				group_header( "Config Profiles" );

				xui::text_input( "##cfg_search", detail::search_buf, 64, "search configs..." );

				constexpr auto btn_h{ 26.0f };
				const auto [ avail_w, avail_h ] = xui::layout::avail( );
				const auto list_h = std::max( 120.0f, avail_h - ( btn_h * 2.0f + s.item_spacing_y * 3.0f ) );

				if ( xui::begin_child( "##cfg_list", avail_w, list_h, true ) )
				{
					const auto row_w = xui::layout::avail( ).first;
					constexpr auto row_h{ 26.0f };
					auto visible_rows{ 0 };

					for ( auto i = 0; i < static_cast< int >( detail::config_list.size( ) ); ++i )
					{
						const auto& wname = detail::config_list[ i ];
						if ( !detail::config_matches_search( wname ) ) continue;

						char narrow[ 128 ]{};
						detail::wide_to_utf8( wname, narrow, sizeof( narrow ) );

						const auto row = xui::layout::item( row_w, row_h );
						const auto is_selected = ( detail::selected == i );
						const auto is_hovered = input.in_rect( row );

						if ( is_hovered && input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
						{
							detail::selected = i;
							detail::confirm_delete = false;
							detail::confirm_reset = false;
						}

						const auto hover_anim = xui::anim::lerp( xui::fnv1a( "cfgrow" ) + i, is_hovered ? 1.0f : 0.0f, 14.0f );
						const auto sel_anim = xui::anim::lerp( xui::fnv1a( "cfgsel" ) + i, is_selected ? 1.0f : 0.0f, 10.0f );

						if ( sel_anim > 0.01f )
						{
							dl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( static_cast< std::uint8_t >( 70.0f * sel_anim ) ), xdraw::corner_radius{ tokens::round_md } );
							dl.rect( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 180 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
						}
						else if ( hover_anim > 0.01f )
						{
							dl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated.alpha( static_cast< std::uint8_t >( 255.0f * hover_anim * 0.5f ) ), xdraw::corner_radius{ tokens::round_md } );
						}

						const auto [ tw, th ] = xdraw::measure_text( narrow );
						const auto text_col = is_selected
							? xui::lerp( tokens::col_text, tokens::col_accent, sel_anim )
							: xui::lerp( tokens::col_text_dim, tokens::col_text, hover_anim );

						if ( wname == detail::active )
						{
							constexpr auto dot_r{ 3.5f };
							const auto dot_cx = row.x + row.w - 14.0f;
							const auto dot_cy = row.y + row.h * 0.5f;
							dl.circle_filled( dot_cx, dot_cy, dot_r, tokens::col_accent );
						}

						dl.text( row.x + 10.0f, row.y + ( row.h - th ) * 0.5f, narrow, text_col );
						visible_rows++;
					}

					if ( visible_rows == 0 )
					{
						const auto row = xui::layout::item( row_w, row_h );
						dl.text( row.x + 10.0f, row.y + 6.0f, detail::config_list.empty( ) ? "no configs found" : "no matches", tokens::col_text_dim );
					}

					xui::end_child( );
				}

				// Action buttons: Row 1 (Load, Save, Delete)
				const auto has_selection = detail::selected >= 0 && detail::selected < static_cast< int >( detail::config_list.size( ) );
				const auto row1_btn_w = ( avail_w - s.item_spacing_x * 2.0f ) / 3.0f;

				if ( xui::button( "load", row1_btn_w, btn_h ) && has_selection )
				{
					const auto& wname = detail::config_list[ detail::selected ];
					if ( config::files::load( wname ) )
					{
						config::files::set_active( wname );
						detail::active = wname;
						settings::finalize_binds( );
						detail::status_msg = "Loaded profile";
						detail::status_timer = 2.5f;
					}
				}
				xui::layout::same_line( );
				if ( xui::button( "save", row1_btn_w, btn_h ) && has_selection )
				{
					const auto& wsave = detail::config_list[ detail::selected ];
					if ( config::files::save( wsave ) )
					{
						config::files::set_active( wsave );
						detail::active = wsave;
						detail::pending_select = wsave;
						detail::needs_refresh = true;
						detail::status_msg = "Saved profile";
						detail::status_timer = 2.5f;
					}
				}
				xui::layout::same_line( );
				if ( detail::confirm_delete )
				{
					if ( xui::button( "confirm##del", row1_btn_w, btn_h ) && has_selection )
					{
						const auto removed = detail::config_list[ detail::selected ];
						config::files::remove( removed );
						if ( config::files::get_active( ) == removed )
						{
							config::files::clear_active( );
							detail::active.clear( );
						}
						detail::selected = -1;
						detail::pending_select.clear( );
						detail::needs_refresh = true;
						detail::confirm_delete = false;
						detail::status_msg = "Deleted profile";
						detail::status_timer = 2.5f;
					}
				}
				else if ( xui::button( "delete", row1_btn_w, btn_h ) && has_selection )
				{
					detail::confirm_delete = true;
					detail::confirm_reset = false;
					detail::confirm_timer = 0.0f;
				}

				// Action buttons: Row 2 (Reset, Import, Export)
				if ( detail::confirm_reset )
				{
					if ( xui::button( "confirm##rst", row1_btn_w, btn_h ) )
					{
						detail::reset_defaults( );
						detail::confirm_reset = false;
					}
				}
				else if ( xui::button( "reset", row1_btn_w, btn_h ) )
				{
					detail::confirm_reset = true;
					detail::confirm_delete = false;
					detail::confirm_timer = 0.0f;
				}
				xui::layout::same_line( );
				if ( xui::button( "import", row1_btn_w, btn_h ) )
				{
					const auto clip = detail::paste_from_clipboard( );
					if ( !clip.empty( ) )
					{
						const auto result = config::import_auto( clip );
						if ( result.success )
						{
							settings::finalize_binds( );
							std::wstring wname = !result.name.empty( ) ? detail::utf8_to_wide( result.name ) : L"imported";
							if ( config::files::save( wname ) )
							{
								config::files::set_active( wname );
								detail::active = wname;
								detail::pending_select = wname;
								detail::needs_refresh = true;
							}
						}
					}
				}
				xui::layout::same_line( );
				if ( xui::button( "export", row1_btn_w, btn_h ) )
				{
					const auto name = has_selection ? detail::selected_name( ) : detail::search_buf;
					const auto code = config::export_share_words( name );
					if ( !code.empty( ) )
					{
						detail::copy_to_clipboard( code );
					}
				}

				xui::end_child( );
			}

			// Right Column: Profile Creation & Real Configuration Presets
			xui::layout::set_cursor( right_x - this->m_x, body_y - this->m_y );
			if ( xui::begin_child( "##cfg_mgmt_panel", col_w, this->m_body_h, true ) )
			{
				group_header( "Profile Management" );

				xui::text( "Create New Configuration:", tokens::col_text );
				xui::text_input( "##cfg_new_name", detail::new_profile_buf, 48, "new profile name..." );

				if ( xui::button( "+ CREATE PROFILE", xui::layout::item_width( ), 26.0f ) )
				{
					auto name = detail::new_profile_buf;
					name.erase( 0, name.find_first_not_of( " \t\r\n" ) );
					name.erase( name.find_last_not_of( " \t\r\n" ) + 1 );

					// Sanitize invalid filesystem characters
					std::string sanitized{};
					for ( const char c : name )
					{
						if ( c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' )
							continue;
						sanitized.push_back( c );
					}

					if ( !sanitized.empty( ) )
					{
						const auto wsave = detail::utf8_to_wide( sanitized );
						if ( config::files::save( wsave ) )
						{
							config::files::set_active( wsave );
							detail::active = wsave;
							detail::config_list = config::files::list( );
							detail::selected = -1;
							for ( auto i = 0; i < static_cast< int >( detail::config_list.size( ) ); ++i )
							{
								if ( detail::config_list[ i ] == wsave )
								{
									detail::selected = i;
									break;
								}
							}
							detail::new_profile_buf.clear( );
							detail::status_msg = "Created & selected profile";
							detail::status_timer = 2.5f;
						}
					}
				}

				xui::layout::spacing( 12.0f );
				group_header( "Active Configuration Status" );

				char active_narrow[ 128 ]{ "none" };
				if ( !detail::active.empty( ) )
				{
					detail::wide_to_utf8( detail::active, active_narrow, sizeof( active_narrow ) );
				}
				xui::text( std::string( "Active Profile: " ) + active_narrow, tokens::col_accent );

				if ( detail::status_timer > 0.0f && !detail::status_msg.empty( ) )
				{
					xui::text( detail::status_msg, tokens::col_accent );
				}

				xui::text( "Directory: %USERPROFILE%\\FemWare\\configs", tokens::col_text_dim );
				xui::text( "Format: Compressed JSON Archive (.cfg)", tokens::col_text_dim );

				if ( xui::button( "Open Configs Folder", xui::layout::item_width( ), 24.0f ) )
				{
					const auto path = config::files::root_path( );
					std::error_code ec;
					std::filesystem::create_directories( path, ec );
					ShellExecuteW( nullptr, L"open", path.c_str( ), nullptr, nullptr, SW_SHOWNORMAL );
				}

				xui::end_child( );
			}

			return;
		}

		// ── Subtab 1: Settings — Unsafe Mode & Themes ──────────────────────
		if ( this->m_subtab == 1 )
		{
			// Left Column: Unsafe Features + Theme picker
			xui::layout::set_cursor( body_x - this->m_x, body_y - this->m_y );
			if ( xui::begin_child( "##cfg_settings_panel", col_w, this->m_body_h, true ) )
			{
				group_header( "Unsafe Features" );

				xui::checkbox( "unsafe mode##cfg", settings::g_cheat.unsafe_mode );

				if ( settings::g_cheat.unsafe_mode )
				{
					xui::text( "Unsafe mode is on - rage settings are fully unlocked.", tokens::col_text_dim );
				}
				else
				{
					xui::text( "Rage is locked while unsafe mode is off.", tokens::col_text_dim );
				}

				xui::layout::spacing( 16.0f );
				group_header( "Theme" );

				static constexpr const char* k_themes[] =
				{
					"FemWare", "Catppuccin", "Coffee", "Tokyo Night",
				};

				static constexpr const char* k_flavors[] =
				{
					"Latte", "Frappe", "Macchiato", "Mocha",
				};

				auto& theme_sel = settings::g_cheat.m_theme;

				for ( int i = 0; i < 4; ++i )
				{
					const auto row_w = xui::layout::avail( ).first;
					const auto row = xui::layout::item( row_w, 32.0f );
					const bool hov = input.in_rect( row );
					const bool act = ( static_cast< int >( theme_sel.selected.value ) == i );

					if ( act )
					{
						dl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 40 ), xdraw::corner_radius{ tokens::round_md } );
						dl.rect( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 180 ), xdraw::corner_radius{ tokens::round_md }, 1.0f );
					}
					else if ( hov )
					{
						dl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated, xdraw::corner_radius{ tokens::round_md } );
					}

					const auto sw = theme_sel.palette_for( static_cast< settings::theme::palette >( i ), settings::theme::role::accent, theme_sel.flavor.value );
					const auto badge = xdraw::color{ static_cast< std::uint8_t >( sw.r * 255.0f ), static_cast< std::uint8_t >( sw.g * 255.0f ), static_cast< std::uint8_t >( sw.b * 255.0f ), 255 };
					dl.rect_filled( row.x + 8.0f, row.y + 6.0f, 20.0f, 20.0f, badge, xdraw::corner_radius{ tokens::round_md } );
					dl.rect( row.x + 8.0f, row.y + 6.0f, 20.0f, 20.0f, xdraw::color{ 255, 255, 255, 60 }, xdraw::corner_radius{ tokens::round_md }, 1.0f );

					dl.text( row.x + 36.0f, row.y + 7.0f, k_themes[ i ], act ? tokens::col_accent : ( hov ? tokens::col_text : tokens::col_text_dim ) );

					if ( hov && input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						settings::g_cheat.m_theme.selected.value = static_cast< settings::theme::palette >( i );
						g_menu.apply_theme_preset( i );
					}
				}

				if ( theme_sel.selected.value == settings::theme::palette::catppuccin )
				{
					xui::layout::spacing( 6.0f );
					auto flavor_idx = static_cast< int >( theme_sel.flavor.value );
					if ( xui::combo( "catppuccin flavor", flavor_idx, k_flavors, 4 ) )
					{
						theme_sel.flavor.value = static_cast< settings::theme::catppuccin_flavor >( flavor_idx );
						g_menu.apply_theme_preset( static_cast< int >( settings::theme::palette::catppuccin ) );
					}
				}

				xui::end_child( );
			}

			// Right Column: Custom Accent
			xui::layout::set_cursor( right_x - this->m_x, body_y - this->m_y );
			if ( xui::begin_child( "##cfg_custom_accent_panel", col_w, this->m_body_h, true ) )
			{
				group_header( "Custom Accent" );

				static config::col s_custom_accent{ { 255, 95, 175, 255 } };
				const auto old_col = s_custom_accent.value;

				xui::color_picker( "custom accent", s_custom_accent );

				if ( s_custom_accent.value.r != old_col.r || s_custom_accent.value.g != old_col.g || s_custom_accent.value.b != old_col.b || s_custom_accent.value.a != old_col.a )
				{
					g_menu.apply_custom_accent( s_custom_accent.value );
				}

				xui::layout::spacing( 16.0f );
				group_header( "Accent Preview" );
				xui::text( "Live theme accent preview:", tokens::col_text_dim );
				xui::button( "Active Accent Button", xui::layout::item_width( ), 26.0f );

				xui::end_child( );
			}

			return;
		}
	}

} // namespace rendering
