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

	std::vector<rage::candidate> rage::gather_candidates( const systems::local::snapshot& local, float max_distance_sq ) const
	{
		const auto& shared_ctx = g_shared.ctx( );
		const auto players = systems::g_entities.get_by_type( systems::entities::type::player );

		std::vector<candidate> out;
		out.reserve( players.size( ) );

		const_cast<rage*>( this )->m_extrapolated_records.clear( );

		for ( const auto& p : players )
		{
			if ( !p.ptr || p.ptr == local.controller )
			{
				continue;
			}

			// Per-player flags: whitelisted players are never valid targets.
			const auto steam_id = memory::read<std::uint64_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
			if ( features::players::get( steam_id ).whitelist( ) )
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

			if ( memory::read<bool>( pawn + SCHEMA( "C_CSPlayerPawn", "m_bGunGameImmunity"_hash ) ) )
			{
				continue;
			}

			auto records = g_shared.lc( ).get_valid_records( pawn );

			if ( records.empty( ) )
			{
				auto extrap = g_shared.lc( ).extrapolate( pawn );
				if ( extrap.has_value( ) )
				{
					const_cast<rage*>( this )->m_extrapolated_records.push_back( std::move( *extrap ) );
					records.push_back( &const_cast<rage*>( this )->m_extrapolated_records.back( ) );
				}
				else
				{
					shared::lagcomp::record live_record{};
					if ( live_record.setup( pawn ) )
					{
						const_cast<rage*>( this )->m_extrapolated_records.push_back( std::move( live_record ) );
						records.push_back( &const_cast<rage*>( this )->m_extrapolated_records.back( ) );
					}
					else
					{
						continue;
					}
				}
			}

			if ( max_distance_sq > 0.0f )
			{
				const auto& origin = systems::g_prediction.pre( ).origin;
				const auto delta_front = records.front( )->origin - origin;
				auto closest_sq = delta_front.x * delta_front.x + delta_front.y * delta_front.y + delta_front.z * delta_front.z;

				if ( records.size( ) > 1 )
				{
					const auto delta_back = records.back( )->origin - origin;
					const auto back_sq = delta_back.x * delta_back.x + delta_back.y * delta_back.y + delta_back.z * delta_back.z;
					closest_sq = std::min( closest_sq, back_sq );
				}

				if ( closest_sq > max_distance_sq )
				{
					continue;
				}
			}

			candidate c{};
			c.pawn = pawn;
			c.steam_id = steam_id;
			c.health = health;
			c.armor = memory::read<int>( pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) );

			const auto pick_record_indices = [ &records ]( std::array<int, k_max_scan_records>& out_indices ) -> int
				{
					const auto count = records.size( );
					if ( count == 0 )
					{
						return 0;
					}

					auto picked{ 0 };
					const auto add_index = [ & ]( int idx )
						{
							if ( picked >= k_max_scan_records )
							{
								return;
							}

							for ( auto i = 0; i < picked; ++i )
							{
								if ( out_indices[ i ] == idx )
								{
									return;
								}
							}

							out_indices[ picked++ ] = idx;
						};

					add_index( 0 );

					// Also offer the record whose tick matches the server's
					// interpolation rewind (client_tick - lerp_ticks). For a
					// moving target, neither the newest nor the oldest record
					// lands on it, which is why only new/old were ever used and
					// the mismatch showed up as intermittent misses.
					const auto rewind_tick = g_shared.ctx( ).current_tick - g_shared.sh( ).lerp_ticks_int( );
					auto best_idx{ 0 };
					auto best_dt{ std::numeric_limits<int>::max( ) };
					for ( auto i = 0; i < static_cast< int >( count ); ++i )
					{
						if ( !records[ static_cast< std::size_t >( i ) ] )
						{
							continue;
						}

						const auto dt = std::abs( records[ static_cast< std::size_t >( i ) ]->tick - rewind_tick );
						if ( dt < best_dt )
						{
							best_dt = dt;
							best_idx = i;
						}
					}
					add_index( best_idx );

					if ( count > 1 )
					{
						add_index( static_cast< int >( count - 1 ) );
					}

					return picked;
				};

			std::array<int, k_max_scan_records> record_indices{};
			const auto picked_count = pick_record_indices( record_indices );

			for ( auto i = 0; i < picked_count; ++i )
			{
				c.records[ i ] = records[ static_cast< std::size_t >( record_indices[ i ] ) ];
			}

			c.record_count = picked_count;

			if ( shared_ctx.weapon_type >= cstypes::weapon_type::pistol && shared_ctx.weapon_type <= cstypes::weapon_type::lmg )
			{
				const auto& config = settings::g_combat.m_ragebot.get_group( shared_ctx.weapon_type );
				c.min_damage = this->get_min_damage( config, health, config.min_damage_override.value );
			}

			out.push_back( c );
		}

		return out;
	}

	void rage::run_gun( systems::input::usercmd* cmd, const aim_context& ctx, const systems::local::snapshot& local, bool allow_fire, std::vector<candidate>* pre_candidates )
	{
		if ( !settings::g_combat.m_ragebot.enabled )
		{
			return;
		}

		auto& shared_ctx = g_shared.ctx( );
		const auto& config = settings::g_combat.m_ragebot.get_group( shared_ctx.weapon_type );

		// Autoscope: pull the zoom up the instant fire is held on a scoped
		// weapon, before any scan path can early-return (the no-spread branch
		// used to bypass this entirely). Holding on attack while the zoom
		// animates lets the first shot leave fully scoped, even during a peek
		// where no target has resolved yet.
		if ( settings::g_combat.m_ragebot.auto_stop.value && shared_ctx.has_scope && !ctx.is_scoped && config.auto_scope.value && ( cmd->buttons.value & cstypes::command_buttons::in_attack ) )
		{
			cmd->buttons.value |= cstypes::command_buttons::in_second_attack;
			cmd->buttons.value_changed |= cstypes::command_buttons::in_second_attack;
			this->m_should_stop = true;
		}

		const auto try_doubletap = [ & ]( bool fire_requested, bool gate_on_lethal, bool lethal )
		{
			if ( config.doubletap.value && shared_ctx.item_def_idx != cstypes::item_definition_index::weapon_r8_revolver )
			{
				return this->process_doubletap( cmd, local, fire_requested, gate_on_lethal, lethal );
			}

			return doubletap_state::passthrough;
		};

		auto candidates = pre_candidates ? std::move( *pre_candidates ) : this->gather_candidates( local );

		{
			std::lock_guard lock( m_debug_mtx );
			m_debug_points.clear( );
		}

		if ( candidates.empty( ) )
		{
			try_doubletap( false, false, false );
			return;
		}

		auto eye_candidates = g_shared.sh( ).get_candidates( );
		if ( eye_candidates.count == 0 )
		{
			eye_candidates.entries[ 0 ].position = g_shared.get_shoot_position( );
			eye_candidates.entries[ 0 ].is_uninterpolated = true;
			eye_candidates.count = 1;
		}

		const auto scan_from_eye_candidates = [ & ]( const math::vector3& eye_offset, float inaccuracy )
		{
			std::vector<scan_hit> hits_out;

			for ( auto i = 0; i < eye_candidates.count; ++i )
			{
				const auto eye = eye_candidates.entries[ i ].position + eye_offset;
				auto hits = this->scan_players( eye, inaccuracy, ctx, candidates, local );
				auto found_direct{ false };

				for ( auto& hit : hits )
				{
					auto source_eye = eye_candidates.entries[ i ];
					source_eye.position = eye;
					hit.source_eye = source_eye;
					found_direct = found_direct || !hit.penetrated;
					hits_out.push_back( std::move( hit ) );
				}

				if ( found_direct )
				{
					break;
				}
			}

			return hits_out;
		};

		if ( config.no_spread.value )
		{
			shared_ctx.inaccuracy = g_shared.get_inaccuracy( false );
			auto all_hits = scan_from_eye_candidates( {}, shared_ctx.inaccuracy );

			if ( all_hits.empty( ) )
			{
				try_doubletap( false, false, false );
				return;
			}

const auto best = this->select_best( ctx, all_hits, shared_ctx.inaccuracy );

			if ( !best.valid )
			{
				try_doubletap( false, false, false );
				return;
			}

			if ( !allow_fire )
			{
				return;
			}

			// Same-target re-fire hold: the last shot at this pawn either
			// missed (resolved) or is still in flight. Keep the aim dialed on
			// it but hold the trigger, so the ragebot stops machine-gunning a
			// player it keeps whiffing on.
			if ( g_shared.target_held( best.hit.pawn, shared_ctx.current_time ) )
			{
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );
				return;
			}

			const auto dt = try_doubletap( true, config.doubletap_lethal.value, best.is_lethal( ) );
			if ( dt == doubletap_state::hold )
			{
				// Lethal-charge hold: keep the aim locked on the shot, pull
				// nothing until the kill flips.
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );
				return;
			}

			// Weapon-cadence gate. With several enemies alive every fresh pawn
			// clears the same-target hold, so without this the trigger re-fires
			// on consecutive ticks (the machine-gun stutter) and the server
			// drops the out-of-cycle shots. doubletap has its own charge gate,
			// so only pace the plain path.
			if ( dt != doubletap_state::burst && !g_shared.can_shoot( cmd, local.controller ) )
			{
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );
				return;
			}

			this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, dt == doubletap_state::burst );
			return;
		}

		const auto primary_eye = eye_candidates.entries[ 0 ].position;
		const auto& prestate = systems::g_prediction.pre( );

		// Current-shot selection is always based on current engine shoot-history.
		auto current_hits = scan_from_eye_candidates( {}, ctx.predicted_inaccuracy );
		const auto best = this->select_best( ctx, current_hits, ctx.predicted_inaccuracy );

		const auto base_needed_hc = config.hitchance_override.value ? static_cast< float >( config.hitchance_override_value ) / 100.0f : static_cast< float >( config.hitchance ) / 100.0f;
		const auto needed_hc = best.valid ? best.needed_hitchance : base_needed_hc;
		const auto duckpeek_active = settings::g_combat.m_duckpeek.enabled.value && ctx.on_ground;
		const auto is_ducked = ( prestate.flags & cstypes::entity_flags::ducking ) != 0;

		// Like the auto revolver, only fire when the accuracy used to justify the
		// shot is the accuracy the weapon actually has at the moment the bullet
		// leaves. The predicted inaccuracy is a look-ahead (end of this command);
		// firing on it lets shots out while autostop is still decelerating, so the
		// real spread at subtick zero is higher than the gate assumed. Gate ground
		// shots on the current velocity-based standing inaccuracy instead, exactly
		// like the revolver waits until it is fully cocked and stopped.
		const auto standing_inaccuracy = ctx.on_ground ? this->get_standing_inaccuracy( local, ctx ) : ctx.predicted_inaccuracy;
		const auto standing_hc = best.valid
			? this->evaluate_hitchance( best.hit, ctx, standing_inaccuracy )
			: 0.0f;

		auto accurate = best.valid && standing_hc >= needed_hc;
		const auto max_acc = g_shared.is_max_accuracy( standing_inaccuracy );
		const auto force = best.valid && ( ctx.on_ground ? ( config.force_shot.value && max_acc ) : ( config.force_shot_air.value && max_acc ) );
		auto shot_viable = accurate || force;

		// Subtick Counter-Strafe Early Hitchance Predictor:
		// If currently moving/decelerating on ground, evaluate if counter-strafe deceleration
		// brings inaccuracy into lethal hitchance early within this subtick cycle!
		if ( !shot_viable && best.valid && ctx.on_ground && config.early_counterstrafe_predict.value && settings::g_combat.m_ragebot.auto_stop.value )
		{
			const auto stop = this->predict_stop( ctx, primary_eye, local );
			if ( stop )
			{
				const auto early_hc = this->evaluate_hitchance( best.hit, ctx, stop->inaccuracy );
				if ( early_hc >= needed_hc )
				{
					this->m_should_stop = true;
				}
			}
		}

		// Autostop planning is independent from firing. Ground movement can use a
		// predicted stopped eye; airborne stopping keeps the current target context.
		if ( ( !shot_viable || !allow_fire ) && this->should_stop_movement( ctx ) )
		{
			const auto stop = this->predict_stop( ctx, primary_eye, local );
			if ( stop )
			{
				const auto future_offset = stop->eye - primary_eye;
				auto planned_hits = scan_from_eye_candidates( future_offset, stop->inaccuracy );
				const auto planned = this->select_best( ctx, planned_hits, stop->inaccuracy );
				this->m_should_stop = planned.valid;
			}
			else
			{
				this->m_should_stop = best.valid;
			}
		}

		if ( !best.valid )
		{
			try_doubletap( false, false, false );
			return;
		}

		if ( duckpeek_active && allow_fire )
		{
			if ( shot_viable )
			{
				this->m_release_duck_for_shot = true;
			}
			else if ( !this->m_duckpeek_reduck )
			{
				this->m_release_duck_for_shot = false;
			}
		}

		auto ready_to_fire = shot_viable;
		if ( duckpeek_active )
		{
			if ( is_ducked )
			{
				ready_to_fire = false;
			}
			else
			{
				ready_to_fire = ready_to_fire && this->m_release_duck_for_shot;
			}
		}

		if ( ready_to_fire && allow_fire )
		{
			// Same-target re-fire hold: keep aiming at the whiffed/in-flight
			// target but hold the trigger until the prior shot resolves.
			if ( g_shared.target_held( best.hit.pawn, shared_ctx.current_time ) )
			{
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );

				if ( duckpeek_active )
				{
					this->m_duckpeek_reduck = true;
					this->m_release_duck_for_shot = false;
				}

				return;
			}

			const auto dt = try_doubletap( true, config.doubletap_lethal.value, best.is_lethal( ) );
			if ( dt == doubletap_state::hold )
			{
				// Lethal-charge hold: aim-lock only, no trigger pull.
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );
			}
			else if ( dt != doubletap_state::burst && !g_shared.can_shoot( cmd, local.controller ) )
			{
				// Cycle not ready (or reloading / empty): keep the aim locked but
				// hold the trigger so a target swap can't squeeze an out-of-cadence
				// shot. doubletap burst already cleared its own charge gate.
				this->fire_gun( cmd, best, false, best.hit.source_eye.position, local, false, true );
			}
			else
			{
				this->fire_gun( cmd, best, !accurate && force, best.hit.source_eye.position, local, dt == doubletap_state::burst );
			}

			if ( duckpeek_active )
			{
				this->m_duckpeek_reduck = true;
				this->m_release_duck_for_shot = false;
			}
		}
		else
		{
			try_doubletap( false, false, false );
		}
	}

	void rage::run_taser( systems::input::usercmd* cmd, const aim_context& ctx, const systems::local::snapshot& local )
	{
		if ( !settings::g_combat.m_zeusbot.enabled )
		{
			return;
		}

		auto candidates = this->gather_candidates( local );
		if ( candidates.empty( ) )
		{
			return;
		}

		auto eye_candidates = g_shared.sh( ).get_candidates( );
		if ( eye_candidates.count == 0 )
		{
			eye_candidates.entries[ 0 ].position = g_shared.get_shoot_position( );
			eye_candidates.entries[ 0 ].is_uninterpolated = true;
			eye_candidates.count = 1;
		}

		std::vector<scan_hit> all_hits;

		for ( auto i = 0; i < eye_candidates.count; ++i )
		{
			auto hits = this->scan_taser( eye_candidates.entries[ i ].position, ctx, candidates, local );

			for ( auto& h : hits )
			{
				h.source_eye = eye_candidates.entries[ i ];
				all_hits.push_back( std::move( h ) );
			}
		}

		if ( all_hits.empty( ) )
		{
			return;
		}

		target best{};

		for ( const auto& h : all_hits )
		{
			if ( !best.valid || h.score > best.score )
			{
				best.hit = h;
				best.hitchance = 1.0f;
				best.score = h.score;
				best.valid = true;
			}
		}

		if ( best.valid )
		{
			this->m_zeus_fired = true;
			this->fire_melee( cmd, best, local );
		}
	}

	void rage::run_knife( systems::input::usercmd* cmd, const aim_context& ctx, const systems::local::snapshot& local )
	{
		if ( !settings::g_combat.m_knifebot.enabled )
		{
			return;
		}

		const auto info = this->get_knife_info( local );
		if ( !info.can_slash && !info.can_stab )
		{
			return;
		}

		constexpr auto max_knife_dist_sq = 150.0f * 150.0f;
		auto candidates = this->gather_candidates( local, max_knife_dist_sq );
		if ( candidates.empty( ) )
		{
			return;
		}

		auto eye_candidates = g_shared.sh( ).get_candidates( );
		if ( eye_candidates.count == 0 )
		{
			eye_candidates.entries[ 0 ].position = g_shared.get_shoot_position( );
			eye_candidates.entries[ 0 ].is_uninterpolated = true;
			eye_candidates.count = 1;
		}

		std::vector<scan_hit> all_hits;

		for ( auto i = 0; i < eye_candidates.count; ++i )
		{
			auto hits = this->scan_knife( eye_candidates.entries[ i ].position, ctx, info, candidates, local );

			for ( auto& h : hits )
			{
				h.source_eye = eye_candidates.entries[ i ];
				all_hits.push_back( std::move( h ) );
			}
		}

		if ( all_hits.empty( ) )
		{
			return;
		}

		target best{};
		target best_backstab{};

		for ( const auto& h : all_hits )
		{
			auto& dest = h.is_backstab ? best_backstab : best;

			if ( !dest.valid || h.score > dest.score )
			{
				dest.hit = h;
				dest.hitchance = 1.0f;
				dest.score = h.score;
				dest.valid = true;
			}
		}

		auto& chosen = best_backstab.valid ? best_backstab : best;
		if ( !chosen.valid )
		{
			return;
		}

		this->m_knife_attack = static_cast< std::uint8_t >( chosen.hit.attack_type );
		this->fire_melee( cmd, chosen, local );
	}

	void rage::auto_revolver( systems::input::usercmd* cmd, const aim_context& ctx, const systems::local::snapshot& local )
	{
		if ( !settings::g_combat.m_ragebot.enabled )
		{
			this->m_revolver_cock_ticks = 0;
			return;
		}

		if ( !g_shared.can_shoot( cmd, local.controller ) )
		{
			this->m_revolver_cock_ticks = 0;
			return;
		}

		if ( !settings::g_combat.m_autos.revolver.value )
		{
			this->m_revolver_cock_ticks = 0;
			return;
		}

		const auto& autos = settings::g_combat.m_autos;
		const auto cock_ticks = std::clamp( autos.revolver_cock_ticks.value, 1, 30 );
		if ( this->m_revolver_cock_ticks >= cock_ticks )
		{
			// End the held cycle. Target selection adds attack back on this
			// command only when the revolver should actually fire.
			cmd->buttons.value &= ~cstypes::command_buttons::in_attack;
			cmd->buttons.value_changed |= cstypes::command_buttons::in_attack;
			cmd->buttons.value_scroll &= ~cstypes::command_buttons::in_attack;
			cmd->csgo_user_cmd.set_attack1_start_history_index( -1 );

			if ( autos.revolver_prefer_right_click.value )
			{
				// Release the RMB hammer hold so the firing tick is a clean LMB
				// pull through run_gun, not a held RMB that flags as attack2.
				cmd->buttons.value &= ~cstypes::command_buttons::in_second_attack;
				cmd->buttons.value_changed |= cstypes::command_buttons::in_second_attack;
			}
			this->m_revolver_cock_ticks = 0;

			this->run_gun( cmd, ctx, local );
			return;
		}

		// Keep target and hitchance planning active throughout the cock cycle.
		// Autostop consumes this command's decision on the following command.
		this->run_gun( cmd, ctx, local, false );

		if ( autos.revolver_prefer_right_click.value )
		{
			// RMB-cock: hold attack2 instead of attack1 so the hammer is drawn
			// with the right trigger; keep attack1 clear so nothing reads as a
			// queued left pull mid-cycle.
			cmd->buttons.value &= ~cstypes::command_buttons::in_attack;
			cmd->buttons.value_changed |= cstypes::command_buttons::in_attack;
			cmd->buttons.value_scroll &= ~cstypes::command_buttons::in_attack;

			cmd->buttons.value |= cstypes::command_buttons::in_second_attack;
			cmd->buttons.value_changed |= cstypes::command_buttons::in_second_attack;
		}
		else
		{
			cmd->buttons.value |= cstypes::command_buttons::in_attack;
			cmd->buttons.value_changed |= cstypes::command_buttons::in_attack;
			cmd->buttons.value_scroll |= cstypes::command_buttons::in_attack;
		}

		const auto history_index = cmd->csgo_user_cmd.input_history_size( ) - 1;
		if ( history_index >= 0 )
		{
			cmd->csgo_user_cmd.set_attack1_start_history_index( history_index );
		}

		++this->m_revolver_cock_ticks;
	}


} // namespace features::combat
