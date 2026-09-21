#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <core/systems/systems.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <core/rendering/rendering.hpp>
#include <core/resources/particle_textures.hpp>
#include <protection/game_addresses.hpp>
#include <utilities//addresses/addresses.hpp>
#include <external/xdraw/xui/xui.hpp>

namespace features::misc {
	namespace {
		inline xdraw::color blend_color( const xdraw::color& a, const xdraw::color& b, float t )
		{
			t = std::clamp( t, 0.0f, 1.0f );
			return xdraw::color{
				static_cast< std::uint8_t >( a.r + ( b.r - a.r ) * t ),
				static_cast< std::uint8_t >( a.g + ( b.g - a.g ) * t ),
				static_cast< std::uint8_t >( a.b + ( b.b - a.b ) * t ),
				static_cast< std::uint8_t >( a.a + ( b.a - a.a ) * t )
			};
		}
	}

	void hud::on_render( xdraw::draw_list& draw_list )
	{
		if ( systems::g_local.is_in_cinematic( ) || systems::g_local.is_in_time_freeze( ) )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.pawn || !local.controller || !systems::g_entities.exists( local.controller ) || !local.is_alive )
		{
			return;
		}

		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto cx = static_cast< float >( screen_w ) * 0.5f;
		const auto cy = static_cast< float >( screen_h ) * 0.5f;

