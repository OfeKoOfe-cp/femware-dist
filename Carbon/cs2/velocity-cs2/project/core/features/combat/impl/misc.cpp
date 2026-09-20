#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/logging/logging.hpp>
#include <core/systems/systems.hpp>
#include <core/features/features.hpp>
#include <protection/game_addresses.hpp>
namespace features::combat {

	void misc::antiaim::reset( )
	{
		// Drop all carry-over state. A stale active flag left over from the
		// previous round made override_view keep force-writing the camera while
		// dead/spec/freezetime ("AA randomly flicks after round restart").
		this->m_antiaim_active = false;
		this->m_should_correct = false;
		this->m_old_angles = {};
		this->m_modified_angles = {};
		this->m_lby_elapsed_ticks = 0;
		this->m_lby_break_ticks = 0;
		this->m_lby_break_now = false;
		this->m_indicator_yaw = 0.0f;
		this->m_steer_valid = false;
		this->m_steer_offset = 0.0f;
	}

	void misc::antiaim::on_create_move( systems::input::usercmd* cmd )
	{
		auto& cfg = settings::g_combat.m_antiaim;

		this->m_antiaim_active = false;
		this->m_lby_break_now = false;

		if ( !cfg.enabled.value )
		{
			return;
		}

		// The ragebot finalizes the aim in this same tick, then we run. When the
		// shot goes out through the subtick path it does not necessarily set the
		// in_attack bit on cmd->buttons, so the old check below missed it and we
		// clobbered the shot with the fake yaw. Defer to a fired shot always.
		//
		// A fired tick still needs the camera/mouse anchor: the ragebot runs a
		// silent aim, so the view setup would otherwise be driven by the sent
		// (aim) angles and the camera snaps hard onto the target for a frame.
		// Anchor the view to the player's real input but leave the command to
		// the ragebot (no fake write, no movement rotation).
		if ( features::combat::g_rage.is_firing_this_tick( ) )
		{
			const auto local = systems::g_local.get( );
			if ( local.pawn && local.is_alive && !systems::g_local.is_in_cinematic( ) && !systems::g_local.is_in_time_freeze( ) )
			{
				bool view_ok{};
				auto real = systems::g_input.get_view_angles( &view_ok );
				if ( view_ok )
				{
					math::helpers::normalize_angles( real );
					systems::g_input.set_view_angles( real );
					if ( const auto eye_offset = SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) )
					{
						memory::write<math::vector3>( local.pawn + eye_offset, real );
					}
					

				// Keep the indicator pinned to the last SENT fake. Calling
				// get_yaw here would consume a jitter/legit_desync flip without
				// transmitting a fake, so the body's L/R phase drifts relative
				// to what the server actually saw right after every shot
				// ("messy AA after firing").
				this->m_old_angles = real;
				this->m_modified_angles = real;
				math::helpers::normalize_angles( this->m_old_angles );
				this->m_modified_angles.y = this->m_indicator_yaw;
				this->m_should_correct = false;
				}
				return;
			}
		}

		if ( systems::g_local.is_in_cinematic( ) || systems::g_local.is_in_time_freeze( ) )
		{
			return;
		}

		if ( cfg.manual_left.value && cfg.manual_right.value )
		{
			cfg.manual_right.value = false;
			cfg.manual_right.bind.active = false;
		}

		if ( cfg.manual_left.value )
		{
			this->m_yaw_side = -1;
		}
		else if ( cfg.manual_right.value )
		{
			this->m_yaw_side = 1;
		}
		else
		{
			this->m_yaw_side = 0;
		}

		const auto local = systems::g_local.get( );
		const auto base = cmd->csgo_user_cmd.mutable_base( );
		if ( !local.pawn || !base )
		{
			return;
		}

		// AA is a movement/animation feature; dead or spectating hands stay out of
		// it (guard the pawn as well so a just-respawned pawn can't inherit stale
		// angles from the death cam).
		if ( !local.is_alive )
		{
			return;
		}

		if ( cmd->buttons.value & cstypes::command_buttons::in_use )
		{
			return;
		}

		const auto& ctx = g_shared.ctx( );
		if ( ( cmd->buttons.value & cstypes::command_buttons::in_attack ) ||
			 ( ( cmd->buttons.value & cstypes::command_buttons::in_second_attack ) && ctx.weapon_type == cstypes::weapon_type::knife ) )
		{
			return;
		}

		if ( ctx.weapon_type == cstypes::weapon_type::grenade )
		{
			if ( memory::read<float>( ctx.weapon + SCHEMA( "C_BaseCSGrenade", "m_fThrowTime"_hash ) ) > 0.0f )  // bail regardless of pin state
			{
				return;
			}
		}

		const auto move_type = memory::read<int>( local.pawn + SCHEMA( "C_BaseEntity", "m_nActualMoveType"_hash ) );
		if ( move_type == cstypes::move_type::ladder || move_type == cstypes::move_type::noclip )
		{
			return;
		}

		if ( this->is_near_ladder( local.pawn ) )
		{
			return;
		}

		// Real reference: the user's true view lives in the engine's input view
		// (CSGOInput), which the mouse layer feeds continuously and which we
		// re-anchor every active tick. The command's pre-AA base angles are NOT
		// a safe "real": prediction writes the previous tick's FAKE into the
		// pawn's eye angles right after we refresh them, and the engine composes
		// this command's baseline from those leaked eye angles, so reading them
		// here re-feeds the fake as the reference every tick. That makes the
		// camera walk a step per tick and the movement correction rotate against
		// a moving target (input feels inverted / "hard to walk"). Use the
		// input view as the ground truth and fall back to the composed field
		// only when the input view is unavailable.
		math::vector3 real_angles{};
		bool view_ok{ false };
		real_angles = systems::g_input.get_view_angles( &view_ok );
		if ( !view_ok )
		{
			if ( const auto va = base->viewangles( ) )
			{
				real_angles = { va->x( ), va->y( ), va->z( ) };
			}
		}
		math::helpers::normalize_angles( real_angles );

		// Legacy lby breaker: while grounded and effectively stationary the fake
		// pose holds still, so on a ~1.1 s cadence we force a single sharp tick
		// toward the pure back direction to break prediction/animation habits.
		if ( cfg.lby_breaker.value )
		{
			const auto& pre = systems::g_prediction.pre( );
			const auto speed_sq = pre.velocity.x * pre.velocity.x + pre.velocity.y * pre.velocity.y;
			const auto stationary = ( pre.flags & cstypes::entity_flags::on_ground ) != 0 && speed_sq < ( 45.0f * 45.0f );

			if ( stationary )
			{
				if ( this->m_lby_break_ticks > 0 )
				{
					--this->m_lby_break_ticks;
					this->m_lby_break_now = true;
				}
				else if ( ++this->m_lby_elapsed_ticks >= this->k_lby_breaker_period )
				{
					this->m_lby_break_ticks = this->k_lby_breaker_hold;
					this->m_lby_elapsed_ticks = 0;
				}
			}
			else
			{
				this->m_lby_elapsed_ticks = 0;
			}
		}

