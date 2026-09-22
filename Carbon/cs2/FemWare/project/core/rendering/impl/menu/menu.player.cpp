#include <pch/pch.hpp>
#include <core/settings.hpp>

#include "../../rendering.hpp"

namespace rendering {

	namespace detail {

		constexpr const char* k_cham_material_names[ ]{
			"liquid", "metallic", "matte", "flat", "bloom", "outlines", "glow", "electric", "distortion", "hologram", "pearl", "crystal", "velvet", "plasma", "glass",
			"liquid (iz)", "matte (iz)", "flat (iz)", "bloom (iz)", "outlines (iz)", "glow (iz)", "distortion (iz)", "hologram (iz)", "electric (iz)", "pearl (iz)", "crystal (iz)", "plasma (iz)", "glass (iz)"
		};
		constexpr auto k_cham_material_count = static_cast< int >( settings::esp::cham_ids::count );

		inline static void draw_chams_layer( const char* label, const char* popup_id, settings::esp::chams_layer& layer )
		{
			xui::checkbox( label, layer.enabled );
			if ( xui::begin_popup( popup_id, 220.0f ) )
			{
				xui::combo( "material", layer.material.value, k_cham_material_names, k_cham_material_count );
				xui::color_picker( "color", layer.color );
				xui::slider_float( "fresnel exponent", layer.fresnel_exponent, 0.1f, 10.0f, "%.1f" );
				xui::checkbox( "wireframe", layer.wireframe );
				xui::end_popup( );
			}
		}

		inline static void draw_chams_config( const char* label, const char* id_suffix, settings::esp::chams_config& cfg, bool show_overlay = true )
		{
			xui::checkbox( label, cfg.enabled );

			char label_buf[ 64 ]{};
			char popup_id[ 64 ]{};

			std::snprintf( label_buf, sizeof( label_buf ), "visible chams##%s", id_suffix );
			std::snprintf( popup_id, sizeof( popup_id ), "##visible_%s", id_suffix );
			draw_chams_layer( label_buf, popup_id, cfg.primary );

			std::snprintf( label_buf, sizeof( label_buf ), "non-visible chams##%s", id_suffix );
			std::snprintf( popup_id, sizeof( popup_id ), "##non_visible_%s", id_suffix );
			draw_chams_layer( label_buf, popup_id, cfg.secondary );

			if ( show_overlay )
			{
				std::snprintf( label_buf, sizeof( label_buf ), "overlay chams##%s", id_suffix );
				std::snprintf( popup_id, sizeof( popup_id ), "##overlay_%s", id_suffix );
				draw_chams_layer( label_buf, popup_id, cfg.overlay );
			}
		}

	} // namespace detail

