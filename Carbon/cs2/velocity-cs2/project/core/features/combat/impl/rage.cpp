#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/logging/logging.hpp>
#include <utilities/threadpool/threadpool.hpp>
#include <core/systems/systems.hpp>
#include <core/features/features.hpp>
#include <core/rendering/rendering.hpp>
#include <protection/game_addresses.hpp>

namespace features::combat {

	void rage::on_create_move( systems::input::usercmd* cmd )
	{
		// Unsafe-mode lock: while unsafe mode is off the rage section is force-
		// disabled. Flush any in-flight fire state on the frame the lock is
		// (re)engaged so a partial rage shot can never stay pending, then no-op
		// the entire aim/fire path (no aim, no shots emitted through rage).
		if ( !settings::g_cheat.unsafe_mode.value )
		{
			this->m_firing_this_tick = false;
			this->m_should_stop = false;
			this->m_revolver_cock_ticks = 0;
			return;
		}

		auto& ctx = g_shared.ctx( );
		const auto local = systems::g_local.get( );
		this->update_penetration_crosshair( local );

		if ( !ctx.valid )
		{
			this->m_revolver_cock_ticks = 0;
			return;
		}

		this->m_should_stop = false;
		this->m_firing_this_tick = false;

		if ( !settings::g_combat.m_duckpeek.enabled.value )
		{
			this->m_release_duck_for_shot = false;
			this->m_duckpeek_reduck = false;
		}

		if ( this->m_zeus_fired )
		{
			this->m_zeus_fired = false;

			if ( settings::g_combat.m_zeusbot.drop_after && !systems::g_local.is_in_deathmatch( ) )
			{
				memory::call<void>(PATTERN (patterns::engine_client_cmd), addresses::globals::source2engine_to_client, 0, "drop", 0x7ffef001 );
			}

			return;
		}

		const auto is_knife = ctx.weapon_type == cstypes::weapon_type::knife;
		const auto is_taser = ctx.weapon_type == cstypes::weapon_type::taser;

		if ( !is_knife && !is_taser && ( ctx.weapon_type < cstypes::weapon_type::pistol || ctx.weapon_type > cstypes::weapon_type::lmg ) )
		{
			return;
		}


		if ( is_knife )
		{
			if ( !g_shared.can_shoot( cmd, local.controller ) )
			{
				return;
			}

			const auto aim_ctx = this->build_context( cmd, local );
			this->run_knife( cmd, aim_ctx, local );
		}
		else if ( is_taser )
		{
			if ( !g_shared.can_shoot( cmd, local.controller ) )
			{
				return;
			}

			const auto aim_ctx = this->build_context( cmd, local );
			this->run_taser( cmd, aim_ctx, local );
		}
		else if ( ctx.item_def_idx == cstypes::item_definition_index::weapon_r8_revolver )
		{
			const auto aim_ctx = this->build_context( cmd, local );
			this->auto_revolver( cmd, aim_ctx, local );
		}
		else
		{
			this->m_revolver_cock_ticks = 0;

			// Cheap pass first. The movement prediction inside build_context is
			// the single heaviest per-tick cost, so skip it entirely when there
			// is nothing scannable.
			auto candidates = this->gather_candidates( local );
			if ( candidates.empty( ) )
			{
				const auto& config = settings::g_combat.m_ragebot.get_group( ctx.weapon_type );
				if ( config.doubletap.value )
				{
					( void ) this->process_doubletap( cmd, local, false, false, false );
				}
				return;
			}

			const auto can_shoot = g_shared.can_shoot( cmd, local.controller );
			const auto aim_ctx = this->build_context( cmd, local );
			this->run_gun( cmd, aim_ctx, local, can_shoot, &candidates );
		}
	}

