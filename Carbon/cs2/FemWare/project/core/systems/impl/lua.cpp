#include <pch/pch.hpp>
#include <external/lua/lua.hpp>
#include <core/systems/systems.hpp>
#include <core/rendering/rendering.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <updater/updater.hpp>
#include <protection/game_addresses.hpp>
#include <random>

namespace systems {
	using ::lua_State;

namespace {
		static xdraw::draw_list* s_current_draw_list{ nullptr };

		// Scratch buffer reused by the polyline helpers — zero steady-state allocation per frame.
		static thread_local std::vector<float> s_poly_points{};

		// Instruction-count watchdog. Lua runs inside the render loop, so a runaway loop
		// must be aborted instead of freezing the frame. Each callback gets its own budget.
		inline constexpr int k_watchdog_top_level = 30'000'000;
		inline constexpr int k_watchdog_callback = 8'000'000;
		inline constexpr int k_max_consecutive_errors = 8;
		static int s_watchdog_limit = k_watchdog_callback;

		static void lua_watchdog_hook( lua_State* L, lua_Debug* )
		{
			luaL_error( L, "script aborted: exceeded the %d-instruction budget", s_watchdog_limit );
		}

		static void install_watchdog( lua_State* L, int limit )
		{
			s_watchdog_limit = limit;
			lua_sethook( L, lua_watchdog_hook, LUA_MASKCOUNT, limit );
		}

		static void remove_watchdog( lua_State* L )
		{
			lua_sethook( L, nullptr, 0, 0 );
		}

		// Helper to read color from Lua stack: supports either (r, g, b, [a]) OR a color table { r, g, b, [a] } or { [1], [2], [3], [4] }
		static xdraw::color get_lua_color( lua_State* L, int start_idx )
		{
			if ( lua_istable( L, start_idx ) )
			{
				lua_getfield( L, start_idx, "r" );
				int r = lua_isnumber( L, -1 ) ? static_cast<int>( lua_tointeger( L, -1 ) ) : -1;
				lua_pop( L, 1 );

				lua_getfield( L, start_idx, "g" );
				int g = lua_isnumber( L, -1 ) ? static_cast<int>( lua_tointeger( L, -1 ) ) : 255;
				lua_pop( L, 1 );

				lua_getfield( L, start_idx, "b" );
				int b = lua_isnumber( L, -1 ) ? static_cast<int>( lua_tointeger( L, -1 ) ) : 255;
				lua_pop( L, 1 );

				lua_getfield( L, start_idx, "a" );
				int a = lua_isnumber( L, -1 ) ? static_cast<int>( lua_tointeger( L, -1 ) ) : 255;
				lua_pop( L, 1 );

				if ( r == -1 )
				{
					lua_rawgeti( L, start_idx, 1 ); r = static_cast<int>( luaL_optinteger( L, -1, 255 ) ); lua_pop( L, 1 );
					lua_rawgeti( L, start_idx, 2 ); g = static_cast<int>( luaL_optinteger( L, -1, 255 ) ); lua_pop( L, 1 );
					lua_rawgeti( L, start_idx, 3 ); b = static_cast<int>( luaL_optinteger( L, -1, 255 ) ); lua_pop( L, 1 );
					lua_rawgeti( L, start_idx, 4 ); a = static_cast<int>( luaL_optinteger( L, -1, 255 ) ); lua_pop( L, 1 );
				}

				return xdraw::color{
					static_cast< std::uint8_t >( std::clamp( r, 0, 255 ) ),
					static_cast< std::uint8_t >( std::clamp( g, 0, 255 ) ),
					static_cast< std::uint8_t >( std::clamp( b, 0, 255 ) ),
					static_cast< std::uint8_t >( std::clamp( a, 0, 255 ) )
				};
			}

			const auto r = static_cast< std::uint8_t >( std::clamp( luaL_optinteger( L, start_idx, 255 ), 0LL, 255LL ) );
			const auto g = static_cast< std::uint8_t >( std::clamp( luaL_optinteger( L, start_idx + 1, 255 ), 0LL, 255LL ) );
			const auto b = static_cast< std::uint8_t >( std::clamp( luaL_optinteger( L, start_idx + 2, 255 ), 0LL, 255LL ) );
			const auto a = static_cast< std::uint8_t >( std::clamp( luaL_optinteger( L, start_idx + 3, 255 ), 0LL, 255LL ) );
			return xdraw::color{ r, g, b, a };
		}

		// render.color(r, g, b, [a]) -> table
		static int lua_render_color( lua_State* L )
		{
			const auto r = static_cast< int >( luaL_optinteger( L, 1, 255 ) );
			const auto g = static_cast< int >( luaL_optinteger( L, 2, 255 ) );
			const auto b = static_cast< int >( luaL_optinteger( L, 3, 255 ) );
			const auto a = static_cast< int >( luaL_optinteger( L, 4, 255 ) );

			lua_newtable( L );
			lua_pushstring( L, "r" ); lua_pushinteger( L, r ); lua_settable( L, -3 );
			lua_pushstring( L, "g" ); lua_pushinteger( L, g ); lua_settable( L, -3 );
			lua_pushstring( L, "b" ); lua_pushinteger( L, b ); lua_settable( L, -3 );
			lua_pushstring( L, "a" ); lua_pushinteger( L, a ); lua_settable( L, -3 );
			return 1;
		}

		// render.measure_text(str) -> w, h
		static int lua_render_measure_text( lua_State* L )
		{
			const char* text = luaL_checkstring( L, 1 );
			const auto [tw, th] = xdraw::measure_text( text );
			lua_pushnumber( L, static_cast< lua_Number >( tw ) );
			lua_pushnumber( L, static_cast< lua_Number >( th ) );
			return 2;
		}

