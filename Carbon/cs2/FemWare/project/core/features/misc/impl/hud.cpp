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
		this->do_recoil( draw_list, cx, cy, static_cast< float >( screen_h ), local.pawn );
		this->do_china_hat( draw_list, local.pawn );
		this->do_soup_jump_rings( draw_list, local.pawn );
		this->do_soup_motes( draw_list, local.pawn );
		this->do_soup_trails( draw_list, local.pawn );
		this->do_velocity( draw_list, cx, static_cast< float >( screen_h ), local.pawn );
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
