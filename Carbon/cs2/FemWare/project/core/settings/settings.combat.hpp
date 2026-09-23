#pragma once

#include <utilities/math/math.hpp>
#include <external/config.hpp>

namespace settings {

	struct combat
	{
		struct ragebot
		{
			static constexpr auto k_group_count{ 6u };

			xui::setting enabled{ true, {}, "enabled", "ragebot" };

			xui::setting auto_stop{ true, {}, "autostop", "ragebot" };
			xui::setting auto_stop_air{ true, {}, "autostop in air", "ragebot" };

			struct weapon_group
			{
				xui::setting silent{ true, {}, "silent", "ragebot" };
				xui::setting no_spread{ false, {}, "no spread", "ragebot" };
				xui::setting doubletap{ false, {}, "doubletap", "ragebot" };
				xui::setting body_aim{ false, {}, "force b-aim", "ragebot" };
				xui::setting auto_scope{ true, {}, "auto scope", "ragebot" };
				xui::setting baim_air{ true, {}, "b-aim in air", "ragebot" };
				xui::setting baim_lethal{ true, {}, "b-aim if lethal", "ragebot" };
				xui::setting force_shot_air{ false, {}, "force shot in air", "ragebot" };
				xui::setting force_shot{ false, {}, "force shot on ground", "ragebot" };

				config::val<float> max_fov{ 180.0f };

				config::val<int> hitchance{ 80 };
				config::val<int> min_damage{ 20 };

				config::val<int> min_damage_override_value{ 11 };
				xui::setting min_damage_override{ false, {}, "min damage override", "ragebot" };

				config::val<int> hitchance_override_value{ 75 };
				xui::setting hitchance_override{ false, {}, "hit chance override", "ragebot" };
				xui::setting adaptive_hitchance{ false, {}, "adaptive hit chance", "ragebot" };
				config::val<int> adaptive_hitchance_reduction{ 15 };

				xui::setting head_priority_low_hp{ false, {}, "head priority at low hp", "ragebot" };
				config::val<int> head_priority_hp{ 35 };

				xui::setting doubletap_lethal{ false, {}, "doubletap lethal", "ragebot" };
				config::val<float> stop_speed_percent{ 34.0f };

			config::val<float> pointscale{ 85.0f };
			xui::setting dynamic_pointscale{ true, {}, "dynamic point scale", "ragebot" };
			xui::setting debug_multipoints{ false, {}, "debug multipoints", "ragebot" };

			config::bools<6> hitboxes{ { true, true, true, true, true, true } };
			xui::setting early_counterstrafe_predict{ false, {}, "early counterstrafe predict", "ragebot" };

			void init( std::string_view cat )
			{
				const auto s = std::string( cat );

				this->silent.category = s;
				this->no_spread.category = s;
				this->doubletap.category = s;
				this->body_aim.category = s;
				this->auto_scope.category = s;
				this->baim_air.category = s;
				this->baim_lethal.category = s;
				this->force_shot_air.category = s;
				this->force_shot.category = s;
				this->min_damage_override.category = s;
				this->hitchance_override.category = s;
				this->adaptive_hitchance.category = s;
				this->head_priority_low_hp.category = s;
				this->doubletap_lethal.category = s;
				this->dynamic_pointscale.category = s;
				this->debug_multipoints.category = s;
				this->early_counterstrafe_predict.category = s;

				this->max_fov.reg( s, "max fov" );
				this->hitchance.reg( s, "hit chance" );
				this->min_damage.reg( s, "min damage" );
				this->min_damage_override_value.reg( s, "min damage override value" );
				this->hitchance_override_value.reg( s, "hit chance override value" );
				this->adaptive_hitchance_reduction.reg( s, "adaptive hit chance reduction" );
				this->head_priority_hp.reg( s, "head priority hp" );
				this->pointscale.reg( s, "point scale" );
				this->stop_speed_percent.reg( s, "stop speed percent" );
				this->hitboxes.reg( s, "hitboxes" );
			}