		// render.world_to_screen(v3 or x, y, z) -> x, y, visible
		static int lua_render_world_to_screen( lua_State* L )
		{
			float wx = 0.0f, wy = 0.0f, wz = 0.0f;
			if ( lua_istable( L, 1 ) )
			{
				lua_getfield( L, 1, "x" ); wx = static_cast<float>( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 1, "y" ); wy = static_cast<float>( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 1, "z" ); wz = static_cast<float>( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
			}
			else
			{
				wx = static_cast< float >( luaL_checknumber( L, 1 ) );
				wy = static_cast< float >( luaL_checknumber( L, 2 ) );
				wz = static_cast< float >( luaL_checknumber( L, 3 ) );
			}

			const auto proj = systems::g_view.project_full( math::vector3{ wx, wy, wz } );
			lua_pushnumber( L, static_cast< lua_Number >( proj.screen.x ) );
			lua_pushnumber( L, static_cast< lua_Number >( proj.screen.y ) );
			lua_pushboolean( L, proj.on_screen );
			return 3;
		}

// render.text(x, y, text, r, g, b, [a], [centered] OR col_table, [centered])
		static int lua_render_text( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const char* text = luaL_checkstring( L, 3 );
			const auto col = get_lua_color( L, 4 );

			const auto centered = lua_istable( L, 4 )
				? lua_toboolean( L, 5 ) != 0
				: lua_toboolean( L, 8 ) != 0;

			if ( centered )
			{
				const auto [tw, th] = xdraw::measure_text( text );
				s_current_draw_list->text( x - tw * 0.5f, y, text, col );
			}
			else
			{
				s_current_draw_list->text( x, y, text, col );
			}
			return 0;
		}

		// render.gradient_rect(x, y, w, h, col1, col2, [horizontal=true], [radius])
		static int lua_render_gradient_rect( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto w = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto h = static_cast< float >( luaL_checknumber( L, 4 ) );

			const auto c1 = get_lua_color( L, 5 );
			const auto c2_start = lua_istable( L, 5 ) ? 6 : 9;
			const auto c2 = get_lua_color( L, c2_start );

			const auto opt_start = lua_istable( L, 5 )
				? ( lua_istable( L, 6 ) ? 7 : 10 )
				: 13;
			const auto horizontal = lua_isnone( L, opt_start ) ? true : lua_toboolean( L, opt_start ) != 0;
			const auto radius = static_cast< float >( luaL_optnumber( L, opt_start + 1, 0.0 ) );

			if ( horizontal )
			{
				if ( radius > 0.0f )
					s_current_draw_list->rect_filled_gradient( x, y, w, h, c1, c2, c2, c1, xdraw::corner_radius{ radius } );
				else
					s_current_draw_list->rect_filled_gradient( x, y, w, h, c1, c2, c2, c1 );
			}
			else
			{
				if ( radius > 0.0f )
					s_current_draw_list->rect_filled_gradient( x, y, w, h, c1, c1, c2, c2, xdraw::corner_radius{ radius } );
				else
					s_current_draw_list->rect_filled_gradient( x, y, w, h, c1, c1, c2, c2 );
			}
			return 0;
		}

		// render.line(x1, y1, x2, y2, r, g, b, [a], [thickness] OR col_table, [thickness])
		static int lua_render_line( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x1 = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y1 = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto x2 = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto y2 = static_cast< float >( luaL_checknumber( L, 4 ) );
			const auto col = get_lua_color( L, 5 );
			const auto th = lua_istable( L, 5 )
				? static_cast< float >( luaL_optnumber( L, 6, 1.0 ) )
				: static_cast< float >( luaL_optnumber( L, 9, 1.0 ) );

			s_current_draw_list->line( x1, y1, x2, y2, col, th );
			return 0;
		}

// render.rect(x, y, w, h, r, g, b, [a], [rounding], [thick] OR col_table, [rounding], [thick])
		static int lua_render_rect( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto w = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto h = static_cast< float >( luaL_checknumber( L, 4 ) );
			const auto col = get_lua_color( L, 5 );
			const auto radius = lua_istable( L, 5 )
				? static_cast< float >( luaL_optnumber( L, 6, 0.0 ) )
				: static_cast< float >( luaL_optnumber( L, 9, 0.0 ) );
			const auto th = lua_istable( L, 5 )
				? static_cast< float >( luaL_optnumber( L, 7, 1.0 ) )
				: static_cast< float >( luaL_optnumber( L, 10, 1.0 ) );

			if ( radius > 0.0f )
				s_current_draw_list->rect( x, y, w, h, col, xdraw::corner_radius{ radius }, th );
			else
				s_current_draw_list->rect( x, y, w, h, col, th );
			return 0;
		}

		// render.rect_filled(x, y, w, h, r, g, b, [a], [radius] OR col_table, [radius])
		static int lua_render_rect_filled( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto w = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto h = static_cast< float >( luaL_checknumber( L, 4 ) );
			const auto col = get_lua_color( L, 5 );
			const auto radius = lua_istable( L, 5 )
				? static_cast< float >( luaL_optnumber( L, 6, 0.0 ) )
				: static_cast< float >( luaL_optnumber( L, 9, 0.0 ) );

			if ( radius > 0.0f )
				s_current_draw_list->rect_filled( x, y, w, h, col, xdraw::corner_radius{ radius } );
			else
				s_current_draw_list->rect_filled( x, y, w, h, col );
			return 0;
		}

		// render.circle(x, y, radius, r, g, b, [a], [thickness] OR col_table, [thickness])
		static int lua_render_circle( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto rad = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto col = get_lua_color( L, 4 );
			const auto th = lua_istable( L, 4 )
				? static_cast< float >( luaL_optnumber( L, 5, 1.0 ) )
				: static_cast< float >( luaL_optnumber( L, 8, 1.0 ) );

			s_current_draw_list->circle( x, y, rad, col, th );
			return 0;
		}

		// render.circle_filled(x, y, radius, r, g, b, [a] OR col_table)
		static int lua_render_circle_filled( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto rad = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto col = get_lua_color( L, 4 );

			s_current_draw_list->circle_filled( x, y, rad, col );
			return 0;
		}

		// client.screen_size() -> w, h
		static int lua_client_screen_size( lua_State* L )
		{
			const auto [sw, sh] = xdraw::viewport_size( );
			lua_pushnumber( L, static_cast< lua_Number >( sw ) );
			lua_pushnumber( L, static_cast< lua_Number >( sh ) );
			return 2;
		}

		// client.log(text, [r, g, b])
		static int lua_client_log( lua_State* L )
		{
			const char* msg = luaL_checkstring( L, 1 );
			xdraw::color col{ 173, 192, 255, 255 };
			if ( lua_gettop( L ) >= 4 )
			{
				col = get_lua_color( L, 2 );
			}
			rendering::g_menu.append_lua_log( "SCRIPT", msg, col );
			return 0;
		}

		// client.get_cursor_pos() -> mx, my
		static int lua_client_get_cursor_pos( lua_State* L )
		{
			const auto& inp = xui::ctx( ).input;
			lua_pushnumber( L, static_cast< lua_Number >( inp.mouse_x ) );
			lua_pushnumber( L, static_cast< lua_Number >( inp.mouse_y ) );
			return 2;
		}

		// client.get_local() -> table or nil
		static int lua_client_get_local( lua_State* L )
		{
			const auto local = systems::g_local.get( );
			if ( !local.is_valid( ) )
			{
				lua_pushnil( L );
				return 1;
			}

			const auto view_pawn = local.view_pawn( );
			const auto health = view_pawn ? memory::read<int>( view_pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) ) : 0;
			const auto armor = view_pawn ? memory::read<int>( view_pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) ) : 0;
			const auto is_scoped = view_pawn ? memory::read<bool>( view_pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsScoped"_hash ) ) : false;
			const auto scene_node = view_pawn ? memory::read<std::uintptr_t>( view_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ) : 0;
			const auto origin = scene_node ? memory::read<math::vector3>( scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) ) : math::vector3{};
			const auto velocity = view_pawn ? memory::read<math::vector3>( view_pawn + SCHEMA( "C_CSPlayerPawn", "m_vecVelocity"_hash ) ) : math::vector3{};

