#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <core/rendering/rendering.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <protection/game_addresses.hpp>

namespace features::esp::player {

	void overlay::on_render( xdraw::draw_list& draw_list )
	{
		const bool sound_esp_enemy = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp.enabled.value;
		const bool sound_esp_team = settings::g_esp.m_player.m_overlay[ 1 ].m_sound_esp.enabled.value;
		const bool sound_esp_any = sound_esp_enemy || sound_esp_team;

		if ( !settings::g_esp.m_player.m_overlay[ 0 ].enabled.value && 
		     !settings::g_esp.m_player.m_overlay[ 1 ].enabled.value && 
		     !sound_esp_any )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.is_valid( ) || !systems::g_entities.exists( local.view_controller( ) ) )
		{
			return;
		}

		auto players = systems::g_entities.get_by_type( systems::entities::type::player );

		// Resolve each player's scene-node origin once instead of inside the
		// comparator, which previously re-read memory O(n log n) times per frame.
		struct sortable_player
		{
			systems::entities::cached entity{};
			float distance_sq{};
		};

		{
			const auto camera = systems::g_view.origin( );

			std::vector<sortable_player> sortable;
			sortable.reserve( players.size( ) );

			for ( auto& player : players )
			{
				auto distance_sq = std::numeric_limits<float>::infinity( );
				if ( player.ptr )
				{
					const auto node = memory::read<std::uintptr_t>( player.ptr + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
					if ( node )
					{
						const auto origin = memory::read<math::vector3>( node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
						distance_sq = ( origin - camera ).length_sqr( );
					}
				}

				sortable.push_back( { player, distance_sq } );
			}

			std::sort( sortable.begin( ), sortable.end( ), [ ]( const sortable_player& a, const sortable_player& b )
				{
					return a.distance_sq > b.distance_sq;
				} );

			players.clear( );
			players.reserve( sortable.size( ) );
			for ( auto& entry : sortable )
			{
				players.push_back( entry.entity );
			}
		}

		// Skeleton/visibility work is the dominant per-frame cost here; only
		// request it when one of the enabled features actually consumes it.
		const auto& ov_enemy = settings::g_esp.m_player.m_overlay[ 0 ];
		const auto& ov_team = settings::g_esp.m_player.m_overlay[ 1 ];
		const bool want_bones =
			ov_enemy.m_skeleton.enabled.value || ov_enemy.m_oof_arrow.enabled.value || ov_enemy.m_view_ray.enabled.value ||
			ov_team.m_skeleton.enabled.value || ov_team.m_oof_arrow.enabled.value || ov_team.m_view_ray.enabled.value;
		const bool want_visible =
			ov_enemy.m_box.enabled.value || ov_enemy.m_skeleton.enabled.value || ov_enemy.m_snap_lines.enabled.value ||
			ov_team.m_box.enabled.value || ov_team.m_skeleton.enabled.value || ov_team.m_snap_lines.enabled.value;

		for ( const auto& player : players )
		{
			const auto info = this->get_info( player, local, want_bones, want_visible );
			if ( !info.valid( ) )
			{
				continue;
			}

			const auto& cfg = settings::g_esp.m_player.m_overlay[ info.is_other_team ? 0 : 1 ];

			// Always process acoustic ESP when enabled, regardless of visual box/skeleton toggles
			if ( cfg.m_sound_esp.enabled.value )
			{
				this->process_sound_esp( info, cfg.m_sound_esp );
			}

			if ( !cfg.enabled.value )
			{
				continue;
			}

			if ( cfg.max_distance.value > 0.0f && info.distance > cfg.max_distance.value )
			{
				continue;
			}

			if ( cfg.visible_only.value && !info.is_visible )
			{
				continue;
			}

			if ( cfg.m_oof_arrow.enabled.value )
			{
				this->add_oof_arrow( draw_list, info, cfg.m_oof_arrow );
			}

			const auto bounds = systems::g_bounds.get( info.pawn );
			if ( !bounds.valid )
			{
				continue;
			}

			overlay::draw_offsets offsets{};

			if ( cfg.m_box.enabled.value )
			{
				this->add_box( draw_list, bounds, cfg.m_box, info.is_visible );
			}

			if ( cfg.m_skeleton.enabled.value )
			{
				this->add_skeleton( draw_list, info, cfg.m_skeleton, info.is_visible, local );
			}

			if ( cfg.m_health_bar.enabled.value )
			{
				this->add_health_bar( draw_list, bounds, info, cfg.m_health_bar, offsets );
			}

			if ( cfg.m_ammo_bar.enabled.value && info.weapon.valid( ) )
			{
				this->add_ammo_bar( draw_list, bounds, info, cfg.m_ammo_bar, offsets );
			}

			if ( cfg.m_name.enabled.value && !info.name.empty( ) )
			{
				this->add_name( draw_list, bounds, info, cfg.m_name, offsets );
			}

			if ( cfg.m_weapon.enabled.value && !info.weapon.name.empty( ) )
			{
				this->add_weapon( draw_list, bounds, info, cfg.m_weapon, offsets );
			}

			if ( cfg.m_info_flags.enabled.value )
			{
				this->add_flags( draw_list, bounds, info, cfg.m_info_flags, offsets );
			}


			if ( cfg.m_snap_lines.enabled.value )
			{
				this->add_snap_lines( draw_list, bounds, cfg.m_snap_lines, info.is_visible );
			}

			if ( cfg.m_view_ray.enabled.value )
			{
				this->add_view_ray( draw_list, info, cfg.m_view_ray );
			}
		}
		if ( sound_esp_any )
		{
			this->render_sound_rings( draw_list, settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp );
		}
	}

	struct player_audio_tracker
	{
		bool initialized{ false };
		bool on_ground{ true };
		bool scoped{ false };
		bool defusing{ false };
		bool reloading{ false };
		bool was_walking{ false };
		bool was_on_ladder{ false };
		bool was_planting{ false };
		bool was_pin_pulled{ false };
		int last_ammo{ -1 };
		int last_shots_fired{ -1 };
		int last_zoom_level{ 0 };
		float last_fired_time{ 0.0f };
		float last_shot_time{ 0.0f };
		float last_vel_z{ 0.0f };
		std::uintptr_t last_weapon_ptr{ 0 };
		std::uint64_t last_step_time{ 0 };
		std::uint64_t last_ladder_step_time{ 0 };
		std::uint64_t last_shot_ring_time{ 0 };
		std::uint64_t last_seen_time{ 0 };
	};

	static inline std::unordered_map<std::uintptr_t, player_audio_tracker> s_audio_trackers{};
	static inline std::uint64_t s_last_tracker_cleanup{ 0 };

	void overlay::process_sound_esp( const info& info, const settings::esp::player::overlay::sound_esp& cfg )
	{
		if ( !cfg.enabled.value || !info.pawn )
		{
			return;
		}

		const auto now = GetTickCount64( );

		// Periodically purge stale trackers
		if ( now - s_last_tracker_cleanup > 5000 )
		{
			s_last_tracker_cleanup = now;
			for ( auto it = s_audio_trackers.begin( ); it != s_audio_trackers.end( ); )
			{
				if ( now - it->second.last_seen_time > 10000 )
				{
					it = s_audio_trackers.erase( it );
				}
				else
				{
					++it;
				}
			}
		}

		auto& trk = s_audio_trackers[ info.pawn ];

		const auto flags = memory::read<std::uint32_t>( info.pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) );
		const bool current_on_ground = ( flags & ( 1 << 0 ) ) != 0;
		const auto vel = memory::read<math::vector3>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_vecVelocity"_hash ) );
		const auto speed_2d = vel.length_2d( );
		const bool is_walking = memory::read<bool>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsWalking"_hash ) );
		const auto move_type = memory::read<std::uint8_t>( info.pawn + SCHEMA( "C_BaseEntity", "m_nActualMoveType"_hash ) );
		const bool on_ladder = ( move_type == cstypes::move_type::ladder );

		const auto shots_fired = memory::read<int>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_iShotsFired"_hash ) );
		const auto fired_time = memory::read<float>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_flLastFiredWeaponTime"_hash ) );

		bool current_reloading = false;
		float shot_time = 0.0f;
		int zoom_level = 0;
		if ( info.weapon.ptr )
		{
			current_reloading = memory::read<bool>( info.weapon.ptr + SCHEMA( "C_CSWeaponBase", "m_bInReload"_hash ) );
			shot_time = memory::read<float>( info.weapon.ptr + SCHEMA( "C_CSWeaponBase", "m_fLastShotTime"_hash ) );
			zoom_level = memory::read<int>( info.weapon.ptr + SCHEMA( "C_CSWeaponBaseGun", "m_zoomLevel"_hash ) );
		}

		const auto movement_services = memory::read<std::uintptr_t>( info.pawn + SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash ) );
		float plant_offset = 0.0f;
		if ( movement_services )
		{
			plant_offset = memory::read<float>( movement_services + SCHEMA( "CCSPlayer_MovementServices", "m_flBombPlantViewOffset"_hash ) );
		}
		const bool is_planting = ( plant_offset > 0.01f );