		this->m_old_angles = real_angles;
		this->m_antiaim_active = true;

		this->m_modified_angles = this->m_old_angles;
		this->m_modified_angles.x = this->get_pitch( this->m_old_angles.x );
		this->m_modified_angles.y = this->get_yaw( this->m_old_angles, local );

		math::helpers::normalize_angles( this->m_modified_angles );

		if ( const auto angles = base->mutable_viewangles( ) )
		{
			angles->set_x( this->m_modified_angles.x );
			angles->set_y( this->m_modified_angles.y );
			angles->set_z( this->m_modified_angles.z );
		}

		if ( cfg.log_detail.value )
		{
			static int s_aa_log_ticks = 0;
			if ( ( s_aa_log_ticks++ & 63 ) == 0 )
			{
				float base_yaw = 0.0f;
				float base_pitch = 0.0f;
				if ( const auto va = base->viewangles( ) )
				{
					base_pitch = va->x( );
					base_yaw = va->y( );
				}
				const auto yaw_leak = [&]( )
				{
					float diff = real_angles.y - base_yaw;
					math::helpers::normalize_angle( diff );
					return std::fabsf( diff );
				}( );
				logging::console::print(
					xs( "[aa] in {:.1f}/{:.1f} base {:.1f}/{:.1f}{} sent {:.1f}/{:.1f}" ),
					real_angles.x, real_angles.y, base_pitch, base_yaw,
					yaw_leak > 0.5f ? xs( " LEAK" ) : xs( "" ),
					this->m_modified_angles.x, this->m_modified_angles.y );
			}
		}

		// CRITICAL: re-anchor the engine input view to the real angles. Writing
		// only the command would let the fake leak into the input baseline on the
		// following tick, which walks the camera every frame and quickly turns
		// the view into an uncontrollable spin ("AA doesn't work"). Anchoring
		// each active tick keeps the mouse/view anchored to reality while the
		// transmitted command still carries the fake pose for the server/body.
		systems::g_input.set_view_angles( real_angles );