	void rage::on_render( xdraw::draw_list& draw_list )
	{
	this->draw_penetration_crosshair( draw_list );

	const auto& config = settings::g_combat.m_ragebot.get_group( g_shared.ctx( ).weapon_type );
	if ( !config.debug_multipoints.value )
	{
		return;
	}

		std::lock_guard lock( m_debug_mtx );

		for ( const auto& pt : m_debug_points )
		{
			const auto screen = systems::g_view.project( pt.position );
			if ( !systems::g_view.projection_valid( screen ) )
			{
				continue;
			}

			xdraw::color col{};
			switch ( pt.hitbox_index )
			{
			case 0:
				col = { 255, 80,  80  }; break; // head Ã¢â‚¬â€ red
			case 2: case 3:
				col = { 220, 220, 60  }; break; // stomach Ã¢â‚¬â€ yellow
			case 4: case 5: case 6:
				col = { 255, 160, 60  }; break; // chest Ã¢â‚¬â€ orange
			case 7: case 8: case 9: case 10: case 11: case 12:
				col = { 80,  160, 255 }; break; // legs Ã¢â‚¬â€ blue
			case 13: case 14: case 15: case 16: case 17: case 18:
				col = { 180, 80,  255 }; break; // arms Ã¢â‚¬â€ purple
			default:
				col = { 200, 200, 200 }; break;
			}

			const auto alpha  = pt.is_center ? std::uint8_t{ 255 } : std::uint8_t{ 160 };
			const auto radius = pt.is_center ? 3.5f : 2.0f;

			draw_list.circle_filled( screen.x, screen.y, radius, col.alpha( alpha ) );
		}
	}