			lua_newtable( L );

			lua_pushstring( L, "health" );
			lua_pushinteger( L, health );
			lua_settable( L, -3 );

			lua_pushstring( L, "armor" );
			lua_pushinteger( L, armor );
			lua_settable( L, -3 );

			lua_pushstring( L, "team" );
			lua_pushinteger( L, local.team );
			lua_settable( L, -3 );

lua_pushstring( L, "is_alive" );
			lua_pushboolean( L, local.is_alive );
			lua_settable( L, -3 );

			lua_pushstring( L, "is_scoped" );
			lua_pushboolean( L, is_scoped );
			lua_settable( L, -3 );

			lua_pushstring( L, "is_grounded" );
			lua_pushboolean( L, view_pawn ? ( memory::read<std::uint32_t>( view_pawn + SCHEMA( "C_BaseEntity", "m_fFlags"_hash ) ) & 1 ) != 0 : false );
			lua_settable( L, -3 );

			const auto speed = velocity.length_2d( );
			lua_pushstring( L, "speed" );
			lua_pushnumber( L, static_cast< lua_Number >( speed ) );
			lua_settable( L, -3 );

			// origin table
			lua_pushstring( L, "origin" );
			lua_newtable( L );
			lua_pushstring( L, "x" ); lua_pushnumber( L, origin.x ); lua_settable( L, -3 );
			lua_pushstring( L, "y" ); lua_pushnumber( L, origin.y ); lua_settable( L, -3 );
			lua_pushstring( L, "z" ); lua_pushnumber( L, origin.z ); lua_settable( L, -3 );
			lua_settable( L, -3 );

			// velocity table
			lua_pushstring( L, "velocity" );
			lua_newtable( L );
			lua_pushstring( L, "x" ); lua_pushnumber( L, velocity.x ); lua_settable( L, -3 );
			lua_pushstring( L, "y" ); lua_pushnumber( L, velocity.y ); lua_settable( L, -3 );
			lua_pushstring( L, "z" ); lua_pushnumber( L, velocity.z ); lua_settable( L, -3 );
			lua_settable( L, -3 );

			return 1;
		}

