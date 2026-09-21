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