	rage::aim_context rage::build_context( systems::input::usercmd* cmd, const systems::local::snapshot& local ) const
	{
		auto& ctx = g_shared.ctx( );
		const auto& prestate = systems::g_prediction.pre( );

		aim_context out{};
		out.velocity = prestate.velocity;
		out.spread = g_shared.get_spread( );
		out.predicted_inaccuracy = g_shared.get_inaccuracy( true );

		systems::g_prediction.simulate( cmd, local, [ & ]
			{
				g_shared.sh( ).snapshot( local.pawn, ctx.weapon_services );

				out.velocity = memory::read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
				out.spread = g_shared.get_spread( );
				out.predicted_inaccuracy = g_shared.get_inaccuracy( true );
			} );

		ctx.spread = out.spread;
		ctx.inaccuracy = out.predicted_inaccuracy;

		out.view_angles = systems::g_input.get_view_angles( );
		out.on_ground = ( prestate.flags & cstypes::entity_flags::on_ground ) != 0;
		out.is_scoped = ctx.is_scoped;

		// Airborne shots: floor the engine inaccuracy with the analytic jump
		// inaccuracy so hitchance is never optimistic while off the ground
		// (an under-estimated inaccuracy is what makes air shots miss).
		if ( !out.on_ground && ctx.weapon_vdata )
		{
			const auto jump_initial = memory::read<float>( ctx.weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flInaccuracyJumpInitial"_hash ) );
			const auto jump_apex = memory::read<float>( ctx.weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flInaccuracyJumpApex"_hash ) );
			out.predicted_inaccuracy = std::max( out.predicted_inaccuracy,
				g_shared.get_air_inaccuracy( out.velocity.z, jump_initial, jump_apex ) );
			ctx.inaccuracy = out.predicted_inaccuracy;
		}
		out.weapon_max_speed = ctx.weapon_max_speed;
		out.accurate_threshold = ctx.weapon_max_speed * 0.34f;

		return out;
	}

	void rage::clear_attack( systems::input::usercmd* cmd )
	{
		// The subtick release is the final state. Do not let the base command
		// turn any leftover pulse into a held attack.
		cmd->buttons.value &= ~cstypes::command_buttons::in_attack;
		cmd->buttons.value_changed |= cstypes::command_buttons::in_attack;
		cmd->buttons.value_scroll &= ~cstypes::command_buttons::in_attack;
		cmd->csgo_user_cmd.set_attack1_start_history_index( -1 );
	}

	rage::doubletap_state rage::process_doubletap( systems::input::usercmd* cmd, const systems::local::snapshot& local, bool fire_requested, bool gate_on_lethal, bool lethal )
	{
		const auto& shared_ctx = g_shared.ctx( );
		const auto& config = settings::g_combat.m_ragebot.get_group( shared_ctx.weapon_type );
		if ( !config.doubletap.value )
		{
			return doubletap_state::passthrough;
		}

		// Just sweeping/clearing (no fire requested this tick, e.g. empty scan
		// results): eat the manual attack so dt owns the trigger, but never
		// emit a pulse without a shot to attach it to.
		if ( !fire_requested )
		{
			this->clear_attack( cmd );
			return doubletap_state::passthrough;
		}

		const auto base_cmd = cmd->csgo_user_cmd.mutable_base( );
		if ( !base_cmd )
		{
			return doubletap_state::passthrough;
		}

		const auto client_tick = base_cmd->client_tick( );
		const auto next_primary = memory::read<int>( shared_ctx.weapon + SCHEMA( "C_BasePlayerWeapon", "m_nNextPrimaryAttackTick"_hash ) );
		const auto tick_base = memory::read<int>( local.controller + SCHEMA( "CBasePlayerController", "m_nTickBase"_hash ) );

		// Cycle gate between bursts: once a shot has been emitted, withhold the
		// next burst until the weapon's per-shot cycle has elapsed. This stops
		// burst-per-tick spam when next-attack reads go stale.
		const auto last_shoot = g_shared.last_shoot_tick( );
		const auto cooldown_ticks = std::max( 2, static_cast< int >( std::ceil( g_shared.cycle_time( ) / cstypes::tick_interval ) ) );
		const auto cycle_ready = last_shoot == 0 || ( tick_base - last_shoot ) >= cooldown_ticks;

		const auto can_attack = cycle_ready &&
			( client_tick >= next_primary || tick_base >= next_primary || next_primary <= 0 ) &&
			!memory::read<bool>( shared_ctx.weapon + SCHEMA( "C_CSWeaponBase", "m_bInReload"_hash ) ) &&
			memory::read<int>( shared_ctx.weapon + SCHEMA( "C_BasePlayerWeapon", "m_iClip1"_hash ) ) > 0;

		// Lethal-charge hold: with doubletap_lethal the trigger is gated until
		// the queued shot becomes lethal. The caller keeps the aim locked on
		// the target and the burst goes out the tick the kill flips.
		if ( gate_on_lethal && !lethal )
		{
			this->clear_attack( cmd );
			return doubletap_state::hold;
		}

		if ( !can_attack )
		{
			// Weapon still cooling down: nothing to suppress, normal engine
			// gating will simply ignore this tick's press.
			return doubletap_state::passthrough;
		}

		const auto emit_pulse = [ & ]( float press_when, float release_when ) -> bool
		{
			const auto press = systems::g_input.acquire_subtick_step( base_cmd->mutable_subtick_moves( ) );
			const auto release = systems::g_input.acquire_subtick_step( base_cmd->mutable_subtick_moves( ) );
			if ( !press || !release )
			{
				return false;
			}

			press->set_button( cstypes::command_buttons::in_attack );
			press->set_pressed( true );
			press->set_when( press_when );
			press->set_analog_forward_delta( 0.0f );
			press->set_analog_left_delta( 0.0f );

			release->set_button( cstypes::command_buttons::in_attack );
			release->set_pressed( false );
			release->set_when( release_when );
			release->set_analog_forward_delta( 0.0f );
			release->set_analog_left_delta( 0.0f );

			return true;
		};

		// Two press/release pulses inside the same command gives the server two
		// discrete pulls: the burst. Weapons whose per-shot cycle fits a tick
		// land both bullets; slower ones consume the first and ignore the
		// second (harmless), then readmit on the next tick. The second press
		// sits on the late half of the tick so it resolves after the first
		// pull's cooldown begins.
		const auto subtick_moves = base_cmd->mutable_subtick_moves( );
		const auto old_size = subtick_moves ? subtick_moves->m_current_size : 0;

		if ( !emit_pulse( 0.0f, 0.30f ) )
		{
			if ( subtick_moves )
			{
				subtick_moves->m_current_size = old_size;
			}
		}
		else if ( !emit_pulse( 0.62f, std::nextafter( 1.0f, 0.0f ) ) )
		{
			// Keep the first pulse; drop only the partial second one.
			if ( subtick_moves )
			{
				subtick_moves->m_current_size = old_size + 2;
			}
		}

		this->clear_attack( cmd );
		return doubletap_state::burst;
	}

	std::optional<rage::stop_prediction> rage::predict_stop( const aim_context& ctx, const math::vector3& current_eye, const systems::local::snapshot& local ) const
	{
		const auto& shared_ctx = g_shared.ctx( );
		const auto& config = settings::g_combat.m_ragebot.get_group( shared_ctx.weapon_type );
		const auto& prestate = systems::g_prediction.pre( );
		const auto speed = prestate.networked_velocity.length_2d( );
		const auto is_sniper_auto_scope = shared_ctx.weapon_type == cstypes::weapon_type::sniper && config.auto_scope.value;
		const auto will_stop = ctx.on_ground && ( speed > ctx.accurate_threshold || ( ( ctx.is_scoped || is_sniper_auto_scope ) && speed > 1.0f ) );

		if ( !will_stop )
		{
			return std::nullopt;
		}

		auto sim_vel = prestate.networked_velocity;
		sim_vel.z = 0.0f;

		const auto sv_friction_cvar = CONVAR("sv_friction");
		const auto sv_stopspeed_cvar = CONVAR("sv_stopspeed");
		const auto sv_accelerate_cvar = CONVAR("sv_accelerate");

		const auto sv_friction = sv_friction_cvar ? sv_friction_cvar->get<float>( ) : 5.2f;
		const auto sv_stopspeed = sv_stopspeed_cvar ? sv_stopspeed_cvar->get<float>( ) : 80.0f;
		const auto sv_accelerate = sv_accelerate_cvar ? sv_accelerate_cvar->get<float>( ) : 5.5f;
		const auto surface_friction = prestate.surface_friction;

		const auto movement_services = memory::read<std::uintptr_t>( local.pawn + SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash ) );
		const auto max_move_speed = movement_services ? memory::read<float>( movement_services + SCHEMA( "CPlayer_MovementServices", "m_flMaxspeed"_hash ) ) : 250.0f;

		for ( auto i = 0; i < 15; ++i )
		{
			const auto sim_speed = sim_vel.length_2d( );
			if ( sim_speed < 1.0f )
			{
				break;
			}

			const auto control = std::fmaxf( sim_speed, sv_stopspeed );
			const auto drop = sv_friction * surface_friction * control * cstypes::tick_interval;
			auto new_speed = std::fmaxf( sim_speed - drop, 0.0f );
			auto accel = sv_accelerate;

			if ( shared_ctx.is_scoped || is_sniper_auto_scope )
			{
				const auto weapon_ratio = std::fminf( 1.0f, shared_ctx.weapon_max_speed / 250.0f );
				const auto scoped_max = std::fmaxf( 250.0f, max_move_speed ) * weapon_ratio * 0.52f;

				if ( new_speed > scoped_max - 5.0f )
				{
					const auto t = 1.0f - std::fmaxf( 0.0f, new_speed - ( scoped_max - 5.0f ) ) / std::fmaxf( 0.01f, 5.0f );
					accel *= std::clamp( t, 0.0f, 1.0f );
				}
			}

			const auto accel_speed = std::fminf( accel * shared_ctx.weapon_max_speed * surface_friction * cstypes::tick_interval, new_speed );
			new_speed = std::fmaxf( new_speed - accel_speed, 0.0f );

			if ( new_speed > 0.0f )
			{
				sim_vel *= ( new_speed / sim_speed );
			}
			else
			{
				sim_vel = {};
				break;
			}
		}

		const auto avg_vel = ( prestate.networked_velocity + sim_vel ) * 0.5f;
		const auto stop_ticks = g_shared.calculate_stop_ticks( prestate.networked_velocity, shared_ctx.weapon_max_speed, local.pawn );
		const auto stop_time = static_cast< float >( stop_ticks ) * cstypes::tick_interval;

		return stop_prediction
		{
			.eye =
			{
				current_eye.x + avg_vel.x * stop_time,
				current_eye.y + avg_vel.y * stop_time,
				current_eye.z
			},
			.inaccuracy = g_shared.get_inaccuracy_at_velocity( local.pawn, sim_vel )
		};
	}


} // namespace features::combat
