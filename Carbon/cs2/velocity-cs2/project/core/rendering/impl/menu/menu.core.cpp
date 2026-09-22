#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
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
#include "menu.assets.hpp"

namespace rendering {

	void menu::initialize_graphics( )
	{
		if ( this->m_intro_base_graphics_ready )
		{
			return;
		}

		constexpr auto icon_target{ 16.0f };
		this->m_textures.logo.resource = xdraw::load_svg( svgs::logo, icon_target / 24.0f, &this->m_textures.logo.width, &this->m_textures.logo.height );

		constexpr float k_intro_logo_view_w{ 4421.68f };
		constexpr float k_intro_logo_px_w{ 240.0f };
		this->m_textures.intro_splash.resource = xdraw::load_svg(
			svgs::intro_splash_logo,
			k_intro_logo_px_w / k_intro_logo_view_w,
			&this->m_textures.intro_splash.width,
			&this->m_textures.intro_splash.height
		);
		this->m_textures.search.resource = xdraw::load_svg( svgs::search, icon_target / 24.0f, &this->m_textures.search.width, &this->m_textures.search.height );
		this->m_textures.settings.resource = xdraw::load_svg( svgs::settings, icon_target / 24.0f, &this->m_textures.settings.width, &this->m_textures.settings.height );

		constexpr float k_dock_icon_scale{ 16.0f / 24.0f };
		this->m_textures.dock_main.resource = xdraw::load_svg( svgs::dock_main, k_dock_icon_scale, &this->m_textures.dock_main.width, &this->m_textures.dock_main.height );
		this->m_textures.dock_skins.resource = xdraw::load_svg( svgs::tab_skins, k_dock_icon_scale, &this->m_textures.dock_skins.width, &this->m_textures.dock_skins.height );
		this->m_textures.dock_models.resource = xdraw::load_svg( svgs::dock_models, k_dock_icon_scale, &this->m_textures.dock_models.width, &this->m_textures.dock_models.height );
		this->m_textures.model_mannequin.resource = xdraw::load_svg( svgs::model_mannequin, 6.0f, &this->m_textures.model_mannequin.width, &this->m_textures.model_mannequin.height );
		this->m_textures.dock_lua.resource = xdraw::load_svg( svgs::dock_lua, k_dock_icon_scale, &this->m_textures.dock_lua.width, &this->m_textures.dock_lua.height );
		this->m_textures.dock_console.resource = xdraw::load_svg( svgs::dock_console, k_dock_icon_scale, &this->m_textures.dock_console.width, &this->m_textures.dock_console.height );
		this->m_textures.dock_docs.resource = xdraw::load_svg( svgs::dock_docs, k_dock_icon_scale, &this->m_textures.dock_docs.width, &this->m_textures.dock_docs.height );
		this->m_textures.dock_players.resource = xdraw::load_svg( svgs::dock_players, k_dock_icon_scale, &this->m_textures.dock_players.width, &this->m_textures.dock_players.height );
		this->m_textures.dock_save.resource = xdraw::load_svg( svgs::dock_save, k_dock_icon_scale, &this->m_textures.dock_save.width, &this->m_textures.dock_save.height );
		this->m_textures.dock_power.resource = xdraw::load_svg( svgs::dock_power, k_dock_icon_scale, &this->m_textures.dock_power.width, &this->m_textures.dock_power.height );

		this->m_textures.lua_icon.resource = xdraw::load_svg( svgs::lua_icon, k_dock_icon_scale, &this->m_textures.lua_icon.width, &this->m_textures.lua_icon.height );
		this->m_textures.lua_docs.resource = xdraw::load_svg( svgs::lua_docs, k_dock_icon_scale, &this->m_textures.lua_docs.width, &this->m_textures.lua_docs.height );
		this->m_textures.lua_copy.resource = xdraw::load_svg( svgs::lua_copy, k_dock_icon_scale, &this->m_textures.lua_copy.width, &this->m_textures.lua_copy.height );
		this->m_textures.lua_clear.resource = xdraw::load_svg( svgs::lua_clear, k_dock_icon_scale, &this->m_textures.lua_clear.width, &this->m_textures.lua_clear.height );
		this->m_textures.lua_play.resource = xdraw::load_svg( svgs::lua_play, k_dock_icon_scale, &this->m_textures.lua_play.width, &this->m_textures.lua_play.height );
		this->m_textures.lua_reload.resource = xdraw::load_svg( svgs::lua_reload, k_dock_icon_scale, &this->m_textures.lua_reload.width, &this->m_textures.lua_reload.height );
		this->m_textures.lua_minus.resource = xdraw::load_svg( svgs::lua_minus, k_dock_icon_scale, &this->m_textures.lua_minus.width, &this->m_textures.lua_minus.height );

		constexpr float k_cfg_icon_scale{ 14.0f / 24.0f };
		this->m_textures.cfg_folder_on.resource = xdraw::load_svg( svgs::cfg_folder_black, k_cfg_icon_scale, &this->m_textures.cfg_folder_on.width, &this->m_textures.cfg_folder_on.height );
		this->m_textures.cfg_folder_off.resource = xdraw::load_svg( svgs::cfg_folder_dim, k_cfg_icon_scale, &this->m_textures.cfg_folder_off.width, &this->m_textures.cfg_folder_off.height );
		this->m_textures.cfg_cloud_on.resource = xdraw::load_svg( svgs::cfg_cloud_black, k_cfg_icon_scale, &this->m_textures.cfg_cloud_on.width, &this->m_textures.cfg_cloud_on.height );
		this->m_textures.cfg_cloud_off.resource = xdraw::load_svg( svgs::cfg_cloud_dim, k_cfg_icon_scale, &this->m_textures.cfg_cloud_off.width, &this->m_textures.cfg_cloud_off.height );
		this->m_textures.cfg_plus.resource = xdraw::load_svg( svgs::cfg_plus, k_cfg_icon_scale, &this->m_textures.cfg_plus.width, &this->m_textures.cfg_plus.height );
		this->m_textures.flag_shield.resource = xdraw::load_svg( svgs::flag_shield, k_dock_icon_scale, &this->m_textures.flag_shield.width, &this->m_textures.flag_shield.height );
		this->m_textures.flag_eye.resource = xdraw::load_svg( svgs::flag_eye, k_dock_icon_scale, &this->m_textures.flag_eye.width, &this->m_textures.flag_eye.height );

		constexpr auto tab_target{ 22.0f };
		constexpr std::array<const char*, 6> tab_svgs{
			svgs::tab_rage,
			svgs::tab_legit,
			svgs::tab_player,
			svgs::tab_world,
			svgs::tab_misc,
			svgs::tab_config
		};
		for ( auto i = 0; i < 6; ++i )
		{
			this->m_textures.tabs[ i ].resource = xdraw::load_svg( tab_svgs[ i ], tab_target / 24.0f, &this->m_textures.tabs[ i ].width, &this->m_textures.tabs[ i ].height );
		}

		this->m_textures.cat_overlay.resource = xdraw::load_texture(
			std::span<const std::byte>( reinterpret_cast<const std::byte*>( resources::images::cat_overlay ), sizeof( resources::images::cat_overlay ) ),
			&this->m_textures.cat_overlay.width, &this->m_textures.cat_overlay.height );

		this->m_textures.fw_logo_static.resource = xdraw::load_texture(
			std::span<const std::byte>( reinterpret_cast<const std::byte*>( resources::images::fw_logo_static ), sizeof( resources::images::fw_logo_static ) ),
			&this->m_textures.fw_logo_static.width, &this->m_textures.fw_logo_static.height, true );

		this->m_textures.fw_logo_glow = xdraw::load_gif(
			std::span<const std::byte>( reinterpret_cast<const std::byte*>( resources::images::fw_logo_glow ), sizeof( resources::images::fw_logo_glow ) ), true );

		this->m_textures.fw_logo_spin = xdraw::load_gif(
			std::span<const std::byte>( reinterpret_cast<const std::byte*>( resources::images::fw_logo_spin ), sizeof( resources::images::fw_logo_spin ) ), true );

		this->m_textures.fw_logo_spin_glow = xdraw::load_gif(
			std::span<const std::byte>( reinterpret_cast<const std::byte*>( resources::images::fw_logo_spin_glow ), sizeof( resources::images::fw_logo_spin_glow ) ), true );

		this->apply_theme_preset( 0 );
		this->m_intro_base_graphics_ready = true;
	}