		// CS2 prediction copies the command's view angles into the pawn's eye
		// angles, and those eye angles seed the NEXT command's composition and
		// its movement basis. Tasking the pawn with the fake leaks it forward:
		// each tick's "real" baseline drifts by the fake, so the delta collapses
		// (desync barely visible) and the movement correction fights a moving
		// target (input feels inverted). Refresh the pawn's real eye angles on
		// every active tick so the camera, mouse baseline, and movement basis all
		// stay locked to the user's true view while only the transmitted command
		// carries the fake.
		if ( const auto eye_offset = SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) )
		{
			memory::write<math::vector3>( local.pawn + eye_offset, real_angles );
		}

		this->m_should_correct = true;

		this->correct_movement( cmd );
	}

	void misc::antiaim::on_override_view( std::uintptr_t view_setup ) const
	{
		const auto& cfg = settings::g_combat.m_antiaim;
		if ( !cfg.enabled.value )
		{
			return;
		}

		// Only re-anchor the camera while the local player is fighting. Dead /
		// spectating / freezetime the game owns the view (killcam, overview,
		// buyzoom); force-writing the input view there is what makes the camera
		// randomly jump around after a round restart.
		const auto local = systems::g_local.get( );
		if ( !local.pawn || !local.is_alive || systems::g_local.is_in_cinematic( ) || systems::g_local.is_in_time_freeze( ) )
		{
			return;
		}

		// The transmitted command carries the fake yaw, and command prediction
		// rewrites the pawn's eye angles from it after our create_move write, so
		// the render camera snaps to the fake and back every tick ("AA flicks the
		// camera"). Re-anchor the final view setup to the LIVE engine input view
		// each frame: it is continuously updated by the mouse layer, so the
		// camera tracks the mouse smoothly (no freeze -- a per-tick snapshot
		// makes camera movement feel laggy/fighting) and never shows the fake.
		bool view_ok{};
		const auto live_view = systems::g_input.get_view_angles( &view_ok );
		if ( view_ok )
		{
			memory::write<math::vector3>( view_setup + 0x4b8, live_view );

			// Keep the pawn's eye angles true for the whole frame too, not just
			// at the end of create_move: prediction rewrites them from the faked
			// command right after our tick and the render thread reads them
			// between ticks (third-person orbit, viewmodel, radar). Without this
			// the model/animation layer still sees a fake<->real flicker.
			if ( const auto eye_offset = SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) )
			{
				memory::write<math::vector3>( local.pawn + eye_offset, live_view );
			}
		}
	}

	void misc::antiaim::on_render( xdraw::draw_list& draw_list ) const
	{
		if ( !settings::g_combat.m_antiaim.enabled.value || !settings::g_combat.m_antiaim.direction_indicator.value )
		{
			return;
		}

		if ( !this->m_antiaim_active )
		{
			return;
		}

		if ( !systems::g_frame_data.valid( ) )
		{
			return;
		}

		const auto origin = systems::g_frame_data.origin( );
		const auto aa_yaw_rad = this->m_indicator_yaw * ( std::numbers::pi_v<float> / 180.0f );

		constexpr auto radius{ 28.0f };
		constexpr auto feet_offset{ -2.0f };
		constexpr auto arc_sweep_deg{ 60.0f };
		constexpr auto arc_segments{ 48 };
		constexpr auto max_thickness{ 3.0f };

		const auto& cfg = settings::g_combat.m_antiaim;
		const auto& color = cfg.direction_indicator_color;
		const auto base = math::vector3{ origin.x, origin.y, origin.z + feet_offset };

		const auto half_sweep = ( arc_sweep_deg * 0.5f ) * ( std::numbers::pi_v<float> / 180.0f );
		const auto start_angle = aa_yaw_rad - half_sweep;
		const auto end_angle = aa_yaw_rad + half_sweep;
		const auto angle_step = ( end_angle - start_angle ) / static_cast< float >( arc_segments );

		std::vector<math::vector2> pts;
		pts.reserve( arc_segments + 1 );

		for ( auto i = 0; i <= arc_segments; ++i )
		{
			const auto angle = start_angle + angle_step * static_cast< float >( i );

			const auto world_pt = math::vector3
			{
				base.x + std::cosf( angle ) * radius,
				base.y + std::sinf( angle ) * radius,
				base.z
			};

			const auto sp = systems::g_view.project( world_pt );

			if ( !systems::g_view.projection_valid( sp ) )
			{
				return;
			}

			pts.push_back( { sp.x, sp.y } );
		}

		if ( pts.size( ) < 2 )
		{
			return;
		}

		const auto total = static_cast< float >( pts.size( ) - 1 );

		const auto fade_at = [ ]( std::size_t idx, float total ) -> float
			{
				const auto frac = static_cast< float >( idx ) / total;
				const auto edge = 1.0f - std::fabsf( frac - 0.5f ) * 2.0f;
				return edge * edge * edge * ( edge * ( edge * 6.0f - 15.0f ) + 10.0f );
			};

		if ( cfg.direction_indicator_glow )
		{
			auto& glow = xdraw::get_glow( );

			for ( auto i = 0ull; i + 1 < pts.size( ); ++i )
			{
				const auto f0 = fade_at( i, total );
				const auto f1 = fade_at( i + 1, total );

				const auto ga0 = static_cast< std::uint8_t >( static_cast< float >( color.value.a ) * cfg.direction_indicator_glow_strength * std::fmaxf( f0, 0.05f ) );
				const auto ga1 = static_cast< std::uint8_t >( static_cast< float >( color.value.a ) * cfg.direction_indicator_glow_strength * std::fmaxf( f1, 0.05f ) );

				const auto thickness = ( max_thickness + 2.0f ) * ( ( f0 + f1 ) * 0.5f * 0.85f + 0.15f );

				const float seg[ ]{ pts[ i ].x, pts[ i ].y, pts[ i + 1 ].x, pts[ i + 1 ].y };
				const xdraw::color cols[ ]{ { color.value.r, color.value.g, color.value.b, ga0 }, { color.value.r, color.value.g, color.value.b, ga1 } };

				glow.polyline_gradient( seg, cols, false, thickness );
			}
		}

		for ( auto i = 0ull; i + 1 < pts.size( ); ++i )
		{
			const auto f0 = fade_at( i, total );
			const auto f1 = fade_at( i + 1, total );

			const auto a0 = static_cast< std::uint8_t >( color.value.a * std::fmaxf( f0, 0.05f ) );
			const auto a1 = static_cast< std::uint8_t >( color.value.a * std::fmaxf( f1, 0.05f ) );

			const auto thickness = max_thickness * ( ( f0 + f1 ) * 0.5f * 0.85f + 0.15f );

			const float seg[ ]{ pts[ i ].x, pts[ i ].y, pts[ i + 1 ].x, pts[ i + 1 ].y };
			const xdraw::color cols[ ]{ { color.value.r, color.value.g, color.value.b, a0 }, { color.value.r, color.value.g, color.value.b, a1 } };

			draw_list.polyline_gradient( seg, cols, false, thickness );
		}
	}

	float misc::antiaim::get_pitch( float view_pitch )
	{
		if ( settings::g_combat.m_antiaim.yaw_style == settings::combat::antiaim::yaw_mode::legit_desync )
		{
			return view_pitch;
		}

		switch ( settings::g_combat.m_antiaim.pitch )
		{
		case settings::combat::antiaim::pitch_mode::down:
			return 89.0f;
		case settings::combat::antiaim::pitch_mode::up:
			return -89.0f;
		default:
			return view_pitch;
		}
	}

	float misc::antiaim::get_yaw( const math::vector3& view_angles, const systems::local::snapshot& local )
	{
		auto base_yaw_offset{ 180.0f };
		if ( settings::g_combat.m_antiaim.yaw_style == settings::combat::antiaim::yaw_mode::legit_desync )
		{
			base_yaw_offset = 0.0f;
		}

		const auto view_yaw = view_angles.y;
		auto base_yaw = view_yaw - base_yaw_offset;

		const auto local_game_scene_node = memory::read<std::uintptr_t>( local.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		const auto local_origin = memory::read<math::vector3>( local_game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
		const auto players = systems::g_entities.get_by_type( systems::entities::type::player );
		const auto eye_pos = local_origin + memory::read<math::vector3>( local.pawn + SCHEMA( "C_BaseModelEntity", "m_vecViewOffset"_hash ) );

		if ( settings::g_combat.m_antiaim.avoid_backstab.value )
		{
			constexpr auto backstab_range_sq = 350.0f * 350.0f;
			auto knife_dist = std::numeric_limits<float>::max( );
			auto knife_yaw{ 0.0f };
			auto knife_found{ false };

			for ( const auto& p : players )
			{
				if ( !p.ptr || p.ptr == local.controller )
				{
					continue;
				}

				if ( !memory::read<bool>( p.ptr + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) ) )
				{
					continue;
				}

				const auto pawn_handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
				const auto pawn = systems::g_entities.lookup( pawn_handle );

				if ( !pawn || pawn == local.pawn )
				{
					continue;
				}

				const auto team = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
				if ( !local.is_this_other_team( team ) )
				{
					continue;
				}

				const auto health = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
				if ( health <= 0 )
				{
					continue;
				}

				const auto enemy_game_scene_node = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
				if ( !enemy_game_scene_node )
				{
					continue;
				}

				const auto enemy_origin = memory::read<math::vector3>( enemy_game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
				const auto dx = enemy_origin.x - local_origin.x;
				const auto dy = enemy_origin.y - local_origin.y;
				const auto dist_sq = dx * dx + dy * dy;

				if ( dist_sq > backstab_range_sq )
				{
					continue;
				}

				const auto weapon_services = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
				if ( !weapon_services )
				{
					continue;
				}

				const auto weapon_handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				if ( !weapon_handle )
				{
					continue;
				}

				const auto weapon = systems::g_entities.lookup( weapon_handle );
				if ( !weapon )
				{
					continue;
				}

				const auto weapon_vdata = memory::read<std::uintptr_t>( weapon + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
				if ( !weapon_vdata )
				{
					continue;
				}

				const auto weapon_type = memory::read<std::uint32_t>( weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_WeaponType"_hash ) );
				if ( weapon_type != cstypes::weapon_type::knife )
				{
					continue;
				}

				if ( dist_sq < knife_dist )
				{
					knife_dist = dist_sq;
					knife_yaw = std::atan2f( dy, dx ) * ( 180.0f / std::numbers::pi_v<float> );
					knife_found = true;
				}
			}

			if ( knife_found )
			{
				this->m_indicator_yaw = knife_yaw;
				return knife_yaw;
			}
		}

		const auto pick_target_yaw = [ & ]( ) -> std::optional<float>
			{
				auto best_yaw = base_yaw;
				auto best_threat_score = std::numeric_limits<float>::max( );

				for ( const auto& p : players )
				{
					if ( !p.ptr || p.ptr == local.controller )
					{
						continue;
					}

					if ( !memory::read<bool>( p.ptr + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) ) )
					{
						continue;
					}

					const auto pawn_handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
					const auto pawn = systems::g_entities.lookup( pawn_handle );

					if ( !pawn || pawn == local.pawn )
					{
						continue;
					}

					const auto team = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
					if ( !local.is_this_other_team( team ) )
					{
						continue;
					}

					if ( memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) ) <= 0 )
					{
						continue;
					}

					if ( memory::read<bool>( pawn + SCHEMA( "C_CSPlayerPawn", "m_bGunGameImmunity"_hash ) ) )
					{
						continue;
					}

					const auto enemy_game_scene_node = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
					if ( !enemy_game_scene_node )
					{
						continue;
					}

					const auto enemy_origin = memory::read<math::vector3>( enemy_game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
					const auto enemy_eye_pos = enemy_origin + memory::read<math::vector3>( pawn + SCHEMA( "C_BaseModelEntity", "m_vecViewOffset"_hash ) );
					const auto angle_to_enemy = math::helpers::calculate_angle( eye_pos, enemy_eye_pos );
					const auto fov = math::helpers::angle_distance( view_angles, angle_to_enemy );
					const auto distance = eye_pos.distance( enemy_eye_pos );

					auto threat_score = fov * 4.0f + distance * 0.01f;

					math::vector3 enemy_forward{};
					const auto enemy_eye_angles = memory::read<math::vector3>( pawn + SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) );
					math::helpers::angle_vectors_left( enemy_eye_angles, &enemy_forward );
					const auto direction_to_us = ( eye_pos - enemy_eye_pos ).normalized( );
					threat_score -= std::clamp( enemy_forward.dot( direction_to_us ), -1.0f, 1.0f ) * 25.0f;

					if ( systems::g_tracing.is_visible( eye_pos, enemy_eye_pos, pawn, local.pawn ) )
					{
						threat_score -= 15.0f;
					}

					if ( threat_score < best_threat_score )
					{
						best_threat_score = threat_score;
						best_yaw = angle_to_enemy.y - base_yaw_offset;
					}
				}

				return best_threat_score < std::numeric_limits<float>::max( ) ? std::optional<float>{ best_yaw } : std::nullopt;
			};

		if ( const auto target_yaw = pick_target_yaw( ) )
		{
			base_yaw = *target_yaw;
		}

		// FIX: apply auto_yaw_adjust to base_yaw BEFORE side offsets so manual
		// left/right still land symmetrically at ±90° from the real back direction.
		if ( settings::g_combat.m_antiaim.auto_yaw_adjust.value )
			base_yaw += settings::g_combat.m_antiaim.yaw_offset.value;

		// lby break: override side offsets and style modulation with a clean
		// one-tick step to the pure back direction.
		if ( this->m_lby_break_now )
		{
			this->m_indicator_yaw = base_yaw;
			return base_yaw;
		}

		auto yaw = base_yaw;
		if ( this->m_yaw_side == -1 )
		{
			yaw -= 90.0f;
		}
		else if ( this->m_yaw_side == 1 )
		{
			yaw += 90.0f;
		}

		const auto& aa = settings::g_combat.m_antiaim;

		switch ( aa.yaw_style )
		{
		case settings::combat::antiaim::yaw_mode::desync:
			yaw += aa.desync_amount.value > 0.0f ? aa.desync_amount.value : 0.0f;
			break;
		case settings::combat::antiaim::yaw_mode::jitter:
			{
				const auto range = std::max( 0.0f, aa.jitter_range.value );
				const auto flip = ( ++this->m_jitter_flip ) % 2 == 0 ? 1.0f : -1.0f;
				yaw += flip * range;
			}
			break;
		case settings::combat::antiaim::yaw_mode::legit_desync:
			{
				const auto flip = ( ++this->m_jitter_flip ) % 2 == 0 ? 1.0f : -1.0f;
				yaw += flip * 26.0f;
			}
			break;
		case settings::combat::antiaim::yaw_mode::backward:
		default:
			break;
		}

		// FIX: sync indicator with the final computed yaw so it always matches
		// what we actually send. Previously indicator was computed without
		// auto_yaw_adjust, causing it to display the wrong direction.
		this->m_indicator_yaw = yaw;

		return yaw;
	}

	namespace {

		// Rotate the command's authored movement (forward/left base vectors, any
		// pre-written subtick analog steps and the button bits) from one yaw
		// basis to another. Shared by AA's correct_movement (which composes the
		// corrected movement under the REAL yaw then translates it into the
		// faked pose) and by the fakelag choke ticks (which re-stamp the command
		// angles to a held pose AFTER this tick's movement was already corrected
		// into that tick's own yaw - without the same re-rotation every choke
		// tick drifts the server-side movement basis, worst with jitter where
		// the held pose sits on the opposite side of the flip).
		void rotate_user_movement( proto::base_usercmd_pb* base, systems::input::usercmd* cmd, float from_yaw_deg, float to_yaw_deg )
		{
			if ( !base )
			{
				return;
			}

			const auto forward_move = base->forwardmove( );
			const auto side_move = base->leftmove( );

			if ( forward_move == 0.0f && side_move == 0.0f )
			{
				return;
			}

			// Wrap the delta into (-180, 180]: the yaw is periodic, and the raw
			// difference when the view sits on the +/-180 seam reads ~360 instead
			// of the short way around. Sub-degree deltas are near-identity.
			auto delta_deg = to_yaw_deg - from_yaw_deg;
			delta_deg = std::remainder( delta_deg, 360.0f );
			if ( std::fabsf( delta_deg ) < 1.0f )
			{
				return;
			}

			const float delta_yaw = delta_deg * ( std::numbers::pi_v<float> / 180.0f );
			const float cos_delta = std::cosf( delta_yaw );
			const float sin_delta = std::sinf( delta_yaw );

			// AG2 handedness: the input layer turns a positive leftmove into a
			// move along the player's LEFT vector, mirrored from Source' right
			// vector convention. Project the intended movement onto the new basis
			// with the matching sign so key directions survive the rotation.
			const float corrected_forward = cos_delta * forward_move + sin_delta * side_move;
			const float corrected_side = cos_delta * side_move - sin_delta * forward_move;

			// Preserve the authored magnitude instead of clamping each axis:
			// a diagonal (forward+side active) rotates to a near-cardinal and a
			// per-axis clamp then both distorts its direction and shortens it,
			// which makes strafing feel heavy and jittery.
			const auto corrected_magnitude = std::sqrtf( corrected_forward * corrected_forward + corrected_side * corrected_side );
			const auto magnitude_scale = corrected_magnitude > 1.001f ? 1.0f / corrected_magnitude : 1.0f;
			base->set_forwardmove( corrected_forward * magnitude_scale );
			base->set_leftmove( corrected_side * magnitude_scale );

			// Rotate any already-authored subtick analog steps by the same delta.
			// While moving, CS2 composes the real WASD/analog input as subtick
			// move steps in the ORIGINAL (real) view basis, and input::apply only
			// synthesizes a step when the command has none. The subtick deltas are
			// a cumulative timeline against the move baseline, so rotating every
			// step by the same matrix preserves the composed path under the new
			// command yaw.
			if ( const auto step_count = base->subtick_moves_size( ); step_count > 0 )
			{
				for ( auto i = 0; i < step_count; ++i )
				{
					auto* step = base->mutable_subtick_moves( i );
					if ( !step )
					{
						continue;
					}

					if ( !step->m_has_bits.test( 0x8 ) && !step->m_has_bits.test( 0x10 ) )
					{
						continue;
					}

					const auto analog_forward = step->analog_forward_delta( );
					const auto analog_left = step->analog_left_delta( );

					const float analog_corrected_forward = cos_delta * analog_forward + sin_delta * analog_left;
					const float analog_corrected_left = cos_delta * analog_left - sin_delta * analog_forward;

					const auto analog_magnitude = std::sqrtf( analog_corrected_forward * analog_corrected_forward + analog_corrected_left * analog_corrected_left );
					const auto analog_scale = analog_magnitude > 1.001f ? 1.0f / analog_magnitude : 1.0f;
					step->set_analog_forward_delta( analog_corrected_forward * analog_scale );
					step->set_analog_left_delta( analog_corrected_left * analog_scale );
				}
			}

			// The server derives its primary move vector from the button bits, so
			// the bits must be re-encoded to match the corrected direction.
			{
				auto buttons = cmd->buttons.value;
				buttons &= ~static_cast<std::uintptr_t>(
					cstypes::command_buttons::in_forward |
					cstypes::command_buttons::in_back |
					cstypes::command_buttons::in_moveleft |
					cstypes::command_buttons::in_moveright );

				if ( base->forwardmove( ) > 0.01f )
				{
					buttons |= cstypes::command_buttons::in_forward;
				}
				else if ( base->forwardmove( ) < -0.01f )
				{
					buttons |= cstypes::command_buttons::in_back;
				}

				if ( base->leftmove( ) > 0.01f )
				{
					buttons |= cstypes::command_buttons::in_moveleft;
				}
				else if ( base->leftmove( ) < -0.01f )
				{
					buttons |= cstypes::command_buttons::in_moveright;
				}

				cmd->buttons.value = buttons;
			}
		}

	} // namespace

	void misc::antiaim::correct_movement( systems::input::usercmd* cmd )
	{
		if ( !this->m_should_correct )
		{
			return;
		}

		this->m_should_correct = false;

		rotate_user_movement( cmd->csgo_user_cmd.mutable_base( ), cmd, this->m_old_angles.y, this->m_modified_angles.y );
	}

	bool misc::antiaim::is_near_ladder( std::uintptr_t local_pawn ) const
	{
		if ( !local_pawn )
			return false;

		const auto game_scene_node = memory::read<std::uintptr_t>(
			local_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !game_scene_node )
			return false;

		const auto origin = memory::read<math::vector3>(
			game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );

		// Trace a short horizontal feeler in 4 cardinal directions.
		// If any hits a ladder surface within 48 units we bail AA out.
		constexpr float k_reach = 48.0f;
		constexpr float k_half_h = 36.0f;

		const math::vector3 dirs[ 4 ] {
			{  k_reach, 0.0f, 0.0f },
			{ -k_reach, 0.0f, 0.0f },
			{ 0.0f,  k_reach, 0.0f },
			{ 0.0f, -k_reach, 0.0f },
		};

		const math::vector3 start { origin.x, origin.y, origin.z + k_half_h };

		for ( const auto& d : dirs )
		{
			const math::vector3 end { start.x + d.x, start.y + d.y, start.z };
			// 0x1c3043 = standard solid mask | CONTENTS_LADDER (0x40)
			const auto tr = systems::g_tracing.trace( start, end, local_pawn, 0x1c3043 );

			// CONTENTS_LADDER = 0x40 in CS2/Source2
			if ( tr.fraction < 0.95f && ( tr.contents & 0x40 ) )
				return true;
		}

		return false;
	}

	namespace {

		math::vector3 quickpeek_ground_snap( std::uintptr_t skip_pawn, const math::vector3& feet_pos )
		{
			const auto start = math::vector3{ feet_pos.x, feet_pos.y, feet_pos.z + 64.0f };
			const auto end = math::vector3{ feet_pos.x, feet_pos.y, feet_pos.z - 8192.0f };
			const auto tr = systems::g_tracing.trace( start, end, skip_pawn );

			if ( tr.fraction <= 0.0f || tr.fraction >= 0.997f )
			{
				return feet_pos;
			}

			auto out = tr.position;
			out.z += 1.0f;
			return out;
		}

	} // namespace

	void misc::duckpeek::on_create_move( systems::input::usercmd* cmd )
	{
		this->m_fake_stand_active = false;

		if ( !cmd || !settings::g_combat.m_duckpeek.enabled.value )
		{
			this->m_was_active = false;
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.is_alive || !local.pawn || systems::g_local.is_in_cinematic( ) || systems::g_local.is_in_time_freeze( ) )
		{
			this->m_was_active = false;
			return;
		}

		this->m_was_active = true;

		if ( g_rage.should_release_duck_for_shot( ) )
		{
			cmd->buttons.value &= ~cstypes::command_buttons::in_duck;
			this->m_fake_stand_active = true;
			return;
		}

		cmd->buttons.value |= cstypes::command_buttons::in_duck;

		if ( g_rage.duckpeek_wants_reduck( ) )
		{
			g_rage.clear_duckpeek_reduck( );
		}
	}

	void misc::duckpeek::on_override_view( std::uintptr_t view_setup )
	{
		(void)view_setup;

		if ( !settings::g_combat.m_duckpeek.enabled.value )
		{
			this->m_was_active = false;
			this->m_fake_stand_active = false;
		}
	}

	void misc::fakeduck::on_create_move( systems::input::usercmd* cmd )
	{
		this->m_was_active = false;

		if ( !cmd || !settings::g_combat.m_fakeduck.enabled.value )
		{
			this->m_ducking = false;
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.is_alive || !local.pawn || systems::g_local.is_in_cinematic( ) || systems::g_local.is_in_time_freeze( ) )
		{
			this->m_ducking = false;
			return;
		}

		const auto base = cmd->csgo_user_cmd.mutable_base( );
		if ( !base )
		{
			return;
		}

		this->m_was_active = true;

		// CT/hold: release duck while the ragebot wants to shoot so the shot
		// comes from a standing hitbox, then re-hold on the next tick.
		if ( g_rage.should_release_duck_for_shot( ) && settings::g_combat.m_fakeduck.hide_shots.value )
		{
			cmd->buttons.value &= ~cstypes::command_buttons::in_duck;
			this->m_ducking = false;
			return;
		}

		auto& buttons = cmd->buttons.value;
		buttons |= cstypes::command_buttons::in_duck;

		if ( ( systems::g_prediction.pre( ).flags & cstypes::entity_flags::on_ground ) == 0 )
		{
			this->m_ducking = false;
			return;
		}

		const auto subtick_moves = base->mutable_subtick_moves( );

		// Alternate a duck-down at the start of the tick and a duck-up shortly
		// after, so the engine re-evaluates crouch each tick. This fakes a
		// sustained short-crouch that desyncs the lower body from the view.
		this->m_ducking = !this->m_ducking;

		constexpr auto k_press_frac{ 0.0f };
		constexpr auto k_release_frac{ 2.0f / 64.0f };

		if ( this->m_ducking )
		{
			if ( const auto duck_down = systems::g_input.acquire_subtick_step( subtick_moves ) )
			{
				duck_down->set_button( cstypes::command_buttons::in_duck );
				duck_down->set_pressed( true );
				duck_down->set_when( k_press_frac );
				duck_down->set_analog_forward_delta( 0.0f );
				duck_down->set_analog_left_delta( 0.0f );
			}
		}
		else
		{
			if ( const auto duck_up = systems::g_input.acquire_subtick_step( subtick_moves ) )
			{
				duck_up->set_button( cstypes::command_buttons::in_duck );
				duck_up->set_pressed( false );
				duck_up->set_when( k_release_frac );
				duck_up->set_analog_forward_delta( 0.0f );
				duck_up->set_analog_left_delta( 0.0f );
			}
		}
	}

	void misc::quickpeek::on_create_move( systems::input::usercmd* cmd )
	{
		if ( !settings::g_combat.m_quickpeek.enabled.value )
		{
			this->reset( );
			return;
		}

		const auto local = systems::g_local.get( );
		const auto& ctx = g_shared.ctx( );

		if ( ctx.weapon_type < cstypes::weapon_type::pistol || ctx.weapon_type > cstypes::weapon_type::lmg )
		{
			this->reset( );
			return;
		}

		const auto base = cmd->csgo_user_cmd.mutable_base( );
		constexpr auto movement_cancel_mask = static_cast< std::uintptr_t >( cstypes::command_buttons::in_forward | cstypes::command_buttons::in_back | cstypes::command_buttons::in_moveleft | cstypes::command_buttons::in_moveright );
		const auto curr_movement_bits = cmd->buttons.value & movement_cancel_mask;

		const auto game_scene_node = memory::read<std::uintptr_t>( local.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		const auto origin = memory::read<math::vector3>( game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );

		if ( this->m_saved_origin.length_sqr( ) < 0.001f )
		{
			this->m_saved_origin = quickpeek_ground_snap( local.pawn, origin );
			this->m_should_retrack = false;
			this->m_fired = false;
			this->m_active = true;
			this->create_particle( );
			this->m_prev_movement_bits = curr_movement_bits;
			return;
		}

		this->update_particle( );

		const auto distance = ( origin - this->m_saved_origin ).length_2d( );

		if ( this->m_should_retrack && ( curr_movement_bits & ~this->m_prev_movement_bits ) != 0 )
		{
			this->m_should_retrack = false;
		}

		if ( this->m_should_retrack && ( systems::g_prediction.pre( ).flags & cstypes::entity_flags::on_ground ) )
		{
			const auto velocity = memory::read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto speed = velocity.length_2d( );

			if ( distance < 5.0f && speed < 15.0f )
			{
				this->m_should_retrack = false;
				this->m_fired = false;
			}
			else if ( distance < speed * 0.1f && speed > 15.0f )
			{
				const auto vel_angle = math::helpers::vector_to_angle( velocity * -1.0f );
				const auto yaw_diff = math::helpers::deg_to_rad( base->viewangles( )->y( ) - vel_angle.y );

				base->set_forwardmove( std::cosf( yaw_diff ) );
				base->set_leftmove( -std::sinf( yaw_diff ) );

				auto buttons = cmd->buttons.value;
				buttons &= ~static_cast< std::uintptr_t >( cstypes::command_buttons::in_forward | cstypes::command_buttons::in_back | cstypes::command_buttons::in_moveleft | cstypes::command_buttons::in_moveright );

				if ( base->forwardmove( ) > 0.0f )
				{
					buttons |= cstypes::command_buttons::in_forward;
				}
				else if ( base->forwardmove( ) < 0.0f )
				{
					buttons |= cstypes::command_buttons::in_back;
				}

				if ( base->leftmove( ) > 0.0f )
				{
					buttons |= cstypes::command_buttons::in_moveleft;
				}
				else if ( base->leftmove( ) < 0.0f )
				{
					buttons |= cstypes::command_buttons::in_moveright;
				}

				cmd->buttons.value = buttons;
			}
			else
			{
				const auto diff = this->m_saved_origin - origin;
				const auto angle_to_pos = math::helpers::vector_to_angle( diff );
				const auto yaw_diff = math::helpers::deg_to_rad( base->viewangles( )->y( ) - angle_to_pos.y );

				base->set_forwardmove( std::cosf( yaw_diff ) );
				base->set_leftmove( -std::sinf( yaw_diff ) );

				auto buttons = cmd->buttons.value;
				buttons &= ~static_cast< std::uintptr_t >( cstypes::command_buttons::in_forward | cstypes::command_buttons::in_back | cstypes::command_buttons::in_moveleft | cstypes::command_buttons::in_moveright );

				if ( base->forwardmove( ) > 0.0f )
				{
					buttons |= cstypes::command_buttons::in_forward;
				}
				else if ( base->forwardmove( ) < 0.0f )
				{
					buttons |= cstypes::command_buttons::in_back;
				}

				if ( base->leftmove( ) > 0.0f )
				{
					buttons |= cstypes::command_buttons::in_moveleft;
				}
				else if ( base->leftmove( ) < 0.0f )
				{
					buttons |= cstypes::command_buttons::in_moveright;
				}

				cmd->buttons.value = buttons;
			}
		}

		if ( ( cmd->buttons.value & cstypes::command_buttons::in_attack ) && !g_rage.is_cocking_revolver( ) )
		{
			this->m_should_retrack = true;
			this->m_fired = true;
		}

		this->m_prev_movement_bits = curr_movement_bits;
	}

	void misc::quickpeek::reset_if_needed( )
	{
		if ( !this->m_active )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.is_alive || !local.pawn )
		{
			this->reset( );
			return;
		}

		if ( !settings::g_combat.m_quickpeek.enabled.value )
		{
			this->reset( );
		}
	}

	void misc::quickpeek::create_particle( )
	{
		const auto particle_manager = memory::read<std::uintptr_t>( addresses::globals::particle_manager );
		if ( !particle_manager )
		{
			return;
		}

		constexpr auto particle_path{ "particles/embedded/halo.vpcf" };

		if ( !this->m_particle_loaded )
		{
			struct buffer_string
			{
				std::uint32_t m_unknown1{};
				std::uint32_t m_unknown2{ 0xc00000c8 };

				union
				{
					std::uintptr_t m_str_ptr;
					std::uint8_t data[ 0xc8 ];
				};

				std::uintptr_t m_unknown3{ 0 };
				std::uintptr_t m_unknown4{ 0 };
			} buffer;

			memory::call<void>(PATTERN (patterns::init_particle_path_buffer), &buffer, particle_path );
			buffer.m_unknown4 = 'fcpv';
			memory::call<void>(PATTERN (patterns::resource_system_precache), addresses::globals::resource_system, &buffer, "" );

			this->m_particle_loaded = true;
		}

		auto effect_index{ invalid_effect_index };
		memory::call<int*>(PATTERN (patterns::particle_create_effect), particle_manager, &effect_index, particle_path, 8, 0ll, 0ll, 0ll, 0 );

		this->m_particle_effect = effect_index;

		if ( effect_index == invalid_effect_index )
		{
			return;
		}

		memory::call<bool>(PATTERN (patterns::particle_set_control_point), particle_manager, effect_index, 0, &this->m_saved_origin, 0 );
	}

	void misc::quickpeek::update_particle( )
	{
		if ( this->m_particle_effect == invalid_effect_index )
		{
			return;
		}

		const auto particle_manager = memory::read<std::uintptr_t>( addresses::globals::particle_manager );
		if ( !particle_manager )
		{
			return;
		}

		const auto& cfg = settings::g_combat.m_quickpeek;
		const auto& col = this->m_should_retrack ? cfg.retrack_color : cfg.color;
		const auto color = math::vector3{ static_cast< float >( col.value.r ), static_cast< float >( col.value.g ), static_cast< float >( col.value.b ) };

		memory::call<bool>(PATTERN (patterns::particle_set_control_point), particle_manager, this->m_particle_effect, 1, &color, 0 );
		memory::call<bool>(PATTERN (patterns::particle_set_control_point), particle_manager, this->m_particle_effect, 0, &this->m_saved_origin, 0 );
	}

	void misc::quickpeek::release_particle( )
	{
		if ( this->m_particle_effect == invalid_effect_index )
		{
			return;
		}

		const auto particle_manager = memory::read<std::uintptr_t>( addresses::globals::particle_manager );
		if ( particle_manager )
		{
			memory::call<void>(PATTERN (patterns::particle_destroy_effect), particle_manager, this->m_particle_effect, true, true );
		}

		this->m_particle_effect = invalid_effect_index;
	}

	void misc::quickpeek::reset( )
	{
		this->release_particle( );
		this->m_saved_origin = {};
		this->m_should_retrack = false;
		this->m_fired = false;
		this->m_active = false;
		this->m_prev_movement_bits = 0;
	}

	void misc::autostop::on_create_move( systems::input::usercmd* cmd )
	{
		if ( !features::combat::g_rage.should_stop( ) )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		const auto movement_services = memory::read<std::uintptr_t>( local.pawn + SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash ) );
		const auto base = cmd->csgo_user_cmd.mutable_base( );
		const auto& prestate = systems::g_prediction.pre( );
		const auto& ctx = g_shared.ctx( );

		if ( !( prestate.flags & cstypes::entity_flags::on_ground ) )
		{
			return;
		}

		auto velocity = prestate.networked_velocity;
		auto speed = velocity.length_2d( );

		if ( speed <= 1.0f )
		{
			return;
		}

		const auto cvar_friction = CONVAR ("sv_friction");
		const auto cvar_stopspeed = CONVAR ("sv_stopspeed");
		const auto sv_friction = cvar_friction ? cvar_friction->get<float>( ) : 5.2f;
		const auto sv_stopspeed = cvar_stopspeed ? cvar_stopspeed->get<float>( ) : 80.0f;
		const auto surface_friction = prestate.surface_friction;

		const auto control = std::fmaxf( speed, sv_stopspeed );
		const auto drop = control * sv_friction * surface_friction * cstypes::tick_interval;
		const auto post_friction = std::fmaxf( speed - drop, 0.0f );

		if ( post_friction > 0.0f )
		{
			velocity *= ( post_friction / speed );
			speed = post_friction;
		}
		else
		{
			base->set_forwardmove( 0.0f );
			base->set_leftmove( 0.0f );
			return;
		}

		if ( speed < 2.0f )
		{
			base->set_forwardmove( 0.0f );
			base->set_leftmove( 0.0f );
			return;
		}

		const auto cvar_accel = CONVAR ("sv_accelerate");
		auto accel = cvar_accel ? cvar_accel->get<float>( ) : 5.5f;
		const auto accel_base = this->get_effective_accel_base( local.pawn, movement_services, prestate.flags, ctx.weapon_max_speed );

		if ( ctx.is_scoped )
		{
			const auto weapon_ratio = std::fminf( 1.0f, ctx.weapon_max_speed / 250.0f );
			const auto v20 = std::fmaxf( 250.0f, memory::read<float>( movement_services + SCHEMA( "CPlayer_MovementServices", "m_flMaxspeed"_hash ) ) ) * weapon_ratio;
			const auto scoped_max = v20 * 0.52f;

			if ( speed > scoped_max - 5.0f )
			{
				const auto t = 1.0f - std::fmaxf( 0.0f, speed - ( scoped_max - 5.0f ) ) / std::fmaxf( 0.01f, 5.0f );
				accel *= std::clamp( t, 0.0f, 1.0f );
			}
		}

		const auto wish_x = -velocity.x / speed;
		const auto wish_y = -velocity.y / speed;
		const auto accel_speed = std::fminf( accel * accel_base * surface_friction * cstypes::tick_interval, speed );

		velocity.x += wish_x * accel_speed;
		velocity.y += wish_y * accel_speed;

		const auto move_magnitude = std::clamp( speed / ctx.weapon_max_speed, 0.0f, 1.0f );
		const auto yaw_rad = base->viewangles( )->y( ) * ( std::numbers::pi_v<float> / 180.0f );
		const auto sy = std::sinf( yaw_rad );
		const auto cy = std::cosf( yaw_rad );

		const auto forward_move = std::clamp( ( wish_x * cy + wish_y * sy ) * move_magnitude, -1.0f, 1.0f );
		const auto left_move = std::clamp( ( wish_x * sy - wish_y * cy ) * -move_magnitude, -1.0f, 1.0f );

		base->set_forwardmove( forward_move );
		base->set_leftmove( left_move );

		const auto subtick_moves = base->mutable_subtick_moves( );
		if ( subtick_moves )
		{
			const auto step = systems::g_input.acquire_subtick_step( subtick_moves );
			if ( step )
			{
				step->set_button( 0 );
				step->set_pressed( false );
				step->set_when( 0.0f );
				step->set_analog_forward_delta( forward_move - prestate.last_movement_impulses.x );
				step->set_analog_left_delta( left_move - prestate.last_movement_impulses.y );
			}
		}

		if ( forward_move > 0.0f )
		{
			cmd->buttons.value |= cstypes::command_buttons::in_forward;
		}
		else if ( forward_move < 0.0f )
		{
			cmd->buttons.value |= cstypes::command_buttons::in_back;
		}

		if ( left_move > 0.0f )
		{
			cmd->buttons.value |= cstypes::command_buttons::in_moveleft;
		}
		else if ( left_move < 0.0f )
		{
			cmd->buttons.value |= cstypes::command_buttons::in_moveright;
		}
	}

	float misc::autostop::get_effective_accel_base( std::uintptr_t local_pawn, std::uintptr_t movement_services, std::uint32_t flags, float max_weapon_speed ) const
	{
		const auto max_speed_base = memory::read<float>( movement_services + SCHEMA( "CPlayer_MovementServices", "m_flMaxspeed"_hash ) );
		const auto is_ducked = ( flags & 4 ) != 0;
		const auto ducking_state = memory::read<bool>( movement_services + SCHEMA( "CPlayer_MovementServices", "m_bDucking"_hash ) );
		const auto is_scoped = g_shared.ctx( ).is_scoped;
		const auto is_ducking = is_ducked || ducking_state;
		const auto v19 = std::fmaxf( 250.0f, max_speed_base );

		auto friction_scale{ 1.0f };

		const auto cvar_use_weapon_speed = CONVAR ("sv_accelerate_use_weapon_speed");
		if ( cvar_use_weapon_speed && cvar_use_weapon_speed->get<bool>( ) )
		{
			const auto weapon_ratio = std::fminf( 1.0f, max_weapon_speed / 250.0f );

			if ( !is_ducking && !is_scoped )
			{
				friction_scale = weapon_ratio;
			}
		}

		if ( is_ducking )
		{
			friction_scale = std::fminf( 0.34f, friction_scale );
		}

		auto accel_base = v19 * friction_scale;

		if ( is_scoped && !is_ducking )
		{
			accel_base *= 0.52f;
		}

		return accel_base;
	}

	int misc::fakelag::resolve_amount( const math::vector3& velocity ) const
	{
		const auto& cfg = settings::g_combat.m_fakelag;

		if ( !cfg.adaptive.value )
		{
			return std::clamp( cfg.amount.value, 1, 14 );
		}

		// The faster you move, the less you choke: at top speed natural
		// interpolation already hides the gaps, standing still you can
		// afford maximum choke without looking fully frozen.
		const auto speed = std::min( velocity.length_2d( ), 250.0f );
		const auto factor = math::helpers::inverse_lerp( 0.0f, 250.0f, speed );
		const auto amount = math::helpers::lerp(
			static_cast< float >( cfg.adaptive_max.value ),
			static_cast< float >( cfg.adaptive_min.value ),
			factor );

		return std::clamp( static_cast< int >( std::round( amount ) ), 1, 14 );
	}

	void misc::fakelag::on_create_move( systems::input::usercmd* cmd )
	{
		const auto& cfg = settings::g_combat.m_fakelag;

		if ( !cfg.enabled.value )
		{
			this->m_hold_counter = 0;
			this->m_has_held_angles = false;
			return;
		}

		const auto base = cmd->csgo_user_cmd.mutable_base( );
		if ( !base )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.pawn || !local.is_alive )
		{
			this->m_hold_counter = 0;
			this->m_has_held_angles = false;
			return;
		}

		// Never interrupt a shot: rage silent aim writes real aim angles, holding
		// here would break the sequence. Buttons alone miss every subtick shot -
		// doubletap and subtick attacks clear in_attack and ride on subtick
		// steps, so a shot can be in flight with the attack bit long gone;
		// stomping the angle then would swing the barrel away mid-sequences.
		if ( cfg.hide_on_shoot.value && ( ( cmd->buttons.value & cstypes::command_buttons::in_attack ) || features::combat::g_rage.is_firing_this_tick( ) ) )
		{
			this->m_hold_counter = 0;
			this->m_has_held_angles = false;
			return;
		}

		const auto& prestate = systems::g_prediction.pre( );
		const auto speed = prestate.networked_velocity.length_2d( );
		const auto on_ground = ( prestate.flags & cstypes::entity_flags::on_ground ) != 0;

		auto should_lag = true;
		if ( !cfg.lag_on_peek.value )
		{
			switch ( cfg.mode.value )
			{
			case settings::combat::fakelag::mode::while_standing:
				should_lag = on_ground && speed < cfg.moving_speed_threshold.value;
				break;
			case settings::combat::fakelag::mode::while_moving:
				should_lag = speed >= cfg.moving_speed_threshold.value;
				break;
			case settings::combat::fakelag::mode::while_in_air:
				should_lag = !on_ground;
				break;
			case settings::combat::fakelag::mode::always:
			default:
				break;
			}
		}

		if ( !should_lag )
		{
			this->m_hold_counter = 0;
			this->m_has_held_angles = false;
			return;
		}

		const auto amount = this->resolve_amount( prestate.networked_velocity );

		if ( this->m_hold_counter < amount )
		{
			// Choke tick: freeze the angle that was last released so the
			// server keeps seeing a stale pose for `amount` ticks.
			if ( this->m_has_held_angles )
			{
				const auto cur_yaw = base->viewangles( ) ? base->viewangles( )->y( ) : this->m_held_angles.y;

				const auto angles = base->mutable_viewangles( );
				angles->set_x( this->m_held_angles.x );
				angles->set_y( this->m_held_angles.y );
				angles->set_z( this->m_held_angles.z );

				// This tick's movement was already rotated (by AA into the fake
				// pose, or by the legit input composer into the real view) for
				// the CURRENT yaw. Re-stamping the frozen held pose changes the
				// transmitted yaw a second time, so rotate the movement onward
				// into the held basis - otherwise every choke tick integrates
				// the movement against the wrong yaw, which reads as direction
				// drift on the receiving server (worst with jitter, where the
				// held pose sits on the opposite side of the flip).
				rotate_user_movement( base, cmd, cur_yaw, this->m_held_angles.y );
			}

			++this->m_hold_counter;
		}
		else
		{
			// Release tick: snapshot the current (post AA/legit) angle and
			// let it flow through once.
			const auto angles = base->viewangles( );
			this->m_held_angles = { angles ? angles->x( ) : 0.0f, angles ? angles->y( ) : 0.0f, angles ? angles->z( ) : 0.0f };
			this->m_has_held_angles = true;
			this->m_hold_counter = 0;
		}
	}

} // namespace features::combat
