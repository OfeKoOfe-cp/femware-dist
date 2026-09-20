#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/random/random.hpp>
#include <core/systems/systems.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>

#include "../movement.hpp"
#include <protection/game_addresses.hpp>

namespace features::movement {

	namespace {

		[[nodiscard]] std::optional<float> predict_landing_fraction(
			std::uintptr_t local_pawn,
			std::uintptr_t movement_services,
			const systems::prediction::state& prestate,
			bool holding_duck )
		{
			if ( prestate.networked_velocity.z > 0.0f )
			{
				return std::nullopt;
			}

			const auto duck_amount = memory::read<float>( movement_services + SCHEMA( "CCSPlayer_MovementServices", "m_flDuckAmount"_hash ) );
			const auto mins = memory::read<math::vector3>( local_pawn + SCHEMA( "C_BaseModelEntity", "m_Collision"_hash ) + SCHEMA( "CCollisionProperty", "m_vecMins"_hash ) );
			auto maxs = memory::read<math::vector3>( local_pawn + SCHEMA( "C_BaseModelEntity", "m_Collision"_hash ) + SCHEMA( "CCollisionProperty", "m_vecMaxs"_hash ) );

			auto trace_origin = prestate.networked_origin;
			if ( holding_duck && duck_amount > 0.0f )
			{
				const auto standing_height{ 72.0f };
				const auto duck_hull_diff = standing_height - maxs.z;
				trace_origin.z -= duck_hull_diff * 0.5f;
				maxs.z = standing_height;
			}

			auto trace_mask{ 0ull };
			{
				const auto pawn_ptr = memory::read<std::uintptr_t>( movement_services + 56 );
				trace_mask = memory::read<std::uintptr_t>( pawn_ptr + 0xd48 );

				if ( !pawn_ptr || ( memory::read<std::uint32_t>( pawn_ptr + 0x3f8 ) & 0x10 ) )
				{
					trace_mask |= 0x20;
				}
			}

			const auto filter = systems::g_tracing.make_player_movement_filter( local_pawn, trace_mask, 11 );
			const auto sv_gravity = CONVAR ("sv_gravity")->get<float>( );
			const auto sv_standable_normal = CONVAR ("sv_standable_normal")->get<float>( );
			const auto gravity_scale = memory::read<float>( local_pawn + SCHEMA( "C_BaseEntity", "m_flGravityScale"_hash ) );

			auto velocity = prestate.networked_velocity;
			velocity.z -= ( gravity_scale * sv_gravity * cstypes::tick_interval ) * 0.5f;

			const math::vector3 trace_start = trace_origin;
			math::vector3 trace_end{};

			trace_end.x = trace_origin.x + velocity.x * cstypes::tick_interval;
			trace_end.y = trace_origin.y + velocity.y * cstypes::tick_interval;
			trace_end.z = trace_origin.z + velocity.z * cstypes::tick_interval;
			trace_end.z -= 2.0f;

			const auto result = systems::g_tracing.trace_player_bbox( trace_start, trace_end, { mins, maxs }, filter, movement_services );
			if ( result.fraction <= 0.0f || result.fraction >= 1.0f || result.normal.z < sv_standable_normal )
			{
				return std::nullopt;
			}

			return std::clamp( std::round( result.fraction * 64.0f ) / 64.0f, 1.0f / 64.0f, 63.0f / 64.0f );
		}

		void apply_landing_jump( proto::base_usercmd_pb* base, float when )
		{
			const auto subtick_moves = base->mutable_subtick_moves( );
			const auto release_when = std::clamp( when - 1.0f / 64.0f, 1.0f / 64.0f, 63.0f / 64.0f );

			if ( release_when < when )
			{
				if ( const auto jump_up = systems::g_input.acquire_subtick_step( subtick_moves ) )
				{
					jump_up->set_button( cstypes::command_buttons::in_jump );
					jump_up->set_pressed( false );
					jump_up->set_when( release_when );
					jump_up->set_analog_forward_delta( 0.0f );
					jump_up->set_analog_left_delta( 0.0f );
				}
			}

			if ( const auto jump_down = systems::g_input.acquire_subtick_step( subtick_moves ) )
			{
				jump_down->set_button( cstypes::command_buttons::in_jump );
				jump_down->set_pressed( true );
				jump_down->set_when( when );
				jump_down->set_analog_forward_delta( 0.0f );
				jump_down->set_analog_left_delta( 0.0f );
			}
		}

	} // namespace