				void set_default_binds( )
				{
					this->force_shot_air.bind = { .key = VK_XBUTTON1, .mode = xui::bind_mode::hold_on };
					this->min_damage_override.bind = { .key = VK_XBUTTON2, .mode = xui::bind_mode::hold_on };
					this->hitchance_override.bind = { .key = VK_SPACE, .mode = xui::bind_mode::hold_on };
				}
			};

			std::array<weapon_group, k_group_count> groups{};

			config::val<int> target_loyalty{ 14 };
			config::val<float> pointscale_bias{ 0.9f };

			ragebot( )
			{
				constexpr const char* weapon_names[ ]{ "pistol", "smg", "rifle", "shotgun", "sniper", "lmg" };

				for ( std::uint32_t i = 0; i < k_group_count; ++i )
				{
					this->groups[ i ].init( std::string( "ragebot - " ) + weapon_names[ i ] );
				}

				this->groups[ 0 ].min_damage.value = 15;
				this->groups[ 1 ].min_damage.value = 15;
				this->groups[ 2 ].min_damage.value = 20;
				this->groups[ 3 ].min_damage.value = 20;
				this->groups[ 4 ].min_damage.value = 101;
				this->groups[ 5 ].min_damage.value = 20;

				this->groups[ 0 ].set_default_binds( );
				this->groups[ 4 ].set_default_binds( );

				this->target_loyalty.reg( "ragebot", "target loyalty" );
				this->pointscale_bias.reg( "ragebot", "pointscale bias" );
			}

			weapon_group& get_group( std::uint32_t weapon_type )
			{
				const auto idx = weapon_type - cstypes::weapon_type::pistol;
				return this->groups[ idx < k_group_count ? idx : 2 ];
			}

			const weapon_group& get_group( std::uint32_t weapon_type ) const
			{
				const auto idx = weapon_type - cstypes::weapon_type::pistol;
				return this->groups[ idx < k_group_count ? idx : 2 ];
			}
		} m_ragebot{};

		struct legitbot
		{
			static constexpr auto k_group_count{ 6u };

			struct weapon_group
			{
				xui::setting aimbot{ true, {}, "aimbot", "legitbot" };
				config::val<float> fov{ 5.0f };
				config::val<int> smooth{ 5 };
				config::bools<5> hitboxes{ { true, true, true, false, false } };

				xui::setting rcs{ true, {}, "recoil control", "legitbot" };
				config::val<int> rcs_min{ 95 };
				config::val<int> rcs_max{ 105 };

				xui::setting standalone_rcs{ false, {}, "standalone rcs", "legitbot" };
				config::val<int> standalone_rcs_strength{ 100 };
				config::val<int> standalone_rcs_min{ 95 };
				config::val<int> standalone_rcs_max{ 105 };

				xui::setting triggerbot{ false, {}, "triggerbot", "legitbot" };
				config::val<int> trigger_delay{ 5 };
				config::val<int> trigger_hitchance{ 80 };
				xui::setting trigger_head_only{ false, {}, "trigger head only", "legitbot" };
				xui::setting give_me_your_seed{ false, {}, "trigger seed mode", "legitbot" };

				xui::setting autowall{ true, {}, "autowall", "legitbot" };
				config::val<int> min_damage{ 1 };

				enum class fov_circle_style : std::uint8_t { outline, filled, glow, pulse };

				xui::setting visualize_fov{ true, {}, "visualize fov", "legitbot" };
				config::col fov_color{ { 255, 255, 255, 150 } };
				config::enm<fov_circle_style> fov_style{ fov_circle_style::outline };
				config::val<float> fov_thickness{ 1.5f };
				xui::setting fov_glow{ true, {}, "fov glow", "legitbot" };
				config::col fov_fill_color{ { 255, 255, 255, 22 } };
				xui::setting fov_center_dot{ false, {}, "fov center dot", "legitbot" };
				config::val<float> fov_dot_size{ 2.0f };

