#include <pch/pch.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <core/features/changer/model_store.hpp>
#include <utilities/tunnel.hpp>
#include <core/features/misc/impl/radar_server.hpp>
#include <core/systems/systems.hpp>
#include <utilities/memory/memory.hpp>

#include "../../rendering.hpp"

namespace rendering {

	namespace detail {

		constexpr const char* sound_types[ ]{ "shop click", "home click", "bell", "killcard", "bullet casing", "coin pickup", "item drop", "popcan", "key press", "custom" };
		constexpr auto k_sound_type_count{ static_cast< int >( std::size( sound_types ) ) };

		void draw_custom_sound_picker( config::str& file_setting, std::string_view combo_label, std::string_view preview_id, float preview_volume )
		{
			const auto files = features::misc::impacts::list_custom_sounds( );

			static std::vector<std::string> cached_files{};
			static std::vector<const char*> cached_ptrs{};
			cached_files = files;
			cached_ptrs.clear( );
			cached_ptrs.reserve( cached_files.size( ) );

			for ( const auto& file : cached_files )
			{
				cached_ptrs.push_back( file.c_str( ) );
			}

			if ( !cached_ptrs.empty( ) )
			{
				auto selected{ 0 };
				for ( auto i = 0; i < static_cast< int >( cached_files.size( ) ); ++i )
				{
					if ( cached_files[ static_cast< std::size_t >( i ) ] == file_setting.value )
					{
						selected = i;
						break;
					}
				}

				if ( xui::combo( combo_label, selected, cached_ptrs.data( ), static_cast< int >( cached_ptrs.size( ) ) ) )
				{
					file_setting = cached_files[ static_cast< std::size_t >( selected ) ];
				}
			}

			xui::text_input( "file", file_setting.value, 64, "hit.wav" );

			if ( xui::button( preview_id, 96.0f, 22.0f ) )
			{
				features::misc::g_impacts.play_custom_sound( file_setting.value, preview_volume );
			}
		}
		constexpr const char* marker_types[ ]{ "classic", "damage", "both" };
		constexpr const char* impact_types[ ]{ "overlay", "sparks", "both" };

		constexpr const char* primary_weapons[ ]{ "none", "rifle", "scoped rifle", "scout", "awp", "auto sniper" };
		constexpr const char* secondary_weapons[ ]{ "none", "dual elites", "five-seven/tec-9", "deagle", "revolver" };
		constexpr const char* grenade_names[ ]{ "molotov", "he grenade", "smoke", "flashbang", "decoy" };

		constexpr const char* spark_types[ ]{ "stars", "hearts", "bloom", "glyph", "blink", "coron", "dollar", "flame", "geometric", "snowflake", "virus", "sword", "network", "cube", "pyramid" };
		constexpr const char* spark_physics[ ]{ "fall", "fly", "emerge" };
	} // namespace detail