	void bhop::on_create_move( systems::input::usercmd* cmd ) const
	{
		if ( !settings::g_movement.bhop.value )
		{
			return;
		}

		if (CONVAR ("sv_autobunnyhopping")->get<bool>( ) )
		{
			return;
		}

		static int s_consecutive_hops{ 0 };

		const auto& prestate = systems::g_prediction.pre( );
		const auto on_ground = ( prestate.flags & cstypes::entity_flags::on_ground ) != 0;

		if ( on_ground )
		{
			s_consecutive_hops = 0;
		}

		if ( !( cmd->buttons.value & cstypes::command_buttons::in_jump ) )
		{
			return;
		}

		if ( features::movement::g_jumpbug.active_this_tick( ) )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.pawn )
		{
			return;
		}

		const auto move_type = memory::read<std::uint8_t>( local.pawn + SCHEMA( "C_BaseEntity", "m_nActualMoveType"_hash ) );
		if ( move_type == cstypes::move_type::ladder || move_type == cstypes::move_type::noclip )
		{
			return;
		}

		const auto base = cmd->csgo_user_cmd.mutable_base( );
		if ( !base )
		{
			return;
		}

		// Always drop the held jump so a fresh press can be injected. Without
		// this, landing on a ledge or after a long fall never re-triggers because
		// the engine only jumps on a release->press transition.
		cmd->buttons.value &= ~cstypes::command_buttons::in_jump;

		const auto max_hops = settings::g_movement.bhop_max_consecutive.value;
		if ( max_hops > 0 && s_consecutive_hops >= max_hops )
		{
			s_consecutive_hops = 0;
			return;
		}

		const auto hitchance = settings::g_movement.bhop_hitchance.value;
		const auto hop_allowed = [ & ]( )
			{
				if ( hitchance >= 100 )
				{
					return true;
				}

				return random::floating( 0.0f, 100.0f ) <= static_cast< float >( hitchance );
			};

		// Grounded: jump immediately. This is the case the landing predictor
		// cannot cover (edge run-offs, landing late in a tick, fast falls).
		if ( on_ground )
		{
			if ( !hop_allowed( ) )
			{
				return;
			}

			s_consecutive_hops++;
			apply_landing_jump( base, 0.0f );
			return;
		}

		const auto movement_services = memory::read<std::uintptr_t>( local.pawn + SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash ) );
		if ( !movement_services )
		{
			return;
		}

		const auto holding_duck = ( cmd->buttons.value & cstypes::command_buttons::in_duck ) != 0;
		const auto landing = predict_landing_fraction( local.pawn, movement_services, prestate, holding_duck );
		if ( !landing )
		{
			return;
		}

		if ( !hop_allowed( ) )
		{
			return;
		}

		s_consecutive_hops++;

		// Perfect-bhop avoidance. CS2 buffers +jump, so a frame-perfect
		// landing-buffers on the exact 1/64 tick the moment the feet kiss the
		// ground is the classic scripted tell. When the avoidance slider is on
		// we pull the injected jump a random sub-tick amount EARLY (clamped to
		// half a tick so a long slider can't fire a mid-air +jump and break the
		// strafe) so the hop no longer lands on the perfect edge, yet still
		// buffers before ground contact.
		//
		// Safety lock: this is a SAFE-mode feature. With Unsafe mode engaged the
		// user has opted into the raw behavior, so perfects are allowed and the
		// slider is ignored -- the safe parameter is locked out.
		auto jump_when = *landing;
		const auto unsafe_on = settings::g_cheat.unsafe_mode.value;
		const auto avoid_perfection = settings::g_movement.bhop_avoid_perfection.value && !unsafe_on;
		if ( avoid_perfection )
		{
			const auto tick_ms = cstypes::tick_interval * 1000.0f;
			const auto delay_ms = random::floating( 0.0f, static_cast< float >( settings::g_movement.bhop_perfection_delay_ms.value ) );
			const auto delay_frac = std::clamp( delay_ms / tick_ms, 0.0f, 0.5f );
			jump_when = std::max( 1.0f / 64.0f, *landing - delay_frac );
		}

		apply_landing_jump( base, jump_when );
	}

} // namespace features::movement