	void menu::draw_player( float group_w ) const
	{
		auto& esp = settings::g_esp;
		auto& p = esp.m_player;

		const auto col_w = ( this->m_body_w - tokens::gap ) * 0.5f;
		const auto subtab = this->m_subtab;
		const auto has_overlay = ( subtab <= 1 );

		auto& glow = (subtab == 0) ? p.m_glow.enemy : p.m_glow.team;
		auto& glow_ragdoll = (subtab == 0) ? p.m_glow.enemy_ragdoll : p.m_glow.team_ragdoll;

		const auto gap      = tokens::gap;
		const auto card1_h  = std::floor( ( this->m_body_h - gap ) * 0.72f );
		const auto card2_h  = this->m_body_h - card1_h - gap;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( has_overlay )
		{
			auto& ov = p.m_overlay[ subtab ];

			if ( xui::begin_child( "##player_esp", col_w, card1_h, true ) )
			{
				group_header( subtab == 0 ? "Enemy ESP Overlay" : "Teammate ESP Overlay" );
				xui::checkbox( "esp overlay", ov.enabled );
				xui::checkbox( "visible only", ov.visible_only );
				xui::slider_float( "max distance", ov.max_distance, 0.0f, 200.0f, "%.0f m" );

				xui::checkbox( "bounding box", ov.m_box.enabled );
				if ( xui::begin_popup( "##box_popup", 220.0f ) )
				{
					constexpr const char* box_styles[ ]{ "full", "cornered" };
					xui::combo( "style##box", ov.m_box.style.value, box_styles, 2 );

					xui::checkbox( "fill", ov.m_box.fill );
					xui::checkbox( "outline", ov.m_box.outline );
					xui::slider_float( "corner length", ov.m_box.corner_length, 2.0f, 20.0f, "%.0f" );
					xui::color_picker( "visible color##box", ov.m_box.visible_color );
					xui::color_picker( "occluded color##box", ov.m_box.occluded_color );
					xui::end_popup( );
				}

				xui::checkbox( "skeleton", ov.m_skeleton.enabled );
				if ( xui::begin_popup( "##skeleton_popup", 220.0f ) )
				{
					constexpr const char* skel_modes[ ]{ "normal", "backtrack" };
					xui::combo( "mode##skel", ov.m_skeleton.type.value, skel_modes, 2 );
					xui::checkbox( "head dot##skel", ov.m_skeleton.head_dot );

					xui::slider_float( "thickness##skel", ov.m_skeleton.thickness, 0.5f, 4.0f, "%.1f" );
					xui::color_picker( "visible color##skel", ov.m_skeleton.visible_color );
					xui::color_picker( "occluded color##skel", ov.m_skeleton.occluded_color );
					xui::end_popup( );
				}

				xui::checkbox( "health bar", ov.m_health_bar.enabled );
				if ( xui::begin_popup( "##health_popup", 220.0f ) )
				{
					constexpr const char* bar_positions[ ]{ "left", "top", "bottom" };
					xui::combo( "position##hp", ov.m_health_bar.position.value, bar_positions, 3 );

					xui::checkbox( "outline##hp", ov.m_health_bar.outline_setting );
					xui::checkbox( "gradient##hp", ov.m_health_bar.gradient );
					xui::checkbox( "show value##hp", ov.m_health_bar.show_value );
					xui::checkbox( "glow##hp", ov.m_health_bar.glow );
					xui::checkbox( "segmented##hp", ov.m_health_bar.segmented );
					xui::checkbox( "damage drop##hp", ov.m_health_bar.damage_drop );
					xui::color_picker( "full color##hp", ov.m_health_bar.full_color );
					xui::color_picker( "low color##hp", ov.m_health_bar.low_color );
					xui::color_picker( "background##hp", ov.m_health_bar.background_color );
					xui::color_picker( "outline color##hp", ov.m_health_bar.outline_color );
					xui::color_picker( "text color##hp", ov.m_health_bar.text_color );
					xui::color_picker( "glow color##hp", ov.m_health_bar.glow_color );
					xui::color_picker( "damage drop color##hp", ov.m_health_bar.damage_drop_color );
					xui::slider_float( "glow strength##hp", ov.m_health_bar.glow_strength, 0.1f, 1.0f, "%.2f" );
					xui::end_popup( );
				}

				xui::checkbox( "ammo bar", ov.m_ammo_bar.enabled );
				if ( xui::begin_popup( "##ammo_popup", 220.0f ) )
				{
					constexpr const char* bar_positions[ ]{ "left", "top", "bottom" };
					xui::combo( "position##ammo", ov.m_ammo_bar.position.value, bar_positions, 3 );

					xui::checkbox( "outline##ammo", ov.m_ammo_bar.outline_setting );
					xui::checkbox( "gradient##ammo", ov.m_ammo_bar.gradient );
					xui::checkbox( "show value##ammo", ov.m_ammo_bar.show_value );
					xui::checkbox( "glow##ammo", ov.m_ammo_bar.glow );
					xui::color_picker( "full color##ammo", ov.m_ammo_bar.full_color );
					xui::color_picker( "low color##ammo", ov.m_ammo_bar.low_color );
					xui::color_picker( "background##ammo", ov.m_ammo_bar.background_color );
					xui::color_picker( "outline color##ammo", ov.m_ammo_bar.outline_color );
					xui::color_picker( "text color##ammo", ov.m_ammo_bar.text_color );
					xui::color_picker( "glow color##ammo", ov.m_ammo_bar.glow_color );
					xui::slider_float( "glow strength##ammo", ov.m_ammo_bar.glow_strength, 0.1f, 1.0f, "%.2f" );
					xui::end_popup( );
				}

				xui::checkbox( "name", ov.m_name.enabled );
				if ( xui::begin_popup( "##name_popup", 264.0f ) )
				{
					constexpr const char* name_fonts[ ]{ "inter", "bold", "pixel" };
					xui::combo( "font##name", ov.m_name.font.value, name_fonts, 3 );
					xui::slider_float( "size##name", ov.m_name.size, 8.0f, 20.0f, "%.0f" );
					xui::checkbox( "outline##name", ov.m_name.outline );
					xui::checkbox( "show id##name", ov.m_name.show_id );
					xui::checkbox( "show distance##name", ov.m_name.show_distance );
					xui::slider_float( "fade start##name", ov.m_name.fade_start, 100.0f, 4000.0f, "%.0f" );
					xui::slider_float( "fade end##name", ov.m_name.fade_end, 100.0f, 5000.0f, "%.0f" );

					constexpr const char* name_positions[ ]{ "above", "inline top" };
					int pos_idx = static_cast<int>( ov.m_name.position.value );
					if ( xui::combo( "position##name", pos_idx, name_positions, 2 ) )
						ov.m_name.position.value = static_cast< settings::esp::player::overlay::name::name_position >( pos_idx );

					xui::checkbox( "name box##name", ov.m_name.draw_box );

					xui::checkbox( "background##name", ov.m_name.background );
					xui::color_picker( "color##name", ov.m_name.color );
					xui::color_picker( "background color##name", ov.m_name.background_color );

					xui::layout::separator( );

					constexpr const char* theme_items[ ]{ "Femware", "Catppuccin", "Coffee", "Tokyo Night" };
					int theme_idx = static_cast<int>( settings::g_cheat.m_theme.selected.value );
					if ( xui::combo( "name theme##name", theme_idx, theme_items, 4 ) )
					{
						settings::g_cheat.m_theme.selected.value = static_cast< settings::theme::palette >( theme_idx );
					}
					xui::checkbox( "use theme color##name", ov.m_name.use_theme_color );
					xui::checkbox( "use theme background##name", ov.m_name.use_theme_background );
					xui::checkbox( "override theme colors##name", settings::g_cheat.m_theme.use_custom );
					if ( settings::g_cheat.m_theme.use_custom.value )
					{
						xui::color_picker( "theme text##name", settings::g_cheat.m_theme.text_color );
						xui::color_picker( "theme accent##name", settings::g_cheat.m_theme.accent_color );
						xui::color_picker( "theme background##name", settings::g_cheat.m_theme.background_color );
						xui::color_picker( "theme subtext##name", settings::g_cheat.m_theme.subtext_color );
					}

					xui::end_popup( );
				}

				xui::checkbox( "weapon", ov.m_weapon.enabled );
				if ( xui::begin_popup( "##weapon_popup", 220.0f ) )
				{
					constexpr const char* display_types[ ]{ "text", "icon", "text + icon" };
					xui::combo( "display##wep", ov.m_weapon.display.value, display_types, 3 );
					xui::slider_float( "icon scale##wep", ov.m_weapon.icon_scale, 0.4f, 2.0f, "%.2f" );

					xui::color_picker( "text color##wep", ov.m_weapon.text_color );
					xui::color_picker( "icon color##wep", ov.m_weapon.icon_color );
					xui::end_popup( );
				}

				xui::checkbox( "info flags", ov.m_info_flags.enabled );
				if ( xui::begin_popup( "##flags_popup", 220.0f ) )
				{
					constexpr const char* flag_names[ ]{ "money", "armor", "kit", "scoped", "defusing", "flashed", "ping", "distance" };
					xui::multicombo( "flags##mc", ov.m_info_flags.flags, flag_names, settings::esp::player::overlay::info_flags::count );

					xui::color_picker( "money##flags", ov.m_info_flags.money_color );
					xui::color_picker( "armor##flags", ov.m_info_flags.armor_color );
					xui::color_picker( "kit##flags", ov.m_info_flags.kit_color );
					xui::color_picker( "scoped##flags", ov.m_info_flags.scoped_color );
					xui::color_picker( "defusing##flags", ov.m_info_flags.defusing_color );
					xui::color_picker( "flashed##flags", ov.m_info_flags.flashed_color );
					xui::color_picker( "distance##flags", ov.m_info_flags.distance_color );
					xui::end_popup( );
				}

				xui::checkbox( "oof arrows", ov.m_oof_arrow.enabled );
				if ( xui::begin_popup( "##oof_popup", 220.0f ) )
				{
					xui::checkbox( "glow##oof", ov.m_oof_arrow.glow );
					xui::checkbox( "show distance##oof", ov.m_oof_arrow.show_distance );
					xui::checkbox( "distance fade##oof", ov.m_oof_arrow.distance_fade );
					xui::slider_float( "width##oof", ov.m_oof_arrow.width, 4.0f, 40.0f, "%.0f" );
					xui::slider_float( "height##oof", ov.m_oof_arrow.height, 4.0f, 40.0f, "%.0f" );
					xui::slider_float( "radius x##oof", ov.m_oof_arrow.radius_x, 50.0f, 600.0f, "%.0f" );
					xui::slider_float( "radius y##oof", ov.m_oof_arrow.radius_y, 50.0f, 600.0f, "%.0f" );
					xui::slider_float( "glow strength##oof", ov.m_oof_arrow.glow_strength, 0.1f, 1.0f, "%.2f" );
					xui::color_picker( "visible color##oof", ov.m_oof_arrow.visible_color );
					xui::color_picker( "occluded color##oof", ov.m_oof_arrow.occluded_color );
					xui::end_popup( );
				}


				xui::checkbox( "snap lines", ov.m_snap_lines.enabled );
				if ( xui::begin_popup( "##snap_popup", 220.0f ) )
				{
					constexpr const char* origins[ ]{ "screen bottom", "screen center" };
					xui::combo( "origin##snap", ov.m_snap_lines.origin.value, origins, 2 );
					xui::color_picker( "color##snap", ov.m_snap_lines.color );
					xui::end_popup( );
				}

				xui::checkbox( "view direction", ov.m_view_ray.enabled );
				if ( xui::begin_popup( "##ray_popup", 220.0f ) )
				{
					xui::slider_float( "length##ray", ov.m_view_ray.length, 30.0f, 300.0f, "%.0f" );
					xui::color_picker( "color##ray", ov.m_view_ray.color );
					xui::end_popup( );
				}

				xui::checkbox( "sound esp", ov.m_sound_esp.enabled );
				if ( xui::begin_popup( "##sound_esp_popup", 220.0f ) )
				{
					xui::slider_float( "radius##sound_esp", ov.m_sound_esp.max_radius, 20.0f, 150.0f, "%.0f" );
					xui::slider_float( "duration##sound_esp", ov.m_sound_esp.duration, 0.5f, 4.0f, "%.1fs" );
					xui::checkbox( "show label##sound_esp", ov.m_sound_esp.show_label );
					xui::checkbox( "offscreen arrows##sound_esp", ov.m_sound_esp.show_oof );
					xui::checkbox( "radar pulse##sound_esp", ov.m_sound_esp.radar_pulse );
					xui::color_picker( "color##sound_esp", ov.m_sound_esp.color );
					xui::end_popup( );
				}

				xui::end_child ();

				if ( xui::begin_child( "##player_glow", col_w, card2_h, true ) )
				{
					group_header( "Player Glow Effects" );
					xui::checkbox( "glow", glow.enabled );
					if ( xui::begin_popup( "##glow_popup", 220.0f ) )
					{
						xui::color_picker( "color##glow", glow.color );
						xui::end_popup( );
					}

					xui::checkbox( "ragdoll glow", glow_ragdoll.enabled );
					if ( xui::begin_popup( "##glow_rag_popup", 220.0f ) )
					{
						xui::color_picker( "color##glow_rag", glow_ragdoll.color );
						xui::end_popup( );
					}

					xui::end_child( );
				}
			}
		}
		else
		{
			if ( xui::begin_child( "##local_chams_glow", col_w, this->m_body_h, true ) )
			{
				group_header( "Local Player Models" );
				detail::draw_chams_config( "chams", "local_main", p.m_chams.local );

				xui::layout::separator( );

				xui::checkbox( "lower opacity", esp.m_local_alpha.enabled );
				if ( xui::begin_popup( "##local_alpha_popup", 220.0f ) )
				{
					xui::slider_float( "opacity", esp.m_local_alpha.opacity, 0.0f, 1.0f, "%.2f" );
					xui::checkbox( "only when scoped", esp.m_local_alpha.only_scoped );
					xui::end_popup( );
				}

				xui::layout::separator( );

				detail::draw_chams_config( "ragdoll chams", "local_ragdoll", p.m_chams.local_ragdoll, false );

				xui::layout::separator( );

				xui::checkbox( "glow", p.m_glow.local.enabled );
				if ( xui::begin_popup( "##local_glow_popup", 220.0f ) )
				{
					xui::color_picker( "color##local_glow", p.m_glow.local.color );
					xui::end_popup( );
				}

				xui::checkbox( "ragdoll glow", p.m_glow.local_ragdoll.enabled );
				if ( xui::begin_popup( "##local_glow_rag_popup", 220.0f ) )
				{
					xui::color_picker( "color##local_glow_rag", p.m_glow.local_ragdoll.color );
					xui::end_popup( );
				}

				xui::end_child( );
			}
		}

		xui::layout::set_cursor( this->m_body_x - this->m_x + col_w + tokens::gap, this->m_body_y - this->m_y );

		if ( has_overlay )
		{
			auto& chams = ( subtab == 0 ) ? p.m_chams.enemy : p.m_chams.team;
			auto& chams_ragdoll = ( subtab == 0 ) ? p.m_chams.enemy_ragdoll : p.m_chams.team_ragdoll;

			if ( xui::begin_child( "##player_chams", col_w, this->m_body_h, true ) )
			{
				group_header( subtab == 0 ? "Enemy Chams & Lagcomp" : "Teammate Chams" );
				detail::draw_chams_config( "chams", "main", chams );

				xui::layout::separator( );

				detail::draw_chams_config( "ragdoll chams", "ragdoll", chams_ragdoll, false );

				if ( subtab == 0 )
				{
					xui::layout::separator( );

					detail::draw_chams_config( "backtrack chams", "bt", p.m_chams.backtrack, false );
					detail::draw_chams_config( "onshot chams", "os", p.m_chams.onshot, false );

					xui::slider_float( "fade##ft", p.m_chams.onshot_fade_time, 0.1f, 5.0f, "%.0f" );
				}

				xui::end_child( );
			}
		}
		else
		{
			if ( xui::begin_child( "##viewmodel", col_w, this->m_body_h, true ) )
			{
				group_header( "Viewmodel & Hands" );
				detail::draw_chams_config( "weapon chams", "vm_weapon", esp.m_viewmodel.weapon );

				xui::layout::separator( );

				detail::draw_chams_config( "arms chams", "vm_arms", esp.m_viewmodel.arms );

				xui::end_child( );
			}
		}
	}

} // namespace rendering