		// client.get_weapon() -> name, def_index (nil if unarmed/invalid)
		static int lua_client_get_weapon( lua_State* L )
		{
			const auto local = systems::g_local.get( );
			const auto pawn = local.view_pawn( );
			if ( !pawn )
			{
				lua_pushnil( L );
				lua_pushnil( L );
				return 2;
			}

			const auto weapon_services = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
			if ( !weapon_services )
			{
				lua_pushnil( L );
				lua_pushnil( L );
				return 2;
			}

			const auto handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
			const auto wep = handle ? systems::g_entities.lookup( handle ) : 0;
			if ( !wep )
			{
				lua_pushnil( L );
				lua_pushnil( L );
				return 2;
			}

			const auto vdata = memory::read<std::uintptr_t>( wep + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
			const auto name_ptr = vdata ? memory::read<const char*>( vdata + SCHEMA( "CCSWeaponBaseVData", "m_szName"_hash ) ) : nullptr;
			std::string name = name_ptr ? memory::read_string( reinterpret_cast< std::uintptr_t >( name_ptr ), 64 ) : std::string{};
			if ( name.starts_with( "weapon_" ) )
			{
				name.erase( 0, 7 );
			}

			const auto iv = wep + SCHEMA( "C_EconEntity", "m_AttributeManager"_hash ) + SCHEMA( "C_AttributeContainer", "m_Item"_hash );
			const auto def_index = memory::read<std::uint16_t>( iv + SCHEMA( "C_EconItemView", "m_iItemDefinitionIndex"_hash ) );

			lua_pushstring( L, name.c_str( ) );
			lua_pushinteger( L, def_index );
			return 2;
		}

		// events.listen(event_name, callback) / events.register(event_name, callback)
		static int lua_events_register( lua_State* L )
		{
			const char* ev_name = luaL_checkstring( L, 1 );
			if ( !lua_isfunction( L, 2 ) )
			{
				return luaL_error( L, "Expected function as callback argument" );
			}

			if ( std::strcmp( ev_name, "render" ) == 0 || std::strcmp( ev_name, "paint" ) == 0 || std::strcmp( ev_name, "on_draw" ) == 0 )
			{
				// Push copy of function to top and get registry reference
				lua_pushvalue( L, 2 );
				const int ref = luaL_ref( L, LUA_REGISTRYINDEX );
				systems::g_lua.add_render_callback( ref );
			}

			return 0;
		}

		// engine.get_ping() -> int
		static int lua_engine_get_ping( lua_State* L )
		{
			const auto local = systems::g_local.get( );
			const auto controller = local.view_controller( );
			if ( !controller )
			{
				lua_pushinteger( L, 0 );
				return 1;
			}
			const auto ping = memory::read<std::uint32_t>( controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
			lua_pushinteger( L, static_cast< lua_Integer >( ping ) );
			return 1;
		}

		// client.is_key_down(vk) -> bool
		static int lua_client_is_key_down( lua_State* L )
		{
			const auto vk = static_cast< int >( luaL_checkinteger( L, 1 ) );
			lua_pushboolean( L, ( GetAsyncKeyState( vk ) & 0x8000 ) != 0 );
			return 1;
		}

		// client.get_view_angles() -> pitch, yaw, roll
		static int lua_client_get_view_angles( lua_State* L )
		{
			const auto angles = systems::g_input.get_view_angles( );
			lua_pushnumber( L, angles.x );
			lua_pushnumber( L, angles.y );
			lua_pushnumber( L, angles.z );
			return 3;
		}

		// engine.get_time() -> seconds since boot
		static int lua_engine_get_time( lua_State* L )
		{
			lua_pushnumber( L, static_cast< lua_Number >( GetTickCount64( ) ) * 0.001 );
			return 1;
		}

		// engine.is_in_game() -> bool
		static int lua_engine_is_in_game( lua_State* L )
		{
			const auto local = systems::g_local.get( );
			if ( carbon::offsets::dwNetworkGameClient == 0 )
			{
				lua_pushboolean( L, local.is_valid( ) );
				return 1;
			}

			const auto ngc = memory::read<std::uintptr_t>( addresses::modules::engine2 + carbon::offsets::dwNetworkGameClient );
			if ( !ngc )
			{
				lua_pushboolean( L, false );
				return 1;
			}

			const auto sign_on = memory::read<int>( ngc + carbon::offsets::dwNetworkGameClient_signOnState );
			lua_pushboolean( L, sign_on >= 6 );
			return 1;
		}

		// engine.get_map_name() -> string
		static int lua_engine_get_map_name( lua_State* L )
		{
			const auto cv = CONVAR( "map" );
			const auto name_ptr = cv ? cv->m_value.sz : nullptr;
			if ( name_ptr )
			{
				lua_pushstring( L, memory::read_string( reinterpret_cast< std::uintptr_t >( name_ptr ), 96 ).c_str( ) );
			}
			else
			{
				lua_pushstring( L, "" );
			}
			return 1;
		}

		// client.get_players([enemies_only]) -> array of { name, team, ping, is_local, is_alive,
//                                      steam_id, health, armor, is_enemy, distance,
//                                      origin={x,y,z}, screen={x,y,visible} }
		static int lua_client_get_players( lua_State* L )
		{
			const auto enemies_only = lua_toboolean( L, 1 ) != 0;

			const auto local = systems::g_local.get( );
			const auto local_ctrl = local.controller;

			math::vector3 local_origin{};
			if ( const auto lpawn = local.view_pawn( ) )
			{
				if ( const auto scene = memory::read<std::uintptr_t>( lpawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ) )
				{
					local_origin = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
				}
			}

			lua_newtable( L );
			auto index = 1;
			for ( const auto& player : systems::g_entities.get_by_type( systems::entities::type::player ) )
			{
				const auto ctrl = player.ptr;
				if ( !ctrl )
				{
					continue;
				}

				lua_newtable( L );

				const auto name_ptr = memory::read<std::uintptr_t>( ctrl + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
				const auto name = name_ptr ? memory::read_string( name_ptr, 128 ) : std::string{};
				lua_pushstring( L, "name" ); lua_pushstring( L, name.c_str( ) ); lua_settable( L, -3 );

				const auto team = memory::read<int>( ctrl + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );

				lua_pushstring( L, "team" );
				lua_pushinteger( L, team );
				lua_settable( L, -3 );

				lua_pushstring( L, "ping" );
				lua_pushinteger( L, memory::read<int>( ctrl + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) ) );
				lua_settable( L, -3 );

				lua_pushstring( L, "is_local" );
				lua_pushboolean( L, ctrl == local_ctrl );
				lua_settable( L, -3 );

				lua_pushstring( L, "is_alive" );
				lua_pushboolean( L, memory::read<bool>( ctrl + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) ) );
				lua_settable( L, -3 );

				lua_pushstring( L, "steam_id" );
				const auto sid = std::to_string( memory::read<std::uint64_t>( ctrl + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) ) );
				lua_pushstring( L, sid.c_str( ) );
				lua_settable( L, -3 );

				const auto pawn_handle = memory::read<std::uint32_t>( ctrl + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
				const auto pawn = pawn_handle ? systems::g_entities.lookup( pawn_handle ) : 0;

				lua_pushstring( L, "health" );
				lua_pushinteger( L, pawn ? memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) ) : 0 );
				lua_settable( L, -3 );

				lua_pushstring( L, "armor" );
				lua_pushinteger( L, pawn ? memory::read<int>( pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) ) : 0 );
				lua_settable( L, -3 );

				const auto is_enemy = local.is_valid( ) && local.team > 1 && team > 1 && local.team != team;

				lua_pushstring( L, "is_enemy" );
				lua_pushboolean( L, is_enemy );
				lua_settable( L, -3 );

				math::vector3 origin{};
				if ( pawn )
				{
					if ( const auto scene = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ) )
					{
						origin = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
					}
				}

				lua_pushstring( L, "distance" );
				lua_pushnumber( L, static_cast< lua_Number >( ( origin - local_origin ).length( ) ) );
				lua_settable( L, -3 );

				lua_pushstring( L, "origin" );
				lua_newtable( L );
				lua_pushstring( L, "x" ); lua_pushnumber( L, origin.x ); lua_settable( L, -3 );
				lua_pushstring( L, "y" ); lua_pushnumber( L, origin.y ); lua_settable( L, -3 );
				lua_pushstring( L, "z" ); lua_pushnumber( L, origin.z ); lua_settable( L, -3 );
				lua_settable( L, -3 );

				const auto screen = systems::g_view.project( origin );
				lua_pushstring( L, "screen" );
				lua_newtable( L );
				lua_pushstring( L, "x" ); lua_pushnumber( L, screen.x ); lua_settable( L, -3 );
				lua_pushstring( L, "y" ); lua_pushnumber( L, screen.y ); lua_settable( L, -3 );
				lua_pushstring( L, "visible" ); lua_pushboolean( L, systems::g_view.projection_valid( screen ) ); lua_settable( L, -3 );
				lua_settable( L, -3 );

				if ( enemies_only && !is_enemy )
				{
					lua_pop( L, 1 );
					continue;
				}

				lua_rawseti( L, -2, index++ );
			}

			return 1;
		}