		this->do_scope( draw_list, cx, cy, static_cast< float >( screen_h ), local.pawn );
		this->do_crosshair( draw_list, cx, cy, local.pawn );
		this->do_combat_badges( draw_list, cx, cy, local.pawn );
		this->do_recoil( draw_list, cx, cy, static_cast< float >( screen_h ), local.pawn );
		this->do_china_hat( draw_list, local.pawn );
		this->do_soup_jump_rings( draw_list, local.pawn );
		this->do_soup_motes( draw_list, local.pawn );
		this->do_soup_trails( draw_list, local.pawn );
		this->do_velocity( draw_list, cx, static_cast< float >( screen_h ), local.pawn );
	}

	void hud::do_combat_badges( xdraw::draw_list& draw_list, float cx, float cy, std::uintptr_t local_pawn ) const
	{
		( void )local_pawn;
		const auto& cfg = settings::g_misc.m_hud.m_combat_badges;
		if ( !cfg.enabled.value )
		{
			return;
		}

		struct badge_entry
		{
			std::string text{};
			xdraw::color col{};
		};

		std::vector<badge_entry> badges{};

		const auto& ctx = features::combat::g_shared.ctx( );
		const auto has_weapon = ctx.valid && ctx.weapon_type >= cstypes::weapon_type::pistol && ctx.weapon_type <= cstypes::weapon_type::lmg;

		if ( has_weapon && settings::g_combat.m_ragebot.enabled.value )
		{
			const auto& group = settings::g_combat.m_ragebot.get_group( ctx.weapon_type );

			if ( cfg.show_dmg.value && group.min_damage_override.bind.active )
			{
				badges.push_back( { "DMG " + std::to_string( group.min_damage_override_value.value ), xdraw::color{ 255, 112, 67, 255 } } );
			}

			if ( cfg.show_hc.value && group.hitchance_override.bind.active )
			{
				badges.push_back( { "HC " + std::to_string( group.hitchance_override_value.value ) + "%", xdraw::color{ 56, 189, 248, 255 } } );
			}

			if ( cfg.show_baim.value && group.body_aim.bind.active )
			{
				badges.push_back( { "BAIM", xdraw::color{ 251, 191, 36, 255 } } );
			}
		}

		if ( cfg.show_fd.value && settings::g_combat.m_fakeduck.enabled.bind.active )
		{
			badges.push_back( { "FD", xdraw::color{ 52, 211, 153, 255 } } );
		}

		if ( cfg.show_peek.value && settings::g_combat.m_quickpeek.enabled.bind.active )
		{
			badges.push_back( { "PEEK", xdraw::color{ 167, 139, 250, 255 } } );
		}

		if ( badges.empty( ) )
		{
			return;
		}

		float cur_y = cy + 26.0f;
		constexpr float pill_h = 16.0f;
		constexpr float pad_x = 6.0f;

		for ( const auto& b : badges )
		{
			const auto [tw, th] = xdraw::measure_text( b.text );
			const float pill_w = tw + pad_x * 2.0f;
			const float pill_x = std::floor( cx - pill_w * 0.5f );

			draw_list.rect_filled( pill_x, cur_y, pill_w, pill_h, xdraw::color{ 14, 16, 24, 220 }, xdraw::corner_radius{ 3.0f } );
			draw_list.rect( pill_x, cur_y, pill_w, pill_h, b.col.alpha( 110 ), xdraw::corner_radius{ 3.0f }, 1.0f );
			draw_list.text( pill_x + pad_x, cur_y + std::floor( ( pill_h - th ) * 0.5f ), b.text, b.col );

			cur_y += pill_h + 3.0f;
		}
	}

	void hud::do_crosshair( xdraw::draw_list& draw_list, float cx, float cy, std::uintptr_t local_pawn ) const
	{
		const auto& cfg = settings::g_misc.m_hud.m_crosshair;
		if ( !cfg.enabled.value )
		{
			return;
		}

		if ( this->m_scope_anim > 0.01f )
		{
			return;
		}

		// Calculate dynamic recoil & spread penalty expansion
		float spread_offset = 0.0f;
		if ( cfg.dynamic_spread.value && local_pawn )
		{
			const auto weapon_services = memory::read<std::uintptr_t>( local_pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
			if ( weapon_services )
			{
				const auto weapon_handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				const auto weapon = ( weapon_handle && weapon_handle != 0xffffffff ) ? systems::g_entities.lookup( weapon_handle ) : 0ull;
				if ( weapon )
				{
					const auto acc_pen = memory::read<float>( weapon + SCHEMA( "C_CSWeaponBase", "m_fAccuracyPenalty"_hash ) );
					spread_offset = std::clamp( acc_pen * 120.0f, 0.0f, 35.0f );
				}
			}

			// Gentle breathing motion when idle
			const float breathing = std::sinf( static_cast<float>( GetTickCount64( ) ) * 0.0035f ) * 0.85f;
			spread_offset += breathing;
		}

		// Hit pulse calculation
		const auto now = GetTickCount64( );
		float hit_anim = 0.0f;
		if ( cfg.hit_pulse.value )
		{
			if ( rendering::s_last_kill_time > 0 && now - rendering::s_last_kill_time < 350 )
			{
				hit_anim = 1.0f - static_cast< float >( now - rendering::s_last_kill_time ) / 350.0f;
			}
			else if ( rendering::s_last_shot_time > 0 && now - rendering::s_last_shot_time < 200 )
			{
				hit_anim = ( 1.0f - static_cast< float >( now - rendering::s_last_shot_time ) / 200.0f ) * 0.5f;
			}
		}

		const auto s = cfg.size.value;
		const auto o = cfg.outline.value;
		auto main_col = cfg.color.value;
		if ( hit_anim > 0.01f )
		{
			main_col = blend_color( main_col, xdraw::color{ 255, 255, 255, 255 }, hit_anim );

			if ( cfg.glow_strength.value > 0.05f )
			{
				auto& glow = xdraw::get_glow( );
				const float pulse_r = s * 3.5f + ( 1.0f - hit_anim ) * 16.0f;
				const auto pulse_a = static_cast< std::uint8_t >( 180.0f * hit_anim * cfg.glow_strength.value );
				glow.circle( cx, cy, pulse_r, xdraw::color{ 255, 255, 255, pulse_a }, 2.0f );
				glow.circle_filled( cx, cy, pulse_r * 0.5f, main_col.alpha( static_cast< std::uint8_t >( 70.0f * hit_anim ) ) );
			}
		}

		const auto style = static_cast< settings::misc::hud::crosshair::style_type >( cfg.style.value );
		switch ( style )
		{
			case settings::misc::hud::crosshair::style_type::dot:
			{
				if ( o > 0.0f )
				{
					draw_list.rect_filled( cx - s - o, cy - s - o, ( s + o ) * 2.0f, ( s + o ) * 2.0f, cfg.outline_color );
				}
				draw_list.rect_filled( cx - s, cy - s, s * 2.0f, s * 2.0f, main_col );
				break;
			}
			case settings::misc::hud::crosshair::style_type::cross:
			{
				const float gap = s * 1.2f + 2.0f + spread_offset;
				const float len = s * 3.0f;
				const float th = std::clamp( s * 0.4f, 1.0f, 3.0f );

				if ( o > 0.0f )
				{
					draw_list.line( cx - gap - len, cy, cx - gap, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx + gap, cy, cx + gap + len, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx, cy - gap - len, cx, cy - gap, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx, cy + gap, cx, cy + gap + len, cfg.outline_color.value, th + o * 2.0f );
				}

				draw_list.line( cx - gap - len, cy, cx - gap, cy, main_col, th );
				draw_list.line( cx + gap, cy, cx + gap + len, cy, main_col, th );
				draw_list.line( cx, cy - gap - len, cx, cy - gap, main_col, th );
				draw_list.line( cx, cy + gap, cx, cy + gap + len, main_col, th );
				break;
			}
			case settings::misc::hud::crosshair::style_type::circle:
			{
				const float r = s * 2.0f + 2.0f + spread_offset * 0.5f;
				const float th = std::clamp( s * 0.35f, 1.0f, 2.5f );

				if ( o > 0.0f )
				{
					draw_list.circle( cx, cy, r, cfg.outline_color.value, th + o * 2.0f );
					draw_list.circle_filled( cx, cy, 1.5f + o, cfg.outline_color.value );
				}

				draw_list.circle( cx, cy, r, main_col, th );
				draw_list.circle_filled( cx, cy, 1.5f, main_col );
				break;
			}
			case settings::misc::hud::crosshair::style_type::t_style:
			{
				const float gap = s * 1.2f + 2.0f + spread_offset;
				const float len = s * 3.0f;
				const float th = std::clamp( s * 0.4f, 1.0f, 3.0f );

				if ( o > 0.0f )
				{
					draw_list.line( cx - gap - len, cy, cx - gap, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx + gap, cy, cx + gap + len, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx, cy + gap, cx, cy + gap + len, cfg.outline_color.value, th + o * 2.0f );
				}

				draw_list.line( cx - gap - len, cy, cx - gap, cy, main_col, th );
				draw_list.line( cx + gap, cy, cx + gap + len, cy, main_col, th );
				draw_list.line( cx, cy + gap, cx, cy + gap + len, main_col, th );
				break;
			}
			case settings::misc::hud::crosshair::style_type::cross_dot:
			{
				const float gap = s * 1.2f + 3.0f + spread_offset;
				const float len = s * 3.0f;
				const float th = std::clamp( s * 0.4f, 1.0f, 3.0f );

				if ( o > 0.0f )
				{
					draw_list.line( cx - gap - len, cy, cx - gap, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx + gap, cy, cx + gap + len, cy, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx, cy - gap - len, cx, cy - gap, cfg.outline_color.value, th + o * 2.0f );
					draw_list.line( cx, cy + gap, cx, cy + gap + len, cfg.outline_color.value, th + o * 2.0f );
					draw_list.rect_filled( cx - 1.5f - o, cy - 1.5f - o, ( 1.5f + o ) * 2.0f, ( 1.5f + o ) * 2.0f, cfg.outline_color );
				}

				draw_list.line( cx - gap - len, cy, cx - gap, cy, main_col, th );
				draw_list.line( cx + gap, cy, cx + gap + len, cy, main_col, th );
				draw_list.line( cx, cy - gap - len, cx, cy - gap, main_col, th );
				draw_list.line( cx, cy + gap, cx, cy + gap + len, main_col, th );
				draw_list.rect_filled( cx - 1.5f, cy - 1.5f, 3.0f, 3.0f, main_col );
				break;
			}
		}
	}

	void hud::do_scope( xdraw::draw_list& draw_list, float cx, float cy, float screen_h, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_scope;
		if ( !cfg.enabled.value )
		{
			return;
		}

		const auto is_scoped = memory::read<bool>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsScoped"_hash ) );

		this->m_scope_anim = std::lerp( this->m_scope_anim, is_scoped ? 1.0f : 0.0f, std::min( xdraw::delta_time( ) * cfg.anim_speed, 1.0f ) );
		if ( this->m_scope_anim < 0.01f )
		{
			this->m_cached_spread_pixels = 0.0f;
			this->m_scope_update_frame = 0;
			return;
		}

		++this->m_scope_update_frame;
		const auto should_recalc = this->m_scope_update_frame >= 2 || this->m_cached_spread_pixels <= 0.0f;

		if ( should_recalc )
		{
			this->m_scope_update_frame = 0;

			const auto weapon_services = memory::read<std::uintptr_t>( local_pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
			if ( weapon_services )
			{
				const auto weapon_handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				const auto weapon = ( weapon_handle && weapon_handle != 0xffffffff ) ? systems::g_entities.lookup( weapon_handle ) : 0ull;

				if ( weapon )
				{
					const auto weapon_vdata = memory::read<std::uintptr_t>( weapon + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );

					if ( weapon_vdata )
					{
						const auto vel = memory::read<math::vector3>( local_pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
						const auto fire_mode = memory::read<int>( weapon + SCHEMA( "C_CSWeaponBase", "m_weaponMode"_hash ) );
						const auto accuracy_penalty = memory::read<float>( weapon + SCHEMA( "C_CSWeaponBase", "m_fAccuracyPenalty"_hash ) );
						const auto turning_inaccuracy = memory::read<float>( weapon + SCHEMA( "C_CSWeaponBase", "m_flTurningInaccuracy"_hash ) );

						const auto max_speed_pair = memory::read<std::pair<float, float>>( weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flMaxSpeed"_hash ) );
						const auto inaccuracy_move_pair = memory::read<std::pair<float, float>>( weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flInaccuracyMove"_hash ) );
						const auto inaccuracy_jump_initial = memory::read<float>( weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flInaccuracyJumpInitial"_hash ) );
						const auto inaccuracy_jump_apex = memory::read<float>( weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_flInaccuracyJumpApex"_hash ) );

						const auto max_speed = fire_mode ? max_speed_pair.second : max_speed_pair.first;
						const auto inaccuracy_move = fire_mode ? inaccuracy_move_pair.second : inaccuracy_move_pair.first;

						const auto speed = vel.length_2d( );
						const auto flags = memory::read<std::uint32_t>( local_pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) );
						const auto is_walking = memory::read<bool>( local_pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsWalking"_hash ) );
						const auto on_ground = ( flags & 1 ) != 0;

						const auto edge0 = max_speed * 0.34f;
						const auto edge1 = max_speed * 0.95f;

						auto move_factor = ( edge0 == edge1 ) ? ( speed >= edge1 ? 1.0f : 0.0f ) : std::clamp( ( speed - edge0 ) / ( edge1 - edge0 ), 0.0f, 1.0f );
						auto move_inac{ 0.0f };

						if ( move_factor > 0.0f )
						{
							if ( !is_walking )
							{
								move_factor = std::powf( move_factor, 0.25f );
							}

							move_inac = move_factor * inaccuracy_move;
						}

						auto air_inac{ 0.0f };

						if ( !on_ground )
						{
							const auto jump_impulse = CONVAR ("sv_jump_impulse")->get<float>( );
							const auto sqrt_threshold = std::sqrtf( std::fabsf( jump_impulse ) );
							const auto sqrt_vertical = std::sqrtf( std::fabsf( vel.z ) );
							const auto lo = sqrt_threshold * 0.25f;

							if ( lo == sqrt_threshold )
							{
								air_inac = ( sqrt_vertical >= sqrt_threshold ) ? inaccuracy_jump_initial : inaccuracy_jump_apex;
							}
							else
							{
								const auto frac = ( sqrt_vertical - lo ) / ( sqrt_threshold - lo );
								air_inac = inaccuracy_jump_apex + frac * ( inaccuracy_jump_initial - inaccuracy_jump_apex );
							}

							air_inac = std::clamp( air_inac, 0.0f, inaccuracy_jump_initial * 2.0f );
						}

						const auto inaccuracy = std::fminf( 1.0f, turning_inaccuracy + move_inac + air_inac + accuracy_penalty );

						const auto fov_rad = systems::g_view.fov( ) * ( std::numbers::pi_v<float> / 180.0f );
						this->m_cached_spread_pixels = inaccuracy * 320.0f / std::tanf( fov_rad * 0.5f ) * ( screen_h / 480.0f );
					}
				}
			}
		}

		this->m_spread_smooth = std::lerp( this->m_spread_smooth, this->m_cached_spread_pixels, std::min( xdraw::delta_time( ) * 50.0f, 1.0f ) );

		const auto gap = std::max( cfg.gap.value, this->m_spread_smooth ) + ( 40.0f * ( 1.0f - this->m_scope_anim ) );
		const auto length = cfg.line_length * this->m_scope_anim;
		const auto alpha = static_cast< std::uint8_t >( cfg.color.value.a * this->m_scope_anim );

		const auto col_clear = xdraw::color{ cfg.color.value.r, cfg.color.value.g, cfg.color.value.b, 0 };
		const auto col_solid = xdraw::color{ cfg.color.value.r, cfg.color.value.g, cfg.color.value.b, alpha };
		const auto& col_start = cfg.fade_in ? col_clear : col_solid;

		if ( cfg.glow && alpha > 0 )
		{
			auto& glow = xdraw::get_glow( );
			const auto glow_a = static_cast< std::uint8_t >( static_cast< float >( alpha ) * cfg.glow_strength );
			const auto glow_col = xdraw::color{ cfg.color.value.r, cfg.color.value.g, cfg.color.value.b, glow_a };

			const auto glow_draw_line = [ & ]( float x1, float y1, float x2, float y2 )
				{
					const auto mx = ( x1 + x2 ) * 0.5f;
					const auto my = ( y1 + y2 ) * 0.5f;

					const auto glow_thickness = cfg.thickness + 0.5f;

					const auto gc_clear = xdraw::color{ cfg.color.value.r, cfg.color.value.g, cfg.color.value.b, 0 };
					const auto& gc_start = cfg.fade_in ? gc_clear : glow_col;

					const float pts[ ]{ x1, y1, mx, my, x2, y2 };
					const xdraw::color cols[ ]{ gc_start, glow_col, glow_col };

					glow.polyline_gradient( pts, cols, false, glow_thickness );
				};

			glow_draw_line( cx, cy - gap, cx, cy - gap - length );
			glow_draw_line( cx, cy + gap, cx, cy + gap + length );
			glow_draw_line( cx - gap, cy, cx - gap - length, cy );
			glow_draw_line( cx + gap, cy, cx + gap + length, cy );
		}

		const auto draw_line = [ & ]( float x1, float y1, float x2, float y2 )
			{
				const auto mx = ( x1 + x2 ) * 0.5f;
				const auto my = ( y1 + y2 ) * 0.5f;

				const float pts[ ]{ x1, y1, mx, my, x2, y2 };
				const xdraw::color cols[ ]{ col_start, col_solid, col_solid };

				draw_list.polyline_gradient( pts, cols, false, cfg.thickness );
			};

		draw_line( cx, cy - gap, cx, cy - gap - length );
		draw_line( cx, cy + gap, cx, cy + gap + length );
		draw_line( cx - gap, cy, cx - gap - length, cy );
		draw_line( cx + gap, cy, cx + gap + length, cy );
	}

	void hud::do_recoil( xdraw::draw_list& draw_list, float cx, float cy, float screen_h, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_recoil;
		if ( !cfg.shown.value && !cfg.show_recoil_dot.value )
		{
			return;
		}

		if ( this->m_scope_anim > 0.01f )
		{
			return;
		}

		const auto& ctx = features::combat::g_shared.ctx( );
		if ( !ctx.valid )
		{
			return;
		}

		const auto fov_rad = systems::g_view.fov( ) * ( std::numbers::pi_v<float> / 180.0f );
		const auto scale = ( screen_h * 0.5f ) / std::tanf( fov_rad * 0.5f );

		if ( cfg.shown.value )
		{
			const auto spread_radius = ( ctx.inaccuracy + ctx.spread ) * scale;
			draw_list.circle( cx, cy, std::max( spread_radius, 1.0f ), cfg.spread_color, cfg.spread_thickness.value );
		}

		if ( cfg.show_recoil_dot.value )
		{
			const auto aim_punch = features::combat::g_shared.get_aim_punch( local_pawn );
			const auto punch_yaw_rad = aim_punch.y * ( std::numbers::pi_v<float> / 180.0f );
			const auto punch_pitch_rad = aim_punch.x * ( std::numbers::pi_v<float> / 180.0f );

			const auto dot_x = cx + std::tanf( punch_yaw_rad ) * scale;
			const auto dot_y = cy - std::tanf( punch_pitch_rad ) * scale;

			draw_list.circle_filled( dot_x, dot_y, cfg.dot_size.value, cfg.dot_color, 8 );
		}
	}

	namespace {
		constexpr int k_ring_points{ 24 };
		constexpr int k_hex_points{ 6 };

		struct ring_lut_t
		{
			float cos_val;
			float sin_val;
		};

		const auto k_circle_lut = []() {
			std::array<ring_lut_t, k_ring_points> arr{};
			for ( int i = 0; i < k_ring_points; ++i )
			{
				const float a = ( static_cast< float >( i ) / static_cast< float >( k_ring_points ) ) * 2.0f * std::numbers::pi_v< float >;
				arr[ i ] = { std::cosf( a ), std::sinf( a ) };
			}
			return arr;
		}();

		const auto k_hex_lut = []() {
			std::array<ring_lut_t, k_hex_points> arr{};
			for ( int i = 0; i < k_hex_points; ++i )
			{
				const float a = ( static_cast< float >( i ) / static_cast< float >( k_hex_points ) ) * 2.0f * std::numbers::pi_v< float >;
				arr[ i ] = { std::cosf( a ), std::sinf( a ) };
			}
			return arr;
		}();

		struct active_jump_ring
		{
			math::vector3 origin{};
			float spawn_time{ 0.0f };
			float duration{ 0.55f };
			float max_radius{ 45.0f };
			float thickness{ 1.5f };
			settings::misc::hud::jump_rings::ring_style style{ settings::misc::hud::jump_rings::ring_style::ring };
			xdraw::color primary_color{};
			xdraw::color secondary_color{};
			bool active{ false };
		};

		static std::array<active_jump_ring, 32> s_active_rings{};
		static std::size_t s_ring_head{ 0 };

		struct ground_particle
		{
			math::vector3 origin{};
			math::vector3 vel{};
			float spawn_time{ 0.0f };
			float lifetime{ 0.55f };
			float size{ 2.5f };
			settings::misc::hud::jump_rings::particle_type type{ settings::misc::hud::jump_rings::particle_type::sparks };
			xdraw::color col{};
			float rot{ 0.0f };
			bool active{ false };
		};

		static constexpr std::size_t k_max_ground_particles = 128;
		static std::array<ground_particle, k_max_ground_particles> s_ground_particles{};
		static std::size_t s_ground_particle_head{ 0 };

		static void spawn_jump_ring( const math::vector3& pos, float cur_time, float duration, float max_radius, float thickness,
			settings::misc::hud::jump_rings::ring_style style, settings::misc::hud::jump_rings::particle_type p_type, const xdraw::color& pri, const xdraw::color& sec )
		{
			auto& r = s_active_rings[ s_ring_head ];
			r.origin = pos;
			r.spawn_time = cur_time;
			r.duration = duration;
			r.max_radius = max_radius;
			r.thickness = thickness;
			r.style = style;
			r.primary_color = pri;
			r.secondary_color = sec;
			r.active = true;
			s_ring_head = ( s_ring_head + 1 ) % s_active_rings.size( );

			if ( p_type == settings::misc::hud::jump_rings::particle_type::none )
			{
				return;
			}

			const int p_count = ( p_type == settings::misc::hud::jump_rings::particle_type::smoke ) ? 10 : 16;
			for ( int i = 0; i < p_count; ++i )
			{
				const float ang = ( static_cast< float >( i ) / static_cast< float >( p_count ) ) * 6.2831853f + ( static_cast< float >( ( s_ring_head * 17 + i * 31 ) % 100 ) / 100.0f - 0.5f ) * 0.35f;

				float spd = 45.0f;
				float up_lift = 20.0f;
				float p_size = 2.5f;
				float p_life = duration * 0.85f;

				if ( p_type == settings::misc::hud::jump_rings::particle_type::sparks )
				{
					spd = 55.0f + static_cast< float >( ( i * 37 + s_ring_head * 19 ) % 65 );
					up_lift = 25.0f + static_cast< float >( ( i * 29 ) % 40 );
					p_size = 2.2f + static_cast< float >( ( i * 7 ) % 15 ) * 0.08f;
					p_life = duration * ( 0.70f + static_cast< float >( ( i * 13 ) % 40 ) * 0.01f );
				}
				else if ( p_type == settings::misc::hud::jump_rings::particle_type::embers )
				{
					spd = 18.0f + static_cast< float >( ( i * 23 ) % 25 );
					up_lift = 35.0f + static_cast< float >( ( i * 31 ) % 35 );
					p_size = 3.0f + static_cast< float >( ( i * 11 ) % 20 ) * 0.1f;
					p_life = duration * ( 1.0f + static_cast< float >( ( i * 17 ) % 30 ) * 0.01f );
				}
				else if ( p_type == settings::misc::hud::jump_rings::particle_type::smoke )
				{
					spd = 22.0f + static_cast< float >( ( i * 19 ) % 20 );
					up_lift = 12.0f + static_cast< float >( ( i * 17 ) % 15 );
					p_size = 5.0f + static_cast< float >( ( i * 13 ) % 20 ) * 0.15f;
					p_life = duration * 1.15f;
				}
				else if ( p_type == settings::misc::hud::jump_rings::particle_type::neon_runes )
				{
					spd = 15.0f + static_cast< float >( ( i * 21 ) % 18 );
					up_lift = 28.0f + static_cast< float >( ( i * 25 ) % 22 );
					p_size = 4.0f;
					p_life = duration * 0.95f;
				}

				auto& spk = s_ground_particles[ s_ground_particle_head ];
				spk.origin = pos + math::vector3{ 0.0f, 0.0f, 2.0f };
				spk.vel = math::vector3{ std::cosf( ang ) * spd, std::sinf( ang ) * spd, up_lift };
				spk.spawn_time = cur_time;
				spk.lifetime = p_life;
				spk.size = p_size;
				spk.type = p_type;
				spk.rot = static_cast< float >( ( i * 47 ) % 360 );
				spk.col = ( ( i % 2 ) == 0 ) ? sec : pri;
				spk.active = true;
				s_ground_particle_head = ( s_ground_particle_head + 1 ) % k_max_ground_particles;
			}
		}
	}

	// Soup's Visuals / ChinaHat: a wide flat brim disc with a raised top, a
	// darker underside, center->edge alpha fade, and a thick spinning outline.
	void hud::do_china_hat( xdraw::draw_list& draw_list, std::uintptr_t local_pawn ) const
	{
		const auto& cfg = settings::g_misc.m_hud.m_hat;
		if ( !cfg.enabled.value || !systems::g_frame_data.valid( ) )
		{
			return;
		}

		const auto render_for_pawn = [ & ]( std::uintptr_t pawn )
		{
			if ( !pawn ) return;

			const auto game_scene_node = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( !game_scene_node ) return;

			const auto hitbox_set = systems::g_hitboxes.query( game_scene_node, false );
			if ( hitbox_set.count < 1 || hitbox_set.entries[ 0 ].bone < 0 || hitbox_set.entries[ 0 ].bone >= 128 ) return;

			const auto& head_hb = hitbox_set.entries[ 0 ];
			const auto head_bone = systems::g_bones.get( pawn, cstypes::bone_ids::head );
			if ( head_bone.position.length_sqr( ) < 1.0f ) return;

			const auto hb_mid = ( head_hb.mins + head_hb.maxs ) * 0.5f;
			const auto center = head_bone.rotation.rotate_vector( hb_mid ) + head_bone.position;
			const auto cap_a = head_bone.rotation.rotate_vector( head_hb.mins - hb_mid ) + center;
			const auto cap_b = head_bone.rotation.rotate_vector( head_hb.maxs - hb_mid ) + center;
			const auto top_cap = ( cap_a.z > cap_b.z ) ? cap_a : cap_b;
			const auto up_world = ( top_cap - center ).normalized( );

			auto right_world = up_world.cross( math::vector3{ 0.0f, 1.0f, 0.0f } );
			right_world = right_world.length_sqr( ) < 0.001f ? math::vector3{ 1.0f, 0.0f, 0.0f } : right_world.normalized( );
			const auto forward_world = up_world.cross( right_world ).normalized( );

			const auto rot_deg = static_cast< float >( GetTickCount64( ) % 360000 ) * 0.001f * cfg.rotation_speed.value * 360.0f;
			const auto rot_rad = rot_deg * ( std::numbers::pi_v< float > / 180.0f );
			const auto cos_r = std::cosf( rot_rad );
			const auto sin_r = std::sinf( rot_rad );
			const auto rot_right = right_world * cos_r + forward_world * sin_r;
			const auto rot_forward = forward_world * cos_r - right_world * sin_r;

			using hat_type = settings::misc::hud::hat::hat_type;
			const auto kind = cfg.type.value;

			const auto origin = top_cap + up_world * ( 0.55f + cfg.vertical_offset.value );
			const auto radius = std::max( 4.0f, cfg.radius.value );
			const auto height = std::max( 3.0f, cfg.height.value );

			const auto& col = cfg.color.value;
			const auto center_a = static_cast< std::uint8_t >( std::clamp( cfg.center_alpha.value, 0.0f, 1.0f ) * static_cast< float >( col.a ) );
			const auto edge_a = static_cast< std::uint8_t >( std::clamp( cfg.edge_alpha.value, 0.0f, 1.0f ) * static_cast< float >( col.a ) );
			const auto top_col = xdraw::color{ col.r, col.g, col.b, center_a };
			const auto rim_col = xdraw::color{ col.r, col.g, col.b, edge_a };
			const auto bottom_col = xdraw::color{ static_cast< std::uint8_t >( col.r * 0.25f ), static_cast< std::uint8_t >( col.g * 0.25f ), static_cast< std::uint8_t >( col.b * 0.25f ), edge_a };
			const auto outline_col = cfg.glow.value
				? xdraw::color{ col.r, col.g, col.b, static_cast< std::uint8_t >( std::min( 255.0f, static_cast< float >( edge_a ) * ( 1.0f + cfg.glow_strength.value * 2.0f ) ) ) }
				: rim_col;
			const auto outline_w = cfg.glow.value ? 4.0f : 3.0f;
			const auto rib_col = xdraw::color{ col.r, col.g, col.b, static_cast< std::uint8_t >( static_cast< float >( edge_a ) * 0.6f ) };

			draw_list.ensure_cmd( nullptr );

			const auto project_ring = [ & ]( float rr, float z_off, math::vector2* out ) -> bool
				{
					for ( int i = 0; i < k_ring_points; ++i )
					{
						const auto& lut = k_circle_lut[ i ];
						const auto dir = rot_right * lut.cos_val + rot_forward * lut.sin_val;
						const auto sp = systems::g_view.project( origin + up_world * z_off + dir * rr );
						if ( !systems::g_view.projection_valid( sp ) ) return false;
						out[ i ] = { sp.x, sp.y };
					}
					return true;
				};

			const auto project_point = [ & ]( const math::vector3& world, math::vector2& out ) -> bool
				{
					const auto sp = systems::g_view.project( world );
					if ( !systems::g_view.projection_valid( sp ) ) return false;
					out = { sp.x, sp.y };
					return true;
				};

			const auto band_fill = [ & ]( const math::vector2* lo, const math::vector2* hi,
										  const xdraw::color& clo, const xdraw::color& chi )
				{
					for ( int i = 0; i < k_ring_points; ++i )
					{
						const int n = ( i + 1 ) % k_ring_points;
						const auto a = draw_list.emit_vtx( lo[ i ].x, lo[ i ].y, 0.5f, 0.5f, clo );
						const auto b = draw_list.emit_vtx( hi[ i ].x, hi[ i ].y, 0.5f, 0.5f, chi );
						const auto c = draw_list.emit_vtx( hi[ n ].x, hi[ n ].y, 0.5f, 0.5f, chi );
						const auto d = draw_list.emit_vtx( lo[ n ].x, lo[ n ].y, 0.5f, 0.5f, clo );
						draw_list.emit_quad( a, b, c, d );
					}
				};

			const auto fan_fill = [ & ]( const math::vector2& c, const math::vector2* rim,
										 const xdraw::color& cc, const xdraw::color& cr )
				{
					for ( int i = 0; i < k_ring_points; ++i )
					{
						const int n = ( i + 1 ) % k_ring_points;
						const auto a = draw_list.emit_vtx( c.x, c.y, 0.5f, 0.5f, cc );
						const auto b = draw_list.emit_vtx( rim[ i ].x, rim[ i ].y, 0.5f, 0.5f, cr );
						const auto d = draw_list.emit_vtx( rim[ n ].x, rim[ n ].y, 0.5f, 0.5f, cr );
						draw_list.emit_idx( a, b, d );
					}
				};

			const auto outline_ring = [ & ]( const math::vector2* rim, const xdraw::color& c, float w )
				{
					for ( int i = 0; i < k_ring_points; ++i )
					{
						const int n = ( i + 1 ) % k_ring_points;
						draw_list.line( rim[ i ].x, rim[ i ].y, rim[ n ].x, rim[ n ].y, c, w );
					}
				};

			math::vector2 ring_a[ k_ring_points ];
			math::vector2 ring_b[ k_ring_points ];
			math::vector2 ring_c[ k_ring_points ];

			if ( kind == hat_type::halo )
			{
				// Flat floating ring (torus seen edge-on becomes an annulus).
				const auto z = std::max( 8.0f, height + 6.0f );
				if ( !project_ring( radius, z, ring_a ) || !project_ring( radius * 0.68f, z, ring_b ) ) return;

				if ( cfg.glow.value )
				{
					for ( int i = 0; i < k_ring_points; ++i )
					{
						const int n = ( i + 1 ) % k_ring_points;
						draw_list.line( ring_a[ i ].x, ring_a[ i ].y, ring_a[ n ].x, ring_a[ n ].y,
							outline_col.alpha( static_cast< std::uint8_t >( static_cast< float >( col.a ) * 0.22f ) ), 11.0f );
					}
				}

				band_fill( ring_b, ring_a, top_col, rim_col );
				outline_ring( ring_a, outline_col, outline_w );
				outline_ring( ring_b, outline_col, outline_w * 0.6f );
			}
			else if ( kind == hat_type::crown )
			{
				// Cylindrical band with triangular spikes.
				const auto band_h = std::max( 4.0f, height * 0.6f );
				const auto spike_h = std::max( 5.0f, height * 1.15f );
				if ( !project_ring( radius, 0.0f, ring_a ) || !project_ring( radius, band_h, ring_b ) ) return;

				band_fill( ring_a, ring_b, bottom_col, rim_col );

				for ( int i = 0; i < k_ring_points; i += 2 )
				{
					const int n = ( i + 1 ) % k_ring_points;
					const auto& lut = k_circle_lut[ i ];
					const auto dir = ( rot_right * lut.cos_val + rot_forward * lut.sin_val ).normalized( );
					math::vector2 apex{};
					if ( !project_point( origin + up_world * ( band_h + spike_h ) + dir * radius, apex ) ) return;
					draw_list.triangle_filled( ring_b[ i ].x, ring_b[ i ].y, ring_b[ n ].x, ring_b[ n ].y, apex.x, apex.y, rim_col );
				}

				outline_ring( ring_a, outline_col, outline_w );
				outline_ring( ring_b, rim_col, 1.6f );
			}
			else if ( kind == hat_type::tophat )
			{
				// Brim + tube + flat top.
				const auto tube_r = radius * 0.62f;
				const auto brim_r = radius * 1.3f;
				const auto tube_h = std::max( 6.0f, height * 1.7f );

				if ( !project_ring( brim_r, 0.0f, ring_a ) ) return;
				if ( !project_ring( tube_r, 0.6f, ring_b ) ) return;

				band_fill( ring_b, ring_a, bottom_col, rim_col );
				outline_ring( ring_a, outline_col, outline_w );

				if ( !project_ring( tube_r, tube_h, ring_c ) ) return;
				band_fill( ring_b, ring_c, rim_col, top_col );

				math::vector2 cap{};
				if ( !project_point( origin + up_world * tube_h, cap ) ) return;
				fan_fill( cap, ring_c, top_col, rim_col );
				outline_ring( ring_c, outline_col, outline_w );
			}
			else // china: a real conical rice hat
			{
				const auto cone_h = std::max( 4.0f, height * 1.2f );
				const auto apex_r = radius * 0.16f;

				if ( !project_ring( radius, 0.0f, ring_a ) ) return;
				if ( !project_ring( apex_r, cone_h, ring_b ) ) return;

				band_fill( ring_a, ring_b, bottom_col, top_col );
				outline_ring( ring_a, outline_col, outline_w );

				math::vector2 apex{};
				if ( !project_point( origin + up_world * cone_h, apex ) ) return;
				fan_fill( apex, ring_b, top_col, rim_col );

				for ( int i = 0; i < k_ring_points; i += k_ring_points / 8 )
				{
					draw_list.line( apex.x, apex.y, ring_a[ i ].x, ring_a[ i ].y, rib_col, 1.4f );
				}
			}
		};

		using hat_target = settings::misc::hud::hat::hat_target;
		const auto target = cfg.target.value;

		if ( target == hat_target::all_entities )
		{
			for ( const auto& p : systems::g_entities.get_by_type( systems::entities::type::player ) )
			{
				if ( !p.ptr ) continue;
				const auto handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
				const auto pawn = systems::g_entities.lookup( handle );
				if ( pawn ) render_for_pawn( pawn );
			}
		}
		else
		{
			render_for_pawn( systems::g_local.get( ).pawn );
		}
	}

	// Soup's Visuals / JumpCircles: a ground disc with a three-stage
	// appear/exist/disappear animation, per-stage interpolation and spin.
	void hud::do_soup_jump_rings( xdraw::draw_list& draw_list, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_jump_rings;
		if ( !cfg.enabled.value || !local_pawn )
		{
			return;
		}

		struct soup_ring
		{
			math::vector3 pos{};
			float spawn{ 0.0f };
			float spin{ 0.0f };
		};

		static std::array<soup_ring, 32> s_rings{};
		static std::size_t s_head{ 0 };
		static bool s_last_on_ground{ true };
		static std::uint32_t s_seed{ 0x51CE };

		const auto frand = [ ]( std::uint32_t& seed ) -> float
			{
				seed = seed * 1664525u + 1013904223u;
				return static_cast< float >( seed & 0xFFFF ) / 65535.0f;
			};

		const auto& prestate = systems::g_prediction.pre( );
		const auto on_ground = ( prestate.flags & cstypes::entity_flags::on_ground ) != 0;
		const auto now = static_cast< float >( GetTickCount64( ) ) * 0.001f;

		using trigger_mode = settings::misc::hud::jump_rings::trigger_mode;
		const auto trigger = cfg.trigger.value;

		auto spawn = false;
		if ( s_last_on_ground && !on_ground && ( trigger == trigger_mode::jump || trigger == trigger_mode::both ) ) spawn = true;
		if ( !s_last_on_ground && on_ground && ( trigger == trigger_mode::landing || trigger == trigger_mode::both ) ) spawn = true;
		s_last_on_ground = on_ground;

		if ( spawn )
		{
			const auto scene = memory::read<std::uintptr_t>( local_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( scene )
			{
				auto pos = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
				pos.z += 1.0f;
				s_rings[ s_head ] = { pos, now, frand( s_seed ) * 2.0f + 1.0f };
				s_head = ( s_head + 1 ) % s_rings.size( );
			}
		}

		using ring_interp = settings::misc::hud::jump_rings::ring_interp;
		using ring_animation = settings::misc::hud::jump_rings::ring_animation;

		const auto interp = [ ]( ring_interp i, float p ) -> float
			{
				p = std::clamp( p, 0.0f, 1.0f );
				switch ( i )
				{
				case ring_interp::linear: return p;
				case ring_interp::smooth: return p * p;
				case ring_interp::fast: return 1.0f - ( 1.0f - p ) * ( 1.0f - p );
				case ring_interp::bounce:
					{
						constexpr auto n1 = 7.5625f, d1 = 2.75f;
						auto x = p;
						if ( x < 1.0f / d1 ) return n1 * x * x;
						if ( x < 2.0f / d1 ) { x -= 1.5f / d1; return n1 * x * x + 0.75f; }
						if ( x < 2.5f / d1 ) { x -= 2.25f / d1; return n1 * x * x + 0.9375f; }
						x -= 2.625f / d1; return n1 * x * x + 0.984375f;
					}
				case ring_interp::elastic:
					return p <= 0.0f ? 0.0f : std::pow( 2.0f, -10.0f * p ) * std::sinf( ( p - 0.075f ) * 12.56637f / 0.3f ) + 1.0f;
				case ring_interp::ease_out: return 1.0f - std::pow( 1.0f - p, 3.0f );
				default: return 1.0f - ( std::expf( -5.0f * p ) * std::cosf( 5.0f * p ) );
				}
			};

		const auto appear = std::max( 0.01f, cfg.appear_duration.value );
		const auto exist = std::max( 0.0f, cfg.exist_duration.value );
		const auto disappear = std::max( 0.01f, cfg.disappear_duration.value );
		const auto total = appear + exist + disappear;
		const auto base_radius = std::clamp( cfg.max_radius.value, 8.0f, 120.0f );
		const auto animation = cfg.animation.value;

		for ( auto& r : s_rings )
		{
			if ( r.spawn <= 0.0f ) continue;

			const auto t = now - r.spawn;
			if ( t > total ) { r.spawn = 0.0f; continue; }

			float anim = 1.0f;
			if ( t <= appear ) anim = interp( cfg.appear_interp.value, t / appear );
			else if ( t <= appear + exist ) anim = 1.0f;
			else anim = 1.0f - interp( cfg.disappear_interp.value, ( t - appear - exist ) / disappear );

			float alpha = 1.0f;
			float scale = cfg.scale.value;
			if ( animation == ring_animation::fade ) alpha = std::clamp( anim, 0.0f, 1.0f );
			else if ( animation == ring_animation::scale ) scale *= std::clamp( anim, 0.0f, 1.0f );
			else { alpha = std::clamp( anim, 0.0f, 1.0f ); scale *= std::clamp( anim, 0.0f, 1.0f ); }

			const auto radius = base_radius * std::max( 0.05f, scale );
			const auto spin = now * cfg.rotate_speed.value * 2.0f;

			const auto& col = cfg.color.value;
			const auto ring_col = xdraw::color{ col.r, col.g, col.b, static_cast< std::uint8_t >( std::clamp( alpha * static_cast< float >( col.a ), 0.0f, 255.0f ) ) };

			using ring_shape = settings::misc::hud::jump_rings::ring_shape;
			const auto shape = cfg.shape.value;
			const auto segs = ( shape == ring_shape::hexagon ) ? 6 : 32;
			const auto thickness = std::clamp( cfg.thickness.value, 1.0f, 6.0f );

			// Sprite-based jump-ring styles (the circle / portal / femware art).
			// Drawn as a world-space quad on the ground so the ring lies flat in
			// the world instead of billboarding toward the camera.
			if ( shape >= ring_shape::circle )
			{
				const int sprite = static_cast< int >( shape ) - static_cast< int >( ring_shape::circle );
				if ( auto* srv = particle_textures::get_ring_sprite( sprite ) )
				{
					const auto half = std::max( 4.0f, radius * 1.3f );
					const auto sc = std::cosf( spin );
					const auto ss = std::sinf( spin );
					const auto corner = [ & ]( float lx, float ly ) -> math::vector3
						{
							return math::vector3{ r.pos.x + lx * sc - ly * ss, r.pos.y + lx * ss + ly * sc, r.pos.z };
						};

					const auto p0 = systems::g_view.project( corner( -half, -half ) );
					const auto p1 = systems::g_view.project( corner( half, -half ) );
					const auto p2 = systems::g_view.project( corner( half, half ) );
					const auto p3 = systems::g_view.project( corner( -half, half ) );

					if ( systems::g_view.projection_valid( p0 ) && systems::g_view.projection_valid( p1 )
						&& systems::g_view.projection_valid( p2 ) && systems::g_view.projection_valid( p3 ) )
					{
						draw_list.ensure_cmd( srv );
						const auto va = draw_list.emit_vtx( p0.x, p0.y, 0.0f, 1.0f, ring_col );
						const auto vb = draw_list.emit_vtx( p1.x, p1.y, 1.0f, 1.0f, ring_col );
						const auto vc = draw_list.emit_vtx( p2.x, p2.y, 1.0f, 0.0f, ring_col );
						const auto vd = draw_list.emit_vtx( p3.x, p3.y, 0.0f, 0.0f, ring_col );
						draw_list.emit_quad( va, vb, vc, vd );
						continue;
					}
				}
			}

			// Fixed scratch buffer instead of a vector allocated per ring/frame.
			std::array< float, 64 > ring_pts{};
			const auto build_ring = [ & ]( float rr ) -> int
				{
					for ( auto i = 0; i < segs; ++i )
					{
						const auto a = static_cast< float >( i ) / static_cast< float >( segs ) * 6.2831853f + spin;
						const auto wp = r.pos + math::vector3{ std::cosf( a ) * rr, std::sinf( a ) * rr, 0.0f };
						const auto sp = systems::g_view.project( wp );
						if ( !systems::g_view.projection_valid( sp ) ) return 0;
						ring_pts[ i * 2 ] = sp.x;
						ring_pts[ i * 2 + 1 ] = sp.y;
					}
					return segs * 2;
				};

			const auto count = build_ring( radius );
			if ( count == 0 ) continue;
			const auto pts = std::span< const float >( ring_pts.data( ), static_cast< std::size_t >( count ) );

			if ( shape == ring_shape::disc )
			{
				draw_list.convex_filled( pts, ring_col.alpha( static_cast< std::uint8_t >( ring_col.a / 3 ) ) );
				draw_list.polyline( pts, ring_col, true, thickness );
			}
			else if ( shape == ring_shape::ripples )
			{
				for ( auto k = 0; k < 3; ++k )
				{
					const auto rc = build_ring( radius * ( 1.0f - static_cast< float >( k ) * 0.28f ) );
					if ( rc == 0 ) break;
					auto c = ring_col;
					c.a = static_cast< std::uint8_t >( static_cast< float >( ring_col.a ) * ( 1.0f - static_cast< float >( k ) * 0.3f ) );
					draw_list.polyline( std::span< const float >( ring_pts.data( ), static_cast< std::size_t >( rc ) ), c, true, thickness );
				}
			}
			else
			{
				draw_list.polyline( pts, ring_col, true, thickness );
			}
		}
	}

	// Soup's Visuals / AmbientParticles: regular + firefly motes around the
	// player with physics, optional firefly trails and network links.
	void hud::do_soup_motes( xdraw::draw_list& draw_list, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_ambient_motes;
		if ( !cfg.enabled.value || !local_pawn )
		{
			return;
		}

		struct mote
		{
			math::vector3 pos{};
			math::vector3 vel{};
			float size{};
			float phase{};
			bool firefly{};
		};

		static constexpr std::size_t k_max = 256;
		static std::array<mote, k_max> s_motes{};
		static bool s_init{ false };
		static std::uint32_t s_seed{ 0xBEEF };

		const auto frand = [ ]( std::uint32_t& seed ) -> float
			{
				seed = seed * 1664525u + 1013904223u;
				return static_cast< float >( seed & 0xFFFF ) / 65535.0f;
			};

		const auto radius = std::clamp( cfg.radius.value, 50.0f, 900.0f );
		const auto height = std::clamp( cfg.height.value, 1.0f, 30.0f ) * 16.0f;
		const auto physics = static_cast< std::uint8_t >( cfg.soup_physics.value );
		const auto mode = static_cast< std::uint8_t >( cfg.soup_mode.value );
		const auto regular = static_cast< std::size_t >( std::clamp( cfg.regular_count.value, 0, 200 ) );
		const auto fireflies = static_cast< std::size_t >( std::clamp( cfg.firefly_count.value, 0, 64 ) );
		const auto active = std::min( k_max, regular + fireflies );

		const auto scene = memory::read<std::uintptr_t>( local_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
		if ( !scene ) return;
		const auto center = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );

		// Motes live in world space; they are only spawned/recycled around the
		// player and never translated by the camera.
		if ( !s_init )
		{
			for ( std::size_t i = 0; i < k_max; ++i )
			{
				auto& m = s_motes[ i ];
				m.pos = center + math::vector3{ ( frand( s_seed ) * 2.0f - 1.0f ) * radius, ( frand( s_seed ) * 2.0f - 1.0f ) * radius, frand( s_seed ) * height };
				m.vel = { frand( s_seed ) * 2.0f - 1.0f, frand( s_seed ) * 2.0f - 1.0f, ( frand( s_seed ) * 2.0f - 1.0f ) * 0.3f };
				m.phase = frand( s_seed ) * 6.2831853f;
			}
			s_init = true;
		}

		const auto dt = std::min( xdraw::delta_time( ), 0.05f );
		const auto speed = std::clamp( cfg.speed.value * 0.02f, 0.05f, 3.0f );

		const auto& col = cfg.color.value;
		const auto& sec = cfg.secondary_color.value;

		math::vector2 screen_pts[ k_max ];
		bool screen_valid[ k_max ]{};
		math::vector3 world_pts[ k_max ];

		for ( std::size_t i = 0; i < active; ++i )
		{
			auto& m = s_motes[ i ];
			m.firefly = ( i >= regular );
			m.size = m.firefly ? cfg.firefly_scale.value : cfg.regular_scale.value;

			if ( physics == 0 ) m.vel.z -= 3.0f * dt;
			else if ( physics == 2 ) m.vel.z += 1.5f * dt;

			const auto damp = 1.0f - std::clamp( 0.6f * dt, 0.0f, 1.0f );
			m.vel.x *= damp;
			m.vel.y *= damp;
			if ( physics != 0 ) m.vel.z *= damp;

			m.pos += m.vel * speed * dt;

			// Recycle only when it drifts outside the player's neighbourhood.
			const auto rel = m.pos - center;
			if ( rel.length_sqr( ) > radius * radius * 4.0f || rel.z < -height || rel.z > height * 3.0f )
			{
				m.pos = center + math::vector3{ ( frand( s_seed ) * 2.0f - 1.0f ) * radius, ( frand( s_seed ) * 2.0f - 1.0f ) * radius, frand( s_seed ) * height };
				m.vel = { frand( s_seed ) * 2.0f - 1.0f, frand( s_seed ) * 2.0f - 1.0f, ( frand( s_seed ) * 2.0f - 1.0f ) * 0.3f };
			}

			world_pts[ i ] = m.pos;

			const auto sp = systems::g_view.project( world_pts[ i ] );
			if ( systems::g_view.projection_valid( sp ) )
			{
				screen_pts[ i ] = { sp.x, sp.y };
				screen_valid[ i ] = true;
			}
		}

		// Network links.
		if ( cfg.links.value )
		{
			const auto max_d = std::max( 20.0f, cfg.link_distance.value * 10.0f );
			const auto max_d_sqr = max_d * max_d;
			const auto max_links = std::clamp( cfg.max_links.value, 1, 10 );

			for ( std::size_t i = 0; i < active; ++i )
			{
				if ( !screen_valid[ i ] ) continue;
				auto links = 0;
				// Each unordered pair is only tested once.
				for ( std::size_t j = i + 1; j < active && links < max_links; ++j )
				{
					if ( !screen_valid[ j ] ) continue;
					const auto d_sqr = ( world_pts[ i ] - world_pts[ j ] ).length_sqr( );
					if ( d_sqr > max_d_sqr ) continue;
					const auto& lc = cfg.link_color.value;
					const auto a = static_cast< std::uint8_t >( std::clamp( 1.0f - std::sqrt( d_sqr ) / max_d, 0.0f, 1.0f ) * static_cast< float >( lc.a ) );
					draw_list.line( screen_pts[ i ].x, screen_pts[ i ].y, screen_pts[ j ].x, screen_pts[ j ].y, xdraw::color{ lc.r, lc.g, lc.b, a }, 1.0f );
					++links;
				}
			}
		}

		const auto now = static_cast< float >( GetTickCount64( ) ) * 0.001f;

		for ( std::size_t i = 0; i < active; ++i )
		{
			if ( !screen_valid[ i ] ) continue;

			const auto& m = s_motes[ i ];
			const auto t = 0.5f + 0.5f * std::sin( now * 3.0f + m.phase );
			const auto tint = xdraw::color{
				static_cast< std::uint8_t >( col.r + ( sec.r - col.r ) * t ),
				static_cast< std::uint8_t >( col.g + ( sec.g - col.g ) * t ),
				static_cast< std::uint8_t >( col.b + ( sec.b - col.b ) * t ),
				col.a };

			const auto s = std::max( 0.6f, m.size ) * ( m.firefly ? 1.5f : 1.0f );
			const auto cx = screen_pts[ i ].x;
			const auto cy = screen_pts[ i ].y;

			if ( m.firefly )
			{
				const auto trail = std::clamp( cfg.firefly_trail.value, 0, 60 );
				if ( trail > 0 )
				{
					const auto tail_world = m.pos - m.vel * ( static_cast< float >( trail ) * 0.01f );
					const auto tail = systems::g_view.project( tail_world );
					if ( systems::g_view.projection_valid( tail ) )
					{
						draw_list.line( tail.x, tail.y, cx, cy, tint.alpha( static_cast< std::uint8_t >( col.a / 3 ) ), 1.0f );
					}
				}
			}

			auto tex = particle_textures::id::star;
			switch ( mode )
			{
			case 0: tex = particle_textures::id::star; break;
			case 1: tex = particle_textures::id::heart; break;
			case 2: tex = particle_textures::id::bloom; break;
			case 3: tex = particle_textures::id::bloom_soft; break;
			case 4: tex = particle_textures::id::flame; break;
			case 5: tex = particle_textures::id::snowflake; break;
			case 6: tex = particle_textures::id::geometric; break;
			case 7: tex = particle_textures::id::virus; break;
			case 8: tex = particle_textures::id::dollar; break;
			case 9: tex = particle_textures::id::coron; break;
			case 10: tex = particle_textures::id::blink; break;
			case 11: tex = particle_textures::id::firefly; break;
			case 12: tex = particle_textures::id::glyph_star; break;
			case 13: tex = particle_textures::id::glyph_cross; break;
			case 14: tex = particle_textures::id::glyph_circle; break;
			case 15: tex = particle_textures::id::glyph_triangle; break;
			case 16: tex = particle_textures::id::glyph_quad; break;
			case 17: tex = particle_textures::id::glyph_line; break;
			case 18: tex = particle_textures::id::glyph_zigzag; break;
			case 19: tex = particle_textures::id::glyph_arrow; break;
			case 20: tex = particle_textures::id::glyph_inf; break;
			default: tex = particle_textures::id::glyph_abs; break;
			}

			if ( auto* srv = particle_textures::get( tex ) )
			{
				const auto px = std::max( 2.0f, s * 3.0f );
				draw_list.image( cx - px * 0.5f, cy - px * 0.5f, px, px, srv, tint );
			}
		}
	}

	// Soup's Visuals / Trails: a vertical ribbon behind the player with Solid /
	// Faded (mid-height peak) / Invert (edge-weighted) alpha styles.
	void hud::do_soup_trails( xdraw::draw_list& draw_list, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_motion_trails;
		if ( !cfg.enabled.value )
		{
			return;
		}

		struct sample
		{
			math::vector3 pos{};   // attach point (feet/waist/weapon)
			math::vector3 base{};  // player origin (feet), the ribbon's vertical anchor
			float time{};
		};

		static constexpr std::size_t k_max = 48;

		struct tracker
		{
			std::uintptr_t pawn{};
			std::array<sample, k_max> samples{};
			std::size_t head{};
			std::size_t count{};
			math::vector3 last{};
			float last_time{};
		};

		static constexpr std::size_t k_max_trackers = 16;
		static std::array<tracker, k_max_trackers> s_trackers{};

		const auto cur = static_cast< float >( GetTickCount64( ) ) * 0.001f;
		const auto dur = std::clamp( cfg.duration.value, 0.2f, 4.0f );
		const auto band = std::clamp( cfg.height_pct.value / 100.0f, 0.05f, 1.0f ) * 34.0f;
		const auto down = std::clamp( cfg.down.value, -1.0f, 1.0f ) * 30.0f;
		const auto global_a = std::clamp( cfg.trail_alpha.value / 100.0f, 0.0f, 1.0f );
		const auto style = cfg.soup_style.value;

		// Each attachment lifts the whole ribbon so the choices actually differ.
		using attach_point = settings::misc::hud::motion_trails::attach_point;
		const auto attach_z_off = cfg.attachment.value == attach_point::weapon ? 36.0f
			: cfg.attachment.value == attach_point::waist ? 18.0f : 0.0f;

		const auto alpha_for = [ & ]( float y_frac ) -> float
			{
				switch ( style )
				{
				case settings::misc::hud::motion_trails::soup_trail_style::solid:
					return 1.0f;
				case settings::misc::hud::motion_trails::soup_trail_style::faded:
					return std::clamp( 1.0f - std::fabs( y_frac - 0.5f ) * 2.0f, 0.0f, 1.0f );
				default:
					{
						const auto af = std::clamp( cfg.alpha_factor.value / 100.0f, 0.0f, 1.0f );
						const auto edge = std::fabs( y_frac - 0.5f ) * 2.0f;
						return af + ( 1.0f - af ) * edge;
					}
				}
			};

		const auto process = [ & ]( std::uintptr_t pawn, tracker& tr )
			{
				if ( !pawn ) return;
				tr.pawn = pawn;

				const auto scene = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
				if ( !scene ) return;
				const auto origin = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );

				math::vector3 attach{};
				if ( cfg.attachment.value == settings::misc::hud::motion_trails::attach_point::weapon )
				{
					const auto h = systems::g_bones.get( pawn, cstypes::bone_ids::right_hand );
					attach = ( h.position.length_sqr( ) > 1.0f ) ? h.position : origin + math::vector3{ 0.0f, 0.0f, 48.0f };
				}
				else if ( cfg.attachment.value == settings::misc::hud::motion_trails::attach_point::waist )
				{
					const auto p = systems::g_bones.get( pawn, cstypes::bone_ids::pelvis );
					attach = ( p.position.length_sqr( ) > 1.0f ) ? p.position : origin + math::vector3{ 0.0f, 0.0f, 36.0f };
				}
				else
				{
					attach = origin + math::vector3{ 0.0f, 0.0f, 3.5f };
				}

				if ( ( attach - tr.last ).length_sqr( ) > 2.0f || ( cur - tr.last_time ) >= 0.02f )
				{
					tr.head = ( tr.head + 1 ) % k_max;
					tr.samples[ tr.head ] = { attach, origin, cur };
					if ( tr.count < k_max ) ++tr.count;
					tr.last = attach;
					tr.last_time = cur;
				}

				if ( tr.count < 2 ) return;

				math::vector3 wpos[ k_max ];
				math::vector3 wbase[ k_max ];
				float ts[ k_max ];
				std::size_t n = 0;

				for ( std::size_t i = 0; i < tr.count; ++i )
				{
					const std::size_t idx = ( tr.head + k_max - i ) % k_max;
					const auto& s = tr.samples[ idx ];
					const auto age = cur - s.time;
					if ( age > dur ) break;

					wpos[ n ] = s.pos;
					wbase[ n ] = s.base;
					ts[ n ] = age / dur;
					++n;
				}

				if ( n < 2 ) return;

				const auto& pri = cfg.color.value;
				const auto& sec = cfg.secondary_color.value;
				const auto taper = std::clamp( cfg.taper.value, 0.0f, 1.0f );
				const auto render_mode = cfg.render.value;

				const auto base_col = [ & ]( float t ) -> xdraw::color
					{
						const auto w = ( 1.0f - t ) * ( 1.0f - t );
						auto r = pri.r * ( 1.0f - t ) + sec.r * t;
						auto g = pri.g * ( 1.0f - t ) + sec.g * t;
						auto b = pri.b * ( 1.0f - t ) + sec.b * t;
						r = r * ( 1.0f - w ) + 255.0f * w;
						g = g * ( 1.0f - w ) + 255.0f * w;
						b = b * ( 1.0f - w ) + 255.0f * w;
						return xdraw::color{
							static_cast< std::uint8_t >( std::clamp( r, 0.0f, 255.0f ) ),
							static_cast< std::uint8_t >( std::clamp( g, 0.0f, 255.0f ) ),
							static_cast< std::uint8_t >( std::clamp( b, 0.0f, 255.0f ) ),
							255 };
					};

				const auto project_row = [ & ]( std::size_t i, float world_z, math::vector2& out ) -> bool
					{
						const auto sp = systems::g_view.project( math::vector3{ wpos[ i ].x, wpos[ i ].y, world_z } );
						if ( !systems::g_view.projection_valid( sp ) ) return false;
						out = { sp.x, sp.y };
						return true;
					};

				// Three rows (bottom / middle / top) so the vertical alpha
				// profiles (solid, faded, invert) are actually representable.
				const auto draw_ribbon = [ & ]( float band_scale, float alpha_scale )
					{
						draw_list.ensure_cmd( nullptr );
						for ( std::size_t i = 0; i + 1 < n; ++i )
						{
							const auto t0 = ts[ i ];
							const auto t1 = ts[ i + 1 ];
							const auto c0 = base_col( t0 );
							const auto c1 = base_col( t1 );

							const auto h0 = band * ( 1.0f - taper * t0 ) * band_scale;
							const auto h1 = band * ( 1.0f - taper * t1 ) * band_scale;
							const auto z0 = wbase[ i ].z + attach_z_off;
							const auto z1 = wbase[ i + 1 ].z + attach_z_off;

							math::vector2 b0{}, m0{}, p0{}, b1{}, m1{}, p1{};
							if ( !project_row( i, z0 - down, b0 ) || !project_row( i, z0 + h0 * 0.5f, m0 ) || !project_row( i, z0 + h0, p0 ) ||
								 !project_row( i + 1, z1 - down, b1 ) || !project_row( i + 1, z1 + h1 * 0.5f, m1 ) || !project_row( i + 1, z1 + h1, p1 ) )
							{
								continue;
							}

							const auto vcol = [ & ]( const xdraw::color& c, float y_frac, float t )
								{
									const auto a = alpha_for( y_frac ) * global_a * ( 1.0f - t ) * alpha_scale;
									return xdraw::color{ c.r, c.g, c.b, static_cast< std::uint8_t >( std::clamp( a * 255.0f, 0.0f, 255.0f ) ) };
								};

							const auto cbl = vcol( c0, 0.0f, t0 );
							const auto cml = vcol( c0, 0.5f, t0 );
							const auto ctl = vcol( c0, 1.0f, t0 );
							const auto cbr = vcol( c1, 0.0f, t1 );
							const auto cmr = vcol( c1, 0.5f, t1 );
							const auto ctr = vcol( c1, 1.0f, t1 );

							// Lower half (bottom -> middle).
							{
								const auto a = draw_list.emit_vtx( m0.x, m0.y, 0.0f, 0.0f, cml );
								const auto b = draw_list.emit_vtx( b0.x, b0.y, 0.0f, 0.0f, cbl );
								const auto c = draw_list.emit_vtx( b1.x, b1.y, 0.0f, 0.0f, cbr );
								const auto d = draw_list.emit_vtx( m1.x, m1.y, 0.0f, 0.0f, cmr );
								draw_list.emit_quad( a, b, c, d );
							}
							// Upper half (middle -> top).
							{
								const auto a = draw_list.emit_vtx( p0.x, p0.y, 0.0f, 0.0f, ctl );
								const auto b = draw_list.emit_vtx( m0.x, m0.y, 0.0f, 0.0f, cml );
								const auto c = draw_list.emit_vtx( m1.x, m1.y, 0.0f, 0.0f, cmr );
								const auto d = draw_list.emit_vtx( p1.x, p1.y, 0.0f, 0.0f, ctr );
								draw_list.emit_quad( a, b, c, d );
							}
						}
					};

				// Beads: Soup sprites strung along the path.
				const auto draw_beads = [ & ]( )
					{
						const auto bead_id = particle_textures::particle_order[
							static_cast< std::size_t >( std::clamp( cfg.bead_style.value, 0,
								static_cast< int >( particle_textures::particle_order.size( ) ) - 1 ) ) ];
						auto* srv = particle_textures::get( bead_id );
						if ( !srv )
						{
							return;
						}

						const auto bead_size = std::clamp( cfg.bead_size.value, 2.0f, 24.0f );
						for ( std::size_t i = 0; i < n; ++i )
						{
							const auto t = ts[ i ];
							const auto h = band * ( 1.0f - taper * t );
							const auto sp = systems::g_view.project(
								math::vector3{ wpos[ i ].x, wpos[ i ].y, wbase[ i ].z + attach_z_off + h * 0.5f } );
							if ( !systems::g_view.projection_valid( sp ) )
							{
								continue;
							}

							const auto c = base_col( t );
							const auto a = static_cast< std::uint8_t >( std::clamp(
								alpha_for( 0.5f ) * global_a * ( 1.0f - t ) * 255.0f, 0.0f, 255.0f ) );
							const auto px = bead_size * ( 1.0f - taper * t );
							draw_list.image( sp.x - px * 0.5f, sp.y - px * 0.5f, px, px, srv,
								xdraw::color{ c.r, c.g, c.b, a } );
						}
					};

				using trail_render = settings::misc::hud::motion_trails::trail_render;
				if ( cfg.glow.value && render_mode != trail_render::beads )
				{
					const auto gs = std::clamp( cfg.glow_strength.value, 0.2f, 2.0f );
					draw_ribbon( 1.7f, 0.35f * gs );
				}
				if ( render_mode != trail_render::beads )
				{
					draw_ribbon( 1.0f, 1.0f );
				}
				if ( render_mode != trail_render::ribbon )
				{
					draw_beads( );
				}
			};

		const auto get_tracker = [ & ]( std::uintptr_t pawn ) -> tracker&
			{
				for ( auto& tr : s_trackers )
				{
					if ( tr.pawn == pawn ) return tr;
				}
				for ( auto& tr : s_trackers )
				{
					if ( tr.pawn == 0 ) return tr;
				}
				return s_trackers[ 0 ];
			};

		if ( local_pawn )
		{
			process( local_pawn, get_tracker( local_pawn ) );
		}

		if ( cfg.render_others.value )
		{
			for ( const auto& p : systems::g_entities.get_by_type( systems::entities::type::player ) )
			{
				if ( !p.ptr ) continue;
				const auto handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
				const auto pawn = systems::g_entities.lookup( handle );
				if ( !pawn || pawn == local_pawn ) continue;
				process( pawn, get_tracker( pawn ) );
			}
		}
	}

	void hud::do_velocity( xdraw::draw_list& draw_list, float cx, float screen_h, std::uintptr_t local_pawn )
	{
		const auto& cfg = settings::g_misc.m_hud.m_velocity;
		if ( !cfg.counter.value && !cfg.chart.value )
		{
			return;
		}

		const auto velocity = memory::read<math::vector3>( local_pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
		const auto speed = velocity.length_2d( );
		const auto dt = xdraw::delta_time( );

		this->m_velocity_smoothed += ( speed - this->m_velocity_smoothed ) * std::min( 14.0f * dt, 1.0f );
		this->m_velocity_history[ this->m_velocity_history_head ] = this->m_velocity_smoothed;
		this->m_velocity_history_head = ( this->m_velocity_history_head + 1 ) % k_velocity_history;
		this->m_velocity_history_count = std::min( this->m_velocity_history_count + 1, k_velocity_history );

		const xdraw::color accent = cfg.color;
		const auto accent_dim = xdraw::color{ accent.r, accent.g, accent.b, static_cast< std::uint8_t >( accent.a * 0.55f ) };
		const auto accent_fill = xdraw::color{ accent.r, accent.g, accent.b, static_cast< std::uint8_t >( accent.a * 0.22f ) };

		constexpr auto inner_pad{ 4.0f };
		constexpr auto section_gap{ 3.0f };

		const auto chart_w = std::clamp( cfg.chart_width.value, 130.0f, 320.0f );
		const auto chart_h = std::clamp( cfg.chart_height.value, 26.0f, 80.0f );
		const auto chart_inner_h = chart_h - ( inner_pad + 2.0f ) * 2.0f;
		const auto chart_inner_w = chart_w - ( inner_pad + 2.0f ) * 2.0f;

		// Chart/bar scale: quantize the observed peak up to the next 50 u/s step
		// and ease it, so the counter's coverage bar and the chart share one
		// stable denominator instead of re-flickering a raw per-frame max.
		auto peak = 50.0f;
		for ( std::size_t i = 0; i < this->m_velocity_history_count; ++i )
		{
			peak = std::max( peak, this->m_velocity_history[ i ] );
		}
		const auto target_scale = std::ceil( std::max( peak, this->m_velocity_smoothed ) / 50.0f ) * 50.0f;
		this->m_velocity_scale += ( target_scale - this->m_velocity_scale ) * std::min( 6.0f * dt, 1.0f );
		const auto scale = std::max( this->m_velocity_scale, 50.0f );

		char speed_buf[ 16 ]{};
		std::snprintf( speed_buf, sizeof( speed_buf ), "%.0f", this->m_velocity_smoothed );

		const auto [ speed_vw, speed_vh ] = xdraw::measure_text( speed_buf );
		const auto [ tag_w, tag_h ] = xdraw::measure_text( "SPEED" );
		const auto [ speed_uw, speed_uh ] = xdraw::measure_text( " u/s" );

		const float counter_content_w = speed_vw + speed_uw + 16.0f;
		const float counter_pill_w = std::max( 90.0f, counter_content_w );
		const float counter_pill_h = 40.0f;

		const auto panel_w = cfg.chart.value ? chart_w : counter_pill_w + ( inner_pad + 2.0f ) * 2.0f;
		const auto counter_block_h = cfg.counter.value ? counter_pill_h + ( inner_pad + 2.0f ) * 2.0f : 0.0f;
		const auto chart_block_h = cfg.chart.value ? chart_h : 0.0f;
		const auto stack_gap = ( cfg.counter.value && cfg.chart.value ) ? section_gap : 0.0f;
		const auto panel_h = counter_block_h + stack_gap + chart_block_h;

		const auto bottom_offset = std::clamp( cfg.bottom_offset.value, 20.0f, 220.0f );
		const auto panel_x = std::floor( cx - panel_w * 0.5f );
		const auto panel_y = std::floor( screen_h - bottom_offset - panel_h );

		// Rounded panel card
		constexpr auto k_panel_r = xdraw::corner_radius{ 8.0f };
		draw_list.rect_filled( panel_x, panel_y, panel_w, panel_h, xdraw::color{ 14, 12, 18, 255 }, k_panel_r );
		draw_list.rect( panel_x + 1.0f, panel_y + 1.0f, panel_w - 2.0f, panel_h - 2.0f, tokens::col_border, xdraw::corner_radius{ 7.0f }, 1.0f );
		draw_list.rect_filled( panel_x + 2.0f, panel_y + 2.0f, panel_w - 4.0f, panel_h - 4.0f, tokens::col_dark, xdraw::corner_radius{ 6.0f } );
		// Top accent line (inset so it follows the rounded frame)
		draw_list.rect_filled( panel_x + 10.0f, panel_y + 2.0f, panel_w - 20.0f, 2.0f, accent, xdraw::corner_radius{ 1.0f } );

		auto content_y = panel_y + inner_pad + 2.0f;

		if ( cfg.counter.value )
		{
			const auto pill_x = panel_x + inner_pad + 2.0f;
			const auto pill_w = panel_w - ( inner_pad + 2.0f ) * 2.0f;
			draw_list.rect_filled( pill_x, content_y, pill_w, counter_pill_h, tokens::col_card, xdraw::corner_radius{ 6.0f } );
			draw_list.rect( pill_x, content_y, pill_w, counter_pill_h, tokens::col_border, xdraw::corner_radius{ 6.0f }, 1.0f );

			// "SPEED" micro label on left
			draw_list.text( pill_x + 8.0f, content_y + ( counter_pill_h - tag_h ) * 0.5f, "SPEED", tokens::col_text_dim );

			// Readout stacked: the value sits top-right in accent with the unit
			// tucked under it and right-aligned, so the row reads as one clean
			// number instead of a number dragging trailing dim text.
			const float num_x = pill_x + pill_w - speed_vw - 10.0f;
			const float unit_x = num_x + speed_vw - speed_uw;
			const auto readout_gap{ 2.0f };
			const auto readout_h = speed_vh + speed_uh + readout_gap;
			const auto readout_y = content_y + ( counter_pill_h - readout_h ) * 0.5f;
			draw_list.text( num_x, readout_y, speed_buf, accent );
			draw_list.text( unit_x, readout_y + speed_vh + readout_gap, " u/s", tokens::col_text_dim );

			// Coverage bar along the pill bottom, mirroring the chart fill at
			// the shared eased scale so the two sections read as one instrument.
			const auto bar_frac = std::clamp( this->m_velocity_smoothed / scale, 0.0f, 1.0f );
			const auto bar_w = ( pill_w - 8.0f ) * bar_frac;
			if ( bar_w >= 1.0f )
			{
				draw_list.rect_filled( pill_x + 4.0f, content_y + counter_pill_h - 3.0f, bar_w, 2.0f, accent, xdraw::corner_radius{ 1.0f } );
			}

			content_y += counter_pill_h + stack_gap + inner_pad;
		}

		if ( !cfg.chart.value || this->m_velocity_history_count < 2 || chart_inner_w <= 1.0f || chart_inner_h <= 1.0f )
		{
			return;
		}

		const auto chart_x = panel_x + inner_pad + 2.0f;
		const auto chart_y = content_y;
		const auto actual_chart_w = panel_w - ( inner_pad + 2.0f ) * 2.0f;
			draw_list.rect_filled( chart_x, chart_y, actual_chart_w, chart_inner_h, tokens::col_card, xdraw::corner_radius{ 5.0f } );
		draw_list.rect( chart_x, chart_y, actual_chart_w, chart_inner_h, tokens::col_border, 1.0f );

		const auto plot_x = chart_x + 3.0f;
		const auto plot_y = chart_y + 3.0f;
		const auto plot_w = actual_chart_w - 6.0f;
		const auto plot_h = chart_inner_h - 6.0f;
		const auto baseline_y = std::floor( plot_y + plot_h );

		draw_list.line( plot_x, baseline_y, plot_x + plot_w, baseline_y, tokens::col_line, 1.0f, false );

		const auto sample_count = this->m_velocity_history_count;
		const auto step_x = plot_w / static_cast< float >( sample_count - 1 );

		std::array<float, k_velocity_history * 2> points{};
		std::array<xdraw::color, k_velocity_history> point_colors{};

		for ( std::size_t i = 0; i < sample_count; ++i )
		{
			const auto idx = ( this->m_velocity_history_head + k_velocity_history - sample_count + i ) % k_velocity_history;
			const auto value = this->m_velocity_history[ idx ];
			const auto nx = plot_x + step_x * static_cast< float >( i );
			const auto ny = std::floor( plot_y + plot_h - ( value / scale ) * plot_h );

			points[ i * 2 ] = nx;
			points[ i * 2 + 1 ] = ny;
			point_colors[ i ] = accent;
		}

		for ( std::size_t i = 0; i + 1 < sample_count; ++i )
		{
			const auto x0 = points[ i * 2 ];
			const auto y0 = points[ i * 2 + 1 ];
			const auto x1 = points[ ( i + 1 ) * 2 ];
			const auto y1 = points[ ( i + 1 ) * 2 + 1 ];

			const auto fill_top = std::min( y0, y1 );
			const auto fill_h = std::max( 0.0f, baseline_y - fill_top );
			if ( fill_h > 0.0f )
			{
				const auto seg_w = std::max( 1.0f, x1 - x0 );
				draw_list.rect_filled_gradient(
					x0,
					fill_top,
					seg_w,
					fill_h,
					accent_fill,
					accent_fill,
					xdraw::color{ accent_fill.r, accent_fill.g, accent_fill.b, 0 },
					xdraw::color{ accent_fill.r, accent_fill.g, accent_fill.b, 0 }
				);
			}
		}

		draw_list.polyline_gradient(
			std::span<const float>{ points.data( ), sample_count * 2 },
			std::span<const xdraw::color>{ point_colors.data( ), sample_count },
			false,
			1.5f,
			false
		);

		const auto dot_x = points[ ( sample_count - 1 ) * 2 ];
		const auto dot_y = points[ ( sample_count - 1 ) * 2 + 1 ];
		draw_list.circle_filled( dot_x, dot_y, 2.5f, accent );
		draw_list.circle_filled( dot_x, dot_y, 1.2f, xdraw::color{ 255, 255, 255, 255 } );
	}

} // namespace features::misc
