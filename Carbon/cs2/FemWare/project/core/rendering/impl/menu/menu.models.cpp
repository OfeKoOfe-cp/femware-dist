#include <pch/pch.hpp>
#include <windows.h>
#include <shellapi.h>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>
#include <core/resources/workspace.hpp>

#include "../../rendering.hpp"

#include <core/features/changer/model_store.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace rendering {

	void menu::draw_models_studio( float sw, float sh )
	{
		( void )sw;
		( void )sh;

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_models", this->m_models_x, this->m_models_y, this->m_models_w, this->m_models_h, false, 700.0f, 460.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_models_x;
		const auto wy = this->m_models_y;
		const auto ww = this->m_models_w;
		const auto wh = this->m_models_h;
		const auto& inp = xui::ctx( ).input;

		for ( int s_i = 5; s_i >= 1; --s_i )
		{
			const float spread = static_cast< float >( s_i ) * 3.0f;
			const std::uint8_t a = static_cast< std::uint8_t >( 6.0f * ( 6 - s_i ) * reveal );
			dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
				xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
		}
		dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast< std::uint8_t >( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );
		dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

		// ── Title bar ─────────────────────────────────────────────────────────
		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "CUSTOM MODELS" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

			dl.text( wx + 12.0f, cy, "CUSTOM MODELS", tokens::col_accent );

			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_models = false;
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

		// Inset accent top-edge line
		dl.rect_filled( wx + tokens::round_lg, wy + 1.0f, ww - tokens::round_lg * 2.0f, 2.0f,
			tokens::col_accent.alpha( static_cast< std::uint8_t >( tokens::col_accent.a * reveal ) ),
			xdraw::corner_radius{ 1.0f } );

		const float pad = 8.0f;
		const float body_x = wx + pad;
		const float body_y = wy + tokens::title_bar_h + pad;
		const float body_w = ww - pad * 2.0f;
		const float body_h = wh - tokens::title_bar_h - pad * 2.0f;

		xui::layout::set_cursor( body_x - wx, body_y - wy );

		// ── Toolbar: open folder + reload on a single row ─────────────────────
		{
			if ( xui::button( "open folder##md", 130.0f, 22.0f ) )
			{
				const auto dir = workspace::models( );
				std::error_code ec;
				std::filesystem::create_directories( dir, ec );
				ShellExecuteW( nullptr, L"open", dir.c_str( ), nullptr, nullptr, SW_SHOWNORMAL );
			}

			xui::layout::same_line( 8.0f );
			if ( xui::button( "reload##md", 90.0f, 22.0f ) )
			{
				features::changer::model_store::refresh( );
				features::changer::model_store::load_mappings( );
			}

			if ( features::changer::model_store::pending_refresh( ).load( ) )
			{
				features::changer::model_store::poll( );
				features::changer::model_store::load_mappings( );
			}

			if ( !features::changer::model_store::last_status( ).empty( ) )
			{
				xui::layout::new_line( );
				xui::layout::spacing( 4.0f );
				xui::text( features::changer::model_store::last_status( ), tokens::col_text_dim );
			}

			xui::layout::new_line( );
			xui::layout::spacing( 6.0f );
		}

		const float content_top = xui::layout::current_window( )->cursor_y;

		// ── Split: left preview panel / right card grid ───────────────────────
		const float preview_w = body_w * 0.30f;
		const float grid_w = body_w - preview_w - 8.0f;

		using mode_t = settings::changer::agent_selection_field::agent_mode;
		const auto mode = static_cast< mode_t >( settings::g_changer.agents.mode );

		// Per-player assignment is only available in "selected" mode; carve
		// real space for it out of the grid whenever that mode is active.
		const float assign_h = ( mode == mode_t::selected ) ? std::clamp( body_h * 0.32f, 96.0f, 220.0f ) : 0.0f;
		const float used_top = content_top - ( body_y - wy );
		const float grid_h = body_h - used_top - assign_h;

		auto& m_entries = features::changer::model_store::entries( );

		// Selected entry (by file). Default to CT-equipped, then T, then none.
		const auto& sel_ct = settings::g_changer.agents.custom_ct;
		const auto& sel_t = settings::g_changer.agents.custom_t;

		std::string preview_name;
		std::string preview_file;
		bool has_selection = false;
		if ( this->m_models_selected >= 0 && this->m_models_selected < static_cast< int >( m_entries.size( ) ) )
		{
			preview_name = m_entries[ this->m_models_selected ].name;
			preview_file = m_entries[ this->m_models_selected ].file;
			has_selection = true;
		}
		else if ( !sel_ct.empty( ) || !sel_t.empty( ) )
		{
			const std::string target = !sel_ct.empty( ) ? sel_ct : sel_t;
			for ( std::size_t i = 0; i < m_entries.size( ); ++i )
			{
				if ( m_entries[ i ].file == target )
				{
					this->m_models_selected = static_cast< int >( i );
					preview_name = m_entries[ i ].name;
					preview_file = m_entries[ i ].file;
					has_selection = true;
					break;
				}
			}
		}

		// ── Preview panel ─────────────────────────────────────────────────────
		xui::layout::set_cursor( body_x - wx, content_top );
		if ( xui::begin_child( "##models_preview", preview_w, grid_h, false ) )
		{
			auto& pdl = xui::draw::current( );
			const auto avail = xui::layout::avail( );
			const auto card = xui::layout::item( avail.first, avail.second );

			pdl.rect_filled( card.x, card.y, card.w, card.h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_md } );
			pdl.rect( card.x, card.y, card.w, card.h, tokens::col_border, 1.0f );
			pdl.rect_filled( card.x, card.y, card.w, 2.0f, tokens::col_accent.alpha( 200 ) );

			// Mannequin silhouette (only when something is selected)
			if ( has_selection && this->m_textures.model_mannequin.resource )
			{
				constexpr float img_sz = 96.0f;
				const float img_x = std::floor( card.x + ( card.w - img_sz ) * 0.5f );
				const float img_y = card.y + 26.0f;
				pdl.image( img_x, img_y, img_sz, img_sz, this->m_textures.model_mannequin.resource.Get( ), tokens::col_accent );
			}

			if ( has_selection )
			{
				const auto truncated = xui::truncate( preview_name, card.w - 16.0f );
				const auto [tw, th] = xdraw::measure_text( truncated );
				pdl.text( std::floor( card.x + ( card.w - tw ) * 0.5f ), card.y + 138.0f, truncated, tokens::col_text );

				const auto file_trunc = xui::truncate( preview_file, card.w - 16.0f );
				const auto [fw, fh] = xdraw::measure_text( file_trunc );
				pdl.text( std::floor( card.x + ( card.w - fw ) * 0.5f ), card.y + 162.0f, file_trunc, tokens::col_text_dim );

				const auto internal_path = features::changer::model_store::resolve_internal( preview_file );
				pdl.rect_filled( card.x + 8.0f, card.y + 186.0f, card.w - 16.0f, 1.0f, tokens::col_border );
				pdl.text( card.x + 10.0f, card.y + 194.0f, xui::truncate( internal_path, card.w - 16.0f ), tokens::col_text_dim );
				pdl.rect_filled( card.x + 8.0f, card.y + 210.0f, card.w - 16.0f, 1.0f, tokens::col_border );
				const auto installed_ok = features::changer::model_store::is_installed( preview_file );
				pdl.text( card.x + 10.0f, card.y + 218.0f,
					internal_path.empty( ) ? "unknown internal path" : ( installed_ok ? "installed in game" : "not installed - set ct/t installs it" ),
					internal_path.empty( ) ? tokens::col_text_dim : ( installed_ok ? xdraw::color{ 74, 222, 128, 255 } : xdraw::color{ 251, 191, 36, 255 } ) );
			}
			else
			{
				// Quiet empty card — models are brought in through the toolbar's
				// open folder + reload, so there's no inline add-model affordance.
			}

			// Mode selector anchors to the bottom of the card, above the action rows.
			// Uses the native labeled combo so the "mode:" label and the
			// local/all/selected dropdown line up exactly like every other
			// combo in the UI.
			{
				const auto* win_preview = xui::layout::current_window( );
				const auto& st = xui::ctx( ).style;
				const auto sp = st.item_spacing_y * 0.25f;
				const auto mlh = xdraw::measure_text( "mode:" ).second;

				constexpr const char* model_modes[] = { "local", "all", "selected" };
				const float combo_w = card.w - 28.0f;
				const float box_top = card.y + card.h - 84.0f;
				const float cursor_y = box_top - mlh - sp;
				xui::layout::set_cursor( card.x + 14.0f - win_preview->bounds.x, cursor_y - win_preview->bounds.y );
				xui::combo( "mode:", settings::g_changer.agents.mode, model_modes, 3, combo_w );
			}

			// Action buttons -- two rows under the preview info.
			const float b_w = ( card.w - 24.0f - 4.0f ) * 0.5f;
			const float ab_row1 = card.y + card.h - 56.0f;
			const float ab_row2 = card.y + card.h - 30.0f;

			// Boxed action buttons: filled card pill, border outline, accent for
			// REMOVE; hover breathes an elevated fill.
			auto action_btn = [ & ]( float bx, float by, float bw, const char* label, bool enabled, bool accent )
				{
					const xui::rect br{ bx, by, bw, 22.0f };
					const bool hovered = enabled && !xui::ctx( ).overlay_blocking( ) && inp.in_rect( br );
					const auto hover_anim = xui::anim::lerp( xui::fnv1a( "mdl_act_" ) + xui::fnv1a( label ), hovered ? 1.0f : 0.0f, 14.0f );

					auto bg = tokens::col_card;
					bg = xui::lerp( bg, tokens::col_elevated, hover_anim );
					if ( !enabled ) bg = tokens::col_dark;

					auto border = accent ? tokens::col_accent.alpha( 200 ) : tokens::col_border;
					border = xui::lerp( border, tokens::col_accent.alpha( 220 ), hover_anim );

					dl.rect_filled( br.x, br.y, br.w, br.h, bg, xdraw::corner_radius{ tokens::btn_rounding } );
					dl.rect( br.x, br.y, br.w, br.h, border, xdraw::corner_radius{ tokens::btn_rounding }, 1.0f );

					auto col = !enabled ? tokens::col_text_dim : ( accent ? tokens::col_accent : tokens::col_text_dim );
					col = xui::lerp( col, accent ? tokens::col_accent : tokens::col_text, hover_anim );
					const auto [lw, lh] = xdraw::measure_text( label );
					dl.text( std::floor( br.x + ( br.w - lw ) * 0.5f ), std::floor( br.y + ( br.h - lh ) * 0.5f ), label, col );

					return hovered && inp.mouse_clicked;
				};

const auto equip_model = [ & ]( const std::string& file )
					{
						const auto installed = features::changer::model_store::ensure_installed( file );
						char buf[ 192 ];
						if ( installed )
						{
							std::snprintf( buf, sizeof( buf ), "equipped - installed to game" );
						}
						else
						{
							std::snprintf( buf, sizeof( buf ), "model files missing or no internal path" );
						}
						features::changer::model_store::last_status( ) = buf;
					};

					if ( has_selection )
					{
						if ( action_btn( card.x + 8.0f, ab_row1, b_w, "SET CT", true, false ) )
						{
							settings::g_changer.agents.custom_ct = preview_file;
							equip_model( preview_file );
						}
						if ( action_btn( card.x + 8.0f + b_w + 4.0f, ab_row1, b_w, "SET T", true, false ) )
						{
							settings::g_changer.agents.custom_t = preview_file;
							equip_model( preview_file );
						}
						if ( action_btn( card.x + 8.0f, ab_row2, card.w - 16.0f, "REMOVE", true, true ) )
						{
							if ( sel_ct == preview_file ) settings::g_changer.agents.custom_ct.clear( );
							if ( sel_t == preview_file ) settings::g_changer.agents.custom_t.clear( );
							this->m_models_selected = -1;
						}
					}

			xui::end_child( );
		}

		// ── Card grid ─────────────────────────────────────────────────────────
		xui::layout::set_cursor( body_x + preview_w + 8.0f - wx, content_top );
		if ( xui::begin_child( "##models_grid", grid_w, grid_h, true ) )
		{
			const auto win = xui::layout::current_window( );
			const auto inner_w = grid_w - 8.0f;
			constexpr int cols = 4;
			constexpr float gap = 8.0f;
			const float card_w = std::floor( ( inner_w - ( cols - 1 ) * gap ) / static_cast< float >( cols ) );
			const float card_h = 96.0f;

			// Search filter
			xui::layout::set_cursor( 4.0f, 4.0f );
			xui::text_input( "##md_search", this->m_models_search, 48, "search model...", inner_w - 8.0f );

			std::vector<const features::changer::model_store::entry*> filtered;
			std::string filter = this->m_models_search;
			for ( auto& c : filter ) c = static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) );
			for ( const auto& m : m_entries )
			{
				if ( !filter.empty( ) )
				{
					std::string n = m.name;
					for ( auto& c : n ) c = static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) );
					if ( n.find( filter ) == std::string::npos ) continue;
				}
				filtered.push_back( &m );
			}

			const auto rows = ( static_cast< int >( filtered.size( ) ) + cols - 1 ) / cols;
			const auto grid_h_total = 34.0f + rows * card_h + ( rows > 0 ? ( rows - 1 ) * gap : 0.0f );

			xui::layout::set_cursor( 4.0f, 40.0f );
			xui::layout::item( inner_w - 8.0f, grid_h_total );

			const auto base_x = win->bounds.x + 4.0f;
			const auto grid_top_y = win->bounds.y + 40.0f - win->scroll_y;

			for ( std::size_t idx = 0; idx < filtered.size( ); ++idx )
			{
				const auto col = idx % cols;
				const auto row = idx / cols;
				const auto cx = std::floor( base_x + col * ( card_w + gap ) );
				const auto cy = std::floor( grid_top_y + row * ( card_h + gap ) );

				if ( cy + card_h < win->bounds.y || cy > win->bounds.bottom( ) ) continue;

				const auto* m = filtered[ idx ];
				const bool is_ct = ( settings::g_changer.agents.custom_ct == m->file );
				const bool is_t = ( settings::g_changer.agents.custom_t == m->file );
				const bool equipped = is_ct || is_t;
				const auto card = xui::rect{ cx, cy, card_w, card_h };
				const bool hovered = !xui::ctx( ).overlay_blocking( ) && inp.in_rect( card );
				const auto hover_anim = xui::anim::lerp( xui::fnv1a( "mdl_card" ) + idx, hovered ? 1.0f : 0.0f, 14.0f );

				auto card_bg = tokens::col_card;
				card_bg = xui::lerp( card_bg, xui::lighten( card_bg, 1.4f ), hover_anim * 0.5f );
				dl.rect_filled( card.x, card.y, card.w, card.h, card_bg, xdraw::corner_radius{ tokens::btn_rounding } );

				if ( equipped )
				{
					dl.rect( card.x, card.y, card.w, card.h, tokens::col_accent, xdraw::corner_radius{ tokens::btn_rounding }, 1.5f );
				}
				else if ( hover_anim > 0.01f )
				{
					dl.rect( card.x, card.y, card.w, card.h, tokens::col_border, xdraw::corner_radius{ tokens::btn_rounding }, 1.0f );
				}

				// Mannequin glyph + name side by side
				constexpr float glyph_sz = 40.0f;
				const float gx = card.x + 10.0f;
				const float gy = card.y + std::floor( ( card_h - glyph_sz ) * 0.5f );
				if ( this->m_textures.model_mannequin.resource )
				{
					dl.image( gx, gy, glyph_sz, glyph_sz, this->m_textures.model_mannequin.resource.Get( ), equipped ? tokens::col_accent : tokens::col_text_dim );
				}

				const float tx = gx + glyph_sz + 8.0f;
				const float avail_nw = card.x + card.w - tx - 6.0f;
				const auto ntrunc = xui::truncate( m->name, avail_nw );
				dl.text( tx, card.y + 10.0f, ntrunc, equipped ? tokens::col_text : tokens::col_text );

				const auto ntr = xui::truncate( m->file, avail_nw );
				dl.text( tx, card.y + 26.0f, ntr, tokens::col_text_dim );

				// Equip tags
				std::string tag;
				if ( is_ct && is_t ) tag = "CT + T";
				else if ( is_ct ) tag = "CT";
				else if ( is_t ) tag = "T";
				else tag = "click to equip";

				const auto [tw, th] = xdraw::measure_text( tag );
				const auto tag_col = equipped ? tokens::col_accent : tokens::col_text_dim;
				dl.text( tx, card.y + 50.0f, tag, tag_col );

				// Selected spine
				if ( this->m_models_selected >= 0 && this->m_models_selected < static_cast< int >( m_entries.size( ) )
					&& m_entries[ this->m_models_selected ].file == m->file )
				{
					dl.rect_filled( card.x, card.y + 8.0f, 2.5f, card_h - 16.0f, tokens::col_accent, xdraw::corner_radius{ 1.25f } );
				}

				if ( hovered && inp.mouse_clicked )
				{
					if ( equipped )
					{
						// Click again to clear both sides.
						settings::g_changer.agents.custom_ct.clear( );
						settings::g_changer.agents.custom_t.clear( );
					}
					else
					{
						settings::g_changer.agents.custom_ct = m->file;
						settings::g_changer.agents.custom_t = m->file;
						if ( features::changer::model_store::ensure_installed( m->file ) )
						{
							features::changer::model_store::last_status( ) = "equipped - installed to game";
						}
						else
						{
							features::changer::model_store::last_status( ) = "model files missing or no internal path";
						}
					}
					for ( std::size_t i = 0; i < m_entries.size( ); ++i )
					{
						if ( m_entries[ i ].file == m->file )
						{
							this->m_models_selected = static_cast< int >( i );
							break;
						}
					}
				}
			}

			xui::end_child( );
		}

		// ── Per-player assignment (selected mode) ─────────────────────────────
		if ( mode == mode_t::selected )
		{
			xui::layout::set_cursor( body_x - wx, content_top + grid_h );
			if ( xui::begin_child( "##models_assign", body_w, assign_h, true ) )
			{
				xui::text( "assign a model to a player  (selected mode)", tokens::col_text_dim );

				auto& m_entries_list = features::changer::model_store::entries( );
				std::vector< const char* > model_ptrs;
				model_ptrs.reserve( m_entries_list.size( ) + 1 );
				model_ptrs.push_back( "none" );
				for ( const auto& e : m_entries_list )
				{
					model_ptrs.push_back( e.name.c_str( ) );
				}

				const auto index_of = [ & ]( const std::string& file ) -> int
					{
						if ( file.empty( ) ) return 0;
						for ( std::size_t i = 0; i < m_entries_list.size( ); ++i )
						{
							if ( m_entries_list[ i ].name == file || m_entries_list[ i ].file == file )
							{
								return static_cast< int >( i + 1 );
							}
						}
						return 0;
					};

				for ( const auto& p : systems::g_entities.get_by_type( systems::entities::type::player ) )
				{
					if ( !p.ptr ) continue;

					const auto steam = memory::read< std::uint64_t >( p.ptr + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
					if ( steam == 0 ) continue;

					const auto name_ptr = memory::read< std::uintptr_t >( p.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
					std::string label = name_ptr ? memory::read_string( name_ptr ) : std::to_string( steam );
					label += "##md_assign_" + std::to_string( steam );

					int idx = index_of( features::changer::model_store::find( steam ) );
					if ( xui::combo( label.c_str( ), idx, model_ptrs.data( ), static_cast< int >( model_ptrs.size( ) ) ) )
					{
						auto& sel = features::changer::model_store::selections( );
						if ( idx > 0 )
						{
							sel[ steam ] = m_entries_list[ idx - 1 ].file;
						}
						else
						{
							sel.erase( steam );
						}
						features::changer::model_store::save( );
					}
				}

				xui::end_child( );
			}
		}

		xui::end_window( );
	}


} // namespace rendering