				xui::setting spread_circle{ false, {}, "spread circle", "legitbot" };
				config::col spread_color{ { 255, 200, 80, 180 } };
				config::val<float> spread_thickness{ 1.5f };
				config::col spread_fill_color{ { 255, 200, 80, 28 } };

				config::val<int> reaction_delay{ 0 }; // human reaction delay in ms (0 = instant)
				config::val<int> target_switch_delay{ 0 }; // min ms to dwell on target before switching (0 = instant)

				xui::setting flash_check{ true, {}, "flash check", "legitbot" };
				xui::setting smoke_check{ false, {}, "smoke check", "legitbot" };
				xui::setting curve_smooth{ true, {}, "curve smoothing", "legitbot" };
				xui::setting target_indicator{ true, {}, "target indicator", "legitbot" };
				config::col target_indicator_color{ { 255, 100, 160, 200 } };

				xui::setting auto_pistol{ true, {}, "auto pistol", "legitbot" };
				config::val<int> kill_delay{ 150 };
				xui::setting dynamic_fov{ false, {}, "dynamic fov", "legitbot" };
				xui::setting backtrack{ true, {}, "backtrack", "legitbot" };
				config::val<int> backtrack_ticks{ 12 };
				xui::setting jump_check{ true, {}, "jump check", "legitbot" };
				xui::setting scope_check{ true, {}, "scope check", "legitbot" };
				xui::setting nearest_hitbox{ false, {}, "nearest hitbox", "legitbot" };
				xui::setting bezier_curve{ true, {}, "bezier curve pathing", "legitbot" };
				config::val<float> max_acceleration{ 12.0f };
				xui::setting trigger_jitter{ true, {}, "gaussian trigger jitter", "legitbot" };

				void init( std::string_view cat )
				{
					const auto s = std::string( cat );

					this->aimbot.category = s;
					this->rcs.category = s;
					this->standalone_rcs.category = s;
					this->triggerbot.category = s;
					this->trigger_head_only.category = s;
					this->give_me_your_seed.category = s;
					this->autowall.category = s;
					this->visualize_fov.category = s;
					this->fov_glow.category = s;
					this->fov_center_dot.category = s;
					this->spread_circle.category = s;
					this->target_indicator.category = s;
					this->flash_check.category = s;
					this->smoke_check.category = s;
					this->curve_smooth.category = s;
					this->auto_pistol.category = s;
					this->dynamic_fov.category = s;
					this->backtrack.category = s;
					this->jump_check.category = s;
					this->scope_check.category = s;
					this->nearest_hitbox.category = s;
					this->bezier_curve.category = s;
					this->trigger_jitter.category = s;

					this->fov.reg( s, "fov" );
					this->smooth.reg( s, "smooth" );
					this->hitboxes.reg( s, "hitboxes" );
					this->rcs_min.reg( s, "rcs min" );
					this->rcs_max.reg( s, "rcs max" );
					this->standalone_rcs_strength.reg( s, "standalone rcs strength" );
					this->standalone_rcs_min.reg( s, "standalone rcs min" );
					this->standalone_rcs_max.reg( s, "standalone rcs max" );
					this->trigger_delay.reg( s, "trigger delay" );
					this->trigger_hitchance.reg( s, "trigger hitchance" );
					this->min_damage.reg( s, "min damage" );
					this->fov_color.reg( s, "fov color" );
					this->fov_style.reg( s, "fov style" );
					this->fov_thickness.reg( s, "fov thickness" );
					this->fov_fill_color.reg( s, "fov fill color" );
					this->fov_dot_size.reg( s, "fov dot size" );
					this->spread_color.reg( s, "spread color" );
					this->spread_thickness.reg( s, "spread thickness" );
					this->spread_fill_color.reg( s, "spread fill color" );
					this->target_indicator_color.reg( s, "target indicator color" );
					this->reaction_delay.reg( s, "reaction delay" );
					this->target_switch_delay.reg( s, "target switch delay" );
					this->kill_delay.reg( s, "kill delay" );
					this->backtrack_ticks.reg( s, "backtrack ticks" );
					this->max_acceleration.reg( s, "max acceleration" );
				}
			};