// render.polyline({ {x,y}, ... } or { x1,y1,x2,y2,... }, col, [closed], [thick])
		static int lua_render_polyline( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			luaL_checktype( L, 1, LUA_TTABLE );

			const auto n = static_cast< int >( luaL_len( L, 1 ) );
			if ( n <= 0 )
			{
				return 0;
			}

			lua_rawgeti( L, 1, 1 );
			const auto point_tables = lua_istable( L, -1 ) != 0;
			lua_pop( L, 1 );

			s_poly_points.clear( );
			s_poly_points.reserve( static_cast< std::size_t >( n ) * 2 );
			if ( point_tables )
			{
				for ( int i = 1; i <= n; ++i )
				{
					lua_rawgeti( L, 1, i );
					lua_getfield( L, -1, "x" );
					s_poly_points.push_back( static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ) );
					lua_pop( L, 1 );
					lua_getfield( L, -1, "y" );
					s_poly_points.push_back( static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ) );
					lua_pop( L, 1 );
					lua_pop( L, 1 );
				}
			}
			else
			{
				for ( int i = 1; i <= n; ++i )
				{
					lua_rawgeti( L, 1, i );
					s_poly_points.push_back( static_cast< float >( luaL_checknumber( L, -1 ) ) );
					lua_pop( L, 1 );
				}
			}

			const auto col = get_lua_color( L, 2 );
			const auto closed = lua_toboolean( L, 3 ) != 0;
			const auto th = static_cast< float >( luaL_optnumber( L, 4, 1.0 ) );

			s_current_draw_list->polyline( s_poly_points, col, closed, th, true );
			return 0;
		}

		// render.triangle(x1, y1, x2, y2, x3, y3, col, [filled], [thick])
		static int lua_render_triangle( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x0 = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y0 = static_cast< float >( luaL_checknumber( L, 2 ) );
			const auto x1 = static_cast< float >( luaL_checknumber( L, 3 ) );
			const auto y1 = static_cast< float >( luaL_checknumber( L, 4 ) );
			const auto x2 = static_cast< float >( luaL_checknumber( L, 5 ) );
			const auto y2 = static_cast< float >( luaL_checknumber( L, 6 ) );
			const auto col = get_lua_color( L, 7 );

			const auto opt = lua_istable( L, 7 ) ? 8 : 11;
			const auto filled = lua_toboolean( L, opt ) != 0;
			const auto th = static_cast< float >( luaL_optnumber( L, opt + 1, 1.0 ) );

			if ( filled )
			{
				s_current_draw_list->triangle_filled( x0, y0, x1, y1, x2, y2, col, true );
			}
			else
			{
				s_poly_points.clear( );
				s_poly_points.reserve( 6 );
				s_poly_points.push_back( x0 );
				s_poly_points.push_back( y0 );
				s_poly_points.push_back( x1 );
				s_poly_points.push_back( y1 );
				s_poly_points.push_back( x2 );
				s_poly_points.push_back( y2 );
				s_current_draw_list->polyline( s_poly_points, col, true, th, true );
			}
			return 0;
		}

		// render.text_outlined(x, y, str, col, [centered]) — 1px outline pass for readability
		static int lua_render_text_outlined( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const char* text = luaL_checkstring( L, 3 );
			const auto col = get_lua_color( L, 4 );
			const auto centered = lua_istable( L, 4 )
				? lua_toboolean( L, 5 ) != 0
				: lua_toboolean( L, 8 ) != 0;

			if ( centered )
			{
				const auto [tw, th] = xdraw::measure_text( text );
				s_current_draw_list->text( x - tw * 0.5f, y, text, col, xdraw::text_style::outlined );
			}
			else
			{
				s_current_draw_list->text( x, y, text, col, xdraw::text_style::outlined );
			}
			return 0;
		}

		// render.text_shadowed(x, y, str, col, [centered])
		static int lua_render_text_shadowed( lua_State* L )
		{
			if ( !s_current_draw_list ) return 0;
			const auto x = static_cast< float >( luaL_checknumber( L, 1 ) );
			const auto y = static_cast< float >( luaL_checknumber( L, 2 ) );
			const char* text = luaL_checkstring( L, 3 );
			const auto col = get_lua_color( L, 4 );
			const auto centered = lua_istable( L, 4 )
				? lua_toboolean( L, 5 ) != 0
				: lua_toboolean( L, 8 ) != 0;

			if ( centered )
			{
				const auto [tw, th] = xdraw::measure_text( text );
				s_current_draw_list->text( x - tw * 0.5f, y, text, col, xdraw::text_style::shadowed );
			}
			else
			{
				s_current_draw_list->text( x, y, text, col, xdraw::text_style::shadowed );
			}
			return 0;
		}

		// render.delta_time() -> seconds since the last frame (frame-independent animation helper)
		static int lua_render_delta_time( lua_State* L )
		{
			lua_pushnumber( L, static_cast< lua_Number >( xdraw::delta_time( ) ) );
			return 1;
		}

		// client.get_eye_pos() -> x, y, z
		static int lua_client_get_eye_pos( lua_State* L )
		{
			const auto local = systems::g_local.get( );
			const auto pawn = local.view_pawn( );
			if ( !pawn )
			{
				lua_pushnumber( L, 0.0 );
				lua_pushnumber( L, 0.0 );
				lua_pushnumber( L, 0.0 );
				return 3;
			}

			math::vector3 origin{};
			if ( const auto scene = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ) )
			{
				origin = memory::read<math::vector3>( scene + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
			}
			const auto eye = origin + memory::read<math::vector3>( pawn + SCHEMA( "C_BaseModelEntity", "m_vecViewOffset"_hash ) );

			lua_pushnumber( L, static_cast< lua_Number >( eye.x ) );
			lua_pushnumber( L, static_cast< lua_Number >( eye.y ) );
			lua_pushnumber( L, static_cast< lua_Number >( eye.z ) );
			return 3;
		}

		// engine.get_fps() -> smoothed frame rate
		static int lua_engine_get_fps( lua_State* L )
		{
			lua_pushnumber( L, static_cast< lua_Number >( xdraw::framerate( ) ) );
			return 1;
		}

		// engine.get_delta_time() -> seconds since the last frame
		static int lua_engine_get_delta_time( lua_State* L )
		{
			lua_pushnumber( L, static_cast< lua_Number >( xdraw::delta_time( ) ) );
			return 1;
		}

		// convar.get(name) -> value (string | number | boolean) or nil. Read-only.
		static int lua_convar_get( lua_State* L )
		{
			const char* name = luaL_checkstring( L, 1 );
			const auto cv = addresses::globals::cvar->find( ::protection::addresses::hash_const( name ) );
			if ( !cv )
			{
				lua_pushnil( L );
				return 1;
			}

			switch ( cv->m_type )
			{
			case 0: // CVTYPE_BOOL
				lua_pushboolean( L, cv->get<bool>( ) );
				return 1;
			case 1: // CVTYPE_FLOAT
				lua_pushnumber( L, static_cast< lua_Number >( cv->get<float>( ) ) );
				return 1;
			case 2: // CVTYPE_INT32
				lua_pushinteger( L, cv->get<int>( ) );
				return 1;
			case 3: // CVTYPE_STRING
			{
				const auto str = memory::read_string( reinterpret_cast< std::uintptr_t >( cv->m_value.sz ), 256 );
				lua_pushlstring( L, str.data( ), str.size( ) );
				return 1;
			}
			default:
				lua_pushnil( L );
				return 1;
			}
		}

		// math.clamp(v, lo, hi)
		static int lua_math_clamp( lua_State* L )
		{
			const auto v = luaL_checknumber( L, 1 );
			const auto lo = luaL_optnumber( L, 2, 0.0 );
			const auto hi = luaL_optnumber( L, 3, 1.0 );
			lua_pushnumber( L, std::clamp( v, lo, hi ) );
			return 1;
		}

		// math.lerp(a, b, t)
		static int lua_math_lerp( lua_State* L )
		{
			const auto a = luaL_checknumber( L, 1 );
			const auto b = luaL_checknumber( L, 2 );
			const auto t = luaL_checknumber( L, 3 );
			lua_pushnumber( L, a + ( b - a ) * t );
			return 1;
		}

		// math.saturate(t) -> clamp(v, 0, 1)
		static int lua_math_saturate( lua_State* L )
		{
			lua_pushnumber( L, std::clamp( luaL_checknumber( L, 1 ), 0.0, 1.0 ) );
			return 1;
		}

		// math.pingpong(t, len) -> 0..len..0..len oscillation
		static int lua_math_pingpong( lua_State* L )
		{
			const auto t = static_cast< double >( luaL_checknumber( L, 1 ) );
			const auto len = static_cast< double >( luaL_optnumber( L, 2, 1.0 ) );
			if ( len <= 0.0 )
			{
				lua_pushnumber( L, 0.0 );
				return 1;
			}
			const auto cycles = std::floor( t / len );
			const auto phase = t - cycles * len;
			lua_pushnumber( L, ( static_cast< long long >( cycles ) & 1 ) ? len - phase : phase );
			return 1;
		}

		// math.normalize_yaw(yaw) -> wrap into (-180, 180]
		static int lua_math_normalize_yaw( lua_State* L )
		{
			lua_pushnumber( L, std::remainder( luaL_checknumber( L, 1 ), 360.0 ) );
			return 1;
		}

		// math.angle_diff(a, b) -> shortest signed difference in degrees, (-180, 180]
		static int lua_math_angle_diff( lua_State* L )
		{
			lua_pushnumber( L, std::remainder( luaL_checknumber( L, 1 ) - luaL_checknumber( L, 2 ), 360.0 ) );
			return 1;
		}

		// math.distance(x1,y1,z1, x2,y2,z2) or (vec_a, vec_b) -> world units
		static int lua_math_distance( lua_State* L )
		{
			math::vector3 a{};
			math::vector3 b{};
			if ( lua_istable( L, 1 ) && lua_istable( L, 2 ) )
			{
				lua_getfield( L, 1, "x" ); a.x = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 1, "y" ); a.y = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 1, "z" ); a.z = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 2, "x" ); b.x = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 2, "y" ); b.y = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
				lua_getfield( L, 2, "z" ); b.z = static_cast< float >( luaL_optnumber( L, -1, 0.0 ) ); lua_pop( L, 1 );
			}
			else
			{
				a = math::vector3{
					static_cast< float >( luaL_checknumber( L, 1 ) ),
					static_cast< float >( luaL_checknumber( L, 2 ) ),
					static_cast< float >( luaL_checknumber( L, 3 ) )
				};
				b = math::vector3{
					static_cast< float >( luaL_checknumber( L, 4 ) ),
					static_cast< float >( luaL_checknumber( L, 5 ) ),
					static_cast< float >( luaL_checknumber( L, 6 ) )
				};
			}
			lua_pushnumber( L, static_cast< lua_Number >( ( a - b ).length( ) ) );
			return 1;
		}

		// math.vector_to_angle(x, y, z) -> pitch, yaw (degrees, FPS-style)
		static int lua_math_vector_to_angle( lua_State* L )
		{
			constexpr double k_rad_to_deg = 57.295779513082320876798154814105;
			const auto len = static_cast< double >( math::vector3{
				static_cast< float >( luaL_checknumber( L, 1 ) ),
				static_cast< float >( luaL_checknumber( L, 2 ) ),
				static_cast< float >( luaL_checknumber( L, 3 ) )
			}.length( ) );
			if ( len < 0.0001 )
			{
				lua_pushnumber( L, 0.0 );
				lua_pushnumber( L, 0.0 );
				return 2;
			}
			const auto pitch = std::asin( std::clamp( -luaL_checknumber( L, 3 ) / len, -1.0, 1.0 ) ) * k_rad_to_deg;
			const auto yaw = std::atan2( luaL_checknumber( L, 2 ), luaL_checknumber( L, 1 ) ) * k_rad_to_deg;
			lua_pushnumber( L, pitch );
			lua_pushnumber( L, yaw );
			return 2;
		}

		// math.randomf(min, max) -> uniform float in [min, max]
		static int lua_math_randomf( lua_State* L )
		{
			const auto lo = static_cast< double >( luaL_checknumber( L, 1 ) );
			const auto hi = static_cast< double >( luaL_checknumber( L, 2 ) );
			static std::mt19937_64 rng{ std::random_device{ }( ) };
			std::uniform_real_distribution<double> dist( lo, hi );
			lua_pushnumber( L, dist( rng ) );
			return 1;
		}
	} // namespace

	bool lua::initialize( )
	{
		std::lock_guard<std::mutex> lock( this->m_mutex );
		if ( this->m_L )
		{
			return true;
		}

		this->m_L = luaL_newstate( );
		if ( !this->m_L )
		{
			return false;
		}

// Standard safe libraries
		luaL_openlibs( this->m_L );

		// Scripting sandbox: strip stdlib surfaces that can touch the process or the disk.
		for ( const char* blocked : { "os", "io", "debug", "package", "dofile", "loadfile", "load", "require", "collectgarbage" } )
		{
			lua_pushnil( this->m_L );
			lua_setglobal( this->m_L, blocked );
		}

		// GC hysteresis tuned for a per-frame render loop: collect less eagerly, faster per step.
		lua_gc( this->m_L, LUA_GCSETPAUSE, 150 );
		lua_gc( this->m_L, LUA_GCSETSTEPMUL, 200 );

// Register "render" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_render_text ); lua_setfield( this->m_L, -2, "text" );
		lua_pushcfunction( this->m_L, lua_render_text_outlined ); lua_setfield( this->m_L, -2, "text_outlined" );
		lua_pushcfunction( this->m_L, lua_render_text_shadowed ); lua_setfield( this->m_L, -2, "text_shadowed" );
		lua_pushcfunction( this->m_L, lua_render_line ); lua_setfield( this->m_L, -2, "line" );
		lua_pushcfunction( this->m_L, lua_render_rect ); lua_setfield( this->m_L, -2, "rect" );
		lua_pushcfunction( this->m_L, lua_render_rect_filled ); lua_setfield( this->m_L, -2, "rect_filled" );
		lua_pushcfunction( this->m_L, lua_render_gradient_rect ); lua_setfield( this->m_L, -2, "gradient_rect" );
		lua_pushcfunction( this->m_L, lua_render_circle ); lua_setfield( this->m_L, -2, "circle" );
		lua_pushcfunction( this->m_L, lua_render_circle_filled ); lua_setfield( this->m_L, -2, "circle_filled" );
		lua_pushcfunction( this->m_L, lua_render_polyline ); lua_setfield( this->m_L, -2, "polyline" );
		lua_pushcfunction( this->m_L, lua_render_triangle ); lua_setfield( this->m_L, -2, "triangle" );
		lua_pushcfunction( this->m_L, lua_render_color ); lua_setfield( this->m_L, -2, "color" );
		lua_pushcfunction( this->m_L, lua_render_measure_text ); lua_setfield( this->m_L, -2, "measure_text" );
		lua_pushcfunction( this->m_L, lua_client_screen_size ); lua_setfield( this->m_L, -2, "screen_size" );
		lua_pushcfunction( this->m_L, lua_render_world_to_screen ); lua_setfield( this->m_L, -2, "world_to_screen" );
		lua_pushcfunction( this->m_L, lua_render_delta_time ); lua_setfield( this->m_L, -2, "delta_time" );
		lua_setglobal( this->m_L, "render" );

		// Register "client" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_client_screen_size ); lua_setfield( this->m_L, -2, "screen_size" );
		lua_pushcfunction( this->m_L, lua_client_log ); lua_setfield( this->m_L, -2, "log" );
		lua_pushcfunction( this->m_L, lua_client_get_local ); lua_setfield( this->m_L, -2, "get_local" );
		lua_pushcfunction( this->m_L, lua_client_get_weapon ); lua_setfield( this->m_L, -2, "get_weapon" );
		lua_pushcfunction( this->m_L, lua_client_get_eye_pos ); lua_setfield( this->m_L, -2, "get_eye_pos" );
		lua_pushcfunction( this->m_L, lua_client_is_key_down ); lua_setfield( this->m_L, -2, "is_key_down" );
		lua_pushcfunction( this->m_L, lua_client_get_view_angles ); lua_setfield( this->m_L, -2, "get_view_angles" );
		lua_pushcfunction( this->m_L, lua_client_get_cursor_pos ); lua_setfield( this->m_L, -2, "get_cursor_pos" );
		lua_pushcfunction( this->m_L, lua_client_get_players ); lua_setfield( this->m_L, -2, "get_players" );
		lua_setglobal( this->m_L, "client" );

		// Register "events" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_events_register ); lua_setfield( this->m_L, -2, "register" );
		lua_pushcfunction( this->m_L, lua_events_register ); lua_setfield( this->m_L, -2, "listen" );
		lua_setglobal( this->m_L, "events" );

		// Register "engine" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_engine_get_ping ); lua_setfield( this->m_L, -2, "get_ping" );
		lua_pushcfunction( this->m_L, lua_engine_get_time ); lua_setfield( this->m_L, -2, "get_time" );
		lua_pushcfunction( this->m_L, lua_engine_get_fps ); lua_setfield( this->m_L, -2, "get_fps" );
		lua_pushcfunction( this->m_L, lua_engine_get_delta_time ); lua_setfield( this->m_L, -2, "get_delta_time" );
		lua_pushcfunction( this->m_L, lua_engine_is_in_game ); lua_setfield( this->m_L, -2, "is_in_game" );
		lua_pushcfunction( this->m_L, lua_engine_get_map_name ); lua_setfield( this->m_L, -2, "get_map_name" );
		lua_pushcfunction( this->m_L, lua_client_is_key_down ); lua_setfield( this->m_L, -2, "is_key_down" );
		lua_setglobal( this->m_L, "engine" );

		// Register "convar" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_convar_get ); lua_setfield( this->m_L, -2, "get" );
		lua_setglobal( this->m_L, "convar" );

		// Register "math" extensions onto the existing standard math library
		lua_getglobal( this->m_L, "math" );
		lua_pushcfunction( this->m_L, lua_math_clamp ); lua_setfield( this->m_L, -2, "clamp" );
		lua_pushcfunction( this->m_L, lua_math_lerp ); lua_setfield( this->m_L, -2, "lerp" );
		lua_pushcfunction( this->m_L, lua_math_saturate ); lua_setfield( this->m_L, -2, "saturate" );
		lua_pushcfunction( this->m_L, lua_math_pingpong ); lua_setfield( this->m_L, -2, "pingpong" );
		lua_pushcfunction( this->m_L, lua_math_normalize_yaw ); lua_setfield( this->m_L, -2, "normalize_yaw" );
		lua_pushcfunction( this->m_L, lua_math_angle_diff ); lua_setfield( this->m_L, -2, "angle_diff" );
		lua_pushcfunction( this->m_L, lua_math_distance ); lua_setfield( this->m_L, -2, "distance" );
		lua_pushcfunction( this->m_L, lua_math_vector_to_angle ); lua_setfield( this->m_L, -2, "vector_to_angle" );
		lua_pushcfunction( this->m_L, lua_math_randomf ); lua_setfield( this->m_L, -2, "randomf" );
		lua_pop( this->m_L, 1 );

		// Register "entity" table
		lua_newtable( this->m_L );
		lua_pushcfunction( this->m_L, lua_client_get_local ); lua_setfield( this->m_L, -2, "get_local" );
		lua_setglobal( this->m_L, "entity" );

		return true;
	}

