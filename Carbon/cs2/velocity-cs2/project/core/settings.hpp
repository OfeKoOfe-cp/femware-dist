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
				legit_desync
			};

			config::enm<yaw_mode> yaw_style{ yaw_mode::desync, "anti aim", "yaw style" };
			config::val<float> jitter_range{ 30.0f, "anti aim", "jitter range" };
			config::val<float> desync_amount{ 120.0f, "anti aim", "desync amount" };
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

	struct esp
	{
		enum class cham_ids : std::uint8_t
		{
			liquid, metallic, matte, flat, bloom, outlines, glow, electric, distortion, hologram, pearl, crystal, velvet, plasma, glass,
			liquid_ignorez, matte_ignorez, flat_ignorez, bloom_ignorez, outlines_ignorez, glow_ignorez, distortion_ignorez, hologram_ignorez, electric_ignorez, pearl_ignorez, crystal_ignorez, plasma_ignorez, glass_ignorez,
			count
		};

		struct chams_layer
		{
			xui::setting enabled{ false, {}, "chams layer", "chams" };
			config::col color{ { 255, 255, 255, 255 } };
			config::enm<cham_ids> material{ cham_ids::matte };
			config::val<float> fresnel_exponent{ 2.5f };
			xui::setting wireframe{ false, {}, "wireframe", "chams" };

			void init( std::string_view cat, std::string_view layer_name )
			{
				const auto s = std::string( cat );
				this->enabled.name = std::string( layer_name );
				this->enabled.category = s;
				this->color.reg( s, std::string( layer_name ) + " color" );
				this->material.reg( s, std::string( layer_name ) + " material" );
				this->fresnel_exponent.reg( s, std::string( layer_name ) + " fresnel exponent" );
				this->wireframe.name = std::string( layer_name ) + " wireframe";
				this->wireframe.category = s;
			}
		};

		struct chams_config
		{
			xui::setting enabled{ false, {}, "chams", "chams" };
			chams_layer primary{};
			chams_layer secondary{};
			chams_layer overlay{};

			chams_config( )
			{
				this->secondary.material.value = cham_ids::matte_ignorez;
			}

			void init( std::string_view cat, std::string_view name = "chams" )
			{
				const auto s = std::string( cat );
				this->enabled.category = s;
				this->enabled.name = std::string( name );
				this->primary.init( s, "visible chams" );
				this->secondary.init( s, "non-visible chams" );
				this->overlay.init( s, "overlay chams" );
			}
		};

		struct glow_target
		{
			xui::setting enabled{ false, {}, "glow", "glow" };
			config::col color{ { 173, 192, 255, 75 } };

			void init( std::string_view cat, std::string_view color_name = "color" )
			{
				const auto s = std::string( cat );
				this->enabled.category = s;
				this->enabled.name = "glow";
				this->color.reg( s, color_name );
			}
		};

		struct player
		{
			struct overlay
			{
				xui::setting enabled{ true, {}, "esp overlay", "esp" };
				xui::setting visible_only{};
				config::val<float> max_distance{ 0.0f };

				struct box
				{
					enum class style_type : std::uint8_t { full, cornered };

					xui::setting enabled{};
					config::enm<style_type> style{ style_type::cornered };
					xui::setting fill{};
					xui::setting outline{};
					config::val<float> corner_length{ 10.0f };

					config::col visible_color{ { 173, 192, 255, 255 } };
					config::col occluded_color{ { 255, 171, 234, 255 } };

					box( ) = default;

					explicit box( const std::string& prefix )
						: enabled{ false, {}, "bounding box", prefix + " box" }
						, fill{ true, {}, "fill", prefix + " box" }
						, outline{ true, {}, "outline", prefix + " box" }
					{
						const auto cat = prefix + " box";
						this->style.reg( cat, "style" );
						this->corner_length.reg( cat, "corner length" );
						this->visible_color.reg( cat, "visible color" );
						this->occluded_color.reg( cat, "occluded color" );
					}
				};

				struct skeleton
				{
					enum class mode : std::uint8_t { normal, backtrack };

					xui::setting enabled{};
					xui::setting head_dot{};
					config::enm<mode> type{ mode::backtrack };
					config::val<float> thickness{ 1.5f };

					config::col visible_color{ { 173, 192, 255, 255 } };
					config::col occluded_color{ { 220, 225, 240, 255 } };

					skeleton( ) = default;

					explicit skeleton( const std::string& prefix )
						: enabled{ false, {}, "skeleton", prefix + " skeleton" }
						, head_dot{ true, {}, "head dot", prefix + " skeleton" }
					{
						const auto cat = prefix + " skeleton";
						this->type.reg( cat, "mode" );
						this->thickness.reg( cat, "thickness" );
						this->visible_color.reg( cat, "visible color" );
						this->occluded_color.reg( cat, "occluded color" );
					}
				};

				struct health_bar
				{
					enum class position_type : std::uint8_t { left, top, bottom };

					xui::setting enabled{};
					config::enm<position_type> position{ position_type::left };
					xui::setting outline_setting{};
					xui::setting gradient{};
					xui::setting show_value{};
					xui::setting glow{};
					xui::setting segmented{};
					xui::setting damage_drop{};

					config::col full_color{ { 173, 192, 255, 255 } };
					config::col low_color{ { 130, 145, 200, 255 } };
					config::col background_color{ { 0, 0, 0, 255 } };
					config::col outline_color{ { 0, 0, 0, 255 } };
					config::col text_color{ { 255, 255, 255, 255 } };
					config::col glow_color{ { 173, 192, 255, 255 } };
					config::val<float> glow_strength{ 0.55f };
					config::col damage_drop_color{ { 255, 180, 80, 220 } };

					health_bar( ) = default;

					explicit health_bar( const std::string& prefix )
						: enabled{ true, {}, "health bar", prefix + " health" }
						, outline_setting{ true, {}, "outline", prefix + " health" }
						, gradient{ true, {}, "gradient", prefix + " health" }
						, show_value{ true, {}, "show value", prefix + " health" }
						, glow{ true, {}, "glow", prefix + " health" }
						, segmented{ false, {}, "segmented battery", prefix + " health" }
						, damage_drop{ true, {}, "damage drop animation", prefix + " health" }
					{
						const auto cat = prefix + " health";
						this->position.reg( cat, "position" );
						this->full_color.reg( cat, "full color" );
						this->low_color.reg( cat, "low color" );
						this->background_color.reg( cat, "background" );
						this->outline_color.reg( cat, "outline color" );
						this->text_color.reg( cat, "text color" );
						this->glow_color.reg( cat, "glow color" );
						this->glow_strength.reg( cat, "glow strength" );
						this->damage_drop_color.reg( cat, "damage drop color" );
					}
				};

				struct ammo_bar
				{
					enum class position_type : std::uint8_t { left, top, bottom };

					xui::setting enabled{};
					config::enm<position_type> position{ position_type::bottom };
					xui::setting outline_setting{};
					xui::setting gradient{};
					xui::setting show_value{};
					xui::setting glow{};

					config::col full_color{ { 255, 171, 234, 255 } };
					config::col low_color{ { 255, 210, 244, 255 } };
					config::col background_color{ { 0, 0, 0, 255 } };
					config::col outline_color{ { 0, 0, 0, 255 } };
					config::col text_color{ { 255, 255, 255, 255 } };
					config::col glow_color{ { 173, 192, 255, 255 } };
					config::val<float> glow_strength{ 0.55f };

					ammo_bar( ) = default;

					explicit ammo_bar( const std::string& prefix )
						: enabled{ true, {}, "ammo bar", prefix + " ammo" }
						, outline_setting{ true, {}, "outline", prefix + " ammo" }
						, gradient{ true, {}, "gradient", prefix + " ammo" }
						, show_value{ false, {}, "show value", prefix + " ammo" }
						, glow{ true, {}, "glow", prefix + " ammo" }
					{
						const auto cat = prefix + " ammo";
						this->position.reg( cat, "position" );
						this->full_color.reg( cat, "full color" );
						this->low_color.reg( cat, "low color" );
						this->background_color.reg( cat, "background" );
						this->outline_color.reg( cat, "outline color" );
						this->text_color.reg( cat, "text color" );
						this->glow_color.reg( cat, "glow color" );
						this->glow_strength.reg( cat, "glow strength" );
					}
				};

				struct info_flags
				{
					enum flag : std::uint8_t
					{
						money = 0, armor, kit, scoped, defusing, flashed, ping, distance, count
					};

					xui::setting enabled{};
					config::bools<count> flags{ { false, false, false, true, true, true, true, false } };

					config::col money_color{ { 160, 210, 140, 255 } };
					config::col armor_color{ { 220, 225, 240, 255 } };
					config::col kit_color{ { 173, 192, 255, 255 } };
					config::col scoped_color{ { 220, 225, 240, 255 } };
					config::col defusing_color{ { 173, 192, 255, 255 } };
					config::col flashed_color{ { 240, 230, 170, 255 } };
					config::col distance_color{ { 185, 190, 205, 255 } };

					info_flags( ) = default;

					explicit info_flags( const std::string& prefix ) : enabled{ true, {}, "info flags", prefix + " flags" }
					{
						const auto cat = prefix + " flags";
						this->flags.reg( cat, "flags" );
						this->money_color.reg( cat, "money color" );
						this->armor_color.reg( cat, "armor color" );
						this->kit_color.reg( cat, "kit color" );
						this->scoped_color.reg( cat, "scoped color" );
						this->defusing_color.reg( cat, "defusing color" );
						this->flashed_color.reg( cat, "flashed color" );
						this->distance_color.reg( cat, "distance color" );
					}

					[[nodiscard]] bool has( flag f ) const { return this->flags[ f ]; }
				};

			struct name
			{
				enum class name_font : std::uint8_t { inter, bold, pixel };
				enum class name_position : std::uint8_t { above, inline_top };

				xui::setting enabled{};
				xui::setting background{};
				config::enm<name_font> font{ name_font::inter };

				// New customization
				config::val<float> size{ 13.0f };
				xui::setting outline{};
				xui::setting show_id{};
				xui::setting show_distance{};
				config::val<float> fade_start{ 1500.0f };
				config::val<float> fade_end{ 3000.0f };
				config::enm<name_position> position{ name_position::above };
				xui::setting use_theme_color{};
				xui::setting use_theme_background{};
				xui::setting draw_box{};

				config::col color{ { 255, 255, 255, 225 } };
				config::col background_color{ { 0, 0, 0, 160 } };

				name( ) = default;

				explicit name( const std::string& prefix )
					: enabled{ true, {}, "name", prefix + " name" }
					, background{ false, {}, "name background", prefix + " name" }
					, outline{ true, {}, "outline", prefix + " name" }
					, show_id{ false, {}, "show id", prefix + " name" }
					, show_distance{ false, {}, "show distance", prefix + " name" }
					, use_theme_color{ true, {}, "theme color", prefix + " name" }
					, use_theme_background{ true, {}, "theme background", prefix + " name" }
					, draw_box{ false, {}, "box", prefix + " name" }
				{
					const auto cat = prefix + " name";
					this->font.reg( cat, "font" );
					this->size.reg( cat, "size" );
					this->fade_start.reg( cat, "fade start" );
					this->fade_end.reg( cat, "fade end" );
					this->position.reg( cat, "position" );
					this->color.reg( cat, "color" );
					this->background_color.reg( cat, "background color" );
				}
			};

				struct weapon
				{
					enum class display_type : std::uint8_t { text, icon, text_and_icon };

					xui::setting enabled{};
					config::enm<display_type> display{ display_type::text_and_icon };
					config::val<float> icon_scale{ 1.0f };

					config::col text_color{ { 255, 255, 255, 225 } };
					config::col icon_color{ { 255, 255, 255, 225 } };

					weapon( ) = default;

					explicit weapon( const std::string& prefix ) : enabled{ true, {}, "weapon", prefix + " weapon" }
					{
						const auto cat = prefix + " weapon";
						this->display.reg( cat, "display" );
						this->icon_scale.reg( cat, "icon scale" );
						this->text_color.reg( cat, "text color" );
						this->icon_color.reg( cat, "icon color" );
					}
				};

				struct oof_arrow
				{
					xui::setting enabled{};
					xui::setting glow{};
					xui::setting show_distance{};
					xui::setting distance_fade{};
					config::val<float> width{ 20.0f };
					config::val<float> height{ 15.0f };
					config::val<float> radius_x{ 200.0f };
					config::val<float> radius_y{ 200.0f };
					config::val<float> glow_strength{ 1.0f };

					config::col visible_color{ { 255, 171, 234, 255 } };
					config::col occluded_color{ { 173, 192, 255, 255 } };

					oof_arrow( ) = default;

					explicit oof_arrow( const std::string& prefix ) : enabled{ true, {}, "oof arrow", prefix + " oof" }, glow{ true, {}, "glow", prefix + " oof" }, show_distance{ true, {}, "show distance", prefix + " oof" }, distance_fade{ true, {}, "distance fade", prefix + " oof" }
					{
						const auto cat = prefix + " oof";
						this->width.reg( cat, "width" );
						this->height.reg( cat, "height" );
						this->radius_x.reg( cat, "radius x" );
						this->radius_y.reg( cat, "radius y" );
						this->glow_strength.reg( cat, "glow strength" );
						this->visible_color.reg( cat, "visible color" );
						this->occluded_color.reg( cat, "occluded color" );
					}
				};

				box m_box{};
				skeleton m_skeleton{};
				health_bar m_health_bar{};
				ammo_bar m_ammo_bar{};
				info_flags m_info_flags{};
				name m_name{};
				weapon m_weapon{};
				oof_arrow m_oof_arrow{};

				struct snap_lines
				{
					xui::setting enabled{ false, {}, "snap lines", "esp" };
					config::col color{ { 255, 175, 220, 200 } };
					config::val<int> origin{ 0 };

					snap_lines( ) = default;
					explicit snap_lines( const std::string& prefix )
						: enabled{ false, {}, "snap lines", prefix }
					{
						const auto cat = prefix + " snap lines";
						this->color.reg( cat, "color" );
						this->origin.reg( cat, "origin" );
					}
				};

				struct view_ray
				{
					xui::setting enabled{ false, {}, "view direction", "esp" };
					config::col color{ { 173, 192, 255, 220 } };
					config::val<float> length{ 100.0f };

					view_ray( ) = default;
					explicit view_ray( const std::string& prefix )
						: enabled{ false, {}, "view direction", prefix }
					{
						const auto cat = prefix + " view direction";
						this->color.reg( cat, "color" );
						this->length.reg( cat, "length" );
					}
				};

				struct sound_esp
				{
					xui::setting enabled{ false, {}, "sound esp", "esp" };
					config::col color{ { 255, 175, 235, 220 } };
					config::val<float> max_radius{ 75.0f };
					config::val<float> duration{ 1.8f };
					xui::setting show_label{ true, {}, "sound label", "esp" };
					xui::setting show_oof{ true, {}, "sound oof", "esp" };
					xui::setting radar_pulse{ true, {}, "sound radar", "esp" };

					sound_esp( ) = default;
					explicit sound_esp( const std::string& prefix )
						: enabled{ false, {}, "sound esp", prefix }
						, show_label{ true, {}, "sound label", prefix }
						, show_oof{ true, {}, "sound oof", prefix }
						, radar_pulse{ true, {}, "sound radar", prefix }
					{
						const auto cat = prefix + " sound esp";
						this->color.reg( cat, "color" );
						this->max_radius.reg( cat, "radius" );
						this->duration.reg( cat, "duration" );
					}
				};

				snap_lines m_snap_lines{};
				view_ray m_view_ray{};
				sound_esp m_sound_esp{};

				overlay( ) = default;

				explicit overlay( const char* prefix, bool enabled_default = true )
					: overlay{ std::string{ prefix }, enabled_default }
				{
				}

				explicit overlay( const std::string& prefix, bool enabled_default = true )
					: enabled{ enabled_default, {}, "esp overlay", prefix }
					, visible_only{ false, {}, "visible only", prefix }
					, m_box{ prefix }
					, m_skeleton{ prefix }
					, m_health_bar{ prefix }
					, m_ammo_bar{ prefix }
					, m_info_flags{ prefix }
					, m_name{ prefix }
					, m_weapon{ prefix }
					, m_oof_arrow{ prefix }
					, m_snap_lines{ prefix }
					, m_view_ray{ prefix }
					, m_sound_esp{ prefix }
				{
					this->max_distance.reg( prefix, "max distance" );
				}
			};

			std::array<overlay, 2> m_overlay{ { overlay{ "esp enemy" }, overlay{ "esp team", false } } };

			struct chams
			{
				chams_config enemy{};
				chams_config enemy_ragdoll{};
				chams_config team{};
				chams_config team_ragdoll{};
				chams_config local{};
				chams_config local_ragdoll{};
				chams_config backtrack{};
				chams_config onshot{};
				config::val<float> onshot_fade_time {0.8f, "chams onshot", "fade time"};

				chams( )
				{
					this->enemy.init( "chams enemy", "chams" );
					this->enemy.enabled.value = true;
					this->enemy.primary.enabled.value = true;
					this->enemy.primary.color = { 173, 192, 255, 255 };
					this->enemy.primary.material = cham_ids::flat;
					this->enemy.secondary.enabled.value = true;
					this->enemy.secondary.color = { 255, 208, 243, 220 };
					this->enemy.secondary.material = cham_ids::flat_ignorez;

					this->enemy_ragdoll.init( "chams enemy ragdoll", "ragdoll chams" );

					this->team.init( "chams team", "chams" );
					this->team_ragdoll.init( "chams team ragdoll", "ragdoll chams" );

					this->local.init( "chams local", "chams" );
					this->local.enabled.value = true;
					this->local.overlay.enabled.value = true;
					this->local.overlay.color = { 173, 192, 255, 175 };
					this->local.overlay.material = cham_ids::outlines;

					this->local_ragdoll.init( "chams local ragdoll", "ragdoll chams" );

					this->backtrack.init( "chams backtrack", "backtrack chams" );
					this->backtrack.primary.color = { 173, 192, 255, 25 };
					this->backtrack.primary.material = cham_ids::flat;
					this->backtrack.secondary.color = { 173, 192, 255, 255 };
					this->backtrack.secondary.material = cham_ids::outlines;

					this->onshot.init( "chams onshot", "onshot chams" );
					this->onshot.primary.enabled.value = true;
					this->onshot.primary.color = { 255, 100, 100, 200 };
					this->onshot.primary.material = cham_ids::flat;
					this->onshot.secondary.color = { 255, 100, 100, 100 };
					this->onshot.secondary.material = cham_ids::flat_ignorez;
					this->onshot.overlay.color = { 255, 100, 100, 255 };
					this->onshot.overlay.material = cham_ids::outlines;
				}
			} m_chams{};

			struct glow
			{
				glow_target enemy{ .enabled = { true, {}, "glow", "glow enemy" }, .color = { { 173, 192, 255, 40 }, "glow enemy", "color" } };
				glow_target enemy_ragdoll{ .enabled = { false, {}, "ragdoll glow", "glow enemy" }, .color = { { 173, 192, 255, 40 }, "glow enemy", "ragdoll color" } };
				glow_target team{ .enabled = { true, {}, "glow", "glow team" }, .color = { { 225, 225, 225, 40 }, "glow team", "color" } };
				glow_target team_ragdoll{ .enabled = { false, {}, "ragdoll glow", "glow team" }, .color = { { 173, 192, 255, 40 }, "glow team", "ragdoll color" } };
				glow_target local{ .enabled = { false, {}, "glow", "glow local" }, .color = { { 252, 217, 240, 50 }, "glow local", "color" } };
				glow_target local_ragdoll{ .enabled = { false, {}, "ragdoll glow", "glow local" }, .color = { { 173, 192, 255, 40 }, "glow local", "ragdoll color" } };
		} m_glow{};

	} m_player{};

		struct viewmodel
		{
			chams_config weapon{};
			chams_config arms{};

			viewmodel( )
			{
				this->weapon.init( "viewmodel weapon", "weapon chams" );
				this->weapon.enabled.value = true;
				this->weapon.primary.enabled.value = true;
				this->weapon.primary.color = { 255, 95, 175, 255 };
				this->weapon.overlay.enabled.value = true;
				this->weapon.overlay.color = { 217, 173, 202, 175 };
				this->weapon.overlay.material = cham_ids::glow;

				this->arms.init( "viewmodel arms", "arms chams" );
				this->arms.enabled.value = true;
				this->arms.primary.color = { 173, 192, 255, 255 };
				this->arms.primary.material = cham_ids::outlines;
				this->arms.overlay.enabled.value = true;
				this->arms.overlay.color = { 173, 192, 255, 255 };
				this->arms.overlay.material = cham_ids::outlines;
			}
		} m_viewmodel{};

		struct local_alpha
		{
			xui::setting enabled{ true, {}, "lower opacity", "chams local" };
			config::val<float> opacity{ 0.5f, "chams local", "opacity" };
			xui::setting only_scoped{ true, {}, "only when scoped", "chams local" };
		} m_local_alpha{};

		struct item
		{
			static constexpr auto k_group_count{ 6u };
			static constexpr const char* k_group_names[ ]{ "pistol", "smg", "rifle", "shotgun", "sniper", "utility" };

			struct overlay
			{
				struct group
				{
					enum class display_type : std::uint8_t { text, icon, text_and_icon };

					config::enm<display_type> display{ display_type::icon };
					config::val<float> max_distance{ 50.0f };
					config::col text_color{ { 255, 255, 255, 225 } };
					config::col icon_color{ { 255, 255, 255, 225 } };

					void init( std::string_view cat )
					{
						const auto s = std::string( cat );
						this->display.reg( s, "display" );
						this->max_distance.reg( s, "max distance" );
						this->text_color.reg( s, "text color" );
						this->icon_color.reg( s, "icon color" );
					}
				};

				xui::setting enabled{ true, {}, "item esp", "esp items" };
				xui::setting pistol{ true, {}, "pistol", "esp items" };
				xui::setting smg{ true, {}, "smg", "esp items" };
				xui::setting rifle{ true, {}, "rifle", "esp items" };
				xui::setting shotgun{ true, {}, "shotgun", "esp items" };
				xui::setting sniper{ true, {}, "sniper", "esp items" };
				xui::setting utility{ true, {}, "utility", "esp items" };

				std::array<group, k_group_count> groups{};

				overlay( )
				{
					for ( auto i = 0u; i < k_group_count; ++i )
					{
						this->groups[ i ].init( std::string( "esp items - " ) + k_group_names[ i ] );
					}

					this->groups[ 4 ].display = group::display_type::text_and_icon;
					this->groups[ 4 ].max_distance = 100.0f;
					this->groups[ 5 ].display = group::display_type::text_and_icon;
					this->groups[ 5 ].max_distance = 100.0f;
				}

				xui::setting& group_toggle( std::uint32_t id )
				{
					switch ( id )
					{
					case 0: return this->pistol;
					case 1: return this->smg;
					case 2: return this->rifle;
					case 3: return this->shotgun;
					case 4: return this->sniper;
					case 5: return this->utility;
					default: return this->pistol;
					}
				}

				[[nodiscard]] bool is_active( std::uint32_t group_id ) const
				{
					switch ( group_id )
					{
					case 0: return this->pistol.value;
					case 1: return this->smg.value;
					case 2: return this->rifle.value;
					case 3: return this->shotgun.value;
					case 4: return this->sniper.value;
					case 5: return this->utility.value;
					default: return false;
					}
				}

				group& get_group( std::uint32_t group_id )
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}

				const group& get_group( std::uint32_t group_id ) const
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}
			} m_overlay{};

			struct chams
			{
				xui::setting enabled{ true, {}, "item chams", "chams items" };
				xui::setting pistol{ false, {}, "pistol", "chams items" };
				xui::setting smg{ false, {}, "smg", "chams items" };
				xui::setting rifle{ false, {}, "rifle", "chams items" };
				xui::setting shotgun{ false, {}, "shotgun", "chams items" };
				xui::setting sniper{ true, {}, "sniper", "chams items" };
				xui::setting utility{ true, {}, "utility", "chams items" };

				std::array<chams_config, k_group_count> groups{};

				chams( )
				{
					for ( auto i = 0u; i < k_group_count; ++i )
					{
						const auto cat = std::string( "chams items - " ) + k_group_names[ i ];
						this->groups[ i ].init( cat, "item chams" );
					}

					this->groups[ 4 ].primary.enabled.value = true;
					this->groups[ 4 ].primary.color = { 173, 192, 255, 255 };
					this->groups[ 4 ].primary.material = cham_ids::flat;

					this->groups[ 5 ].primary.enabled.value = true;
					this->groups[ 5 ].primary.color = { 173, 192, 255, 255 };
					this->groups[ 5 ].primary.material = cham_ids::flat;
				}

				xui::setting& group_toggle( std::uint32_t id )
				{
					switch ( id )
					{
					case 0: return this->pistol;
					case 1: return this->smg;
					case 2: return this->rifle;
					case 3: return this->shotgun;
					case 4: return this->sniper;
					case 5: return this->utility;
					default: return this->pistol;
					}
				}

				[[nodiscard]] bool is_active( std::uint32_t group_id ) const
				{
					switch ( group_id )
					{
					case 0: return this->pistol.value;
					case 1: return this->smg.value;
					case 2: return this->rifle.value;
					case 3: return this->shotgun.value;
					case 4: return this->sniper.value;
					case 5: return this->utility.value;
					default: return false;
					}
				}

				chams_config& get_group( std::uint32_t group_id )
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}

				const chams_config& get_group( std::uint32_t group_id ) const
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}
			} m_chams{};

			struct glow
			{
				xui::setting enabled{ true, {}, "item glow", "glow items" };
				xui::setting pistol{ false, {}, "pistol", "glow items" };
				xui::setting smg{ false, {}, "smg", "glow items" };
				xui::setting rifle{ false, {}, "rifle", "glow items" };
				xui::setting shotgun{ false, {}, "shotgun", "glow items" };
				xui::setting sniper{ true, {}, "sniper", "glow items" };
				xui::setting utility{ true, {}, "utility", "glow items" };

				std::array<glow_target, k_group_count> groups{};

				glow( )
				{
					for ( auto i = 0u; i < k_group_count; ++i )
					{
						const auto cat = std::string( "glow items - " ) + k_group_names[ i ];
						this->groups[ i ].init( cat );
					}

					this->groups[ 4 ].color = { 173, 192, 255, 50 };
					this->groups[ 5 ].color = { 173, 192, 255, 50 };
				}

				xui::setting& group_toggle( std::uint32_t id )
				{
					switch ( id )
					{
					case 0: return this->pistol;
					case 1: return this->smg;
					case 2: return this->rifle;
					case 3: return this->shotgun;
					case 4: return this->sniper;
					case 5: return this->utility;
					default: return this->pistol;
					}
				}

				[[nodiscard]] bool is_active( std::uint32_t group_id ) const
				{
					switch ( group_id )
					{
					case 0: return this->pistol.value;
					case 1: return this->smg.value;
					case 2: return this->rifle.value;
					case 3: return this->shotgun.value;
					case 4: return this->sniper.value;
					case 5: return this->utility.value;
					default: return false;
					}
				}

				glow_target& get_group( std::uint32_t group_id )
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}

				const glow_target& get_group( std::uint32_t group_id ) const
				{
					return this->groups[ group_id < k_group_count ? group_id : 2 ];
				}
			} m_glow{};
		} m_item{};

		struct projectile
		{
			static constexpr auto k_group_count{ 6u };
			static constexpr const char* k_group_names[ ]{ "he grenade", "flashbang", "smoke", "molotov", "decoy", "inferno" };

			struct overlay
			{
				struct infernos
				{
					config::col fill_color{ { 173, 192, 255, 50 }, "esp inferno", "fill color" };
					config::col outline_color{ { 255, 171, 234, 150 }, "esp inferno", "outline color" };
					config::val<float> outline_thickness{ 1.5f, "esp inferno", "outline thickness" };
					xui::setting glow{ true, {}, "glow", "esp inferno" };
					config::val<float> glow_strength{ 0.55f, "esp inferno", "glow strength" };
				} m_infernos{};

				struct smoke_timer
				{
					xui::setting enabled{ true, {}, "smoke timer", "esp smoke" };
					xui::setting ground_ring{ true, {}, "ground ring", "esp smoke" };
					config::col color{ { 160, 210, 255, 200 }, "esp smoke", "color" };
					config::col warning_color{ { 255, 95, 120, 240 }, "esp smoke", "warning color" };
					xui::setting glow{ true, {}, "glow", "esp smoke" };
				} m_smoke_timer{};

				struct indicator
				{
					static constexpr auto k_group_count{ 3u };
					static constexpr const char* k_group_names[ ]{ "he grenade", "molotov", "inferno" };

					struct group
					{
						xui::setting enabled{ true, {}, "indicator", "esp indicator" };
						config::col arc_color{ { 173, 192, 255, 225 } };
						config::col icon_color{ { 255, 255, 255, 225 } };
						config::col background_color{ { 0, 0, 0, 175 } };
						xui::setting glow{ true, {}, "glow", "esp indicator" };
						config::val<float> glow_strength{ 1.0f };

						void init( std::string_view cat )
						{
							const auto s = std::string( cat );
							this->enabled.category = s;
							this->enabled.name = "indicator";
							this->arc_color.reg( s, "arc color" );
							this->icon_color.reg( s, "icon color" );
							this->background_color.reg( s, "background color" );
							this->glow.category = s;
							this->glow.name = "glow";
							this->glow_strength.reg( s, "glow strength" );
						}
					};

					std::array<group, k_group_count> groups{};

					indicator( )
					{
						for ( auto i = 0u; i < k_group_count; ++i )
						{
							this->groups[ i ].init( std::string( "esp indicator - " ) + k_group_names[ i ] );
						}

						this->groups[ 2 ].arc_color = { 255, 171, 234, 225 };
					}

					group& get_group( std::uint32_t id )
					{
						return this->groups[ id < k_group_count ? id : 0 ];
					}

					const group& get_group( std::uint32_t id ) const
					{
						return this->groups[ id < k_group_count ? id : 0 ];
					}
				} m_indicator{};

				struct group
				{
					enum class display_type : std::uint8_t { text, icon, text_and_icon };

					config::enm<display_type> display{ display_type::text_and_icon };
					config::val<float> max_distance{ 100.0f };
					config::col text_color{ { 255, 255, 255, 225 } };
					config::col icon_color{ { 255, 255, 255, 225 } };

					void init( std::string_view cat )
					{
						const auto s = std::string( cat );
						this->display.reg( s, "display" );
						this->max_distance.reg( s, "max distance" );
						this->text_color.reg( s, "text color" );
						this->icon_color.reg( s, "icon color" );
					}
				};

				xui::setting enabled{ true, {}, "projectile esp", "esp projectiles" };
				xui::setting he_grenade{ true, {}, "he grenade", "esp projectiles" };
				xui::setting flashbang{ true, {}, "flashbang", "esp projectiles" };
				xui::setting smoke{ true, {}, "smoke", "esp projectiles" };
				xui::setting molotov{ true, {}, "molotov", "esp projectiles" };
				xui::setting decoy{ true, {}, "decoy", "esp projectiles" };
				xui::setting inferno{ true, {}, "inferno", "esp projectiles" };

				std::array<group, 5> groups{};

				overlay( )
				{
					for ( auto i = 0u; i < 5u; ++i )
					{
						this->groups[ i ].init( std::string( "esp projectiles - " ) + k_group_names[ i ] );
					}
				}

				xui::setting& group_toggle( std::uint32_t id )
				{
					switch ( id )
					{
					case 0: return this->he_grenade;
					case 1: return this->flashbang;
					case 2: return this->smoke;
					case 3: return this->molotov;
					case 4: return this->decoy;
					case 5: return this->inferno;
					default: return this->he_grenade;
					}
				}

				[[nodiscard]] bool is_active( std::uint32_t group_id ) const
				{
					switch ( group_id )
					{
					case 0: return this->he_grenade.value;
					case 1: return this->flashbang.value;
					case 2: return this->smoke.value;
					case 3: return this->molotov.value;
					case 4: return this->decoy.value;
					case 5: return this->inferno.value;
					default: return false;
					}
				}

				group& get_group( std::uint32_t group_id )
				{
					return this->groups[ group_id < 5 ? group_id : 0 ];
				}

				const group& get_group( std::uint32_t group_id ) const
				{
					return this->groups[ group_id < 5 ? group_id : 0 ];
				}
			} m_overlay{};

		struct tracers
		{
			xui::setting enabled{ true, {}, "projectile tracers", "esp projectiles" };
			config::val<float> duration{ 4.0f, "esp projectiles", "tracer duration" };
			config::val<float> thickness{ 1.5f, "esp projectiles", "tracer thickness" };
			config::col color{ { 173, 192, 255, 255 }, "esp projectiles", "tracer color" };
			config::col glow_color{ { 173, 192, 255, 120 }, "esp projectiles", "tracer glow color" };
			xui::setting glow{ true, {}, "tracer glow", "esp projectiles" };
			config::val<float> glow_strength{ 1.0f, "esp projectiles", "tracer glow strength" };
		} m_tracers{};
		} m_projectile{};

		struct other
		{
			xui::setting bomb_timer{ true, {}, "bomb timer", "other esp" };
			xui::setting spectator_list{ true, {}, "spectator list", "other esp" };

			struct radar
			{
				enum class position_type : std::uint8_t { bottom_left, bottom_right, top_left, top_right };
				enum class mode_type : std::uint8_t { in_game_hud, standalone };
				enum class duration_type : std::uint8_t { min_15, min_30, hour_1, session };
				enum class big_radar_mode_type : std::uint8_t { hold, toggle };

				xui::setting enabled{ false, {}, "radar", "radar" };
				config::enm<mode_type> mode{ mode_type::in_game_hud, "radar", "mode" };

				// In-game HUD radar calibration & ConVar scaling
				config::val<float> in_game_scale{ 1.0f, "radar", "in-game scale" };
				config::val<float> in_game_offset_x{ 0.0f, "radar", "in-game offset x" };
				config::val<float> in_game_offset_y{ 0.0f, "radar", "in-game offset y" };
				xui::setting sync_convar{ true, {}, "sync native radar scale", "radar" };

				// Tactical Overview / Big Radar Hotkey
				xui::setting big_radar_enabled{ false, {}, "big radar overview", "radar" };
				config::val<int> big_radar_key{ 'M', "radar", "big radar key" };
				config::enm<big_radar_mode_type> big_radar_mode{ big_radar_mode_type::hold, "radar", "big radar mode" };
				config::val<float> big_radar_scale{ 1.65f, "radar", "big radar scale" };
				config::val<float> big_radar_zoom{ 0.35f, "radar", "big radar zoom" };

				// Standalone settings
				config::val<float> size{ 220.0f, "radar", "size" };
				config::val<float> range{ 1400.0f, "radar", "range" };
				config::val<float> zoom{ 1.0f, "radar", "zoom" };
				config::enm<position_type> position{ position_type::bottom_left, "radar", "position" };

				config::col background_color{ { 20, 22, 28, 175 }, "radar", "background color" };
				config::col border_color{ { 173, 192, 255, 255 }, "radar", "border color" };
				xui::setting show_cross{ false, {}, "crosshair lines", "radar" };
				xui::setting show_range_rings{ false, {}, "range rings", "radar" };

				xui::setting show_teammates{ true, {}, "show teammates", "radar" };
				config::col teammate_color{ { 120, 170, 255, 255 }, "radar", "teammate color" };
				config::col enemy_color{ { 255, 90, 90, 255 }, "radar", "enemy color" };
				config::col local_color{ { 255, 255, 255, 255 }, "radar", "local color" };
				config::val<float> dot_size{ 3.5f, "radar", "dot size" };

				xui::setting show_look{ true, {}, "look direction", "radar" };
				config::col look_color{ { 230, 235, 250, 200 }, "radar", "look color" };
				config::val<float> look_length{ 14.0f, "radar", "look length" };

				// Advanced Tactical In-Game Radar Overlays
				xui::setting elevation_arrows{ true, {}, "elevation arrows", "radar" };
				xui::setting health_rings{ true, {}, "health rings", "radar" };
				xui::setting view_cones{ true, {}, "view cones", "radar" };
				xui::setting c4_carrier_halo{ true, {}, "c4 carrier halo", "radar" };

				// Tactical Areas & Flying Grenades (both In-Game & WebRadar)
				xui::setting show_smoke{ true, {}, "smoke area", "radar" };
				config::col smoke_color{ { 165, 185, 230, 90 }, "radar", "smoke color" };
				config::val<float> smoke_radius{ 144.0f, "radar", "smoke radius" };

				xui::setting show_molotov{ true, {}, "molotov area", "radar" };
				config::col molotov_color{ { 255, 120, 50, 100 }, "radar", "molotov color" };
				config::val<float> molotov_radius{ 150.0f, "radar", "molotov radius" };

				xui::setting show_grenades{ true, {}, "flying grenades", "radar" };
				config::col grenade_color{ { 255, 255, 255, 255 }, "radar", "grenade color" };

				// WebRadar & Remote Streamer
				xui::setting webradar_enabled{ false, {}, "webradar", "webradar" };
				config::val<int> webradar_port{ 28080, "webradar", "port" };
				config::enm<duration_type> webradar_duration{ duration_type::min_30, "webradar", "duration" };
			} m_radar{};
		} m_other{};
	};

	struct changer
	{
		struct applied_skin
		{
			int paint_kit_id{};
			float wear{ 0.01f };
			int seed{};
			bool stattrak{};
			std::array<int, 5> stickers{};
			std::array<float, 5> sticker_wears{};
			std::array<float, 5> sticker_scales{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
			std::array<float, 5> sticker_rotations{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
			std::array<float, 5> sticker_offset_x{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
			std::array<float, 5> sticker_offset_y{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
			int charm_id{};
			int charm_seed{};
			float charm_offset_x{ 0.0f };
			float charm_offset_y{ 0.0f };
			float charm_offset_z{ 0.0f };

			bool operator==( const applied_skin& ) const = default;
		};

		struct skin_map_field : config::custom_field
		{
			std::unordered_map<std::int16_t, applied_skin> data{};

			nlohmann::json serialize( ) const override
			{
				auto j = nlohmann::json::object( );
				for ( const auto& [def, s] : data )
				{
					auto sj = nlohmann::json
					{
						{ "p", s.paint_kit_id },
						{ "w", s.wear },
						{ "s", s.seed },
						{ "t", s.stattrak },
						{ "cid", s.charm_id },
						{ "cs", s.charm_seed },
						{ "cox", s.charm_offset_x },
						{ "coy", s.charm_offset_y },
						{ "coz", s.charm_offset_z }
					};

					auto st_arr = nlohmann::json::array( );
					for ( const auto st : s.stickers )
					{
						st_arr.push_back( st );
					}
					sj[ "st" ] = st_arr;

					auto stw_arr = nlohmann::json::array( );
					for ( const auto stw : s.sticker_wears )
					{
						stw_arr.push_back( stw );
					}
					sj[ "stw" ] = stw_arr;

					auto stsc_arr = nlohmann::json::array( );
					for ( const auto sc : s.sticker_scales )
					{
						stsc_arr.push_back( sc );
					}
					sj[ "stsc" ] = stsc_arr;

					auto strot_arr = nlohmann::json::array( );
					for ( const auto rot : s.sticker_rotations )
					{
						strot_arr.push_back( rot );
					}
					sj[ "strot" ] = strot_arr;

					auto stox_arr = nlohmann::json::array( );
					for ( const auto ox : s.sticker_offset_x )
					{
						stox_arr.push_back( ox );
					}
					sj[ "stox" ] = stox_arr;

					auto stoy_arr = nlohmann::json::array( );
					for ( const auto oy : s.sticker_offset_y )
					{
						stoy_arr.push_back( oy );
					}
					sj[ "stoy" ] = stoy_arr;

					j[ std::to_string( def ) ] = sj;
				}

				return j;
			}

			void deserialize( const nlohmann::json& j ) override
			{
				data.clear( );

				if ( !j.is_object( ) )
				{
					return;
				}

				for ( auto it = j.begin( ); it != j.end( ); ++it )
				{
					const auto def = static_cast< std::int16_t >( std::strtol( it.key( ).c_str( ), nullptr, 10 ) );
					if ( def == 0 && it.key( ) != "0" )
					{
						continue;
					}

					auto& s = data[ def ];
					s.paint_kit_id = it.value( ).value( "p", 0 );
					s.wear = it.value( ).value( "w", 0.01f );
					s.seed = it.value( ).value( "s", 0 );
					s.stattrak = it.value( ).value( "t", false );
					s.charm_id = it.value( ).value( "cid", 0 );
					s.charm_seed = it.value( ).value( "cs", 0 );
					s.charm_offset_x = it.value( ).value( "cox", 0.0f );
					s.charm_offset_y = it.value( ).value( "coy", 0.0f );
					s.charm_offset_z = it.value( ).value( "coz", 0.0f );

					if ( it.value( ).contains( "st" ) && it.value( )[ "st" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "st" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.stickers.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.stickers[ idx ] = arr[ idx ].get<int>( );
							}
						}
					}

					if ( it.value( ).contains( "stw" ) && it.value( )[ "stw" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "stw" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.sticker_wears.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.sticker_wears[ idx ] = arr[ idx ].get<float>( );
							}
						}
					}

					if ( it.value( ).contains( "stsc" ) && it.value( )[ "stsc" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "stsc" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.sticker_scales.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.sticker_scales[ idx ] = arr[ idx ].get<float>( );
							}
						}
					}

					if ( it.value( ).contains( "strot" ) && it.value( )[ "strot" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "strot" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.sticker_rotations.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.sticker_rotations[ idx ] = arr[ idx ].get<float>( );
							}
						}
					}

					if ( it.value( ).contains( "stox" ) && it.value( )[ "stox" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "stox" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.sticker_offset_x.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.sticker_offset_x[ idx ] = arr[ idx ].get<float>( );
							}
						}
					}

					if ( it.value( ).contains( "stoy" ) && it.value( )[ "stoy" ].is_array( ) )
					{
						const auto& arr = it.value( )[ "stoy" ];
						for ( std::size_t idx = 0; idx < std::min( arr.size( ), s.sticker_offset_y.size( ) ); ++idx )
						{
							if ( arr[ idx ].is_number( ) )
							{
								s.sticker_offset_y[ idx ] = arr[ idx ].get<float>( );
							}
						}
					}
				}
			}
		};

		struct agent_selection_field : config::custom_field
		{
			// 0 = local player, 1 = all players, 2 = selected players only.
			enum class agent_mode : int { local = 0, all = 1, selected = 2 };

			std::int16_t ct_def{};
			std::int16_t t_def{};

			// empty = use the in-game agent above; otherwise the model file name
			// inside the workspace "models" folder.
			std::string custom_ct{};
			std::string custom_t{};

			int mode{ 0 };

			nlohmann::json serialize( ) const override
			{
				return nlohmann::json
				{
					{ "ct", ct_def },
					{ "t", t_def },
					{ "custom_ct", custom_ct },
					{ "custom_t", custom_t },
					{ "mode", mode }
				};
			}

			void deserialize( const nlohmann::json& j ) override
			{
				if ( !j.is_object( ) )
				{
					return;
				}

				ct_def = j.value( "ct", static_cast< std::int16_t >( 0 ) );
				t_def = j.value( "t", static_cast< std::int16_t >( 0 ) );
				custom_ct = j.value( "custom_ct", std::string{} );
				custom_t = j.value( "custom_t", std::string{} );
				mode = j.value( "mode", 0 );
			}
		};

		skin_map_field skins{};
		agent_selection_field agents{};
		config::val<int> knife_def{ 0, "changer", "knife model def" };
		config::val<int> glove_def{ 0, "changer", "glove model def" };

		changer( )
		{
			config::detail::register_field( { .key = config::detail::make_key( "changer", "applied skins" ), .type = config::field_type::custom, .ptr = &skins, .count = 1 } );
			config::detail::register_field( { .key = config::detail::make_key( "changer", "agents" ), .type = config::field_type::custom, .ptr = &agents, .count = 1 } );
		}
	};

	struct misc
	{
		struct scoreboard_weapons
		{
			xui::setting enabled{ false, {}, "scoreboard weapons", "misc" };
			// FemWare badge next to the player name (for local and detected FW users).
			xui::setting fw_logo{ true, {}, "scoreboard fw logo", "misc" };
		} m_scoreboard_weapons{};

		struct name_changer
		{
			xui::setting clantag{ false, {}, "clantag", "name changer" };
			xui::setting override_name{ false, {}, "override name", "name changer" };
			config::str name{ "Player", "name changer", "name" };
		} m_name_changer{};

		struct clantag
		{
			xui::setting enabled{ false, {}, "clantag", "clantag" };
			config::val<float> speed{ 0.5f, "clantag", "clantag speed" };
		} m_clantag{};

		struct projectile_trajectory
		{
			xui::setting enabled{ true, {}, "projectile trajectory", "trajectory" };
			xui::setting straight_throw{ true, {}, "straight throw", "trajectory" };
			xui::setting bounce_rings{ true, {}, "bounce rings", "trajectory" };
			xui::setting blast_radius_preview{ true, {}, "blast radius preview", "trajectory" };
			config::val<float> blast_ring_alpha{ 0.45f, "trajectory", "radius alpha" };

			config::col held_color{ { 173, 192, 255, 255 }, "trajectory", "held color" };
			config::col thrown_color{ { 220, 225, 240, 255 }, "trajectory", "thrown color" };
			config::col will_deal_damage_held_color{ { 252, 217, 240, 255 }, "trajectory", "will damage held color" };
			config::col will_deal_damage_thrown_color{ { 252, 217, 240, 255 }, "trajectory", "will damage thrown color" };

			xui::setting glow{ true, {}, "glow", "trajectory" };
			config::val<float> glow_strength{ 1.0f, "trajectory", "glow strength" };
		} m_projectile_trajectory{};

		struct impacts
		{
			enum class sound_type : int { shop_click, home_click, bell, killcard, bullet_casing, coin_pickup, item_drop, popcan, key_press, custom };
			enum class marker_type : int { classic, damage, both };
			enum class bullet_impact_type : int { overlay, sparks, both };

			xui::setting hit_log{ true, {}, "hit logs", "impacts" };
			config::val<float> hit_log_duration{ 3.5f, "impacts", "hit log duration" };
			xui::setting console_log{ true, {}, "console logs", "impacts" };
			xui::setting chat_log{ false, {}, "chat logs", "impacts" };

			xui::setting miss_log{ true, {}, "miss logs", "impacts" };
			config::val<float> miss_log_duration{ 4.5f, "impacts", "miss log duration" };

			xui::setting hit_sound{ true, {}, "hit sound", "impacts" };
			config::enm<sound_type> hit_sound_type{ sound_type::killcard, "impacts", "hit sound type" };
			config::val<float> hit_sound_volume{ 25.0f, "impacts", "hit sound volume" };
			config::str custom_hit_sound{ "hit.wav", "impacts", "custom hit sound" };

			xui::setting death_sound{ true, {}, "death sound", "impacts" };
			config::enm<sound_type> death_sound_type{ sound_type::bell, "impacts", "death sound type" };
			config::val<float> death_sound_volume{ 20.0f, "impacts", "death sound volume" };
			config::str custom_death_sound{ "kill.wav", "impacts", "custom death sound" };

			xui::setting hit_effect{ true, {}, "hit effect", "impacts" };
			config::col hit_effect_color{ { 173, 192, 255, 255 }, "impacts", "hit effect color" };
			config::val<float> hit_effect_duration{ 0.75f, "impacts", "hit effect duration" };
			config::val<float> hit_effect_strength{ 60.0f, "impacts", "hit effect strength" };

			xui::setting death_effect{ true, {}, "death effect", "impacts" };
			config::col death_effect_color{ { 173, 192, 255, 255 }, "impacts", "death effect color" };

			enum class kill_particle_mode : std::uint8_t { star, heart, bloom, dollar, flame, snowflake, cross, ring };
			enum class kill_physics : std::uint8_t { fall, fly, emerge };
			enum class kill_spawn_style : std::uint8_t { instant, pop, grow, fade_in };
			enum class kill_fade_style : std::uint8_t { fade, shrink, shrink_fade, blink, rise };

			struct kill_effect
			{
				config::enm<kill_particle_mode> mode{ kill_particle_mode::star, "kill effect", "mode" };
				config::enm<kill_physics> physics{ kill_physics::emerge, "kill effect", "physics" };
				config::val<int> amount{ 16, "kill effect", "amount" };
				config::val<float> density{ 1.0f, "kill effect", "density" };
				config::val<float> spread{ 9.0f, "kill effect", "spread" };
				config::enm<kill_spawn_style> spawn_style{ kill_spawn_style::pop, "kill effect", "appear" };
				config::enm<kill_fade_style> fade_style{ kill_fade_style::fade, "kill effect", "disappear" };
				config::val<float> scale{ 1.5f, "kill effect", "scale" };
				config::val<float> lifetime{ 2.0f, "kill effect", "lifetime" };
				config::val<float> speed{ 1.0f, "kill effect", "speed" };
				xui::setting color_by_health{ false, {}, "color by health", "kill effect" };
				config::col color{ { 255, 235, 120, 255 }, "kill effect", "color" };
				config::col health_color{ { 255, 70, 70, 255 }, "kill effect", "high health color" };
				xui::setting thunder{ false, {}, "thunder", "kill effect" };
			} m_kill_effect{};

			xui::setting bullet_impact_effect{ true, {}, "bullet impacts", "impacts" };
			config::enm<bullet_impact_type> bullet_impact_effect_type{ bullet_impact_type::overlay, "impacts", "bullet impact type" };
			config::col bullet_impact_effect_fill_color{ { 173, 192, 255, 85 }, "impacts", "bullet impact fill color" };
			config::col bullet_impact_effect_edge_color{ { 173, 192, 255, 255 }, "impacts", "bullet impact edge color" };
			config::col bullet_impact_effect_color_spark{ { 173, 192, 255, 255 }, "impacts", "bullet impact spark color" };
			config::val<float> bullet_impact_effect_duration{ 2.5f, "impacts", "bullet impact duration" };
			xui::setting bullet_impact_effect_glow{ true, {}, "glow", "bullet impacts" };
			config::val<float> bullet_impact_effect_glow_strength{ 1.0f, "bullet impacts", "glow strength" };
			config::val<int> impact_geometry{ 0, "impacts", "impact geometry" };
			config::val<float> impact_marker_size{ 3.5f, "impacts", "impact marker size" };

			xui::setting bullet_tracers{ false, {}, "bullet tracers", "impacts" };
			config::col bullet_tracer_color{ { 173, 192, 255, 255 }, "impacts", "bullet tracer color" };
			config::val<float> bullet_tracer_duration{ 0.5f, "impacts", "bullet tracer duration" };

			xui::setting bullet_beams{ true, {}, "bullet beams", "impacts" };
			config::val<int> beam_style{ 0, "impacts", "beam style" };
			config::col bullet_beam_color{ { 255, 95, 175, 255 }, "impacts", "bullet beam color" };
			config::val<float> bullet_beam_duration{ 1.8f, "impacts", "bullet beam duration" };
			config::val<float> bullet_beam_thickness{ 1.6f, "impacts", "bullet beam thickness" };

			xui::setting hit_marker{ true, {}, "hit marker", "impacts" };
			config::enm<marker_type> hit_marker_type{ marker_type::classic, "impacts", "hit marker type" };
			config::val<float> hit_marker_duration{ 2.5f, "impacts", "hit marker duration" };
			config::col hit_marker_color{ { 255, 255, 255, 255 }, "impacts", "hit marker color" };
			xui::setting hit_marker_glow{ true, {}, "glow", "hit marker" };
			config::val<float> hit_marker_glow_strength{ 1.0f, "hit marker", "glow strength" };

			struct damage_indicators
			{
				xui::setting enabled{ false, {}, "damage indicators", "impacts" };
				config::val<float> duration{ 1.2f, "impacts", "damage indicator duration" };
				config::val<float> rise{ 45.0f, "impacts", "damage indicator rise" };
				config::val<float> outline{ 1.0f, "impacts", "damage indicator outline" };
				config::col color{ { 255, 255, 255, 255 }, "impacts", "damage indicator color" };
				config::col headshot_color{ { 255, 120, 90, 255 }, "impacts", "damage indicator headshot color" };

				xui::setting stacking{ true, {}, "damage stacking", "damage indicators" };
				config::val<float> stack_window{ 0.25f, "damage indicators", "stack window" };
				xui::setting color_by_damage{ false, {}, "color by damage", "damage indicators" };
				config::col low_color{ { 235, 235, 235, 255 }, "damage indicators", "low damage color" };
				config::col high_color{ { 255, 70, 70, 255 }, "damage indicators", "high damage color" };
				config::val<float> arc{ 0.0f, "damage indicators", "arc drift" };
				xui::setting random_offset{ false, {}, "random offset", "damage indicators" };
				xui::setting crit_marker{ true, {}, "headshot marker", "damage indicators" };
			} m_damage_indicators{};

			struct hit_sparks
			{
				// Soup's Visuals / HitParticles modes.
				enum class spark_type : std::uint8_t
				{
					stars, hearts, bloom, glyph, blink, coron, dollar, flame, geometric,
					snowflake, virus, sword, network, cube, pyramid
				};

				enum class spark_physics : std::uint8_t { fall, fly, emerge };

				xui::setting enabled{ false, {}, "hit sparks", "impacts" };
				config::enm<spark_type> type{ spark_type::stars, "impacts", "spark type" };
				config::enm<spark_physics> physics{ spark_physics::fly, "impacts", "spark physics" };
				config::val<int> count{ 12, "impacts", "spark count" };
				config::val<float> speed{ 85.0f, "impacts", "spark speed" };
				config::val<float> size{ 4.0f, "impacts", "spark size" };
				config::val<float> lifetime{ 0.65f, "impacts", "spark lifetime" };
				config::val<float> gravity{ 45.0f, "impacts", "spark gravity" };
				config::val<float> link_distance{ 3.0f, "impacts", "spark link distance" };
				xui::setting wave{ true, {}, "color wave", "impacts" };
				config::col color{ { 255, 220, 130, 255 }, "impacts", "spark color" };
				config::col secondary_color{ { 255, 140, 80, 255 }, "impacts", "spark secondary color" };
				xui::setting headshot_boost{ true, {}, "headshot boost", "impacts" };
				config::col headshot_color{ { 255, 215, 0, 255 }, "impacts", "spark headshot color" };
			} m_hit_sparks{};

			xui::setting heart_kill_particles{ true, {}, "heart kill particles", "impacts" };
			config::col heart_particle_color{ { 255, 140, 210, 255 }, "impacts", "heart particle color" };
		} m_impacts{};

		struct removals
		{
			xui::setting crosshair{ true, {}, "remove crosshair", "removals" };
			xui::setting scope{ true, {}, "remove scope", "removals" };
			xui::setting skybox_fog{ true, {}, "remove skybox fog", "removals" };
			xui::setting overhead{ true, {}, "remove overhead", "removals" };
			xui::setting legs{ true, {}, "remove legs", "removals" };
			xui::setting skybox_3d{ true, {}, "remove 3d skybox", "removals" };
			xui::setting recoil{ true, {}, "remove recoil", "removals" };
			xui::setting decals{ true, {}, "remove decals", "removals" };
			xui::setting smoke{ true, {}, "remove smoke", "removals" };
			config::val<float> flash_alpha{ 25.0f, "removals", "flash alpha" };
		} m_removals{};

		struct spectate
		{
			xui::setting enabled{ false, {}, "spectate enemies", "spectate" };
			config::val<int> cycle_key{ VK_F4, "spectate", "cycle key" };
			config::val<int> reverse_key{ VK_F3, "spectate", "reverse key" };
			xui::setting auto_switch{ true, {}, "auto switch on target death", "spectate" };
			xui::setting draw_info{ true, {}, "draw spectated info", "spectate" };
			config::col info_color{ { 255, 255, 255, 255 }, "spectate", "info color" };
			config::col hp_color{ { 143, 255, 111, 255 }, "spectate", "hp color" };
			config::col bg_color{ { 15, 17, 26, 210 }, "spectate", "background color" };
		} m_spectate{};

		struct camera
		{
			xui::setting change_fov{ true, {}, "custom fov", "camera" };
			config::val<float> fov{ 115.0f, "camera", "fov" };

			xui::setting scoped_fov_override{ false, {}, "scoped fov override", "camera" };
			config::val<float> scoped_fov{ 40.0f, "camera", "scoped fov" };

			xui::setting thirdperson{ true, { VK_MBUTTON, xui::bind_mode::toggle }, "thirdperson", "camera" };
			config::val<float> thirdperson_distance{ 85.0f, "camera", "thirdperson distance" };
			config::val<float> thirdperson_hull_size{ 12.0f, "camera", "thirdperson hull size" };

			xui::setting change_aspect_ratio{ false, {}, "custom aspect ratio", "camera" };
			config::val<float> aspect_ratio{ 1.333f, "camera", "aspect ratio" };
		} m_camera{};

		struct viewmodel_adjust
		{
			xui::setting enabled{ false, {}, "viewmodel adjust", "viewmodel" };
			config::val<float> offset_x{ 0.0f, "viewmodel", "offset x" };
			config::val<float> offset_y{ 0.0f, "viewmodel", "offset y" };
			config::val<float> offset_z{ 0.0f, "viewmodel", "offset z" };
			config::val<float> fov{ 68.0f, "viewmodel", "viewmodel fov" };
		} m_viewmodel_adjust{};

		struct hud
		{
			struct crosshair
			{
				enum class style_type : std::uint8_t { dot, cross, circle, t_style, cross_dot };
				xui::setting enabled{ true, {}, "crosshair overlay", "crosshair" };
				config::val<int> style{ 1, "crosshair", "style" };
				config::val<float> size{ 3.0f, "crosshair", "size" };
				config::val<float> outline{ 1.0f, "crosshair", "outline" };
				config::col color{ { 0, 255, 128, 255 }, "crosshair", "color" };
				config::col outline_color{ { 15, 15, 25, 200 }, "crosshair", "outline color" };
				xui::setting dynamic_spread{ true, {}, "dynamic recoil spread", "crosshair" };
				xui::setting hit_pulse{ true, {}, "hit pulse", "crosshair" };
				config::val<float> glow_strength{ 0.8f, "crosshair", "glow strength" };
			} m_crosshair{};

			struct scope
			{
				xui::setting enabled{ true, {}, "scope overlay", "scope overlay" };
				config::val<float> line_length{ 125.0f, "scope overlay", "line length" };
				config::val<float> gap{ 8.0f, "scope overlay", "gap" };
				config::val<float> thickness{ 0.5f, "scope overlay", "thickness" };
				config::val<float> anim_speed{ 10.0f, "scope overlay", "anim speed" };
				config::col color{ { 173, 192, 255, 255 }, "scope overlay", "color" };
				xui::setting fade_in{ true, {}, "fade in", "scope overlay" };

				xui::setting glow{ true, {}, "glow", "scope overlay" };
				config::val<float> glow_strength{ 1.0f, "scope overlay", "glow strength" };
			} m_scope{};

			struct hat
			{
				enum class hat_style : std::uint8_t { cone, halo, double_rim, hex_crown };
				enum class hat_shading : std::uint8_t { gradient, wireframe, neon_rim };
				enum class hat_target : std::uint8_t { local_only, spectated, aimbot_target, all_entities };

				xui::setting enabled{ false, {}, "hat", "hat" };
				config::enm<hat_style> style{ hat_style::cone, "hat", "style" };
				config::enm<hat_shading> shading{ hat_shading::gradient, "hat", "shading" };
				config::enm<hat_target> target{ hat_target::local_only, "hat", "target" };
				config::col color{ { 255, 171, 234, 160 }, "hat", "color" };
				config::col secondary_color{ { 173, 192, 255, 160 }, "hat", "secondary color" };
				xui::setting glow{ true, {}, "glow", "hat" };
				config::val<float> glow_strength{ 1.0f, "hat", "glow strength" };
				config::val<float> radius{ 12.0f, "hat", "radius" };
				config::val<float> height{ 6.0f, "hat", "height" };
				config::val<float> vertical_offset{ 1.0f, "hat", "vertical offset" };
				config::val<float> rotation_speed{ 45.0f, "hat", "rotation speed" };

				// Soup's Visuals / ChinaHat.
				enum class hat_type : std::uint8_t { china, halo, crown, tophat };
				config::enm<hat_type> type{ hat_type::china, "hat", "type" };
				config::val<float> center_alpha{ 1.0f, "hat", "center alpha" };
				config::val<float> edge_alpha{ 0.4f, "hat", "edge alpha" };
			} m_hat{};

			struct jump_rings
			{
				enum class ring_style : std::uint8_t { ring, disc, ripples, hexagon };
				enum class trigger_mode : std::uint8_t { jump, landing, both };
				enum class particle_type : std::uint8_t { sparks, embers, smoke, neon_runes, none };
				// Soup's Visuals / JumpCircles.
				enum class ring_animation : std::uint8_t { fade, scale, both };
				enum class ring_interp : std::uint8_t { linear, smooth, fast, bounce, elastic, ease_out, spring };

				xui::setting enabled{ false, {}, "jump rings", "movement visuals" };
				config::enm<ring_style> style{ ring_style::ring, "jump rings", "style" };
				config::enm<particle_type> particles{ particle_type::sparks, "jump rings", "particles" };
				config::enm<trigger_mode> trigger{ trigger_mode::both, "jump rings", "trigger" };
				config::col color{ { 173, 192, 255, 220 }, "jump rings", "color" };
				config::col secondary_color{ { 255, 171, 234, 220 }, "jump rings", "secondary color" };
				config::val<float> max_radius{ 45.0f, "jump rings", "max radius" };
				config::val<float> duration{ 0.55f, "jump rings", "duration" };
				config::val<float> thickness{ 1.5f, "jump rings", "thickness" };
				xui::setting render_others{ false, {}, "render on others", "jump rings" };

				// Soup fields.
				// The last four are the sprite-based jump-ring styles.
				enum class ring_shape : std::uint8_t { ring, disc, hexagon, ripples, circle, circle_bold, portal, femware };
				config::enm<ring_shape> shape{ ring_shape::ring, "soup rings", "shape" };
				config::enm<ring_animation> animation{ ring_animation::both, "soup rings", "animation" };
				config::enm<ring_interp> appear_interp{ ring_interp::bounce, "soup rings", "appear interp" };
				config::enm<ring_interp> disappear_interp{ ring_interp::smooth, "soup rings", "disappear interp" };
				config::val<float> appear_duration{ 0.3f, "soup rings", "appear" };
				config::val<float> exist_duration{ 0.5f, "soup rings", "exist" };
				config::val<float> disappear_duration{ 0.5f, "soup rings", "disappear" };
				config::val<float> rotate_speed{ 2.0f, "soup rings", "rotate speed" };
				config::val<float> scale{ 1.0f, "soup rings", "scale" };
			} m_jump_rings{};

			struct ambient_motes
			{
				enum class mote_type : std::uint8_t { fireflies, constellation, drifting_motes };
				enum class look_type : std::uint8_t { orbs, stars, crystals, runes, sakura };
				// Soup's Visuals / AmbientParticles. Every entry maps to one of
				// the extracted Soup textures.
				enum class mote_mode : std::uint8_t
				{
					stars, hearts, bloom, bloom_soft, flame, snowflake, geometric, virus,
					dollar, coron, blink, firefly,
					glyph_star, glyph_cross, glyph_circle, glyph_triangle, glyph_quad, glyph_line,
					glyph_zigzag, glyph_arrow, glyph_inf, glyph_abs
				};
				enum class mote_physics : std::uint8_t { fall, fly, emerge };

				xui::setting enabled{ false, {}, "atmospheric motes", "custom cosmetics" };
				config::enm<mote_type> type{ mote_type::fireflies, "atmospheric motes", "type" };
				config::enm<look_type> look{ look_type::stars, "atmospheric motes", "look" };
				config::val<int> count{ 30, "atmospheric motes", "count" };
				config::val<float> radius{ 350.0f, "atmospheric motes", "spawn radius" };
				config::val<float> speed{ 25.0f, "atmospheric motes", "speed" };
				config::val<float> size{ 3.0f, "atmospheric motes", "size" };
				config::val<float> link_distance{ 80.0f, "atmospheric motes", "link distance" };
				config::col color{ { 180, 220, 255, 200 }, "atmospheric motes", "color" };
				config::col link_color{ { 160, 200, 255, 80 }, "atmospheric motes", "link color" };

				// Soup fields.
				config::enm<mote_mode> soup_mode{ mote_mode::stars, "soup motes", "mode" };
				config::enm<mote_physics> soup_physics{ mote_physics::fly, "soup motes", "physics" };
				config::val<int> regular_count{ 60, "soup motes", "regular count" };
				config::val<float> regular_scale{ 2.5f, "soup motes", "regular scale" };
				config::val<int> firefly_count{ 20, "soup motes", "firefly count" };
				config::val<float> firefly_scale{ 1.0f, "soup motes", "firefly scale" };
				config::val<int> firefly_trail{ 20, "soup motes", "firefly trail" };
				xui::setting links{ false, {}, "network links", "soup motes" };
				config::val<int> max_links{ 5, "soup motes", "max links" };
				config::val<float> height{ 4.0f, "soup motes", "spawn height" };
				config::col secondary_color{ { 255, 171, 234, 200 }, "soup motes", "secondary color" };
			} m_ambient_motes{};

			struct motion_trails
			{
				enum class trail_style : std::uint8_t { ribbon, neon_line, beads };
				enum class attach_point : std::uint8_t { feet, waist, weapon };
				// Soup's Visuals / Trails styles.
				enum class soup_trail_style : std::uint8_t { solid, faded, invert };
				enum class trail_render : std::uint8_t { ribbon, beads, both };

				xui::setting enabled{ false, {}, "motion trails", "custom cosmetics" };
				config::enm<trail_style> style{ trail_style::ribbon, "motion trails", "style" };
				config::enm<attach_point> attachment{ attach_point::feet, "motion trails", "attachment" };
				config::val<float> duration{ 0.65f, "motion trails", "duration" };
				config::val<float> width{ 3.5f, "motion trails", "width" };
				config::col color{ { 173, 192, 255, 230 }, "motion trails", "color" };
				config::col secondary_color{ { 255, 130, 200, 230 }, "motion trails", "secondary color" };
				xui::setting velocity_color{ true, {}, "velocity reactive", "motion trails" };
				config::col fast_color{ { 255, 85, 105, 255 }, "motion trails", "fast color" };
				xui::setting render_others{ false, {}, "render on others", "motion trails" };

				// Soup fields.
				config::enm<soup_trail_style> soup_style{ soup_trail_style::faded, "soup trails", "style" };
				config::val<float> height_pct{ 42.0f, "soup trails", "height" };
				config::val<float> down{ 0.0f, "soup trails", "down" };
				config::val<float> alpha_factor{ 50.0f, "soup trails", "alpha factor" };
				config::val<float> trail_alpha{ 100.0f, "soup trails", "trail alpha" };
				config::enm<trail_render> render{ trail_render::both, "soup trails", "render" };
				config::val<int> bead_style{ 11, "soup trails", "bead sprite" };
				config::val<float> bead_size{ 7.0f, "soup trails", "bead size" };
				config::val<float> taper{ 0.55f, "soup trails", "taper" };
				xui::setting glow{ true, {}, "glow", "soup trails" };
				config::val<float> glow_strength{ 1.0f, "soup trails", "glow strength" };
			} m_motion_trails{};

			struct velocity
			{
				xui::setting counter{ false, {}, "velocity counter", "velocity hud" };
				xui::setting chart{ false, {}, "velocity chart", "velocity hud" };
				config::col color{ { 255, 95, 175, 255 }, "velocity hud", "color" };
				config::val<float> bottom_offset{ 80.0f, "velocity hud", "bottom offset" };
				config::val<float> chart_width{ 200.0f, "velocity hud", "chart width" };
				config::val<float> chart_height{ 44.0f, "velocity hud", "chart height" };
			} m_velocity{};

			struct recoil
			{
				xui::setting shown{ false, {}, "spread circle", "recoil hud" };
				config::col spread_color{ { 173, 192, 255, 70 }, "recoil hud", "spread color" };
				config::val<float> spread_thickness{ 1.0f, "recoil hud", "spread thickness" };
				xui::setting show_recoil_dot{ true, {}, "recoil dot", "recoil hud" };
				config::col dot_color{ { 255, 171, 234, 255 }, "recoil hud", "recoil dot color" };
				config::val<float> dot_size{ 2.0f, "recoil hud", "recoil dot size" };
			} m_recoil{};

			struct combat_badges
			{
				xui::setting enabled{ true, {}, "combat override badges", "combat badges" };
				xui::setting show_dmg{ true, {}, "damage override badge", "combat badges" };
				xui::setting show_hc{ true, {}, "hitchance override badge", "combat badges" };
				xui::setting show_baim{ true, {}, "body aim badge", "combat badges" };
				xui::setting show_fd{ true, {}, "fake duck badge", "combat badges" };
				xui::setting show_peek{ true, {}, "quick peek badge", "combat badges" };
			} m_combat_badges{};

			struct fw_logo_cfg
			{
				xui::setting enabled{ false, {}, "fw logo widget", "hud" };
				config::val<int> mode{ 1, "hud", "fw logo mode" };
				config::val<float> pos_x{ 100.0f, "hud", "fw logo pos x" };
				config::val<float> pos_y{ 100.0f, "hud", "fw logo pos y" };
				config::val<float> size{ 64.0f, "hud", "fw logo size" };
				config::val<float> speed{ 1.0f, "hud", "fw logo speed" };
				config::val<int> fps{ 30, "hud", "fw logo fps" };
			} m_fw_logo{};
		} m_hud{};

		struct post_process
		{
			struct chromatic_aberration
			{
				xui::setting enabled{ false, {}, "chromatic aberration", "post process" };
				config::val<float> intensity{ 0.003f, "post process", "chromatic aberration intensity" };
			} m_chromatic_aberration{};
		} m_post_process{};

		struct dlight
		{
			xui::setting enabled{ false, {}, "dynamic light", "misc" };
			config::col color{ { 255, 255, 255, 255 }, "dlight", "color" };
			config::val<float> radius{ 300.0f, "dlight", "radius" };
			config::val<float> z_offset{ 2.0f, "dlight", "z offset" };
		} m_dlight{};

		struct autobuy
		{
			xui::setting enabled{ true, {}, "auto buy", "autobuy" };
			config::val<int> primary_weapon{ 3, "autobuy", "primary weapon" };
			config::val<int> secondary_weapon{ 3, "autobuy", "secondary weapon" };
			xui::setting armor{ true, {}, "armor", "autobuy" };
			xui::setting defuser{ true, {}, "defuser", "autobuy" };
			xui::setting taser{ true, {}, "taser", "autobuy" };
			config::bools<5> grenades{ { true, true, true, false, false }, "autobuy", "grenades" };
		} m_autobuy{};

		xui::setting preserve_killfeed{ true, {}, "preserve killfeed", "misc" };
		xui::setting reveal_radar{ true, {}, "reveal radar", "misc" };
		xui::setting disable_game_logs{ true, {}, "disable game logs", "misc" };
		xui::setting auto_accept{ true, {}, "auto accept", "misc" };
		config::val<int> menu_key{ VK_DELETE, "misc", "menu key" };


		struct keybind_list_cfg
		{
			xui::setting enabled{ false, {}, "keybind list", "misc" };
		} m_keybind_list{};

		struct media_player_cfg
		{
			enum class position_preset : std::uint8_t { top_center, top_right, bottom_left, bottom_right, custom };

			xui::setting enabled{ false, {}, "media player hud", "misc" };
			config::enm<position_preset> position{ position_preset::top_center, "misc", "media position" };
			config::val<float> pos_x{ -1.0f, "misc", "media pos x" };
			config::val<float> pos_y{ 40.0f, "misc", "media pos y" };
			xui::setting auto_hide{ false, {}, "auto hide when paused", "misc" };
			config::val<int> key_play_pause{ 0, "misc", "media play pause key" };
			config::val<int> key_next{ 0, "misc", "media next key" };
			config::val<int> key_prev{ 0, "misc", "media prev key" };
			config::col accent_color{ { 214, 26, 158, 255 }, "misc", "media accent color" };

			// Lyrics (fetched from LRCLIB by artist/title).
			xui::setting fetch_lyrics{ true, {}, "lyrics (lrclib)", "misc" };
			config::val<float> lyrics_offset_ms{ 150.0f, "misc", "lyrics offset ms" };
			config::val<float> lyrics_size{ 1.0f, "misc", "lyrics size" };
			config::col lyrics_color{ { 210, 210, 224, 215 }, "misc", "lyrics color" };
			config::col lyrics_highlight{ { 214, 26, 158, 255 }, "misc", "lyrics highlight" };
		} m_media_player{};

		struct spectrum_cfg
		{
			xui::setting enabled{ true, {}, "audio visualizer", "misc" };
			config::val<float> pos_x{ 40.0f, "misc", "visualizer pos x" };
			config::val<float> pos_y{ 120.0f, "misc", "visualizer pos y" };
			config::val<float> width{ 240.0f, "misc", "visualizer width" };
			config::val<float> height{ 48.0f, "misc", "visualizer height" };
			config::val<float> sensitivity{ 1.0f, "misc", "visualizer sensitivity" };
			config::col color{ { 214, 26, 158, 255 }, "misc", "visualizer color" };
		} m_spectrum{};

		struct discord_rpc_cfg
		{
			enum class status_style : std::uint8_t { femboying, competitive, clean };

			xui::setting enabled{ true, {}, "discord rich presence", "misc" };
			xui::setting show_map{ true, {}, "show current map", "misc" };
			xui::setting show_time{ true, {}, "show elapsed time", "misc" };
			config::enm<status_style> style{ status_style::femboying, "misc", "discord style" };
			config::str client_id{ "1548720978279014591", "misc", "discord client id" };
			config::str large_image{ "https://i.imgur.com/LeenC9Y.gif", "misc", "discord large image" };
		} m_discord_rpc{};

		struct streamproof_cfg
		{
			xui::setting enabled{ false, {}, "streamproof", "misc" };
			xui::setting hide_menu{ true, {}, "hide menu on stream", "misc" };
		} m_streamproof{};

		struct watermark_cfg
		{
			xui::setting enabled  { true, {}, "watermark",       "watermark" };
			xui::setting show_fps { true, {}, "show fps",        "watermark" };
			xui::setting show_ping{ true, {}, "show ping",       "watermark" };
			xui::setting show_time{ true, {}, "show time",       "watermark" };
			xui::setting show_user{ true, {}, "show user",       "watermark" };
			xui::setting show_map { true, {}, "show map",        "watermark" };
		xui::setting show_tick{ true, {}, "show tick",       "watermark" };
		xui::setting show_velocity{ true, {}, "show velocity", "watermark" };
		xui::setting show_build{ true, {}, "show build", "watermark" };
		} m_watermark{};

		struct widgets_cfg
		{
			enum class style : std::uint8_t { modern, classic, neo, glass };

			config::enm<style> widget_style{ style::modern, "widgets", "style" };

			struct glass_cfg
			{
				config::col text_color{ { 235, 238, 248, 255 }, "glass widget", "text color" };
				config::col icon_color{ { 173, 192, 255, 255 }, "glass widget", "icon color" };
				xui::setting per_stat_icon_colors{ false, {}, "per stat icon colors", "glass widget" };
				config::col logo_icon_color{ { 173, 192, 255, 255 }, "glass widget", "logo icon color" };
				config::col fps_icon_color{ { 173, 192, 255, 255 }, "glass widget", "fps icon color" };
				config::col ping_icon_color{ { 173, 192, 255, 255 }, "glass widget", "ping icon color" };
				config::col time_icon_color{ { 173, 192, 255, 255 }, "glass widget", "time icon color" };
				config::col vel_icon_color{ { 255, 95, 175, 255 }, "glass widget", "velocity icon color" };
				config::col warn_text_color{ { 255, 92, 92, 255 }, "glass widget", "warn text color" };
				config::col warn_icon_color{ { 255, 92, 92, 255 }, "glass widget", "warn icon color" };
				config::val<int> ping_warn_threshold{ 80, "glass widget", "ping warn threshold" };
				config::col bg_color{ { 12, 14, 20, 155 }, "glass widget", "background color" };
				config::col shadow_color{ { 0, 0, 0, 255 }, "glass widget", "shadow color" };
				config::col avatar_ring_color{ { 255, 255, 255, 40 }, "glass widget", "avatar ring color" };
				config::val<float> blur_strength{ 1.0f, "glass widget", "blur strength" };
				config::val<float> shadow_strength{ 1.4f, "glass widget", "shadow strength" };
				config::val<float> shadow_spread{ 1.2f, "glass widget", "shadow spread" };
				config::val<float> icon_size{ 15.0f, "glass widget", "icon size" };
				config::val<float> pill_height{ 32.0f, "glass widget", "pill height" };
				config::val<float> section_gap{ 16.0f, "glass widget", "section gap" };
				config::val<float> pad_x{ 14.0f, "glass widget", "padding x" };
				xui::setting show_avatar{ true, {}, "show avatar", "glass widget" };
			} m_glass{};
		} m_widgets{};
	};

	struct movement
	{
		xui::setting bhop{ true, {}, "bhop", "movement" };
		config::val<int> bhop_hitchance{ 100, "movement", "bhop hitchance" };
		config::val<int> bhop_max_consecutive{ 0, "movement", "bhop max consecutive" };
		xui::setting airstrafe{ true, {}, "airstrafe", "movement" };
		xui::setting airstrafe_fully_directional{ true, {}, "fully directional", "movement - airstrafe" };
		config::val<float> airstrafe_turn_rate_limit{ 0.0f, "movement - airstrafe", "turn rate limit" };
		config::val<float> airstrafe_wobble{ 0.0f, "movement - airstrafe", "humanized wobble" };
		xui::setting jumpbug{ true, {}, "jumpbug", "movement" };
		xui::setting fastladder{ true, {}, "fastladder", "movement" };
		xui::setting edgejump{ false, { 'E', xui::bind_mode::hold_on}, "edgejump", "movement" };
		xui::setting edgestop{ false, { 'N', xui::bind_mode::hold_on}, "edgestop", "movement" };
		xui::setting edgebug{ false, {}, "edgebug", "movement" };
		/// 0..4 â€” matches jmp table order around \c loc_C80A3A in dump (mode dword selects case before the active path).
		config::val<int> edgebug_mode{ 1, "movement", "edgebug mode" };
		/// Analog of \c xmmword_E22CA4+0xC â€” extra subtick duck cycles (each cycle = press+release pair).
		config::val<int> edgebug_passes{ 1, "movement", "edgebug passes" };
		/// Adds jump up/down subticks like jumpbug after duck sequence (not in every dump path; optional).
		xui::setting edgebug_include_jump_steps{ false, {}, "edgebug jump steps", "movement" };
		xui::setting slowwalk{ false, { 'P', xui::bind_mode::hold_on}, "slowwalk", "movement" };
		config::val<float> slowwalk_speed{ 33.0f, "movement", "slowwalk speed" };

		struct test_strafer
		{
			xui::setting enabled{ false, {}, "test strafer", "movement" };
		} m_test_strafer{};

		struct velocity_debug
		{
			xui::setting enabled{ false, {}, "velocity debug", "movement" };
			xui::setting reset_on_land{ true, {}, "reset peak on land", "movement - velocity debug" };
		} m_velocity_debug{};
	};

	struct world
	{
		struct weather
		{
			enum class weather_type : std::uint8_t { snow, rain, stars, embers };

			xui::setting enabled{ true, {}, "weather", "weather" };
			config::enm<weather_type> type{ weather_type::snow, "weather", "type" };
			config::col color{ { 117, 120, 142, 144 }, "weather", "color" };

			xui::setting fog_enabled{ true, {}, "fog", "weather" };
			config::val<float> fog_density{ 0.5f, "weather", "fog density" };
			config::val<float> fog_anisotropy{ 0.5f, "weather", "fog anisotropy" };
			config::val<float> fog_draw_distance{ 8000.0f, "weather", "fog draw distance" };
			config::col fog_color{ { 160, 175, 210, 255 }, "weather", "fog color" };

			xui::setting wetness{ false, {}, "wetness", "weather" };
			config::val<float> wetness_density{ 1.8f, "weather", "wetness density" };
			config::val<float> wetness_speed{ 0.8f, "weather", "wetness speed" };

			xui::setting wind{ true, {}, "wind", "weather" };
			config::val<float> wind_strength{ 3.0f, "weather", "wind strength" };
			config::val<float> wind_direction{ 0.0f, "weather", "wind direction" };
			config::val<float> wind_turbulence{ 1.0f, "weather", "wind turbulence" };
		} m_weather{};

		struct scene
		{
			struct skyboxing
			{
				xui::setting custom_skybox{ true, {}, "skybox material", "scene" };
				config::val<int> selected_skybox{ 0, "scene", "selected skybox" };

				xui::setting custom_color{ true, {}, "skybox color", "scene" };
				config::col skybox_color{ { 249, 103, 206, 255 }, "scene", "skybox color value" };
				config::col cloud_color{ { 173, 192, 255, 0 }, "scene", "cloud color" };
				config::col sun_color{ { 173, 192, 255, 0 }, "scene", "sun color" };
			};

			skyboxing skybox{};

			xui::setting lighting{ true, {}, "lighting", "scene" };
			config::col lighting_color{ { 173, 192, 255, 255 }, "scene", "lighting color" };
			config::val<float> lighting_intensity{ 0.85f, "scene", "lighting intensity" };
			config::vec3 lighting_rotation{ { -0.9f, 0.3f, 0.2f }, "scene", "lighting rotation" };

			xui::setting world_setting{ true, {}, "world color", "scene" };
			config::col world_color{ { 115, 125, 160, 255 }, "scene", "world color value" };

			xui::setting bloom{ true, {}, "bloom", "scene" };
			config::val<float> bloom_value{ 2.0f, "scene", "bloom value" };

			xui::setting gamma{ true, {}, "gamma", "scene" };
			config::val<float> gamma_value{ 2.2f, "scene", "gamma value" };

			xui::setting dof{ true, {}, "depth of field", "scene" };
			config::val<float> dof_near_blurry{ 0.0f, "scene", "dof near blurry" };
			config::val<float> dof_near_crisp{ 5.0f, "scene", "dof near crisp" };
			config::val<float> dof_far_crisp{ 600.0f, "scene", "dof far crisp" };
			config::val<float> dof_far_blurry{ 1400.0f, "scene", "dof far blurry" };

			xui::setting ambient{ true, {}, "ambient", "scene" };
			config::col ambient_color{ { 233, 145, 255, 255 }, "scene", "ambient color" };
			config::val<float> ambient_intensity{ 1.1f, "scene", "ambient intensity" };

			xui::setting night_mode{ false, {}, "night mode", "scene" };
			config::val<float> night_mode_darkness{ 0.35f, "scene", "night mode darkness" };

			xui::setting candlelight_mood{ false, {}, "candlelight dinner mood", "scene" };
			config::val<float> candlelight_warmth{ 0.75f, "scene", "candlelight warmth" };

			xui::setting cyberpunk_mood{ false, {}, "cyberpunk neon atmosphere", "scene" };
			config::val<float> cyberpunk_glow{ 1.0f, "scene", "cyberpunk glow intensity" };

			xui::setting chromatic_aberration{ false, {}, "chromatic aberration", "scene" };
			config::val<float> chromatic_strength{ 1.0f, "scene", "chromatic strength" };
		} m_scene{};
	};

	struct theme
	{
		enum class palette : std::uint8_t { femware, catppuccin, coffee, tokyo_night };
		enum class catppuccin_flavor : std::uint8_t { latte, frappe, macchiato, mocha };
		enum class role : std::uint8_t { text, accent, background, subtext };

		struct swatch
		{
			float r{}, g{}, b{}, a{};
		};

		config::enm<palette> selected{ palette::femware, "theme", "name palette" };
		config::enm<catppuccin_flavor> flavor{ catppuccin_flavor::mocha, "theme", "catppuccin flavor" };
		xui::setting use_custom{ false, {}, "use custom name colors", "theme" };
		config::col text_color{ { 228, 233, 238, 255 }, "theme", "name text color" };
		config::col accent_color{ { 0, 192, 255, 255 }, "theme", "name accent color" };
		config::col background_color{ { 12, 15, 19, 110 }, "theme", "name background color" };
		config::col subtext_color{ { 156, 165, 178, 210 }, "theme", "name subtext color" };

		[[nodiscard]] static constexpr swatch palette_for( palette p, role r, catppuccin_flavor f = catppuccin_flavor::mocha )
		{
			switch ( p )
			{
			case palette::femware:
				switch ( r )
				{
				case role::text: return { 0.894f, 0.905f, 0.921f, 1.000f };
				case role::accent: return { 0.839f, 0.102f, 0.620f, 1.000f };
				case role::background: return { 0.133f, 0.133f, 0.161f, 0.933f };
				default: return { 0.588f, 0.588f, 0.651f, 1.000f };
				}
			case palette::catppuccin:
				if ( f == catppuccin_flavor::latte )
				{
					switch ( r )
					{
					case role::text: return { 0.298f, 0.310f, 0.412f, 1.000f };
					case role::accent: return { 0.016f, 0.647f, 0.898f, 1.000f };
					case role::background: return { 0.937f, 0.945f, 0.961f, 0.850f };
					default: return { 0.549f, 0.561f, 0.631f, 1.000f };
					}
				}
				else if ( f == catppuccin_flavor::frappe )
				{
					switch ( r )
					{
					case role::text: return { 0.776f, 0.816f, 0.961f, 1.000f };
					case role::accent: return { 0.600f, 0.820f, 0.859f, 1.000f };
					case role::background: return { 0.188f, 0.204f, 0.275f, 0.820f };
					default: return { 0.647f, 0.678f, 0.808f, 1.000f };
					}
				}
				else if ( f == catppuccin_flavor::macchiato )
				{
					switch ( r )
					{
					case role::text: return { 0.792f, 0.827f, 0.961f, 1.000f };
					case role::accent: return { 0.569f, 0.843f, 0.890f, 1.000f };
					case role::background: return { 0.141f, 0.153f, 0.227f, 0.820f };
					default: return { 0.647f, 0.678f, 0.796f, 1.000f };
					}
				}
				switch ( r ) // mocha
				{
				case role::text: return { 0.804f, 0.839f, 0.957f, 1.000f };
				case role::accent: return { 0.537f, 0.863f, 0.922f, 1.000f };
				case role::background: return { 0.118f, 0.118f, 0.180f, 0.820f };
				default: return { 0.651f, 0.678f, 0.784f, 1.000f };
				}
			case palette::coffee:
				switch ( r )
				{
				case role::text: return { 0.886f, 0.824f, 0.737f, 1.000f };
				case role::accent: return { 0.780f, 0.682f, 0.486f, 1.000f };
				case role::background: return { 0.118f, 0.106f, 0.078f, 0.780f };
				default: return { 0.416f, 0.325f, 0.263f, 0.800f };
				}
			default: // tokyo_night
				switch ( r )
				{
				case role::text: return { 0.753f, 0.769f, 0.961f, 1.000f };
				case role::accent: return { 0.482f, 0.639f, 0.972f, 1.000f };
				case role::background: return { 0.106f, 0.108f, 0.199f, 0.800f };
				default: return { 0.820f, 0.659f, 0.816f, 0.902f };
				}
			}
		}

		[[nodiscard]] swatch effective( role r ) const
		{
			if ( this->use_custom )
			{
				const auto norm = []( std::uint8_t v ) -> float { return static_cast< float >( v ) / 255.0f; };
				switch ( r )
				{
				case role::text: return { norm( this->text_color.value.r ), norm( this->text_color.value.g ), norm( this->text_color.value.b ), norm( this->text_color.value.a ) };
				case role::accent: return { norm( this->accent_color.value.r ), norm( this->accent_color.value.g ), norm( this->accent_color.value.b ), norm( this->accent_color.value.a ) };
				case role::background: return { norm( this->background_color.value.r ), norm( this->background_color.value.g ), norm( this->background_color.value.b ), norm( this->background_color.value.a ) };
				default: return { norm( this->subtext_color.value.r ), norm( this->subtext_color.value.g ), norm( this->subtext_color.value.b ), norm( this->subtext_color.value.a ) };
				}
			}
			return palette_for( this->selected.value, r, this->flavor.value );
		}
	};

	struct cheat
	{
		xui::setting unsafe_mode{ false, {}, "unsafe mode", "cheat" };

		theme m_theme{};
	};

	inline combat g_combat{};
	inline esp g_esp{};
	inline changer g_changer{};
	inline misc g_misc{};
	inline movement g_movement{};
	inline world g_world{};
	inline cheat g_cheat{};

	inline void finalize_binds( )
	{
		auto& aa = g_combat.m_antiaim;
		aa.manual_left.bind.excludes = &aa.manual_right;
		aa.manual_right.bind.excludes = &aa.manual_left;

		if ( aa.manual_left.value && aa.manual_right.value )
		{
			aa.manual_right.value = false;
			aa.manual_right.bind.active = false;
		}
	}

} // namespace settings