		const bool is_grenade = ( info.weapon.name == "flashbang" || info.weapon.name == "hegrenade" || 
		                          info.weapon.name == "smokegrenade" || info.weapon.name == "molotov" || 
		                          info.weapon.name == "incgrenade" || info.weapon.name == "decoy" );
		const bool pin_pulled = memory::read<bool>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_bGrenadeParametersStashed"_hash ) );

		if ( !trk.initialized )
		{
			trk.initialized = true;
			trk.on_ground = current_on_ground;
			trk.scoped = info.is_scoped;
			trk.defusing = info.is_defusing;
			trk.reloading = current_reloading;
			trk.was_walking = is_walking;
			trk.was_on_ladder = on_ladder;
			trk.was_planting = is_planting;
			trk.was_pin_pulled = ( is_grenade && pin_pulled );
			trk.last_ammo = info.weapon.ammo;
			trk.last_shots_fired = shots_fired;
			trk.last_fired_time = fired_time;
			trk.last_shot_time = shot_time;
			trk.last_zoom_level = zoom_level;
			trk.last_vel_z = vel.z;
			trk.last_weapon_ptr = info.weapon.ptr;
			trk.last_step_time = now;
			trk.last_ladder_step_time = now;
			trk.last_shot_ring_time = 0;
			trk.last_seen_time = now;
			return;
		}

		// 1. Weapon Firing / Gunshot Detection
		bool fired = false;
		if ( trk.last_ammo > 0 && info.weapon.ammo >= 0 && info.weapon.ammo < trk.last_ammo )
		{
			fired = true;
		}
		else if ( trk.last_shots_fired >= 0 && shots_fired > trk.last_shots_fired )
		{
			fired = true;
		}
		else if ( trk.last_fired_time > 0.0f && fired_time > trk.last_fired_time + 0.0001f )
		{
			fired = true;
		}
		else if ( trk.last_shot_time > 0.0f && shot_time > trk.last_shot_time + 0.0001f )
		{
			fired = true;
		}

		if ( fired && ( now - trk.last_shot_ring_time > 80 ) )
		{
			trk.last_shot_ring_time = now;
			auto shot_col = cfg.color.value;
			shot_col.r = 255; shot_col.g = 90; shot_col.b = 85;
			add_sound_ring( info.origin, shot_col, cfg.max_radius.value * 1.55f, cfg.duration.value * 1.15f, "SHOT" );
		}

		// 2. Jump Liftoff Detection
		if ( trk.on_ground && !current_on_ground && vel.z > 20.0f )
		{
			auto jump_col = cfg.color.value;
			jump_col.r = 120; jump_col.g = 215; jump_col.b = 255;
			add_sound_ring( info.origin, jump_col, cfg.max_radius.value * 1.15f, cfg.duration.value * 0.85f, "JUMP" );
		}

		// 3. Landing Detection (Regular vs Hard Land)
		if ( !trk.on_ground && current_on_ground )
		{
			if ( trk.last_vel_z < -450.0f )
			{
				auto hard_col = cfg.color.value;
				hard_col.r = 255; hard_col.g = 110; hard_col.b = 90;
				add_sound_ring( info.origin, hard_col, cfg.max_radius.value * 1.45f, cfg.duration.value * 1.0f, "HARD LAND" );
			}
			else
			{
				auto land_col = cfg.color.value;
				land_col.r = 130; land_col.g = 210; land_col.b = 255;
				add_sound_ring( info.origin, land_col, cfg.max_radius.value * 1.25f, cfg.duration.value * 0.9f, "LAND" );
			}
		}

		// 4. Footstep Cadence (with Shift Walk suppression)
		if ( current_on_ground && !is_walking && speed_2d > 75.0f )
		{
			const auto step_interval = ( speed_2d > 200.0f ) ? 310 : ( ( speed_2d > 130.0f ) ? 380 : 460 );
			if ( now - trk.last_step_time > static_cast< std::uint64_t >( step_interval ) )
			{
				trk.last_step_time = now;
				add_sound_ring( info.origin, cfg.color.value, cfg.max_radius.value, cfg.duration.value, "STEP" );
			}
		}

		// 5. Ladder Climbing Steps
		if ( on_ladder && vel.length( ) > 45.0f )
		{
			if ( now - trk.last_ladder_step_time > 360 )
			{
				trk.last_ladder_step_time = now;
				auto ladder_col = cfg.color.value;
				ladder_col.r = 160; ladder_col.g = 220; ladder_col.b = 255;
				add_sound_ring( info.origin, ladder_col, cfg.max_radius.value * 0.85f, cfg.duration.value * 0.85f, "LADDER" );
			}
		}

		// 6. Scope & Zoom Level 2
		if ( !trk.scoped && info.is_scoped )
		{
			auto scope_col = cfg.color.value;
			scope_col.r = 255; scope_col.g = 220; scope_col.b = 100;
			add_sound_ring( info.origin, scope_col, cfg.max_radius.value * 0.75f, cfg.duration.value * 0.8f, "SCOPE" );
		}
		else if ( trk.scoped && info.is_scoped && zoom_level > trk.last_zoom_level && zoom_level > 1 )
		{
			auto zoom_col = cfg.color.value;
			zoom_col.r = 255; zoom_col.g = 235; zoom_col.b = 120;
			add_sound_ring( info.origin, zoom_col, cfg.max_radius.value * 0.75f, cfg.duration.value * 0.8f, "ZOOM 2" );
		}

		// 7. Defusing Detection
		if ( !trk.defusing && info.is_defusing )
		{
			auto defuse_col = cfg.color.value;
			defuse_col.r = 90; defuse_col.g = 255; defuse_col.b = 150;
			add_sound_ring( info.origin, defuse_col, cfg.max_radius.value * 1.4f, cfg.duration.value * 1.5f, "DEFUSING" );
		}

		// 8. Bomb Planting Detection
		if ( !trk.was_planting && is_planting )
		{
			auto plant_col = cfg.color.value;
			plant_col.r = 255; plant_col.g = 95; plant_col.b = 95;
			add_sound_ring( info.origin, plant_col, cfg.max_radius.value * 1.5f, cfg.duration.value * 1.5f, "PLANTING" );
		}

		// 9. Reload Detection
		if ( !trk.reloading && current_reloading )
		{
			auto reload_col = cfg.color.value;
			reload_col.r = 255; reload_col.g = 185; reload_col.b = 90;
			add_sound_ring( info.origin, reload_col, cfg.max_radius.value * 0.95f, cfg.duration.value, "RELOAD" );
		}

		// 10. Grenade Pin Pull Detection
		if ( is_grenade && !trk.was_pin_pulled && pin_pulled )
		{
			auto pin_col = cfg.color.value;
			pin_col.r = 255; pin_col.g = 140; pin_col.b = 220;
			add_sound_ring( info.origin, pin_col, cfg.max_radius.value * 0.8f, cfg.duration.value * 0.9f, "PIN PULL" );
		}

		// 11. Weapon Switch / Draw Detection
		if ( trk.last_weapon_ptr != 0 && info.weapon.ptr != 0 && info.weapon.ptr != trk.last_weapon_ptr )
		{
			auto equip_col = cfg.color.value;
			equip_col.r = 190; equip_col.g = 190; equip_col.b = 255;
			add_sound_ring( info.origin, equip_col, cfg.max_radius.value * 0.75f, cfg.duration.value * 0.75f, "DRAW" );
		}