void lua::shutdown( )
	{
		std::lock_guard<std::mutex> lock( this->m_mutex );
		if ( this->m_L )
		{
			for ( const auto ref : this->m_render_callbacks )
			{
				luaL_unref( this->m_L, LUA_REGISTRYINDEX, ref );
			}
			this->m_render_callbacks.clear( );
			this->m_render_error_counts.clear( );

			lua_close( this->m_L );
			this->m_L = nullptr;
		}
	}

	void lua::reset( )
	{
		this->shutdown( );
		this->initialize( );
	}

	void lua::add_render_callback( int ref )
	{
		this->m_render_callbacks.push_back( ref );
	}

	bool lua::execute_buffer( const std::string& buffer, std::string& err_out )
	{
		std::lock_guard<std::mutex> lock( this->m_mutex );
		if ( !this->m_L )
		{
			if ( !this->initialize( ) )
			{
				err_out = "Failed to initialize Lua VM state.";
				return false;
			}
		}

// Clear previous callbacks so re-executing buffer replaces old script
		for ( const auto ref : this->m_render_callbacks )
		{
			luaL_unref( this->m_L, LUA_REGISTRYINDEX, ref );
		}
		this->m_render_callbacks.clear( );
		this->m_render_error_counts.clear( );

		const auto load_status = luaL_loadbuffer( this->m_L, buffer.data( ), buffer.size( ), "lua_script" );
		if ( load_status != LUA_OK )
		{
			err_out = lua_tostring( this->m_L, -1 );
			lua_pop( this->m_L, 1 );
			return false;
		}

		install_watchdog( this->m_L, k_watchdog_top_level );
		const auto pcall_status = lua_pcall( this->m_L, 0, 0, 0 );
		remove_watchdog( this->m_L );

		if ( pcall_status != LUA_OK )
		{
			err_out = lua_tostring( this->m_L, -1 );
			lua_pop( this->m_L, 1 );
			return false;
		}

		lua_gc( this->m_L, LUA_GCSTEP, 0 );
		return true;
	}