			xui::setting enabled{ true, {}, "enabled", "legitbot" };
			std::array<weapon_group, k_group_count> groups{};

			legitbot( )
			{
				constexpr const char* weapon_names[ ]{ "pistol", "smg", "rifle", "shotgun", "sniper", "lmg" };

				for ( auto i = 0u; i < k_group_count; ++i )
				{
					this->groups[ i ].init( std::string( "legitbot - " ) + weapon_names[ i ] );
				}
			}

			weapon_group& get_group( std::uint32_t weapon_type )
			{
				const auto idx = weapon_type - cstypes::weapon_type::pistol;
				return this->groups[ idx < k_group_count ? idx : 2 ];
			}

			const weapon_group& get_group( std::uint32_t weapon_type ) const
			{
				const auto idx = weapon_type - cstypes::weapon_type::pistol;
				return this->groups[ idx < k_group_count ? idx : 2 ];
			}
		} m_legitbot{};

		struct antiaim
		{
			enum class pitch_mode : std::uint8_t
			{
				none,
				down,
				up
			};

			xui::setting enabled{ true, {}, "anti aim", "anti aim" };
			config::enm<pitch_mode> pitch{ pitch_mode::down, "anti aim", "pitch" };
			xui::setting auto_yaw_adjust{true, {}, "correct yaw to compensate for the models inherit sideways roll", "anti aim"};
			config::val<float> yaw_offset{ 33.0f, "anti aim", "yaw offset" };
			xui::setting manual_left{ false, { 'Z', xui::bind_mode::toggle }, "force left", "anti aim" };
			xui::setting manual_right{ false, { 'C', xui::bind_mode::toggle }, "force right", "anti aim" };
			xui::setting hide_shots{ true, {}, "hide onshot", "anti aim" };
			xui::setting avoid_backstab{ true, {}, "avoid backstab", "anti aim" };

			enum class yaw_mode : std::uint8_t
			{
				desync,
				jitter,
				backward,
				legit_desync,
				spin
			};

			config::enm<yaw_mode> yaw_style{ yaw_mode::desync, "anti aim", "yaw style" };
			config::val<float> jitter_range{ 30.0f, "anti aim", "jitter range" };
			config::val<float> desync_amount{ 120.0f, "anti aim", "desync amount" };
			config::val<float> spin_speed{ 6.0f, "anti aim", "spin speed" };
			config::val<int> lby_breaker_interval{ 70, "anti aim", "lby breaker interval" };
			xui::setting lby_breaker{ false, {}, "lby breaker", "anti aim" };

			xui::setting log_detail{ false, {}, "log angle data", "anti aim" };
			xui::setting direction_indicator{ true, {}, "direction indicator", "anti aim" };
			config::col direction_indicator_color{ { 173, 192, 255, 220 }, "anti aim", "direction indicator color" };
			xui::setting direction_indicator_glow{ true, {}, "direction indicator glow", "anti aim" };
			config::val<float> direction_indicator_glow_strength{ 0.55f, "anti aim", "direction indicator glow strength" };

			antiaim( )
			{
				this->manual_left.bind.excludes = &this->manual_right;
				this->manual_right.bind.excludes = &this->manual_left;
			}
		} m_antiaim{};

		struct fakelag
		{
			enum class mode : std::uint8_t
			{
				always,
				while_standing,
				while_moving,
				while_in_air
			};