		// Update state for next frame
		trk.on_ground = current_on_ground;
		trk.scoped = info.is_scoped;
		trk.defusing = info.is_defusing;
		trk.reloading = current_reloading;
		trk.was_walking = is_walking;
		trk.was_on_ladder = on_ladder;
		trk.was_planting = is_planting;
		trk.was_pin_pulled = ( is_grenade && pin_pulled );
		trk.last_ammo = info.weapon.ammo;
		trk.last_shots_fired = shots_fired;
		trk.last_fired_time = fired_time;
		trk.last_shot_time = shot_time;
		trk.last_zoom_level = zoom_level;
		trk.last_vel_z = vel.z;
		trk.last_weapon_ptr = info.weapon.ptr;
		trk.last_seen_time = now;
	}

	struct active_sound_ring
	{
		math::vector3 origin{};
		float max_radius{ 75.0f };
		float current_radius{ 0.0f };
		float life{ 0.0f };
		float max_life{ 1.8f };
		xdraw::color color{ 255, 175, 235, 220 };
		std::string label{ "STEP" };
	};

	static inline std::vector<active_sound_ring> s_sound_rings{};
	static inline std::mutex s_sound_mtx{};
	static inline std::uint64_t s_last_sound_render_time{ 0 };

	void overlay::add_sound_ring( const math::vector3& origin, const xdraw::color& color, float max_radius, float duration, std::string_view label )
	{
		std::unique_lock lock( s_sound_mtx );

		// Deduplication: prevent identical overlapping rings created within 90ms at the same spot
		for ( auto& existing : s_sound_rings )
		{
			if ( existing.label == label && ( existing.origin - origin ).length_sqr( ) < 900.0f && existing.life < 0.09f )
			{
				existing.origin = origin;
				return;
			}
		}

		if ( s_sound_rings.size( ) > 64 )
		{
			s_sound_rings.erase( s_sound_rings.begin( ) );
		}

		active_sound_ring r{};
		r.origin = origin;
		r.max_radius = ( max_radius > 10.0f ) ? max_radius : 75.0f;
		r.current_radius = 5.0f;
		r.life = 0.0f;
		r.max_life = ( duration > 0.1f ) ? duration : 1.8f;
		r.color = color;
		r.label = std::string( label );
		s_sound_rings.push_back( r );
	}

	static bool get_event_player_origin( std::uintptr_t event, math::vector3& out_origin, bool enemy_only = true )
	{
		if ( !event )
		{
			return false;
		}

		const auto userid_key = cstypes::event_hash{ 0, "userid" };
		auto controller = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_controller ), event, &userid_key );
		auto pawn = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_pawn ), event, &userid_key );

		if ( !controller && !pawn )
		{
			const auto user_id_key = cstypes::event_hash{ 0, "user_id" };
			controller = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_controller ), event, &user_id_key );
			pawn = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_pawn ), event, &user_id_key );
		}

		if ( !controller && !pawn )
		{
			const auto attacker_key = cstypes::event_hash{ 0, "attacker" };
			controller = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_controller ), event, &attacker_key );
			pawn = memory::call<std::uintptr_t>( PATTERN( patterns::game_event_get_pawn ), event, &attacker_key );
		}

		const auto local = systems::g_local.get( );
		if ( !local.is_valid( ) )
		{
			return false;
		}

		if ( ( controller && controller == local.controller ) || ( pawn && pawn == local.pawn ) )
		{
			return false;
		}

		int team = 0;
		if ( controller )
		{
			team = memory::read<int>( controller + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
		}
		else if ( pawn )
		{
			team = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
		}

		if ( enemy_only && !local.is_this_other_team( team ) )
		{
			return false;
		}

		if ( !pawn && controller )
		{
			const auto pawn_handle = memory::read<std::uint32_t>( controller + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
			if ( pawn_handle )
			{
				pawn = systems::g_entities.lookup( pawn_handle );
			}
		}

		if ( !pawn )
		{
			return false;
		}

		const auto node = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !node )
		{
			return false;
		}

		out_origin = memory::read<math::vector3>( node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
		return true;
	}

	void overlay::on_player_footstep( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, true ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		add_sound_ring( origin, cfg.color.value, cfg.max_radius.value, cfg.duration.value, "STEP" );
	}

	void overlay::on_weapon_fire( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, true ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 255; col.g = 90; col.b = 85;
		add_sound_ring( origin, col, cfg.max_radius.value * 1.55f, cfg.duration.value * 1.15f, "SHOT" );
	}

	void overlay::on_weapon_reload( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, true ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 255; col.g = 185; col.b = 90;
		add_sound_ring( origin, col, cfg.max_radius.value * 0.95f, cfg.duration.value, "RELOAD" );
	}

	void overlay::on_player_jump( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, true ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 120; col.g = 215; col.b = 255;
		add_sound_ring( origin, col, cfg.max_radius.value * 1.15f, cfg.duration.value * 0.85f, "JUMP" );
	}

	void overlay::on_bomb_beginplant( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, false ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 255; col.g = 95; col.b = 95;
		add_sound_ring( origin, col, cfg.max_radius.value * 1.5f, cfg.duration.value * 1.5f, "PLANTING" );
	}

	void overlay::on_bomb_begindefuse( std::uintptr_t event )
	{
		math::vector3 origin{};
		if ( !get_event_player_origin( event, origin, true ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 90; col.g = 255; col.b = 150;
		add_sound_ring( origin, col, cfg.max_radius.value * 1.4f, cfg.duration.value * 1.5f, "DEFUSING" );
	}

	void overlay::on_grenade_detonate( std::uintptr_t event, std::string_view label )
	{
		if ( !event )
		{
			return;
		}

		const auto x = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "x", 0.0f );
		const auto y = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "y", 0.0f );
		const auto z = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "z", 0.0f );
		if ( !std::isfinite( x ) || !std::isfinite( y ) || !std::isfinite( z ) || ( x == 0.0f && y == 0.0f && z == 0.0f ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 255; col.g = 180; col.b = 80;
		add_sound_ring( { x, y, z }, col, cfg.max_radius.value * 1.4f, cfg.duration.value * 1.2f, label );
	}

	void overlay::on_grenade_bounce( std::uintptr_t event )
	{
		if ( !event )
		{
			return;
		}

		const auto x = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "x", 0.0f );
		const auto y = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "y", 0.0f );
		const auto z = memory::call<float>( PATTERN( patterns::game_event_get_float ), event, "z", 0.0f );
		if ( !std::isfinite( x ) || !std::isfinite( y ) || !std::isfinite( z ) || ( x == 0.0f && y == 0.0f && z == 0.0f ) )
		{
			return;
		}

		const auto& cfg = settings::g_esp.m_player.m_overlay[ 0 ].m_sound_esp;
		auto col = cfg.color.value;
		col.r = 255; col.g = 220; col.b = 100;
		add_sound_ring( { x, y, z }, col, cfg.max_radius.value * 0.85f, cfg.duration.value * 0.9f, "BOUNCE" );
	}

	std::vector<overlay::sound_ring_snap> overlay::get_active_sound_rings( )
	{
		std::unique_lock lock( s_sound_mtx );
		std::vector<sound_ring_snap> result;
		result.reserve( s_sound_rings.size( ) );
		for ( const auto& r : s_sound_rings )
		{
			const float progress = std::clamp( r.life / r.max_life, 0.0f, 1.0f );
			result.push_back( { r.origin, r.current_radius, r.max_radius, 1.0f - progress, r.color, r.label } );
		}
		return result;
	}

	void overlay::render_sound_rings( xdraw::draw_list& draw_list, const settings::esp::player::overlay::sound_esp& cfg )
	{
		std::unique_lock lock( s_sound_mtx );
		if ( s_sound_rings.empty( ) )
		{
			return;
		}

		const auto now = GetTickCount64( );
		if ( s_last_sound_render_time == 0 )
		{
			s_last_sound_render_time = now;
		}

		const float dt = std::clamp( static_cast<float>( now - s_last_sound_render_time ) * 0.001f, 0.001f, 0.1f );
		s_last_sound_render_time = now;

		constexpr int segments = 24;
		constexpr float step_angle = 6.2831853f / static_cast<float>( segments );

		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto sw = static_cast< float >( screen_w );
		const auto sh = static_cast< float >( screen_h );
		const auto center_x = sw * 0.5f;
		const auto center_y = sh * 0.5f;

		for ( auto it = s_sound_rings.begin( ); it != s_sound_rings.end( ); )
		{
			it->life += dt;
			if ( it->life >= it->max_life )
			{
				it = s_sound_rings.erase( it );
				continue;
			}

			const float progress = std::clamp( it->life / it->max_life, 0.0f, 1.0f );
			const float ease_radius = 1.0f - std::pow( 1.0f - progress, 3.0f );
			it->current_radius = 5.0f + ( it->max_radius - 5.0f ) * ease_radius;

			const float alpha_fade = 1.0f - progress;
			auto col = it->color;
			col.a = static_cast<std::uint8_t>( static_cast<float>( col.a ) * alpha_fade );

			// 1. Primary ground sound ring
			math::vector2 prev_screen{};
			bool has_prev = false;

			for ( int i = 0; i <= segments; ++i )
			{
				const float ang = step_angle * static_cast<float>( i % segments );
				const auto world_pt = it->origin + math::vector3{ std::cos( ang ) * it->current_radius, std::sin( ang ) * it->current_radius, 2.0f };
				const auto cur_screen = systems::g_view.project( world_pt );

				if ( systems::g_view.projection_valid( cur_screen ) )
				{
					if ( has_prev )
					{
						draw_list.line( prev_screen.x, prev_screen.y, cur_screen.x, cur_screen.y, col, 1.5f );
					}
					prev_screen = cur_screen;
					has_prev = true;
				}
				else
				{
					has_prev = false;
				}
			}

			// 2. Secondary inner echo ripple
			if ( it->current_radius > 15.0f )
			{
				const float inner_radius = it->current_radius * 0.55f;
				auto inner_col = col;
				inner_col.a = static_cast<std::uint8_t>( static_cast<float>( inner_col.a ) * 0.45f );

				prev_screen = {};
				has_prev = false;

				for ( int i = 0; i <= segments; ++i )
				{
					const float ang = step_angle * static_cast<float>( i % segments );
					const auto world_pt = it->origin + math::vector3{ std::cos( ang ) * inner_radius, std::sin( ang ) * inner_radius, 2.0f };
					const auto cur_screen = systems::g_view.project( world_pt );

					if ( systems::g_view.projection_valid( cur_screen ) )
					{
						if ( has_prev )
						{
							draw_list.line( prev_screen.x, prev_screen.y, cur_screen.x, cur_screen.y, inner_col, 1.0f );
						}
						prev_screen = cur_screen;
						has_prev = true;
					}
					else
					{
						has_prev = false;
					}
				}
			}

			// 3. World center tag
			const auto center_proj = systems::g_view.project_full( it->origin + math::vector3{ 0.0f, 0.0f, 4.0f } );
			if ( center_proj.on_screen && center_proj.w > 0.0f )
			{
				if ( cfg.show_label.value && !it->label.empty( ) )
				{
					draw_list.circle_filled( center_proj.screen.x, center_proj.screen.y, 2.5f, col );
					xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );
					const auto [tw, th] = xdraw::measure_text( it->label );
					draw_list.text( center_proj.screen.x - tw * 0.5f, center_proj.screen.y - th - 4.0f, it->label, col, xdraw::text_style::outlined );
					xdraw::pop_font( );
				}
			}
			else if ( cfg.show_oof.value && systems::g_frame_data.valid( ) )
			{
				// 4. Off-Screen Directional Sound Indicator
				const auto to_sound = it->origin - systems::g_frame_data.origin( );
				if ( to_sound.length_2d( ) >= 1.0f )
				{
					const auto sound_yaw = std::atan2f( to_sound.y, to_sound.x );
					const auto view_yaw = math::helpers::deg_to_rad( systems::g_input.get_view_angles( ).y );
					const auto angle = view_yaw - sound_yaw - std::numbers::pi_v<float> * 0.5f;

					const auto rx = sw * 0.38f;
					const auto ry = sh * 0.38f;
					const auto tip_x = center_x + std::cosf( angle ) * rx;
					const auto tip_y = center_y + std::sinf( angle ) * ry;

					const auto fx = std::cosf( angle );
					const auto fy = std::sinf( angle );
					const auto px = -fy;
					const auto py = fx;
					const auto base_x = tip_x - fx * 11.0f;
					const auto base_y = tip_y - fy * 11.0f;

					const auto bl_x = base_x - px * 5.5f;
					const auto bl_y = base_y - py * 5.5f;
					const auto br_x = base_x + px * 5.5f;
					const auto br_y = base_y + py * 5.5f;

					draw_list.triangle_filled( tip_x, tip_y, bl_x, bl_y, br_x, br_y, col );

					if ( cfg.show_label.value && !it->label.empty( ) )
					{
						xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );
						const auto dist_m = to_sound.length( ) * 0.01905f;
						char tag_buf[ 32 ]{};
						std::snprintf( tag_buf, sizeof( tag_buf ), "%s %.0fm", it->label.c_str( ), dist_m );
						const auto [tw, th] = xdraw::measure_text( tag_buf );
						const auto tag_x = tip_x - fx * 20.0f - tw * 0.5f;
						const auto tag_y = tip_y - fy * 20.0f - th * 0.5f;
						draw_list.text( tag_x, tag_y, tag_buf, col, xdraw::text_style::outlined );
						xdraw::pop_font( );
					}
				}
			}

			++it;
		}
	}

	void overlay::add_snap_lines( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const settings::esp::player::overlay::snap_lines& cfg, bool visible )
	{
		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		math::vector2 start{};
		if ( cfg.origin.value == 0 )
		{
			start = { static_cast<float>( screen_w ) * 0.5f, static_cast<float>( screen_h ) };
		}
		else
		{
			start = { static_cast<float>( screen_w ) * 0.5f, static_cast<float>( screen_h ) * 0.5f };
		}

		const auto target = math::vector2{ ( bounds.min.x + bounds.max.x ) * 0.5f, bounds.max.y };
		auto col = cfg.color.value;
		if ( !visible )
		{
			col.a = static_cast<std::uint8_t>( col.a * 0.5f );
		}

		draw_list.line( start.x, start.y, target.x, target.y, col, 1.2f );
	}

	void overlay::add_view_ray( xdraw::draw_list& draw_list, const info& info, const settings::esp::player::overlay::view_ray& cfg )
	{
		const auto head_bone = info.bones[ 6 ];
		const auto start_pos = head_bone.position;

		const auto eye_angles = memory::read<math::vector3>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_angEyeAngles"_hash ) );
		math::vector3 forward{};
		math::helpers::angle_vectors_left( eye_angles, &forward );

		const auto end_pos = start_pos + forward * cfg.length.value;

		const auto screen_start = systems::g_view.project( start_pos );
		const auto screen_end = systems::g_view.project( end_pos );
		if ( systems::g_view.projection_valid( screen_start ) && systems::g_view.projection_valid( screen_end ) )
		{
			draw_list.line( screen_start.x, screen_start.y, screen_end.x, screen_end.y, cfg.color.value, 1.5f );
			draw_list.circle_filled( screen_end.x, screen_end.y, 2.5f, cfg.color.value );
		}
	}


	void overlay::add_box( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const settings::esp::player::overlay::box& cfg, bool visible )
	{
		const auto& color = visible ? cfg.visible_color : cfg.occluded_color;

		const auto x = std::floorf( bounds.min.x );
		const auto y = std::floorf( bounds.min.y );
		const auto w = std::floorf( bounds.max.x - bounds.min.x );
		const auto h = std::floorf( bounds.max.y - bounds.min.y );

		if ( cfg.fill.value )
		{
			constexpr auto edge_alpha{ 0.5f };
			constexpr auto center_alpha{ 0.08f };
			constexpr auto center_brightness{ 0.4f };
			constexpr auto desaturation{ 0.7f };

			const auto r = color.value.r / 255.0f;
			const auto g = color.value.g / 255.0f;
			const auto b = color.value.b / 255.0f;
			const auto avg = ( r + g + b ) / 3.0f;

			const auto edge_r = r * desaturation + avg * ( 1.0f - desaturation );
			const auto edge_g = g * desaturation + avg * ( 1.0f - desaturation );
			const auto edge_b = b * desaturation + avg * ( 1.0f - desaturation );
			const auto edge_color = xdraw::color( static_cast< std::uint8_t >( edge_r * 255 ), static_cast< std::uint8_t >( edge_g * 255 ), static_cast< std::uint8_t >( edge_b * 255 ), static_cast< std::uint8_t >( 255 * edge_alpha ) );
			const auto center_color = xdraw::color( static_cast< std::uint8_t >( edge_r * 255 * center_brightness ), static_cast< std::uint8_t >( edge_g * 255 * center_brightness ), static_cast< std::uint8_t >( edge_b * 255 * center_brightness ), static_cast< std::uint8_t >( 255 * center_alpha ) );

			const auto mid_y = y + h * 0.5f;

			draw_list.rect_filled_gradient( x + 1, y + 1, w - 2, mid_y - y - 1, edge_color, edge_color, center_color, center_color );
			draw_list.rect_filled_gradient( x + 1, mid_y, w - 2, y + h - mid_y - 1, center_color, center_color, edge_color, edge_color );
		}

		if ( cfg.style == settings::esp::player::overlay::box::style_type::full )
		{
			auto draw_full_rect = [ & ]( float rx, float ry, float rw, float rh, const xdraw::color& col, float thickness )
				{
					const auto t = std::clamp( thickness, 0.0f, std::min( rw, rh ) * 0.5f );

					if ( t <= 0.0f )
					{
						return;
					}

					draw_list.ensure_cmd( nullptr );

					auto v = draw_list.emit_vtx( rx, ry, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx, ry + t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh, 0, 0, col );
					draw_list.emit_vtx( rx, ry + rh, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx, ry + rh - t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx + rw - t, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw - t, ry + rh - t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );
				};

			if ( cfg.outline.value )
			{
				draw_full_rect( x - 1, y - 1, w + 2, h + 2, xdraw::color( 0, 0, 0, 180 ), 1.0f );
				draw_full_rect( x, y, w, h, xdraw::color( 0, 0, 0, 200 ), 2.0f );
			}

			draw_full_rect( x, y, w, h, color, 1.0f );
		}
		else
		{
			const auto corner = std::min( cfg.corner_length.value, std::min( w, h ) * 0.4f );

			auto draw_cornered_rect = [ & ]( float rx, float ry, float rw, float rh, const xdraw::color& col, float corner_len, float thickness )
				{
					const auto max_corner = std::min( rw, rh ) * 0.5f;
					const auto cl = std::min( corner_len, max_corner );
					const auto t = std::clamp( thickness, 0.0f, std::min( rw, rh ) * 0.5f );

					if ( t <= 0.0f )
					{
						return;
					}

					draw_list.ensure_cmd( nullptr );

					auto v = draw_list.emit_vtx( rx, ry, 0, 0, col );
					draw_list.emit_vtx( rx + cl, ry, 0, 0, col );
					draw_list.emit_vtx( rx + cl, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx, ry + t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + cl, 0, 0, col );
					draw_list.emit_vtx( rx, ry + cl, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx + rw - cl, ry, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + rw - cl, ry + t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx + rw - t, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + cl, 0, 0, col );
					draw_list.emit_vtx( rx + rw - t, ry + cl, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx + rw - t, ry + rh - cl, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh - cl, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw - t, ry + rh - t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx + rw - cl, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + rw, ry + rh, 0, 0, col );
					draw_list.emit_vtx( rx + rw - cl, ry + rh, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx, ry + rh - cl, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + rh - cl, 0, 0, col );
					draw_list.emit_vtx( rx + t, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx, ry + rh - t, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );

					v = draw_list.emit_vtx( rx, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + cl, ry + rh - t, 0, 0, col );
					draw_list.emit_vtx( rx + cl, ry + rh, 0, 0, col );
					draw_list.emit_vtx( rx, ry + rh, 0, 0, col );
					draw_list.emit_quad( v, v + 1, v + 2, v + 3 );
				};

			if ( cfg.outline )
			{
				draw_cornered_rect( x - 1, y - 1, w + 2, h + 2, xdraw::color( 0, 0, 0, 180 ), corner + 1, 1.0f );
				draw_cornered_rect( x, y, w, h, xdraw::color( 0, 0, 0, 200 ), corner, 2.0f );
			}

			draw_cornered_rect( x, y, w, h, color, corner, 1.0f );
		}
	}

	void overlay::add_skeleton( xdraw::draw_list& draw_list, const info& info, const settings::esp::player::overlay::skeleton& cfg, bool visible, const systems::local::snapshot& local )
	{
		const auto& color = visible ? cfg.visible_color : cfg.occluded_color;

		constexpr auto none{ ~0u };
		constexpr std::array chains
		{
			std::array{ cstypes::bone_ids::head, cstypes::bone_ids::neck, cstypes::bone_ids::spine_4, cstypes::bone_ids::spine_3, cstypes::bone_ids::spine_2, cstypes::bone_ids::spine_1, cstypes::bone_ids::pelvis },
			std::array{ cstypes::bone_ids::left_hand, cstypes::bone_ids::left_elbow, cstypes::bone_ids::left_shoulder, cstypes::bone_ids::left_clavicle, cstypes::bone_ids::spine_4, none, none },
			std::array{ cstypes::bone_ids::right_hand, cstypes::bone_ids::right_elbow, cstypes::bone_ids::right_shoulder, cstypes::bone_ids::right_clavicle, cstypes::bone_ids::spine_4, none, none },
			std::array{ cstypes::bone_ids::left_foot, cstypes::bone_ids::left_knee, cstypes::bone_ids::left_hip, cstypes::bone_ids::pelvis, none, none, none },
			std::array{ cstypes::bone_ids::right_foot, cstypes::bone_ids::right_knee, cstypes::bone_ids::right_hip, cstypes::bone_ids::pelvis, none, none, none },
		};

		std::array<math::vector3, 27> positions{};
		auto valid_bones{ false };

		if ( cfg.type == settings::esp::player::overlay::skeleton::mode::backtrack && local.is_alive )
		{
			const auto record = combat::g_shared.lc( ).get_oldest_was_valid_visual( info.pawn );
			if ( record )
			{
				if ( record->origin.distance( info.origin ) >= 0.25f )
				{
					for ( auto i = 0; i < 27; ++i )
					{
						positions[ i ] = record->bones[ i ].position;
					}

					valid_bones = true;
				}
			}
		}

		if ( !valid_bones )
		{
			if ( cfg.type == settings::esp::player::overlay::skeleton::mode::backtrack )
			{
				return;
			}

			for ( auto i = 0ull; i < info.bones.size( ) && i < 27; ++i )
			{
				positions[ i ] = info.bones[ i ].position;
			}
		}

		constexpr auto step{ 0.08f };

		const auto flush_segment = [ & ]( std::vector<math::vector2>& screen_points )
			{
				if ( screen_points.size( ) < 2 )
				{
					screen_points.clear( );
					return;
				}

				screen_points.insert( screen_points.begin( ), screen_points.front( ) );
				screen_points.push_back( screen_points.back( ) );

				std::vector<float> spline_points;
				spline_points.reserve( ( screen_points.size( ) * static_cast< std::size_t >( 1.f / step ) + 1 ) * 2 );

				for ( auto i = 0ull; i + 3 < screen_points.size( ); ++i )
				{
					const auto& p0 = screen_points[ i ];
					const auto& p1 = screen_points[ i + 1 ];
					const auto& p2 = screen_points[ i + 2 ];
					const auto& p3 = screen_points[ i + 3 ];

					for ( float t = 0.f; t <= 1.f; t += step )
					{
						const auto t2 = t * t;
						const auto t3 = t2 * t;

						spline_points.push_back( 0.5f * ( ( 2.f * p1.x ) + ( -p0.x + p2.x ) * t + ( 2.f * p0.x - 5.f * p1.x + 4.f * p2.x - p3.x ) * t2 + ( -p0.x + 3.f * p1.x - 3.f * p2.x + p3.x ) * t3 ) );
						spline_points.push_back( 0.5f * ( ( 2.f * p1.y ) + ( -p0.y + p2.y ) * t + ( 2.f * p0.y - 5.f * p1.y + 4.f * p2.y - p3.y ) * t2 + ( -p0.y + 3.f * p1.y - 3.f * p2.y + p3.y ) * t3 ) );
					}
				}

				if ( spline_points.size( ) >= 4 )
				{
					draw_list.polyline( spline_points, color, false, cfg.thickness );
				}

				screen_points.clear( );
			};

		for ( const auto& chain : chains )
		{
			std::vector<math::vector2> screen_points;
			screen_points.reserve( chain.size( ) );

			for ( const auto& b : chain )
			{
				if ( b == none )
				{
					break;
				}

				const auto idx = static_cast< std::size_t >( b );
				if ( idx >= 28 || positions[ idx ].length_sqr( ) < 1.0f )
				{
					flush_segment( screen_points );
					continue;
				}

				const auto projected = systems::g_view.project( positions[ idx ] );
				if ( !systems::g_view.projection_valid( projected ) )
				{
					flush_segment( screen_points );
					continue;
				}

				math::vector2 pt{ projected.x, projected.y };

				if ( !screen_points.empty( ) )
				{
					const auto& prev = screen_points.back( );
					const auto dx = pt.x - prev.x;
					const auto dy = pt.y - prev.y;

					if ( dx * dx + dy * dy > 250000.0f )
					{
						flush_segment( screen_points );
					}
				}

				screen_points.push_back( pt );
			}

			flush_segment( screen_points );
		}

		if ( cfg.head_dot )
		{
			const auto head_idx = static_cast< std::size_t >( cstypes::bone_ids::head );
			if ( head_idx < positions.size( ) && positions[ head_idx ].length_sqr( ) >= 1.0f )
			{
				const auto projected = systems::g_view.project( positions[ head_idx ] );
				if ( systems::g_view.projection_valid( projected ) )
				{
					draw_list.circle_filled( projected.x, projected.y, 3.0f, color );
				}
			}
		}
	}

	void overlay::add_health_bar( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const info& info, const settings::esp::player::overlay::health_bar& cfg, draw_offsets& offsets )
	{
		auto& anim = this->m_animations[ info.controller ];

		constexpr auto bar_size = 3.5f, padding = 4.0f;
		const auto clamped_health = std::clamp( info.health, 0, 100 );
		const auto target_fraction = clamped_health / 100.0f;

		if ( !anim.initialized || ( target_fraction - anim.health.value( ) > 0.5f ) )
		{
			anim.health.snap( target_fraction );
			anim.initialized = true;
			anim.ghost_health = target_fraction;
			anim.last_health = target_fraction;
		}
		else
		{
			if ( target_fraction < anim.last_health )
			{
				anim.ghost_health = std::max( anim.ghost_health, anim.last_health );
			}
			anim.last_health = target_fraction;
			anim.health.set_target( target_fraction );
			anim.health.update( );

			if ( anim.ghost_health > anim.health.value( ) )
			{
				const auto dt = xdraw::delta_time( );
				anim.ghost_health = std::max( anim.health.value( ), anim.ghost_health - dt * 0.40f );
			}
			else
			{
				anim.ghost_health = anim.health.value( );
			}
		}

		const auto fraction = anim.health.value( );
		const auto outline_size = cfg.outline_setting.value ? 1.0f : 0.0f;
		const auto vertical = cfg.position == settings::esp::player::overlay::health_bar::position_type::left;

		const auto bar_w = vertical ? bar_size : std::floorf( bounds.width( ) );
		const auto bar_h = vertical ? std::floorf( bounds.height( ) ) : bar_size;
		const auto filled = ( clamped_health >= 100 ) ? ( vertical ? bar_h : bar_w ) : std::floorf( ( vertical ? bar_h : bar_w ) * fraction );
		const auto ghost_filled = std::clamp( std::floorf( ( vertical ? bar_h : bar_w ) * anim.ghost_health ), filled, vertical ? bar_h : bar_w );

		const auto x = [ & ]( )
			{
				if ( cfg.position == settings::esp::player::overlay::health_bar::position_type::left )
				{
					return std::floorf( bounds.min.x - bar_size - padding - offsets.left - outline_size );
				}

				return std::floorf( bounds.min.x );
			}( );

		const auto y = [ & ]( )
			{
				switch ( cfg.position )
				{
				case settings::esp::player::overlay::health_bar::position_type::left: return std::floorf( bounds.min.y );
				case settings::esp::player::overlay::health_bar::position_type::top: return std::floorf( bounds.min.y - bar_size - padding - offsets.top - outline_size );
				case settings::esp::player::overlay::health_bar::position_type::bottom: return std::floorf( bounds.max.y + padding + offsets.bottom + outline_size );
				}
				return 0.0f;
			}( );

		switch ( cfg.position )
		{
		case settings::esp::player::overlay::health_bar::position_type::left: offsets.left += bar_size + padding + ( outline_size * 2.0f );
			break;
		case settings::esp::player::overlay::health_bar::position_type::top: offsets.top += bar_size + padding + ( outline_size * 2.0f );
			break;
		case settings::esp::player::overlay::health_bar::position_type::bottom: offsets.bottom += bar_size + padding + ( outline_size * 2.0f );
			break;
		}

		if ( cfg.glow )
		{
			auto& glow = xdraw::get_glow( );
			const auto gc = cfg.glow_color.value;
			const auto glow_a = static_cast< std::uint8_t >( static_cast< float >( gc.a ) * cfg.glow_strength );
			const auto glow_col = xdraw::color{ gc.r, gc.g, gc.b, glow_a };

			for ( auto t = 0; t < 3; ++t )
			{
				const auto expand = static_cast< float >( t ) * 0.75f;
				glow.rect_filled( x - expand, y - expand, bar_w + expand * 2.0f, bar_h + expand * 2.0f, glow_col );
			}
		}

		if ( cfg.outline_setting.value )
		{
			draw_list.rect_filled( x - 1, y - 1, bar_w + 2, bar_h + 2, cfg.outline_color );
		}

		draw_list.rect_filled( x, y, bar_w, bar_h, cfg.background_color );

		if ( cfg.segmented.value )
		{
			// Segmented battery. Cells use fractional sizes so they always fit
			// exactly (integer flooring left leftover/overlapping pixels, which
			// is what made short bars look glitchy).
			constexpr int k_cell_count = 10;
			constexpr float k_cell_gap = 1.0f;
			const auto total_gap = static_cast< float >( k_cell_count - 1 ) * k_cell_gap;

			const auto cell_colour = [ & ]( int c ) -> xdraw::color
				{
					return cfg.gradient.value
						? ( ( c >= k_cell_count / 2 ) ? cfg.full_color.value : cfg.low_color.value )
						: cfg.full_color.value;
				};

			if ( vertical )
			{
				const auto cell_h = std::max( 1.0f, ( bar_h - total_gap ) / static_cast< float >( k_cell_count ) );

				for ( int c = 0; c < k_cell_count; ++c )
				{
					const auto cell_y = y + bar_h - ( static_cast< float >( c + 1 ) * cell_h + static_cast< float >( c ) * k_cell_gap );
					const auto cell_thresh = static_cast< float >( c + 1 ) / static_cast< float >( k_cell_count );

					if ( fraction >= cell_thresh - 0.05f )
					{
						draw_list.rect_filled( x, cell_y, bar_w, cell_h + 0.5f, cell_colour( c ) );
					}
					else if ( cfg.damage_drop.value && anim.ghost_health >= cell_thresh - 0.05f )
					{
						draw_list.rect_filled( x, cell_y, bar_w, cell_h + 0.5f, cfg.damage_drop_color );
					}
				}
			}
			else
			{
				const auto cell_w = std::max( 1.0f, ( bar_w - total_gap ) / static_cast< float >( k_cell_count ) );

				for ( int c = 0; c < k_cell_count; ++c )
				{
					const auto cell_x = x + static_cast< float >( c ) * ( cell_w + k_cell_gap );
					const auto cell_thresh = static_cast< float >( c + 1 ) / static_cast< float >( k_cell_count );

					if ( fraction >= cell_thresh - 0.05f )
					{
						draw_list.rect_filled( cell_x, y, cell_w + 0.5f, bar_h, cell_colour( c ) );
					}
					else if ( cfg.damage_drop.value && anim.ghost_health >= cell_thresh - 0.05f )
					{
						draw_list.rect_filled( cell_x, y, cell_w + 0.5f, bar_h, cfg.damage_drop_color );
					}
				}
			}
		}
		else
		{
			// Damage drop decaying ghost bar
			if ( cfg.damage_drop.value && ghost_filled > filled )
			{
				if ( vertical )
				{
					draw_list.rect_filled( x, y + bar_h - ghost_filled, bar_w, ghost_filled - filled, cfg.damage_drop_color );
				}
				else
				{
					draw_list.rect_filled( x + filled, y, ghost_filled - filled, bar_h, cfg.damage_drop_color );
				}
			}

			if ( filled > 0 )
			{
				if ( cfg.gradient )
				{
					if ( vertical )
					{
						draw_list.rect_filled_gradient( x, y + bar_h - filled, bar_w, filled, cfg.full_color, cfg.full_color, cfg.low_color, cfg.low_color );
					}
					else
					{
						draw_list.rect_filled_gradient( x, y, filled, bar_h, cfg.low_color, cfg.full_color, cfg.full_color, cfg.low_color );
					}
				}
				else
				{
					if ( vertical )
					{
						draw_list.rect_filled( x, y + bar_h - filled, bar_w, filled, cfg.full_color );
					}
					else
					{
						draw_list.rect_filled( x, y, filled, bar_h, cfg.full_color );
					}
				}
			}
		}

		if ( cfg.show_value && clamped_health < 100 )
		{
			xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );

			const auto text = std::to_string( clamped_health );
			const auto [text_w, text_h] = xdraw::measure_text( text );
			const auto text_x = std::floorf( x + ( bar_w * 0.5f ) - ( text_w * 0.5f ) );
			const auto text_y = vertical ? std::floorf( y + bar_h - filled - text_h - 2.0f ) : std::floorf( y - text_h - 2.0f );

			draw_list.text( text_x, text_y, text, cfg.text_color, xdraw::text_style::outlined );
			xdraw::pop_font( );
		}
	}

	void overlay::add_ammo_bar( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const info& info, const settings::esp::player::overlay::ammo_bar& cfg, draw_offsets& offsets )
	{
		if ( info.weapon.max_ammo <= 0 )
		{
			return;
		}

		auto& anim = this->m_animations[ info.controller ];

		constexpr auto bar_size = 3.5f, padding = 4.0f;
		const auto clamped_ammo = std::clamp( info.weapon.ammo, 0, info.weapon.max_ammo );
		const auto target_fraction = static_cast< float >( clamped_ammo ) / info.weapon.max_ammo;

		if ( !anim.initialized || ( target_fraction - anim.ammo.value( ) > 0.5f ) )
		{
			anim.ammo.snap( target_fraction );
			anim.initialized = true;
		}
		else
		{
			anim.ammo.set_target( target_fraction );
			anim.ammo.update( );
		}

		const auto fraction = anim.ammo.value( );
		const auto outline_size = cfg.outline_setting.value ? 1.0f : 0.0f;
		const auto vertical = cfg.position == settings::esp::player::overlay::ammo_bar::position_type::left;

		const auto bar_w = vertical ? bar_size : std::floorf( bounds.width( ) );
		const auto bar_h = vertical ? std::floorf( bounds.height( ) ) : bar_size;
		const auto filled = ( clamped_ammo == info.weapon.max_ammo ) ? ( vertical ? bar_h : bar_w ) : std::floorf( ( vertical ? bar_h : bar_w ) * fraction );

		const auto x = [ & ]( )
			{
				if ( cfg.position == settings::esp::player::overlay::ammo_bar::position_type::left )
				{
					return std::floorf( bounds.min.x - bar_size - padding - offsets.left - outline_size );
				}

				return std::floorf( bounds.min.x );
			}( );

		const auto y = [ & ]( )
			{
				switch ( cfg.position )
				{
				case settings::esp::player::overlay::ammo_bar::position_type::left: return std::floorf( bounds.min.y );
				case settings::esp::player::overlay::ammo_bar::position_type::top: return std::floorf( bounds.min.y - bar_size - padding - offsets.top - outline_size );
				case settings::esp::player::overlay::ammo_bar::position_type::bottom: return std::floorf( bounds.max.y + padding + offsets.bottom + outline_size );
				}
				return 0.0f;
			}( );

		switch ( cfg.position )
		{
		case settings::esp::player::overlay::ammo_bar::position_type::left:
			offsets.left += bar_size + padding + ( outline_size * 2.0f );
			break;
		case settings::esp::player::overlay::ammo_bar::position_type::top:
			offsets.top += bar_size + padding + ( outline_size * 2.0f );
			break;
		case settings::esp::player::overlay::ammo_bar::position_type::bottom:
			offsets.bottom += bar_size + padding + ( outline_size * 2.0f );
			break;
		}

		if ( cfg.glow )
		{
			auto& glow = xdraw::get_glow( );
			const auto gc = cfg.glow_color.value;
			const auto glow_a = static_cast< std::uint8_t >( static_cast< float >( gc.a ) * cfg.glow_strength );
			const auto glow_col = xdraw::color{ gc.r, gc.g, gc.b, glow_a };

			for ( auto t = 0; t < 3; ++t )
			{
				const auto expand = static_cast< float >( t ) * 0.75f;
				glow.rect_filled( x - expand, y - expand, bar_w + expand * 2.0f, bar_h + expand * 2.0f, glow_col );
			}
		}

		if ( cfg.outline_setting.value )
		{
			draw_list.rect_filled( x - 1, y - 1, bar_w + 2, bar_h + 2, cfg.outline_color );
		}

		draw_list.rect_filled( x, y, bar_w, bar_h, cfg.background_color );

		if ( filled > 0 )
		{
			if ( cfg.gradient )
			{
				if ( vertical )
				{
					draw_list.rect_filled_gradient( x, y + bar_h - filled, bar_w, filled, cfg.full_color, cfg.full_color, cfg.low_color, cfg.low_color );
				}
				else
				{
					draw_list.rect_filled_gradient( x, y, filled, bar_h, cfg.low_color, cfg.full_color, cfg.full_color, cfg.low_color );
				}
			}
			else
			{
				if ( vertical )
				{
					draw_list.rect_filled( x, y + bar_h - filled, bar_w, filled, cfg.full_color );
				}
				else
				{
					draw_list.rect_filled( x, y, filled, bar_h, cfg.full_color );
				}
			}
		}

		if ( cfg.show_value )
		{
			xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );

			const auto text = std::format( "{}/{}", clamped_ammo, info.weapon.max_ammo );
			const auto [text_w, text_h] = xdraw::measure_text( text );
			const auto text_x = std::floorf( x + ( bar_w * 0.5f ) - ( text_w * 0.5f ) );
			const auto text_y = vertical ? std::floorf( y + bar_h - filled - text_h - 2.0f ) : std::floorf( y + bar_h + 2.0f );

			draw_list.text( text_x, text_y, text, cfg.text_color, xdraw::text_style::outlined );
			xdraw::pop_font( );
		}
	}

	void overlay::add_name( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const info& info, const settings::esp::player::overlay::name& cfg, draw_offsets& offsets )
	{
		using name_font = settings::esp::player::overlay::name::name_font;
		using position = settings::esp::player::overlay::name::name_position;

		const auto& theme = settings::g_cheat.m_theme;

		auto* font = rendering::g_fonts.inter_medium[ rendering::fonts::size::big ];
		switch ( cfg.font.value )
		{
		case name_font::bold:  font = rendering::g_fonts.inter_bold[ rendering::fonts::size::big ]; break;
		case name_font::pixel: font = rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::big ]; break;
		default: break;
		}
		(void)cfg.size; // size is conveyed by the selected font size tier above

		xdraw::push_font( font );

		// Build the displayed label: optional [id] prefix, optional distance.
		auto label = std::string{ info.name };
		if ( cfg.show_id.value )
		{
			if ( const auto id_offset = SCHEMA( "C_BasePlayerController", "m_iUserID"_hash ) )
			{
				const auto user_id = memory::read<int>( info.controller + id_offset );
				label = std::format( "[{}] {}", user_id, info.name );
			}
		}

		// Distance label + opacity ramp by distance. `info.distance` is the
		// precomputed world distance for this player; beyond fade_end the name
		// fades out (acts as the max draw distance), and it is fully opaque
		// inside fade_start.
		float alpha_mul = 1.0f;
		const auto dist = info.distance;
		if ( cfg.show_distance.value )
		{
			label = std::format( "{} :{:.0f}m", label, dist / 100.0f );
		}

		const auto start = cfg.fade_start.value;
		const auto end = cfg.fade_end.value;
		if ( end > start )
		{
			alpha_mul = std::clamp( ( end - dist ) / ( end - start ), 0.0f, 1.0f );
		}

		const auto [text_w, text_h] = xdraw::measure_text( label, font );
		const auto text_x = std::floorf( bounds.min.x + ( bounds.width( ) * 0.5f ) - ( text_w * 0.5f ) );
		float text_y{};
		if ( cfg.position.value == position::inline_top )
		{
			text_y = std::floorf( bounds.min.y + 2.0f + offsets.top );
		}
		else
		{
			text_y = std::floorf( bounds.min.y - text_h - 2.0f - offsets.top );
		}

		// Theme colors, falling back to the per-element colors when the theme
		// override is disabled.
		const auto t_text = theme.effective( settings::theme::role::text );
		const auto t_bg = theme.effective( settings::theme::role::background );
		const auto t_accent = theme.effective( settings::theme::role::accent );
		const auto col_text = cfg.use_theme_color.value
			? xdraw::color{ ( std::uint8_t )( t_text.r * 255 ), ( std::uint8_t )( t_text.g * 255 ), ( std::uint8_t )( t_text.b * 255 ), ( std::uint8_t )( t_text.a * 255 * alpha_mul ) }
			: xdraw::color{ cfg.color.value.r, cfg.color.value.g, cfg.color.value.b, ( std::uint8_t )( cfg.color.value.a * alpha_mul ) };
		const auto col_bg = cfg.use_theme_background.value
			? xdraw::color{ ( std::uint8_t )( t_bg.r * 255 ), ( std::uint8_t )( t_bg.g * 255 ), ( std::uint8_t )( t_bg.b * 255 ), ( std::uint8_t )( t_bg.a * 255 * alpha_mul ) }
			: xdraw::color{ cfg.background_color.value.r, cfg.background_color.value.g, cfg.background_color.value.b, ( std::uint8_t )( cfg.background_color.value.a * alpha_mul ) };
		const auto col_accent = xdraw::color{ ( std::uint8_t )( t_accent.r * 255 ), ( std::uint8_t )( t_accent.g * 255 ), ( std::uint8_t )( t_accent.b * 255 ), ( std::uint8_t )( t_accent.a * 255 * alpha_mul ) };

		if ( cfg.background.value )
		{
			draw_list.rect_filled( text_x - 3.0f, text_y - 1.0f, text_w + 6.0f, text_h + 2.0f, col_bg );
		}

		if ( cfg.draw_box.value )
		{
			draw_list.rect_filled( text_x - 4.0f, text_y - 2.0f, text_w + 8.0f, text_h + 4.0f, col_bg );
			draw_list.rect( text_x - 4.0f, text_y - 2.0f, text_w + 8.0f, text_h + 4.0f, col_accent, xdraw::corner_radius{ 2.0f }, 1.0f );
		}

		const auto style = cfg.outline.value ? xdraw::text_style::outlined : xdraw::text_style::normal;
		draw_list.text( text_x, text_y, label, col_text, style, font );
		offsets.top += text_h + 2.0f;

		xdraw::pop_font( );
	}

	void overlay::add_weapon( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const info& info, const settings::esp::player::overlay::weapon& cfg, draw_offsets& offsets )
	{
		const auto show_icon = cfg.display == settings::esp::player::overlay::weapon::display_type::icon || cfg.display == settings::esp::player::overlay::weapon::display_type::text_and_icon;
		const auto show_text = cfg.display == settings::esp::player::overlay::weapon::display_type::text || cfg.display == settings::esp::player::overlay::weapon::display_type::text_and_icon;
		auto total_height{ 0.0f };

		if ( show_icon )
		{
			const auto icon_name = ( info.weapon.name == "knife_ct" || info.weapon.name == "knife_t" ) ? std::string{ "knife" } : info.weapon.name;
			const auto ico = systems::g_icons.get( icon_name, 0.35f );

			if ( ico && ico->texture )
			{
				const auto icon_scale = std::max( 0.1f, cfg.icon_scale.value );
				const auto iw = static_cast< float >( ico->width ) * icon_scale;
				const auto ih = static_cast< float >( ico->height ) * icon_scale;
				const auto ix = std::floorf( bounds.min.x + ( bounds.width( ) * 0.5f ) - ( iw * 0.5f ) );
				const auto iy = std::floorf( bounds.max.y + 2.0f + offsets.bottom + total_height );
				constexpr auto outline = xdraw::color{ 0, 0, 0, 255 };

				draw_list.image( ix - 1.0f, iy, iw, ih, ico->texture.Get( ), outline );
				draw_list.image( ix + 1.0f, iy, iw, ih, ico->texture.Get( ), outline );
				draw_list.image( ix, iy - 1.0f, iw, ih, ico->texture.Get( ), outline );
				draw_list.image( ix, iy + 1.0f, iw, ih, ico->texture.Get( ), outline );
				draw_list.image( ix, iy, iw, ih, ico->texture.Get( ), cfg.icon_color );

				total_height += ih + 2.0f;
			}
		}

		if ( show_text )
		{
			xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );

			const auto [text_w, text_h] = xdraw::measure_text( info.weapon.name );
			const auto text_x = std::floorf( bounds.min.x + ( bounds.width( ) * 0.5f ) - ( text_w * 0.5f ) );
			const auto text_y = std::floorf( bounds.max.y + 2.0f + offsets.bottom + total_height );

			draw_list.text( text_x, text_y, info.weapon.name, cfg.text_color, xdraw::text_style::outlined );
			xdraw::pop_font( );

			total_height += text_h + 2.0f;
		}

		offsets.bottom += total_height;
	}

	void overlay::add_flags( xdraw::draw_list& draw_list, const systems::bounds::data& bounds, const info& info, const settings::esp::player::overlay::info_flags& cfg, draw_offsets& offsets )
	{
		xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );

		const auto x = std::floorf( bounds.max.x + 4.0f );
		auto y = std::floorf( bounds.min.y );

		const auto draw_flag = [ & ]( const std::string& text, const xdraw::color& color )
			{
				draw_list.text( x, y, text, color, xdraw::text_style::outlined );
				const auto [text_w, text_h] = xdraw::measure_text( text );
				y += text_h;
			};

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::money ) )
		{
			draw_flag( std::format( "${}", info.money ), cfg.money_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::armor ) && info.armor > 0 )
		{
			draw_flag( info.has_helmet ? "hk" : "k", cfg.armor_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::kit ) && info.has_defuser )
		{
			draw_flag( "kit", cfg.kit_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::scoped ) && info.is_scoped )
		{
			draw_flag( "zoom", cfg.scoped_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::defusing ) && info.is_defusing )
		{
			draw_flag( "defusing", cfg.defusing_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::flashed ) && info.is_flashed )
		{
			draw_flag( "flashed", cfg.flashed_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::ping ) )
		{
			const auto ping_color = [ & ]( )
				{
					if ( info.ping <= 20 )
					{
						return xdraw::color( 98, 217, 109, 255 );
					}

					if ( info.ping <= 50 )
					{
						return xdraw::color( 230, 206, 137, 255 );
					}

					if ( info.ping <= 70 )
					{
						return xdraw::color( 230, 156, 110, 255 );
					}

					return xdraw::color( 222, 59, 59, 255 );
				}( );

			draw_flag( std::format( "{}ms", info.ping ), ping_color );
		}

		if ( cfg.has( settings::esp::player::overlay::info_flags::flag::distance ) )
		{
			draw_flag( std::format( "{:.0f}m", info.distance ), cfg.distance_color );
		}

		xdraw::pop_font( );
	}

	void overlay::add_oof_arrow( xdraw::draw_list& draw_list, const info& info, const settings::esp::player::overlay::oof_arrow& cfg )
	{
		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto sw = static_cast< float >( screen_w );
		const auto sh = static_cast< float >( screen_h );
		const auto center_x = sw * 0.5f;
		const auto center_y = sh * 0.5f;
		const auto head = info.bones[ cstypes::bone_ids::head ].position;

		const auto proj = systems::g_view.project_full( head );

		if ( proj.on_screen && proj.w > 0.0f )
		{
			return;
		}

		if ( !systems::g_frame_data.valid( ) )
		{
			return;
		}

		const auto to_target = info.origin - systems::g_frame_data.origin( );
		if ( !std::isfinite( to_target.x ) || !std::isfinite( to_target.y ) || to_target.length_2d( ) < 1.0f )
		{
			return;
		}

		// Match the live input yaw and player origins used by the reference OOF implementation.
		// The view system's cached origin/angles describe a different render structure.
		const auto target_yaw = std::atan2f( to_target.y, to_target.x );
		const auto view_yaw = math::helpers::deg_to_rad( systems::g_input.get_view_angles( ).y );
		const auto angle = view_yaw - target_yaw - std::numbers::pi_v<float> *0.5f;
		const auto rx = cfg.radius_x.value;
		const auto ry = cfg.radius_y.value;

		const auto tip_x = center_x + std::cosf( angle ) * rx;
		const auto tip_y = center_y + std::sinf( angle ) * ry;

		auto color = ( info.is_visible ? cfg.visible_color : cfg.occluded_color ).value;

		// Distance fade & scaling
		if ( cfg.distance_fade.value )
		{
			const float dist_fade = std::clamp( 1.0f - ( info.distance - 5.0f ) / 50.0f, 0.40f, 1.0f );
			color.a = static_cast< std::uint8_t >( static_cast< float >( color.a ) * dist_fade );
		}

		const auto width = cfg.width.value;
		const auto height = cfg.height.value;

		const auto fx = std::cosf( angle );
		const auto fy = std::sinf( angle );
		const auto px = -fy;
		const auto py = fx;

		const auto base_cx = tip_x - fx * height;
		const auto base_cy = tip_y - fy * height;

		const auto half_w = width * 0.5f;

		const auto bl_x = base_cx - px * half_w;
		const auto bl_y = base_cy - py * half_w;
		const auto br_x = base_cx + px * half_w;
		const auto br_y = base_cy + py * half_w;

		// Crisp dark outline
		const auto outline_col = xdraw::color{ 10, 10, 12, static_cast< std::uint8_t >( color.a * 0.85f ) };
		draw_list.line( tip_x, tip_y, bl_x, bl_y, outline_col, 1.5f );
		draw_list.line( bl_x, bl_y, br_x, br_y, outline_col, 1.5f );
		draw_list.line( br_x, br_y, tip_x, tip_y, outline_col, 1.5f );

		// Filled triangle
		draw_list.triangle_filled( tip_x, tip_y, bl_x, bl_y, br_x, br_y, color );

		if ( cfg.glow )
		{
			auto& glow = xdraw::get_glow( );
			const auto glow_a = static_cast< std::uint8_t >( static_cast< float >( color.a ) * cfg.glow_strength );
			const auto glow_col = xdraw::color{ color.r, color.g, color.b, glow_a };

			glow.triangle_filled( tip_x, tip_y, bl_x, bl_y, br_x, br_y, glow_col );
		}

		if ( cfg.show_distance.value && info.distance > 0.1f )
		{
			char dist_buf[ 16 ];
			std::snprintf( dist_buf, sizeof( dist_buf ), "%.0fm", info.distance );
			xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );
			const auto [dw, dh] = xdraw::measure_text( dist_buf );
			const auto tx = base_cx - fx * ( dh + 4.0f ) - dw * 0.5f;
			const auto ty = base_cy - fy * ( dh + 4.0f ) - dh * 0.5f;
			draw_list.text( tx, ty, dist_buf, color, xdraw::text_style::outlined );
			xdraw::pop_font( );
		}
	}

	overlay::info overlay::get_info( const systems::entities::cached& player, const systems::local::snapshot& local, bool want_bones, bool want_visible )
	{
		info info{};
		info.controller = player.ptr;

		if ( !info.controller )
		{
			return info;
		}

		if ( !memory::read<bool>( info.controller + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) ) )
		{
			return info;
		}

		const auto pawn_handle = memory::read<std::uint32_t>( info.controller + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
		if ( !pawn_handle )
		{
			return info;
		}

		info.pawn = systems::g_entities.lookup( pawn_handle );
		if ( !info.pawn || info.pawn == local.view_pawn( ) )
		{
			return info;
		}

		info.health = memory::read<int>( info.pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
		if ( info.health <= 0 )
		{
			return info;
		}

		info.team = memory::read<int>( info.pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
		info.is_other_team = local.is_this_other_team( info.team );

		const auto game_scene_node = memory::read<std::uintptr_t>( info.pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !game_scene_node )
		{
			return info;
		}

		// m_bDormant is ambiguous across the current schema scopes and can
		// resolve to unrelated data. Keep the last known transform, as chams do.

		const auto name_ptr = memory::read<std::uintptr_t>( info.controller + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
		if ( name_ptr )
		{
			info.name = memory::read_string( name_ptr, 128 );
			std::ranges::transform( info.name, info.name.begin( ), [ ]( unsigned char c ) { return std::tolower( c ); } );
		}

		const auto money_services = memory::read<std::uintptr_t>( info.controller + SCHEMA( "CCSPlayerController", "m_pInGameMoneyServices"_hash ) );
		if ( money_services )
		{
			info.money = memory::read<int>( money_services + SCHEMA( "CCSPlayerController_InGameMoneyServices", "m_iAccount"_hash ) );
		}

		const auto item_services = memory::read<std::uintptr_t>( info.pawn + SCHEMA( "C_BasePlayerPawn", "m_pItemServices"_hash ) );
		if ( item_services )
		{
			info.has_helmet = memory::read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasHelmet"_hash ) );
			info.has_defuser = memory::read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasDefuser"_hash ) );
		}

		info.origin = memory::read<math::vector3>( game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
		info.distance = systems::g_view.origin( ).distance( info.origin ) * 0.01905f;
		info.ping = memory::read<int>( info.controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
		info.armor = memory::read<int>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) );
		info.is_scoped = memory::read<bool>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsScoped"_hash ) );
		info.is_defusing = memory::read<bool>( info.pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsDefusing"_hash ) );
		info.is_flashed = memory::read<float>( info.pawn + SCHEMA( "C_CSPlayerPawnBase", "m_flFlashBangTime"_hash ) ) > 0.0f;

		// Skeleton evaluation and the visibility trace are the two most
		// expensive parts of this function. Only pay for them when a feature
		// that consumes the result is actually enabled.
		if ( want_bones || want_visible )
		{
			info.bones = systems::g_bones.get_skeleton( info.pawn );
		}

		if ( want_visible )
		{
			info.is_visible = systems::g_tracing.is_visible( systems::g_view.origin( ), info.bones[ cstypes::bone_ids::head ].position, info.pawn, local.view_pawn( ) );
		}

		const auto weapon_services = memory::read<std::uintptr_t>( info.pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
		if ( weapon_services )
		{
			const auto weapon_handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
			if ( weapon_handle )
			{
				info.weapon.ptr = systems::g_entities.lookup( weapon_handle );
				if ( info.weapon.ptr )
				{
					info.weapon.vdata = memory::read<std::uintptr_t>( info.weapon.ptr + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
					if ( info.weapon.vdata )
					{
						info.weapon.ammo = memory::read<int>( info.weapon.ptr + SCHEMA( "C_BasePlayerWeapon", "m_iClip1"_hash ) );
						info.weapon.max_ammo = memory::read<int>( info.weapon.vdata + SCHEMA( "CBasePlayerWeaponVData", "m_iMaxClip1"_hash ) );

						const auto weapon_name_ptr = memory::read<const char*>( info.weapon.vdata + SCHEMA( "CCSWeaponBaseVData", "m_szName"_hash ) );
						if ( weapon_name_ptr )
						{
							info.weapon.name = memory::read_string( reinterpret_cast< std::uintptr_t >( weapon_name_ptr ), 64 );
							if ( info.weapon.name.starts_with( "weapon_" ) )
							{
								info.weapon.name.erase( 0, 7 );
							}
						}
					}
				}
			}
		}

		return info;
	}

} // namespace features::esp::player