	void menu::shutdown( ) const
	{
		if ( this->m_open )
		{
			memory::call_vfunc<void>( addresses::globals::input_system, 76, this->m_saved_relative_mouse != 0 );
		}
	}

	void menu::apply_saved_cursor( )
	{
		if ( !this->m_has_saved_cursor )
		{
			return;
		}

		const auto vx = GetSystemMetrics( SM_XVIRTUALSCREEN );
		const auto vy = GetSystemMetrics( SM_YVIRTUALSCREEN );
		const auto vw = GetSystemMetrics( SM_CXVIRTUALSCREEN );
		const auto vh = GetSystemMetrics( SM_CYVIRTUALSCREEN );

		const auto x = std::clamp( this->m_saved_cursor_x, vx, vx + std::max( 1, vw ) - 1 );
		const auto y = std::clamp( this->m_saved_cursor_y, vy, vy + std::max( 1, vh ) - 1 );

		SetCursorPos( x, y );
		SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
	}
	// ─────────────────────────────────────────────────────────────────────────
	//  LinoriaLib-style main tab bar
	//  Full-width row of text tabs. Active = accent text + 2px bottom line.
	//  Inactive = dim text. 1px separator line at bottom.
	// ─────────────────────────────────────────────────────────────────────────

	void menu::try_load_user_avatar( )
	{
		if ( this->m_textures.user.resource )
		{
			return;
		}

		this->m_user_avatar_retry_delay -= xdraw::delta_time( );
		if ( this->m_user_avatar_retry_delay > 0.0f )
		{
			return;
		}
		this->m_user_avatar_retry_delay = 1.0f;

		const auto steam_id = steam::user::get_steam_id( );
		if ( steam_id )
		{
			const auto image = steam::friends::get_medium_friend_avatar( steam_id );
			if ( image > 0 )
			{
				std::uint32_t width{}, height{};
				if ( steam::utils::get_image_size( image, &width, &height ) && width && height )
				{
					std::vector<std::uint8_t> rgba( width * height * 4 );
					if ( steam::utils::get_image_rgba( image, rgba.data( ), static_cast< int >( rgba.size( ) ) ) )
					{
						this->m_textures.user.resource = xdraw::create_srv_from_rgba( rgba.data( ), static_cast< int >( width ), static_cast< int >( height ) );
						this->m_textures.user.width = static_cast< int >( width );
						this->m_textures.user.height = static_cast< int >( height );
					}
				}
			}
		}

		if ( !this->m_textures.user.resource )
		{
			this->m_textures.user.resource = xdraw::load_texture(
				std::span<const std::byte>( reinterpret_cast<const std::byte*>( images::user ), sizeof( images::user ) ),
				&this->m_textures.user.width, &this->m_textures.user.height );
		}
	}