			xui::setting enabled{ false, {}, "fakelag", "anti aim" };
			config::val<int> amount{ 8, "anti aim", "fakelag amount" };
			config::enm<mode> mode{ mode::always, "anti aim", "fakelag mode" };
			config::val<float> moving_speed_threshold{ 40.0f, "anti aim", "fakelag move threshold" };
			xui::setting adaptive{ false, {}, "adaptive fakelag", "anti aim" };
			config::val<int> adaptive_min{ 2, "anti aim", "fakelag adaptive min" };
			config::val<int> adaptive_max{ 14, "anti aim", "fakelag adaptive max" };
			xui::setting lag_on_peek{ false, { 'G', xui::bind_mode::hold_on }, "fakelag on peek key", "anti aim" };
			xui::setting hide_on_shoot{ true, {}, "cancel on shot", "anti aim" };
		} m_fakelag{};

		struct quickpeek
		{
			xui::setting enabled{ false, { 'V', xui::bind_mode::hold_on }, "quick peek", "peek assistance" };
			config::col color{ { 173, 192, 255, 255 }, "peek assistance", "quick peek color" };
			config::col retrack_color{ { 255, 171, 234, 255 }, "peek assistance", "retracting color" };
		} m_quickpeek{};

struct duckpeek
			{
			xui::setting enabled{ false, {}, "duck peek", "peek assistance" };
			xui::setting duck_when_standing{ false, {}, "duck when standing", "peek assistance" };
			} m_duckpeek{};

			struct fakeduck
			{
				xui::setting enabled{ false, { 'F', xui::bind_mode::toggle }, "fake duck", "anti aim" };
				xui::setting hide_shots{ true, {}, "hide shots", "anti aim" };
			} m_fakeduck{};

		struct lagcomp_settings
		{
			config::val<int> max_backtrack_ticks{ 12, "ragebot", "max backtrack ticks" };
			xui::setting extrapolation{ true, {}, "extrapolation", "ragebot" };
			config::val<int> max_extrapolate_ticks{ 8, "ragebot", "max extrapolate ticks" };
		} m_lagcomp{};

		struct resolver_settings
		{
			xui::setting enabled{ false, {}, "resolver", "ragebot" };
			config::val<float> moving_speed_threshold{ 35.0f, "ragebot", "resolver speed threshold" };
			config::val<float> desync_epsilon{ 3.0f, "ragebot", "resolver desync epsilon" };
		} m_resolver{};

		struct zeusbot
		{
			xui::setting enabled{ true, {}, "zeusbot", "other 'bots'" };
			xui::setting drop_after{ true, {}, "drop after", "zeusbot" };
			config::val<float> max_fov{ 180.0f, "zeusbot", "max fov" };
		} m_zeusbot{};

		struct autos
		{
			xui::setting revolver{ true, {}, "auto revolver", "autos" };
			config::val<int> revolver_cock_ticks{ 13, "autos", "revolver cock ticks" };
			xui::setting revolver_prefer_right_click{ false, {}, "revolver prefer right click", "autos" };
			xui::setting scope{ true, {}, "auto scope", "autos" };
		} m_autos{};

		struct knifebot
		{
			xui::setting enabled{ true, {}, "knifebot", "other 'bots'" };
			config::val<float> max_fov{ 180.0f, "knifebot", "max fov" };
		} m_knifebot{};

		struct penetration_crosshair
		{
			xui::setting enabled{ false, {}, "penetration crosshair", "pen crosshair" };
			config::col can_penetrate_fill{ { 173, 192, 255, 120 }, "pen crosshair", "can penetrate fill" };
			config::col can_penetrate_outline{ { 173, 192, 255, 210 }, "pen crosshair", "can penetrate outline" };
			config::col blocked_fill{ { 252, 217, 240, 80 }, "pen crosshair", "blocked fill" };
			config::col blocked_outline{ { 252, 217, 240, 160 }, "pen crosshair", "blocked outline" };
			xui::setting glow{ true, {}, "glow", "pen crosshair" };
			config::val<float> glow_strength{ 1.0f, "pen crosshair", "glow strength" };
		} m_penetration_crosshair{};
	};

} // namespace settings