	void menu::draw_misc( float group_w ) const
	{
		auto& m = settings::g_misc;

		const auto wx      = this->m_x;
		const auto wy      = this->m_y;
		const auto body_x  = this->m_body_x;
		const auto body_y  = this->m_body_y;
		const auto body_w  = this->m_body_w;
		const auto col_w   = ( body_w - tokens::col_gap ) * 0.5f;
		const auto right_x = body_x + col_w + tokens::col_gap;

		const auto subtab = this->m_subtab;

		xui::layout::set_cursor( body_x - wx, body_y - wy );

		if ( subtab == 0 )
		{
			auto& mov = settings::g_movement;
			auto& hud = m.m_hud;

			// ── Left Column: Movement Assistance & Strafer ──
			xui::layout::set_cursor( body_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_movement", col_w, this->m_body_h, true ) )
			{
				group_header( "Movement & Strafer" );
				xui::checkbox( "bhop", mov.bhop );
				if ( xui::begin_popup( "##bhop_popup", 220.0f ) )
				{
					xui::slider_int( "hitchance##bhop", mov.bhop_hitchance, 10, 100, "%d%%" );
					xui::slider_int( "max consecutive##bhop", mov.bhop_max_consecutive, 0, 15, "%d hops" );
					xui::checkbox( "avoid perfect bhops", mov.bhop_avoid_perfection );
					xui::slider_int( "perfection delay", mov.bhop_perfection_delay_ms, 1, 150, "%d ms" );
					xui::end_popup( );
				}

				xui::checkbox( "airstrafe", mov.airstrafe );
				if ( xui::begin_popup( "##airstrafe_popup", 220.0f ) )
				{
					xui::checkbox( "fully directional", mov.airstrafe_fully_directional );
					xui::slider_float( "turn rate limit##as", mov.airstrafe_turn_rate_limit, 0.0f, 30.0f, "%.1f deg" );
					xui::slider_float( "humanized wobble##as", mov.airstrafe_wobble, 0.0f, 100.0f, "%.0f%%" );
					xui::end_popup( );
				}

				xui::checkbox( "test strafer", mov.m_test_strafer.enabled );
				xui::checkbox( "jumpbug", mov.jumpbug );
				xui::checkbox( "fastladder", mov.fastladder );
				xui::checkbox( "edgejump", mov.edgejump );
				xui::checkbox( "edgestop", mov.edgestop );
				xui::checkbox( "edgebug", mov.edgebug );
				if ( xui::begin_popup( "##edgebug_popup", 240.0f ) )
				{
					static const char* edgebug_modes[] = { "0: loose", "1: edge trace (default)", "2: no jump held", "3: min speed", "4: strict vz" };
					xui::combo( "mode##eb", mov.edgebug_mode.value, edgebug_modes, 5 );
					xui::slider_int( "passes##eb", mov.edgebug_passes, 1, 5, "%d" );
					xui::checkbox( "jump steps##eb", mov.edgebug_include_jump_steps );
					xui::end_popup( );
				}

				xui::checkbox( "slowwalk", mov.slowwalk );
				if ( xui::begin_popup( "##slowwalk_popup", 220.0f ) )
				{
					xui::slider_float( "speed", mov.slowwalk_speed, 1.0f, 100.0f, "%.2fs" );
					xui::end_popup( );
				}

				xui::end_child( );
			}

			// ── Right Column: Velocity & Movement Indicators ──
			xui::layout::set_cursor( right_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_speedometer", col_w, this->m_body_h, true ) )
			{
				group_header( "Velocity & Indicators" );
				xui::checkbox( "velocity counter", hud.m_velocity.counter );
				xui::checkbox( "velocity chart", hud.m_velocity.chart );
				if ( xui::begin_popup( "##velocity_hud_popup", 220.0f ) )
				{
					xui::color_picker( "color##velocity", hud.m_velocity.color );
					xui::slider_float( "bottom offset", hud.m_velocity.bottom_offset, 20.0f, 200.0f, "%.0f" );
					xui::slider_float( "chart width", hud.m_velocity.chart_width, 120.0f, 320.0f, "%.0f" );
					xui::slider_float( "chart height", hud.m_velocity.chart_height, 24.0f, 80.0f, "%.0f" );
					xui::end_popup( );
				}

				xui::end_child( );
			}
		}

		// ══════════════════════════════════════════════════════════════════════
		// SUBTAB 1: EFFECTS & AUDIO (Hitsounds, Markers, Beams, Reticles)
		// ══════════════════════════════════════════════════════════════════════
		if ( subtab == 1 )
		{
			auto& impacts = m.m_impacts;
			auto& hud = m.m_hud;

			// ── Left Column: Hit Effects, Sounds & Logs ──
			xui::layout::set_cursor( body_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_hit_effects", col_w, this->m_body_h, true ) )
			{
				group_header( "Hit Effects & Audio" );
				xui::checkbox( "hit logs", impacts.hit_log );
				if ( xui::begin_popup( "##hitlog_popup", 220.0f ) )
				{
					xui::slider_float( "duration##hl", impacts.hit_log_duration, 0.5f, 10.0f, "%.1fs" );
					xui::end_popup( );
				}

				xui::checkbox( "console logs", impacts.console_log );
				xui::checkbox( "chat logs", impacts.chat_log );

				xui::checkbox( "miss logs", impacts.miss_log );
				if ( xui::begin_popup( "##misslog_popup", 220.0f ) )
				{
					xui::slider_float( "duration##ml", impacts.miss_log_duration, 0.5f, 10.0f, "%.1fs" );
					xui::end_popup( );
				}

				xui::checkbox( "hit sound", impacts.hit_sound );
				if ( xui::begin_popup( "##hitsound_popup", 220.0f ) )
				{
					xui::combo( "type##hs", impacts.hit_sound_type.value, detail::sound_types, detail::k_sound_type_count );
					xui::slider_float( "volume##hs", impacts.hit_sound_volume, 1.0f, 100.0f, "%.0f%%" );

					if ( impacts.hit_sound_type.value == settings::misc::impacts::sound_type::custom )
					{
						detail::draw_custom_sound_picker( impacts.custom_hit_sound, "sound##hs", "preview##hs", impacts.hit_sound_volume.value );
					}

					xui::end_popup( );
				}

				xui::checkbox( "death sound", impacts.death_sound );
				if ( xui::begin_popup( "##deathsound_popup", 220.0f ) )
				{
					xui::combo( "type##ds", impacts.death_sound_type.value, detail::sound_types, detail::k_sound_type_count );
					xui::slider_float( "volume##ds", impacts.death_sound_volume, 1.0f, 100.0f, "%.0f%%" );

					if ( impacts.death_sound_type.value == settings::misc::impacts::sound_type::custom )
					{
						detail::draw_custom_sound_picker( impacts.custom_death_sound, "sound##ds", "preview##ds", impacts.death_sound_volume.value );
					}

					xui::end_popup( );
				}

				xui::layout::separator( );

				xui::checkbox( "hit marker", impacts.hit_marker );
				if ( xui::begin_popup( "##hitmarker_popup", 220.0f ) )
				{
					xui::combo( "type##hm", impacts.hit_marker_type.value, detail::marker_types, 3 );
					xui::slider_float( "duration##hm", impacts.hit_marker_duration, 0.1f, 5.0f, "%.1fs" );
					xui::color_picker( "color##hm", impacts.hit_marker_color );
					xui::end_popup( );
				}

				xui::checkbox( "damage indicators", impacts.m_damage_indicators.enabled );
				if ( xui::begin_popup( "##damagelog_popup", 220.0f ) )
				{
					xui::slider_float( "duration##di", impacts.m_damage_indicators.duration, 0.2f, 3.0f, "%.1fs" );
					xui::slider_float( "rise##di", impacts.m_damage_indicators.rise, 10.0f, 150.0f, "%.0f" );
					xui::slider_float( "arc drift##di", impacts.m_damage_indicators.arc, -80.0f, 80.0f, "%.0f" );
					xui::slider_float( "outline##di", impacts.m_damage_indicators.outline, 0.0f, 1.0f, "%.2f" );
					xui::checkbox( "random offset##di", impacts.m_damage_indicators.random_offset );
					xui::checkbox( "stacking##di", impacts.m_damage_indicators.stacking );
					if ( impacts.m_damage_indicators.stacking.value )
					{
						xui::slider_float( "stack window##di", impacts.m_damage_indicators.stack_window, 0.05f, 1.0f, "%.2fs" );
					}
					xui::checkbox( "headshot marker##di", impacts.m_damage_indicators.crit_marker );
					xui::checkbox( "color by damage##di", impacts.m_damage_indicators.color_by_damage );
					if ( impacts.m_damage_indicators.color_by_damage.value )
					{
						xui::color_picker( "low color##di", impacts.m_damage_indicators.low_color );
						xui::color_picker( "high color##di", impacts.m_damage_indicators.high_color );
					}
					else
					{
						xui::color_picker( "color##di", impacts.m_damage_indicators.color );
						xui::color_picker( "headshot##di", impacts.m_damage_indicators.headshot_color );
					}
					xui::end_popup( );
				}

				xui::checkbox( "hit effect", impacts.hit_effect );
				if ( xui::begin_popup( "##hitfx_popup", 220.0f ) )
				{
					xui::color_picker( "color##hitfx", impacts.hit_effect_color );
					xui::slider_float( "duration##hitfx", impacts.hit_effect_duration, 0.1f, 5.0f, "%.1fs" );
					xui::slider_float( "strength##hitfx", impacts.hit_effect_strength, 1.0f, 100.0f, "%.0f%%" );
					xui::end_popup( );
				}

				xui::checkbox( "hit sparks", impacts.m_hit_sparks.enabled );
				if ( xui::begin_popup( "##hit_sparks_popup", 230.0f ) )
				{
					xui::combo( "type##sparks", impacts.m_hit_sparks.type.value, detail::spark_types, 15 );
					xui::combo( "physics##sparks", impacts.m_hit_sparks.physics.value, detail::spark_physics, 3 );
					xui::slider_int( "count##sparks", impacts.m_hit_sparks.count, 4, 48 );
					xui::slider_float( "speed##sparks", impacts.m_hit_sparks.speed, 20.0f, 300.0f, "%.0f" );
					xui::checkbox( "color wave##sparks", impacts.m_hit_sparks.wave );
					xui::slider_float( "link distance##sparks", impacts.m_hit_sparks.link_distance, 1.0f, 10.0f, "%.1f" );
					xui::slider_float( "size##sparks", impacts.m_hit_sparks.size, 1.5f, 8.0f, "%.1f" );
					xui::slider_float( "lifetime##sparks", impacts.m_hit_sparks.lifetime, 0.2f, 1.5f, "%.2fs" );
					xui::slider_float( "gravity##sparks", impacts.m_hit_sparks.gravity, 0.0f, 100.0f, "%.0f" );
					xui::color_picker( "color##sparks", impacts.m_hit_sparks.color );
					xui::color_picker( "secondary color##sparks", impacts.m_hit_sparks.secondary_color );
					xui::checkbox( "headshot boost##sparks", impacts.m_hit_sparks.headshot_boost );
					xui::color_picker( "headshot color##sparks", impacts.m_hit_sparks.headshot_color );
					xui::end_popup( );
				}

				xui::checkbox( "death effect", impacts.death_effect );
				if ( xui::begin_popup( "##deathfx_popup", 240.0f ) )
				{
					constexpr const char* kill_modes[ ]{ "star", "heart", "bloom", "dollar", "flame", "snowflake", "cross", "ring" };
					constexpr const char* kill_physics[ ]{ "fall", "fly", "emerge" };
					constexpr const char* kill_appear[ ]{ "instant", "pop", "grow", "fade in" };
					constexpr const char* kill_disappear[ ]{ "fade", "shrink", "shrink + fade", "blink", "rise" };

					xui::combo( "mode##killfx", impacts.m_kill_effect.mode.value, kill_modes, 8 );
					xui::combo( "physics##killfx", impacts.m_kill_effect.physics.value, kill_physics, 3 );
					xui::combo( "appear##killfx", impacts.m_kill_effect.spawn_style.value, kill_appear, 4 );
					xui::combo( "disappear##killfx", impacts.m_kill_effect.fade_style.value, kill_disappear, 5 );
					xui::slider_int( "amount##killfx", impacts.m_kill_effect.amount, 1, 100 );
					xui::slider_float( "density##killfx", impacts.m_kill_effect.density, 0.1f, 2.0f, "%.2fx" );
					xui::slider_float( "spread##killfx", impacts.m_kill_effect.spread, 0.0f, 40.0f, "%.1f" );
					xui::slider_float( "scale##killfx", impacts.m_kill_effect.scale, 0.3f, 5.0f, "%.2f" );
					xui::slider_float( "lifetime##killfx", impacts.m_kill_effect.lifetime, 0.3f, 8.0f, "%.1fs" );
					xui::slider_float( "speed##killfx", impacts.m_kill_effect.speed, 0.1f, 5.0f, "%.2f" );
					xui::checkbox( "color by health##killfx", impacts.m_kill_effect.color_by_health );
					if ( impacts.m_kill_effect.color_by_health.value )
					{
						xui::color_picker( "low color##killfx", impacts.m_kill_effect.color );
						xui::color_picker( "high color##killfx", impacts.m_kill_effect.health_color );
					}
					else
					{
						xui::color_picker( "color##killfx", impacts.m_kill_effect.color );
					}
					xui::checkbox( "thunder##killfx", impacts.m_kill_effect.thunder );
					xui::color_picker( "engine color##deathfx", impacts.death_effect_color );
					xui::end_popup( );
				}

				xui::end_child( );
			}

			// ── Right Column: Bullet Impacts & Weapon Reticles ──
			xui::layout::set_cursor( right_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_bullet_effects", col_w, this->m_body_h, true ) )
			{
				group_header( "Bullet Beams & Tracers" );
				xui::checkbox( "bullet impacts", impacts.bullet_impact_effect );
				if ( xui::begin_popup( "##bulletfx_popup", 220.0f ) )
				{
					xui::combo( "type##bulletfx", impacts.bullet_impact_effect_type.value, detail::impact_types, 3 );

					const auto type = impacts.bullet_impact_effect_type.value;
					const auto show_overlay = type == settings::misc::impacts::bullet_impact_type::overlay || type == settings::misc::impacts::bullet_impact_type::both;
					const auto show_sparks = type == settings::misc::impacts::bullet_impact_type::sparks || type == settings::misc::impacts::bullet_impact_type::both;

					if ( show_overlay )
					{
						constexpr const char* geom_names[ ]{ "square box", "diamond", "glowing dot", "3d cross" };
						xui::combo( "geometry##bulletfx", impacts.impact_geometry.value, geom_names, 4 );
						xui::slider_float( "marker size##bulletfx", impacts.impact_marker_size, 1.0f, 10.0f, "%.1fpx" );
						xui::slider_float( "duration##bulletfx", impacts.bullet_impact_effect_duration, 0.1f, 5.0f, "%.1fs" );
						xui::color_picker( "fill##bulletfx", impacts.bullet_impact_effect_fill_color );
						xui::color_picker( "edge##bulletfx", impacts.bullet_impact_effect_edge_color );

						xui::checkbox( "glow##bulletfx", impacts.bullet_impact_effect_glow );
						if ( impacts.bullet_impact_effect_glow )
						{
							xui::slider_float( "glow strength##bulletfx", impacts.bullet_impact_effect_glow_strength, 0.1f, 1.0f, "%.2f" );
						}
					}

					if ( show_sparks )
					{
						xui::color_picker( "spark##bulletfx", impacts.bullet_impact_effect_color_spark );
					}

					xui::end_popup( );
				}

				xui::checkbox( "bullet tracers", impacts.bullet_tracers );
				if ( xui::begin_popup( "##tracers_popup", 220.0f ) )
				{
					xui::slider_float( "duration##tracer", impacts.bullet_tracer_duration, 0.1f, 5.0f, "%.1fs" );
					xui::color_picker( "color##tracer", impacts.bullet_tracer_color );
					xui::end_popup( );
				}

				xui::checkbox( "bullet beams", impacts.bullet_beams );
				if ( xui::begin_popup( "##beams_popup", 220.0f ) )
				{
					constexpr const char* beam_styles[ ]{ "solid neon", "dashed laser", "fading smoke", "electric arc" };
					xui::combo( "style##beam", impacts.beam_style.value, beam_styles, 4 );
					xui::slider_float( "thickness##beam", impacts.bullet_beam_thickness, 0.5f, 5.0f, "%.1fpx" );
					xui::slider_float( "duration##beam", impacts.bullet_beam_duration, 0.1f, 5.0f, "%.1fs" );
					xui::color_picker( "color##beam", impacts.bullet_beam_color );
					xui::end_popup( );
				}

				xui::layout::separator( );

				group_header( "Crosshairs & Overlays" );
				xui::checkbox( "crosshair overlay", hud.m_crosshair.enabled );
				if ( xui::begin_popup( "##xhair_popup", 230.0f ) )
				{
					constexpr const char* xhair_styles[]{ "dot", "cross", "circle", "t style", "cross dot" };
					auto current_style = static_cast< int >( hud.m_crosshair.style.value );
					if ( xui::combo( "style##xhair", current_style, xhair_styles, 5 ) )
					{
						hud.m_crosshair.style.value = current_style;
					}
					xui::slider_float( "size##xhair", hud.m_crosshair.size, 1.0f, 25.0f, "%.1f" );
					xui::slider_float( "outline##xhair", hud.m_crosshair.outline, 0.0f, 4.0f, "%.1f" );
					xui::checkbox( "dynamic recoil spread##xhair", hud.m_crosshair.dynamic_spread );
					xui::checkbox( "hit pulse##xhair", hud.m_crosshair.hit_pulse );
					xui::slider_float( "glow strength##xhair", hud.m_crosshair.glow_strength, 0.0f, 1.0f, "%.2f" );
					xui::color_picker( "color##xhair", hud.m_crosshair.color );
					xui::color_picker( "outline color##xhair", hud.m_crosshair.outline_color );
					xui::end_popup( );
				}

				xui::checkbox( "scope overlay", hud.m_scope.enabled );
				if ( xui::begin_popup( "##scope_popup", 220.0f ) )
				{
					xui::slider_float( "line length", hud.m_scope.line_length, 10.0f, 500.0f, "%.0f" );
					xui::slider_float( "gap##scope", hud.m_scope.gap, 0.0f, 50.0f, "%.0f" );
					xui::slider_float( "thickness##scope", hud.m_scope.thickness, 0.5f, 5.0f, "%.2f" );
					xui::slider_float( "anim speed", hud.m_scope.anim_speed, 1.0f, 30.0f, "%.0f" );
					xui::color_picker( "color##scope", hud.m_scope.color );
					xui::checkbox( "fade in##scope", hud.m_scope.fade_in );

					xui::layout::separator( );

					xui::checkbox( "glow##scope", hud.m_scope.glow );
					xui::slider_float( "glow strength##scope", hud.m_scope.glow_strength, 0.1f, 1.0f, "%.2f" );
					xui::end_popup( );
				}

				xui::checkbox( "spread circle", hud.m_recoil.shown );
				xui::checkbox( "recoil dot", hud.m_recoil.show_recoil_dot );
				if ( xui::begin_popup( "##recoil_hud_popup", 220.0f ) )
				{
					xui::color_picker( "spread color##recoil", hud.m_recoil.spread_color );
					xui::slider_float( "spread thickness", hud.m_recoil.spread_thickness, 0.5f, 3.0f, "%.1f" );
					xui::color_picker( "dot color##recoil", hud.m_recoil.dot_color );
					xui::slider_float( "dot size", hud.m_recoil.dot_size, 1.0f, 6.0f, "%.1f" );
					xui::end_popup( );
				}

				auto& pen = settings::g_combat.m_penetration_crosshair;
				xui::checkbox( "penetration crosshair", pen.enabled );
				if ( xui::begin_popup( "##pen_popup", 220.0f ) )
				{
					xui::checkbox( "glow##pen", pen.glow );
					xui::slider_float( "glow strength##pen", pen.glow_strength, 0.1f, 1.0f, "%.2f" );
					xui::color_picker( "can penetrate##pen", pen.can_penetrate_fill );
					xui::color_picker( "can pen outline##pen", pen.can_penetrate_outline );
					xui::color_picker( "blocked##pen", pen.blocked_fill );
					xui::color_picker( "blocked outline##pen", pen.blocked_outline );
					xui::end_popup( );
				}

				xui::end_child( );
			}
		}

		// ══════════════════════════════════════════════════════════════════════
		// SUBTAB 2: HUD & OVERLAYS (Watermark, Keybinds, Spectator, Media Player)
		// ══════════════════════════════════════════════════════════════════════
		if ( subtab == 2 )
		{
			auto& hud = m.m_hud;

			// ── Left Column: Interface Overlays & FW Logo ──
			xui::layout::set_cursor( body_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_hud_overlays", col_w, this->m_body_h, true ) )
			{
				group_header( "Interface Overlays" );
				xui::checkbox( "watermark", m.m_watermark.enabled );
				if ( xui::begin_popup( "##watermark_popup", 200.0f ) )
				{
					xui::checkbox( "fps##wm",       m.m_watermark.show_fps );
					xui::checkbox( "ping##wm",      m.m_watermark.show_ping );
					xui::checkbox( "time##wm",      m.m_watermark.show_time );
					xui::checkbox( "user##wm",      m.m_watermark.show_user );
					xui::checkbox( "map##wm",       m.m_watermark.show_map );
					xui::checkbox( "tick rate##wm", m.m_watermark.show_tick );
					xui::checkbox( "velocity##wm",  m.m_watermark.show_velocity );
					xui::end_popup( );
				}

				xui::checkbox( "keybind list", m.m_keybind_list.enabled );

				xui::checkbox( "spectate enemies", m.m_spectate.enabled );
				xui::checkbox( "auto switch on death##sp", m.m_spectate.auto_switch );
				xui::checkbox( "draw spectated info##sp", m.m_spectate.draw_info );
				xui::keybind( "cycle key##sp", m.m_spectate.cycle_key.value );
				xui::keybind( "reverse key##sp", m.m_spectate.reverse_key.value );
				if ( xui::begin_popup( "##misc_spectate_colors", 200.0f ) )
				{
					xui::color_picker( "info##sp", m.m_spectate.info_color );
					xui::color_picker( "hp##sp", m.m_spectate.hp_color );
					xui::color_picker( "background##sp", m.m_spectate.bg_color );
					xui::end_popup( );
				}

				xui::checkbox( "scoreboard weapons", m.m_scoreboard_weapons.enabled );

				xui::checkbox( "fw user logo badge", m.m_scoreboard_weapons.fw_logo );

				xui::layout::separator( );

				group_header( "FW Logo Widget" );
				xui::checkbox( "fw logo", hud.m_fw_logo.enabled );
				if ( xui::begin_popup( "##fw_logo_popup", 220.0f ) )
				{
					static const char* modes[] = { "Normal", "Glowing", "Spinning", "Spinning Glowing" };
					xui::combo( "mode##fw_logo", hud.m_fw_logo.mode.value, modes, 4 );
					xui::slider_float( "size##fw_logo", hud.m_fw_logo.size, 24.0f, 256.0f, "%.0f px" );
					xui::slider_float( "speed##fw_logo", hud.m_fw_logo.speed, 0.1f, 5.0f, "%.1fx" );
					xui::slider_int( "fps##fw_logo", hud.m_fw_logo.fps, 10, 60, "%d fps" );
					xui::end_popup( );
				}

				xui::end_child( );
			}

			// ── Right Column: Streaming & Network ──
			xui::layout::set_cursor( right_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_streaming", col_w, this->m_body_h, true ) )
			{
				group_header( "Streamproof & Utilities" );
				xui::checkbox( "streamproof", m.m_streamproof.enabled );
				if ( m.m_streamproof.enabled.value )
				{
					xui::checkbox( "hide menu on stream", m.m_streamproof.hide_menu );
				}
				else if ( xui::begin_popup( "##streamproof_popup", 200.0f ) )
				{
					xui::checkbox( "hide menu on stream", m.m_streamproof.hide_menu );
					xui::end_popup( );
				}

				xui::checkbox( "reveal radar", m.reveal_radar );
				xui::checkbox( "webradar stream", settings::g_esp.m_other.m_radar.webradar_enabled );
				if ( settings::g_esp.m_other.m_radar.webradar_enabled.value )
				{
					constexpr const char* duration_names[ ]{ "15 minutes", "30 minutes", "1 hour", "session" };
					xui::combo( "duration##wr", settings::g_esp.m_other.m_radar.webradar_duration.value, duration_names, 4 );

					const auto time_str = features::misc::g_radar_server.get_time_remaining_str( );
					const auto status_text = "status: " + time_str;
					xui::text( status_text, xdraw::color{ 96, 165, 250, 255 } );

					if ( xui::button( "copy lan radar link", xui::layout::item_width( ), 22.0f ) )
					{
						features::misc::g_radar_server.copy_lan_link_to_clipboard( );
					}

					if ( !tunnel::running( ) )
					{
						if ( xui::button( "start sharing (secure tunnel)", xui::layout::item_width( ), 22.0f ) )
						{
							tunnel::start( settings::g_esp.m_other.m_radar.webradar_port.value );
						}
					}
					else
					{
						const auto share = tunnel::share_url( );
						if ( !share.empty( ) )
						{
							const auto full_link = share + "?token=" + features::misc::g_radar_server.get_active_token( );
							if ( xui::button( "copy share link##wr", xui::layout::item_width( ), 22.0f ) )
							{
								if ( OpenClipboard( nullptr ) )
								{
									EmptyClipboard( );
									const auto len = full_link.size( ) + 1;
									if ( const auto mem = GlobalAlloc( GMEM_MOVEABLE, len ) )
									{
										std::memcpy( GlobalLock( mem ), full_link.c_str( ), len );
										GlobalUnlock( mem );
										SetClipboardData( CF_TEXT, mem );
									}
									CloseClipboard( );
								}
							}
							xui::text( xui::truncate( share, xui::layout::item_width( ) ), tokens::col_accent );
						}
						else
						{
							xui::text( tunnel::status( ), tokens::col_text_dim );
						}
						if ( xui::button( "stop sharing##wr", xui::layout::item_width( ), 22.0f ) )
						{
							tunnel::stop( );
						}
					}

					if ( xui::button( "new timed token", xui::layout::item_width( ), 22.0f ) )
					{
						features::misc::g_radar_server.generate_new_token( );
					}
				}
				else if ( xui::begin_popup( "##webradar_popup", 220.0f ) )
				{
					constexpr const char* duration_names[ ]{ "15 minutes", "30 minutes", "1 hour", "session" };
					xui::combo( "duration", settings::g_esp.m_other.m_radar.webradar_duration.value, duration_names, 4 );
					if ( xui::button( "copy lan link", xui::layout::item_width( ), 22.0f ) )
					{
						features::misc::g_radar_server.copy_lan_link_to_clipboard( );
					}
					if ( xui::button( "new token", xui::layout::item_width( ), 22.0f ) )
					{
						features::misc::g_radar_server.generate_new_token( );
					}
					xui::end_popup( );
				}

				xui::layout::separator( );

				group_header( "Media Player (Spotify)" );
				xui::checkbox( "media player (spotify)", m.m_media_player.enabled );
				if ( m.m_media_player.enabled.value )
				{
					constexpr const char* pos_names[ ]{ "top center", "top right", "bottom left", "bottom right", "custom / draggable" };
					xui::combo( "position##mp", m.m_media_player.position.value, pos_names, 5 );
					xui::checkbox( "auto hide when paused", m.m_media_player.auto_hide );
					xui::keybind( "play / pause key##mp", m.m_media_player.key_play_pause.value );
					xui::keybind( "next track key##mp", m.m_media_player.key_next.value );
					xui::keybind( "prev track key##mp", m.m_media_player.key_prev.value );
					xui::color_picker( "accent color##mp", m.m_media_player.accent_color );

					xui::checkbox( "fetch lyrics (lrclib)##mp", m.m_media_player.fetch_lyrics );
					if ( m.m_media_player.fetch_lyrics.value )
					{
						xui::slider_float( "lyrics offset##mp", m.m_media_player.lyrics_offset_ms, -1000.0f, 1000.0f, "%.0fms" );
						xui::slider_float( "lyrics size##mp", m.m_media_player.lyrics_size, 0.6f, 2.0f, "%.2f" );
						xui::color_picker( "lyrics color##mp", m.m_media_player.lyrics_color );
						xui::color_picker( "lyrics highlight##mp", m.m_media_player.lyrics_highlight );
					}
				}

				xui::layout::separator( );

				group_header( "Audio Visualizer (HUD)" );
				xui::checkbox( "audio visualizer hud", m.m_spectrum.enabled );
				if ( xui::begin_popup( "##viz_popup", 220.0f ) )
				{
					xui::slider_float( "width##viz", m.m_spectrum.width, 80.0f, 800.0f, "%.0f" );
					xui::slider_float( "height##viz", m.m_spectrum.height, 16.0f, 160.0f, "%.0f" );
					xui::slider_float( "sensitivity##viz", m.m_spectrum.sensitivity, 0.2f, 6.0f, "%.2f" );
					xui::color_picker( "color##viz", m.m_spectrum.color );
					xui::end_popup( );
				}
				xui::text( "drag the widget to reposition while the menu is open", tokens::col_text_dim );

				xui::end_child( );
			}
		}

		// ══════════════════════════════════════════════════════════════════════
		// SUBTAB 3: GENERAL & SOCIAL (Auto Buy, Clan Tag, Discord RPC, Game)
		// ══════════════════════════════════════════════════════════════════════
		if ( subtab == 3 )
		{
			auto& ab = m.m_autobuy;

			// ── Left Column: Gameplay Automation ──
			xui::layout::set_cursor( body_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_automation", col_w, this->m_body_h, true ) )
			{
				group_header( "Gameplay Automation" );
				xui::checkbox( "auto buy", ab.enabled );
				if ( xui::begin_popup( "##autobuy_popup", 220.0f ) )
				{
					xui::combo( "primary##ab", ab.primary_weapon, detail::primary_weapons, 6 );
					xui::combo( "secondary##ab", ab.secondary_weapon, detail::secondary_weapons, 5 );
					xui::checkbox( "armor##ab", ab.armor );
					xui::checkbox( "defuser##ab", ab.defuser );
					xui::checkbox( "taser##ab", ab.taser );
					xui::multicombo( "grenades##ab", ab.grenades, detail::grenade_names, 5 );
					xui::end_popup( );
				}

				xui::checkbox( "auto accept", m.auto_accept );
				xui::checkbox( "preserve killfeed", m.preserve_killfeed );
				xui::checkbox( "disable game logs", m.disable_game_logs );

				xui::end_child( );
			}

			// ── Right Column: Identity & Social ──
			xui::layout::set_cursor( right_x - wx, body_y - wy );

			if ( xui::begin_child( "##misc_social", col_w, this->m_body_h, true ) )
			{
				group_header( "Identity & Social" );
				xui::checkbox( "clantag", m.m_name_changer.clantag );
				xui::checkbox( "override name", m.m_name_changer.override_name );
				if ( xui::begin_popup( "##override_name_popup", 220.0f ) )
				{
					xui::text_input( "name##nc", m.m_name_changer.name.value, 32, "player name..." );
					xui::end_popup( );
				}

				xui::layout::separator( );

				group_header( "Discord Rich Presence" );
				xui::checkbox( "discord rich presence", m.m_discord_rpc.enabled );
				if ( m.m_discord_rpc.enabled.value )
				{
					constexpr const char* rpc_styles[ ]{ "femboying with femware", "competitive match", "clean / minimal" };
					xui::combo( "rpc style##rpc", m.m_discord_rpc.style.value, rpc_styles, 3 );
					xui::checkbox( "show current map##rpc", m.m_discord_rpc.show_map );
					xui::checkbox( "show elapsed time##rpc", m.m_discord_rpc.show_time );
					xui::text_input( "client id##rpc", m.m_discord_rpc.client_id.value, 32, "discord app id..." );
					if ( xui::button( "paste id from clipboard##rpc", xui::layout::item_width( ), 22.0f ) )
					{
						if ( OpenClipboard( nullptr ) )
						{
							if ( HANDLE hData = GetClipboardData( CF_UNICODETEXT ) )
							{
								if ( auto* wstr = static_cast< const wchar_t* >( GlobalLock( hData ) ) )
								{
									int len = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr );
									if ( len > 1 )
									{
										std::string str( len - 1, '\0' );
										WideCharToMultiByte( CP_UTF8, 0, wstr, -1, str.data( ), len, nullptr, nullptr );
										str.erase( std::remove_if( str.begin( ), str.end( ), []( char c ){ return c == '\r' || c == '\n' || c == '\t' || c == ' '; } ), str.end( ) );
										m.m_discord_rpc.client_id.value = str;
									}
									GlobalUnlock( hData );
								}
							}
							else if ( HANDLE hDataA = GetClipboardData( CF_TEXT ) )
							{
								if ( auto* str = static_cast< const char* >( GlobalLock( hDataA ) ) )
								{
									std::string s = str;
									s.erase( std::remove_if( s.begin( ), s.end( ), []( char c ){ return c == '\r' || c == '\n' || c == '\t' || c == ' '; } ), s.end( ) );
									m.m_discord_rpc.client_id.value = s;
									GlobalUnlock( hDataA );
								}
							}
							CloseClipboard( );
						}
					}

					xui::text_input( "icon / gif url##rpc", m.m_discord_rpc.large_image.value, 256, "asset name or https://...gif" );
					if ( xui::button( "paste icon url from clipboard##rpc", xui::layout::item_width( ), 22.0f ) )
					{
						if ( OpenClipboard( nullptr ) )
						{
							if ( HANDLE hData = GetClipboardData( CF_UNICODETEXT ) )
							{
								if ( auto* wstr = static_cast< const wchar_t* >( GlobalLock( hData ) ) )
								{
									int len = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr );
									if ( len > 1 )
									{
										std::string str( len - 1, '\0' );
										WideCharToMultiByte( CP_UTF8, 0, wstr, -1, str.data( ), len, nullptr, nullptr );
										str.erase( std::remove_if( str.begin( ), str.end( ), []( char c ){ return c == '\r' || c == '\n' || c == '\t' || c == ' '; } ), str.end( ) );
										m.m_discord_rpc.large_image.value = str;
									}
									GlobalUnlock( hData );
								}
							}
							else if ( HANDLE hDataA = GetClipboardData( CF_TEXT ) )
							{
								if ( auto* str = static_cast< const char* >( GlobalLock( hDataA ) ) )
								{
									std::string s = str;
									s.erase( std::remove_if( s.begin( ), s.end( ), []( char c ){ return c == '\r' || c == '\n' || c == '\t' || c == ' '; } ), s.end( ) );
									m.m_discord_rpc.large_image.value = s;
									GlobalUnlock( hDataA );
								}
							}
							CloseClipboard( );
						}
					}

					const auto rpc_state = features::misc::g_discord_rpc.get_state( );
					if ( rpc_state == features::misc::discord_rpc::connection_state::connected )
					{
						xui::text( "status: connected", xdraw::color{ 74, 222, 128, 255 } );
					}
					else if ( rpc_state == features::misc::discord_rpc::connection_state::invalid_client_id )
					{
						xui::text( "status: enter discord app id", xdraw::color{ 248, 113, 113, 255 } );
					}
					else if ( rpc_state == features::misc::discord_rpc::connection_state::connecting )
					{
						xui::text( "status: connecting...", xdraw::color{ 250, 204, 21, 255 } );
					}
					else
					{
						xui::text( "status: disconnected", xdraw::color{ 156, 163, 175, 255 } );
					}
				}
				else if ( xui::begin_popup( "##discord_rpc_popup", 220.0f ) )
				{
					constexpr const char* rpc_styles[ ]{ "femboying with femware", "competitive match", "clean / minimal" };
					xui::combo( "rpc style", m.m_discord_rpc.style.value, rpc_styles, 3 );
					xui::checkbox( "show current map", m.m_discord_rpc.show_map );
					xui::checkbox( "show elapsed time", m.m_discord_rpc.show_time );
					xui::text_input( "client id", m.m_discord_rpc.client_id.value, 32, "discord app id..." );
					xui::text_input( "icon / gif url", m.m_discord_rpc.large_image.value, 256, "asset name or https://...gif" );
					xui::end_popup( );
				}

				xui::layout::separator( );
				group_header( "Player Models" );

				constexpr const char* model_modes[ ]{ "local", "all", "selected" };
				xui::combo( "mode##pm", settings::g_changer.agents.mode, model_modes, 3 );

				// Model list from the workspace "models" folder (models.txt mapped).
				static std::vector< std::string > s_model_names{};
				static std::vector< const char* > s_model_ptrs{};

				const auto rebuild_model_list = [ & ]( )
					{
						s_model_names.clear( );
						s_model_names.emplace_back( "none" );
						for ( const auto& e : features::changer::model_store::entries( ) )
						{
							s_model_names.push_back( e.name );
						}
						s_model_ptrs.clear( );
						for ( const auto& n : s_model_names )
						{
							s_model_ptrs.push_back( n.c_str( ) );
						}
					};

				if ( s_model_ptrs.empty( ) )
				{
					rebuild_model_list( );
				}

				if ( xui::button( "reload models##pm", 140.0f, 22.0f ) )
				{
					features::changer::model_store::refresh( );
					features::changer::model_store::load_mappings( );
					rebuild_model_list( );
				}

				if ( xui::button( "add model (browse)...##pm", 170.0f, 22.0f ) )
				{
					// The file dialog blocks, so run it off the render thread;
					// the poll below picks the new files up next frame.
					std::thread( [ ]( ) { features::changer::model_store::browse_and_import( ); } ).detach( );
				}

				if ( features::changer::model_store::pending_refresh( ).load( ) )
				{
					features::changer::model_store::poll( );
					features::changer::model_store::load_mappings( );
					rebuild_model_list( );
				}

				if ( !features::changer::model_store::last_status( ).empty( ) )
				{
					xui::text( features::changer::model_store::last_status( ), tokens::col_text_dim );
				}

				const auto index_of = [ & ]( const std::string& name ) -> int
					{
						for ( auto i = 1; i < static_cast< int >( s_model_names.size( ) ); ++i )
						{
							if ( s_model_names[ i ] == name )
							{
								return i;
							}
						}
						return 0;
					};

				int ct_idx = index_of( settings::g_changer.agents.custom_ct );
				int t_idx = index_of( settings::g_changer.agents.custom_t );

				if ( xui::combo( "ct model##pm", ct_idx, s_model_ptrs.data( ), static_cast< int >( s_model_ptrs.size( ) ) ) )
				{
					settings::g_changer.agents.custom_ct = ( ct_idx > 0 ) ? s_model_names[ ct_idx ] : std::string{};
				}
				if ( xui::combo( "t model##pm", t_idx, s_model_ptrs.data( ), static_cast< int >( s_model_ptrs.size( ) ) ) )
				{
					settings::g_changer.agents.custom_t = ( t_idx > 0 ) ? s_model_names[ t_idx ] : std::string{};
				}

				// Per-player assignment (used by "selected" mode).
				if ( !s_model_ptrs.empty( ) )
				{
					xui::layout::separator( );
					xui::text( "assign a model to a player", tokens::col_text_dim );

					for ( const auto& p : systems::g_entities.get_by_type( systems::entities::type::player ) )
					{
						if ( !p.ptr )
						{
							continue;
						}

						const auto steam = memory::read< std::uint64_t >( p.ptr + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
						if ( steam == 0 )
						{
							continue;
						}

						const auto name_ptr = memory::read< std::uintptr_t >( p.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
						std::string label = name_ptr ? memory::read_string( name_ptr ) : std::to_string( steam );
						label += "##pm_" + std::to_string( steam );

						int idx = index_of( features::changer::model_store::find( steam ) );
						if ( xui::combo( label.c_str( ), idx, s_model_ptrs.data( ), static_cast< int >( s_model_ptrs.size( ) ) ) )
						{
							auto& sel = features::changer::model_store::selections( );
							if ( idx > 0 )
							{
								sel[ steam ] = s_model_names[ idx ];
							}
							else
							{
								sel.erase( steam );
							}
							features::changer::model_store::save( );
						}
					}
				}

				xui::end_child( );
			}
		}
	}

} // namespace rendering