	void menu::sync_theme_style( ) const
	{
		auto& style = xui::ctx( ).style;
		style.window_bg = tokens::col_elevated.alpha( 205 );
		// ── window / child panels (semi-transparent "glass") ───────────────
		style.window_bg           = xdraw::color{ tokens::col_dark.r, tokens::col_dark.g, tokens::col_dark.b, 236 };
		style.window_border       = tokens::col_group_border;
		style.child_bg            = xdraw::color{ tokens::col_card.r, tokens::col_card.g, tokens::col_card.b, 208 };
		style.child_border        = tokens::col_group_border;

		// ── checkbox — small square, accent fill ───────────────────────────
		style.checkbox_size       = 15.0f;
		style.checkbox_bg         = tokens::col_dark;
		style.checkbox_border     = tokens::col_border;
		style.checkbox_mark       = tokens::col_checkbox_on;
		style.checkbox_mark_icon  = tokens::col_dark;

		// ── slider — flat track ────────────────────────────────────────────
		style.slider_h            = 6.0f;
		style.slider_track        = tokens::col_slider_bg;
		style.slider_fill         = tokens::col_slider_fill;

		// ── buttons ────────────────────────────────────────────────────────
		style.button_bg           = tokens::col_card;
		style.button_border       = tokens::col_border;
		style.button_hovered      = tokens::col_elevated;
		style.button_active       = tokens::col_accent;

		// ── keybind ────────────────────────────────────────────────────────
		style.keybind_bg          = tokens::col_dark;
		style.keybind_border      = tokens::col_border;
		style.keybind_waiting     = tokens::col_accent;

		// ── combo / dropdown ───────────────────────────────────────────────
		style.combo_bg            = tokens::col_dark;
		style.combo_border        = tokens::col_border;
		style.combo_arrow         = tokens::col_text_dim;
		style.combo_hovered       = tokens::col_elevated;
		style.combo_popup_bg      = tokens::col_elevated;
		style.combo_popup_border  = tokens::col_border;
		style.combo_popup_item_hovered  = tokens::col_elevated.alpha( 200 );
		style.combo_popup_item_selected = tokens::col_accent.alpha( 46 );

		// ── popups / pickers ───────────────────────────────────────────────
		style.popup_bg            = tokens::col_elevated;
		style.popup_border        = tokens::col_border;
		style.picker_bg           = tokens::col_card;
		style.picker_border       = tokens::col_border;
		style.picker_popup_bg     = tokens::col_elevated;
		style.picker_popup_border = tokens::col_border;

		// ── text input ─────────────────────────────────────────────────────
		style.text_input_bg       = tokens::col_card;
		style.text_input_border   = tokens::col_border;

		// ── misc ───────────────────────────────────────────────────────────
		style.separator           = tokens::col_accent.alpha( 26 );
		style.text                = tokens::col_text;
		style.text_dim            = tokens::col_text_dim;
		style.accent              = tokens::col_accent;

		// ── rounded, soft modern aesthetic ─────────────────────────────────
		style.rounding               = 12.0f;  // window / child panels
		style.checkbox_rounding      = 5.0f;
		style.slider_rounding        = 3.5f;   // pill track
		style.button_rounding        = 8.0f;
		style.keybind_rounding       = 6.0f;
		style.combo_rounding         = 8.0f;
		style.popup_rounding         = 10.0f;
		style.combo_popup_rounding   = 8.0f;
		style.picker_popup_rounding  = 10.0f;
		style.color_swatch_rounding  = 5.0f;
		style.text_input_rounding    = 8.0f;
		// Reference style is borderless — definition comes from layered fills.
		style.border_thickness       = 0.0f;
	}