void lua::on_render( xdraw::draw_list& draw_list )
	{
		if ( !this->m_L || this->m_render_callbacks.empty( ) )
		{
			return;
		}

		std::lock_guard<std::mutex> lock( this->m_mutex );
		s_current_draw_list = &draw_list;

		const auto callbacks = this->m_render_callbacks;
		for ( const auto ref : callbacks )
		{
			lua_rawgeti( this->m_L, LUA_REGISTRYINDEX, ref );
			if ( lua_isfunction( this->m_L, -1 ) )
			{
				install_watchdog( this->m_L, k_watchdog_callback );
				const auto status = lua_pcall( this->m_L, 0, 0, 0 );
				remove_watchdog( this->m_L );

				if ( status != LUA_OK )
				{
					const char* err = lua_tostring( this->m_L, -1 );
					auto& err_count = this->m_render_error_counts[ ref ];

					if ( err_count < k_max_consecutive_errors )
					{
						rendering::g_menu.append_lua_log( "RUNTIME ERROR", err ? err : "Unknown runtime error", xdraw::color{ 248, 113, 113, 255 } );
					}

					if ( ++err_count >= k_max_consecutive_errors )
					{
						rendering::g_menu.append_lua_log( "RUNTIME ERROR", "Callback disabled after " + std::to_string( k_max_consecutive_errors ) + " consecutive errors — re-run the script to re-enable it.", xdraw::color{ 248, 113, 113, 255 } );
						luaL_unref( this->m_L, LUA_REGISTRYINDEX, ref );
						this->m_render_callbacks.erase( std::find( this->m_render_callbacks.begin( ), this->m_render_callbacks.end( ), ref ) );
						this->m_render_error_counts.erase( ref );
					}

					lua_pop( this->m_L, 1 );
				}
				else
				{
					const auto it = this->m_render_error_counts.find( ref );
					if ( it != this->m_render_error_counts.end( ) )
					{
						this->m_render_error_counts.erase( it );
					}
				}
			}
			else
			{
				lua_pop( this->m_L, 1 );
			}
		}

		lua_gc( this->m_L, LUA_GCSTEP, 0 );
		s_current_draw_list = nullptr;
	}

} // namespace systems
