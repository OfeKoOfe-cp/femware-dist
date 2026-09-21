#include <pch/pch.hpp>
#include <core/settings.hpp>

#include "../../rendering.hpp"

namespace rendering {

	namespace detail {

		constexpr const char* hitbox_names[ ]{ "head", "chest", "stomach", "arms", "legs", "paws" };
		constexpr const char* pitch_items[ ]{ "none", "down", "up" };
		constexpr const char* yaw_items[ ]{ "desync", "jitter", "backward", "legit desync" };
		constexpr const char* fakelag_mode_items[ ]{ "always", "while standing", "while moving", "while in air" };

	} // namespace detail

	void menu::draw_ragebot( float group_w ) const
	{
		auto& s = settings::g_combat;
		auto& rb = s.m_ragebot;
		auto& aa = s.m_antiaim;
		auto& qp = s.m_quickpeek;
		auto& dp = s.m_duckpeek;
		auto& fd = s.m_fakeduck;
		auto& zb = s.m_zeusbot;
		auto& kb = s.m_knifebot;
		auto& autos = s.m_autos;
		auto& lg = s.m_lagcomp;
		auto& fl = s.m_fakelag;
		auto& rs = s.m_resolver;
		auto& badges = settings::g_misc.m_hud.m_combat_badges;

		auto& wg = rb.groups[ this->m_subtab ];

		const auto wx       = this->m_x;
		const auto wy       = this->m_y;
		const auto body_x   = this->m_body_x;
		const auto body_y   = this->m_body_y;
		const auto body_w   = this->m_body_w;
		const auto col_w    = ( body_w - tokens::col_gap ) * 0.5f;
		const auto right_x  = body_x + col_w + tokens::col_gap;

		const auto gap      = tokens::gap;

		const auto content_h = this->m_body_h;

		const auto card1_h  = std::floor( ( content_h - gap ) * 0.58f );
		const auto card2_h  = content_h - card1_h - gap;

		// ── Left Column: Aimbot & Targeting + Hitscan & Multipoint ──
		xui::layout::set_cursor( body_x - wx, body_y - wy );

		if ( xui::begin_child( "##ragebot_aimbot", col_w, card1_h, true ) )
		{
			group_header( "Aimbot & Targeting" );
			xui::checkbox( "enabled", rb.enabled );
			xui::checkbox( "silent", wg.silent );
			xui::checkbox( "auto scope", wg.auto_scope );
			xui::checkbox( "no spread", wg.no_spread );
			xui::checkbox( "double tap", wg.doubletap );
			xui::checkbox( "doubletap lethal", wg.doubletap_lethal );
			xui::checkbox( "force shot in air", wg.force_shot_air );
			xui::checkbox( "force shot on ground", wg.force_shot );
			xui::checkbox( "extrapolation", lg.extrapolation );
			xui::slider_float( "max fov", wg.max_fov, 1.0f, 180.0f, "%.0f°" );

			xui::slider_int( "hit chance", wg.hitchance, 25, 100, "%d%%" );
			xui::checkbox( "adaptive hit chance", wg.adaptive_hitchance );
			if ( wg.adaptive_hitchance.value )
			{
				xui::slider_int( "reduction##ahc", wg.adaptive_hitchance_reduction, 5, 50, "%d%%" );
			}

			xui::slider_int( "min damage", wg.min_damage, 5, 125, "%d" );
			xui::checkbox( "head priority at low hp", wg.head_priority_low_hp );
			if ( wg.head_priority_low_hp.value )
			{
				xui::slider_int( "hp threshold##lh", wg.head_priority_hp, 1, 100, "%d" );
			}

			xui::checkbox( "hit chance override", wg.hitchance_override );
			if ( wg.hitchance_override.value )
			{
				xui::slider_int( "value##hc", wg.hitchance_override_value, 0, 100, "%d%%" );
			}

			xui::checkbox( "min damage override", wg.min_damage_override );
			if ( wg.min_damage_override.value )
			{
				xui::slider_int( "value##md", wg.min_damage_override_value, 0, 130, "%d" );
			}

			xui::end_child( );
		}

		if ( xui::begin_child( "##ragebot_extras", col_w, card2_h, true ) )
		{
			group_header( "Hitscan & Multipoints" );
			xui::multicombo( "hitboxes", wg.hitboxes, detail::hitbox_names, 6 );
			xui::slider_float( "point scale", wg.pointscale, 0.0f, 100.0f, "%.0f%%" );
			xui::checkbox( "dynamic point scale", wg.dynamic_pointscale );
			xui::checkbox( "debug multipoints", wg.debug_multipoints );
			xui::checkbox( "force b-aim", wg.body_aim );
			xui::checkbox( "b-aim if lethal", wg.baim_lethal );
			xui::checkbox( "b-aim in air", wg.baim_air );
			xui::checkbox( "early counterstrafe predict", wg.early_counterstrafe_predict );

			xui::end_child( );
		}

		// ── Right Column: Anti-Aim Angles & Exploits/Movement ──
		xui::layout::set_cursor( right_x - wx, body_y - wy );

		if ( xui::begin_child( "##ragebot_antiaim", col_w, card1_h, true ) )
		{
			group_header( "Anti-Aim Angles" );
			xui::checkbox( "anti aim", aa.enabled );

			xui::combo( "pitch", aa.pitch.value, detail::pitch_items, 3 );
			xui::combo( "yaw style", aa.yaw_style.value, detail::yaw_items, 4 );
			xui::slider_float( "yaw offset", aa.yaw_offset, -180.0f, 180.0f, "%.0f°" );
			xui::slider_float( "jitter range", aa.jitter_range, 1.0f, 180.0f, "%.0f°" );
			xui::slider_float( "desync amount", aa.desync_amount, 1.0f, 180.0f, "%.0f°" );
			xui::checkbox( "lby breaker", aa.lby_breaker );
			xui::checkbox( "compensate roll", aa.auto_yaw_adjust );
			xui::checkbox( "force left", aa.manual_left );
			xui::checkbox( "force right", aa.manual_right );
			xui::checkbox( "hide onshot", aa.hide_shots );
			xui::checkbox( "avoid backstab", aa.avoid_backstab );
			xui::checkbox( "direction indicator", aa.direction_indicator );
			xui::checkbox( "log angle data", aa.log_detail );

			if ( xui::begin_popup( "##aa_indicator", 220.0f ) )
			{
				xui::color_picker( "color##aa_ind", aa.direction_indicator_color );
				xui::checkbox( "glow##aa_ind", aa.direction_indicator_glow );
				xui::slider_float( "glow strength##aa_ind", aa.direction_indicator_glow_strength, 0.1f, 1.0f, "%.2f" );
				xui::end_popup( );
			}

			xui::end_child( );
		}

		if ( xui::begin_child( "##ragebot_exploits", col_w, card2_h, true ) )
		{
			group_header( "Fakelag & Secondary Combat" );
			xui::checkbox( "fakelag", fl.enabled );
			xui::slider_int( "amount", fl.amount, 1, 14, "%d" );
			xui::combo( "mode", fl.mode.value, detail::fakelag_mode_items, 4 );
			xui::slider_float( "move threshold", fl.moving_speed_threshold, 10.0f, 300.0f, "%.0f" );
			xui::checkbox( "adaptive", fl.adaptive );
			if ( fl.adaptive.value )
			{
				xui::slider_int( "adaptive min##fl_ad", fl.adaptive_min, 1, 14, "%d" );
				xui::slider_int( "adaptive max##fl_ad", fl.adaptive_max, 1, 14, "%d" );
			}
			xui::checkbox( "on peek bind##fl", fl.lag_on_peek );
			xui::checkbox( "cancel on shot", fl.hide_on_shoot );

			xui::checkbox( "quick peek", qp.enabled );
			if ( xui::begin_popup( "##qp_colors", 220.0f ) )
			{
				xui::color_picker( "base color##qp", qp.color );
				xui::color_picker( "retracting color##qp", qp.retrack_color );
				xui::end_popup( );
			}

			xui::checkbox( "duck peek", dp.enabled );
			xui::checkbox( "fake duck", fd.enabled );

			xui::checkbox( "resolver", rs.enabled );
			if ( rs.enabled.value )
			{
				xui::slider_float( "speed threshold##rs", rs.moving_speed_threshold, 1.0f, 250.0f, "%.0f" );
				xui::slider_float( "desync epsilon##rs", rs.desync_epsilon, 0.5f, 30.0f, "%.1f°" );
			}

			xui::checkbox( "auto revolver", autos.revolver );

			xui::checkbox( "zeusbot", zb.enabled );
			if ( zb.enabled.value )
			{
				xui::slider_float( "max fov##zb", zb.max_fov, 1.0f, 180.0f, "%.0f°" );
				xui::checkbox( "drop after##zb", zb.drop_after );
			}

			xui::checkbox( "knifebot", kb.enabled );
			if ( kb.enabled.value )
			{
				xui::slider_float( "max fov##kb", kb.max_fov, 1.0f, 180.0f, "%.0f°" );
			}

			xui::layout::separator( );

			xui::checkbox( "combat badges", badges.enabled );
			if ( xui::begin_popup( "##combat_badges_popup", 220.0f ) )
			{
				xui::checkbox( "show damage override", badges.show_dmg );
				xui::checkbox( "show hitchance override", badges.show_hc );
				xui::checkbox( "show b-aim", badges.show_baim );
				xui::checkbox( "show fake duck", badges.show_fd );
				xui::checkbox( "show quick peek", badges.show_peek );
				xui::end_popup( );
			}

			xui::end_child( );
		}
	}

} // namespace rendering