	// ─────────────────────────────────────────────────────────────────────────
	//  Theme preset — sets all derived accent tokens from the theme palette.
	//  0=FemWare  1=Catppuccin  2=Coffee  3=Tokyo Night
	// ─────────────────────────────────────────────────────────────────────────
	void menu::apply_theme_preset( int preset )
	{
		const auto& th  = settings::g_cheat.m_theme;
		const auto  pal = static_cast< settings::theme::palette >( std::clamp( preset, 0, 3 ) );
		const auto  flav = th.flavor.value;

		const auto to_u8 = []( float v, float a = 1.0f ) -> std::uint8_t
		{
			return static_cast< std::uint8_t >( std::clamp( v, 0.0f, 1.0f ) * std::clamp( a, 0.0f, 1.0f ) * 255.0f );
		};

		const auto t = th.palette_for( pal, settings::theme::role::text, flav );
		const auto a = th.palette_for( pal, settings::theme::role::accent, flav );
		const auto b = th.palette_for( pal, settings::theme::role::background, flav );
		const auto s = th.palette_for( pal, settings::theme::role::subtext, flav );

		const auto ar = to_u8( a.r ), ag = to_u8( a.g ), ab = to_u8( a.b );
		const auto tr = to_u8( t.r ), tg = to_u8( t.g ), tb = to_u8( t.b );
		const auto sr = to_u8( s.r ), sg = to_u8( s.g ), sb = to_u8( s.b );

		const auto lighten = []( std::uint8_t v, int d ) -> std::uint8_t
		{
			return static_cast< std::uint8_t >( std::clamp( ( int )v + d, 0, 255 ) );
		};

		const auto br = to_u8( b.r ), bg = to_u8( b.g ), bb = to_u8( b.b );

		// Primary accent + white-blended gradient end.
		tokens::col_accent       = { ar, ag, ab, 255 };
		tokens::col_accent_2     = { lighten( ar, ( 255 - ( int )ar ) / 2 ),
		                             lighten( ag, ( 255 - ( int )ag ) / 2 ),
		                             lighten( ab, ( 255 - ( int )ab ) / 2 ), 255 };

		// Translucent glass surfaces derived from the palette background.
		tokens::col_dark         = { br, bg, bb, to_u8( b.a ) };
		tokens::col_title_bar    = { lighten( br, 4 ), lighten( bg, 4 ), lighten( bb, 5 ), 240 };
		tokens::col_card         = { lighten( br, 12 ), lighten( bg, 12 ), lighten( bb, 14 ), 205 };
		tokens::col_elevated     = { lighten( br, 24 ), lighten( bg, 24 ), lighten( bb, 28 ), 235 };
		tokens::col_tab_bg       = tokens::col_card;
		tokens::col_slider_bg    = { 255, 255, 255, 24 };

		// Accent reserved for active / selected elements only.
		tokens::col_tab_active   = { ar, ag, ab, 255 };
		tokens::col_checkbox_on  = { ar, ag, ab, 255 };
		tokens::col_slider_fill  = { ar, ag, ab, 255 };

		// Subtle neutral borders/separators everywhere (no tinted boxes).
		tokens::col_border       = { 255, 255, 255, 22 };
		tokens::col_group_border = { 255, 255, 255, 16 };
		tokens::col_line         = { 255, 255, 255, 12 };
		tokens::col_edge         = { 255, 255, 255, 30 };

		// Text — palette text / subtext.
		tokens::col_text         = { tr, tg, tb, 245 };
		tokens::col_text_dim     = { sr, sg, sb, 255 };

		// Full token editor: when custom UI tokens are on, overlay every
		// token with the saved per-token colors so a theme becomes "preset
		// base + per-token overrides". This runs after the palette derivation
		// above, so the two sources compose instead of fighting.
		if ( th.use_custom_tokens.value )
		{
			tokens::col_accent        = th.custom_accent.value;
			tokens::col_accent_2      = th.custom_accent_2.value;
			tokens::col_dark          = th.custom_dark.value;
			tokens::col_card          = th.custom_card.value;
			tokens::col_elevated      = th.custom_elevated.value;
			tokens::col_title_bar     = th.custom_title_bar.value;
			tokens::col_tab_bg        = th.custom_tab_bg.value;
			tokens::col_text          = th.custom_text.value;
			tokens::col_text_dim      = th.custom_text_dim.value;
			tokens::col_border        = th.custom_border.value;
			tokens::col_group_border  = th.custom_group_border.value;
			tokens::col_line          = th.custom_line.value;
			tokens::col_edge          = th.custom_edge.value;
			tokens::col_tab_active    = th.custom_tab_active.value;
			tokens::col_checkbox_on   = th.custom_checkbox_on.value;
			tokens::col_slider_fill   = th.custom_slider_fill.value;
			tokens::col_slider_bg     = th.custom_slider_bg.value;
		}

		// Dedicated logo slot: default derives from the live accent, so a
		// preset (or custom accent) retints the logo too; a per-theme override
		// pins it to whatever color the user chose.
		tokens::col_logo = th.logo_color_override.value ? th.logo_color.value : tokens::col_accent;

		// HUD widgets that keep saved colors must follow the theme accent too,
		// otherwise switching presets leaves the media player card, its lyrics
		// highlight and the audio visualizer stuck on a stale hue.
		const xdraw::color theme_accent = tokens::col_accent;
		settings::g_misc.m_media_player.accent_color = theme_accent;
		settings::g_misc.m_media_player.lyrics_highlight = theme_accent;
		settings::g_misc.m_spectrum.color = theme_accent;

		// Grenade timers draw with saved colors the same way the media player
		// does, so a theme switch must retint them too: the smoke timer body and
		// the landing / fire-perimeter arcs are surface accents. The smoke
		// timer's "warning (<3s)" hue stays red on purpose (semantic alert, not
		// decoration) and the grenade-type icon stays white for legibility.
		auto& proj_overlay = settings::g_esp.m_projectile.m_overlay;
		proj_overlay.m_smoke_timer.color = theme_accent;
		for ( auto& g : proj_overlay.m_indicator.groups )
		{
			g.arc_color = theme_accent;
		}
	}

	void menu::apply_custom_accent( xdraw::color col )
	{
		tokens::col_accent       = col;
		tokens::col_group_border = col.alpha( 90 );
		tokens::col_edge         = col.alpha( 60 );
		tokens::col_tab_active   = col;
		tokens::col_checkbox_on  = col;
		tokens::col_slider_fill  = col;
		this->sync_theme_style( );
	}

} // namespace rendering
