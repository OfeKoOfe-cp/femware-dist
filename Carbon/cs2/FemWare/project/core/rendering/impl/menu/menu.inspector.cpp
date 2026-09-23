#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>
#include <core/resources/workspace.hpp>
#include <core/features/features.hpp>
#include <protection/game_addresses.hpp>

#include "../../rendering.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cctype>
#include <unordered_set>
#include "menu.workspaces.detail.hpp"

namespace rendering {

	namespace {
		enum class lua_tok
		{
			plain,
			keyword,
			string_lit,
			number_lit,
			comment
		};

		static xdraw::color lua_tok_color( lua_tok t )
		{
			switch ( t )
			{
				case lua_tok::keyword:   return xdraw::color{ 198, 120, 221, 255 };
				case lua_tok::string_lit: return xdraw::color{ 158, 206, 106, 255 };
				case lua_tok::number_lit: return xdraw::color{ 224, 175, 104, 255 };
				case lua_tok::comment:   return xdraw::color{ 127, 139, 155, 255 };
				default:                  return xdraw::color{ 173, 199, 235, 235 };
			}
		}

		static bool lua_is_keyword( const std::string& w )
		{
			static const std::unordered_set<std::string> kws = {
				"and", "break", "do", "else", "elseif", "end", "false", "for", "function",
				"if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true",
				"until", "while"
			};
			return kws.count( w ) != 0;
		}

		static void draw_highlighted_lua( xdraw::draw_list& dl, float x, float y, const std::string& line )
		{
			struct seg
			{
				lua_tok type;
				std::string text;
			};
			std::vector<seg> segs;
			segs.reserve( 16 );

			const auto n = line.size( );
			std::size_t i = 0;
			while ( i < n )
			{
				const char c = line[ i ];

				if ( c == '-' && i + 1 < n && line[ i + 1 ] == '-' )
				{
					segs.push_back( { lua_tok::comment, line.substr( i ) } );
					break;
				}

				if ( c == '"' || c == '\'' )
				{
					std::size_t j = i + 1;
					while ( j < n )
					{
						if ( line[ j ] == '\\' && j + 1 < n ) { j += 2; continue; }
						if ( line[ j ] == c ) { ++j; break; }
						++j;
					}
					segs.push_back( { lua_tok::string_lit, line.substr( i, j - i ) } );
					i = j;
					continue;
				}

				if ( std::isdigit( static_cast< unsigned char >( c ) ) || ( c == '.' && i + 1 < n && std::isdigit( static_cast< unsigned char >( line[ i + 1 ] ) ) ) )
				{
					std::size_t j = i + 1;
					while ( j < n )
					{
						const char d = line[ j ];
						const bool numeric = std::isalnum( static_cast< unsigned char >( d ) ) || d == '.' || d == '_';
						if ( !numeric ) break;
						if ( d == '.' && j > i && line[ j - 1 ] == '.' ) break;
						++j;
					}
					segs.push_back( { lua_tok::number_lit, line.substr( i, j - i ) } );
					i = j;
					continue;
				}

				if ( std::isalpha( static_cast< unsigned char >( c ) ) || c == '_' )
				{
					std::size_t j = i + 1;
					while ( j < n && ( std::isalnum( static_cast< unsigned char >( line[ j ] ) ) || line[ j ] == '_' ) ) ++j;
					const auto word = line.substr( i, j - i );
					segs.push_back( { lua_is_keyword( word ) ? lua_tok::keyword : lua_tok::plain, word } );
					i = j;
					continue;
				}

				std::size_t j = i + 1;
				while ( j < n )
				{
					const char d = line[ j ];
					if ( d == '"' || d == '\'' ) break;
					if ( d == '-' && j + 1 < n && line[ j + 1 ] == '-' ) break;
					if ( std::isalnum( static_cast< unsigned char >( d ) ) || d == '_' ) break;
					if ( std::isdigit( static_cast< unsigned char >( d ) ) ) break;
					if ( d == '.' && j + 1 < n && std::isdigit( static_cast< unsigned char >( line[ j + 1 ] ) ) ) break;
					++j;
				}
				segs.push_back( { lua_tok::plain, line.substr( i, j - i ) } );
				i = j;
			}

			float ox = 0.0f;
			for ( const auto& s : segs )
			{
				if ( s.text.empty( ) ) continue;
				dl.text( x + ox, y, s.text, lua_tok_color( s.type ) );
				const auto [sw, sh] = xdraw::measure_text( s.text );
				ox += sw;
			}
		}
	}

	void menu::draw_documentation( float sw, float sh )
	{
		( void )sw;
		( void )sh;

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_documentation", this->m_docs_x, this->m_docs_y, this->m_docs_w, this->m_docs_h, false, 550.0f, 400.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_docs_x;
		const auto wy = this->m_docs_y;
		const auto ww = this->m_docs_w;
		const auto wh = this->m_docs_h;
		const auto& inp = xui::ctx( ).input;

		// Ambient drop shadow & glow
		for ( int s_i = 5; s_i >= 1; --s_i )
		{
			const float spread = static_cast< float >( s_i ) * 3.0f;
			const std::uint8_t a = static_cast< std::uint8_t >( 6.0f * ( 6 - s_i ) * reveal );
			dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
				xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
		}
		dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast<std::uint8_t>( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );
		dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

		// Title Bar
		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "DOCUMENTATION" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

			dl.text( wx + 12.0f, cy, "DOCUMENTATION", tokens::col_accent );

			// Close button [X]
			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_docs = false;
			}
			if ( cbtn_hov )
			{
				dl.rect_filled( cbtn_rect.x, cbtn_rect.y, cbtn_rect.w, cbtn_rect.h, xdraw::color{ 239, 68, 68, 70 }, xdraw::corner_radius{ tokens::round_md } );
			}
			const float ccx = cbtn_x + cbtn_sz * 0.5f;
			const float ccy = cbtn_y + cbtn_sz * 0.5f;
			const auto cross_col = cbtn_hov ? xdraw::color{ 248, 113, 113, 255 } : tokens::col_text_dim;
			dl.line( ccx - 4.0f, ccy - 4.0f, ccx + 4.0f, ccy + 4.0f, cross_col, 1.4f );
			dl.line( ccx - 4.0f, ccy + 4.0f, ccx + 4.0f, ccy - 4.0f, cross_col, 1.4f );
		}

		// Inset accent top-edge line (follows the rounded frame)
		dl.rect_filled( wx + tokens::round_lg, wy + 1.0f, ww - tokens::round_lg * 2.0f, 2.0f,
			tokens::col_accent.alpha( static_cast< std::uint8_t >( tokens::col_accent.a * reveal ) ),
			xdraw::corner_radius{ 1.0f } );

		const float pad = 8.0f;
		const float body_x = wx + pad;
		const float body_y = wy + tokens::title_bar_h + pad;
		const float body_w = ww - pad * 2.0f;
		const float body_h = wh - tokens::title_bar_h - pad * 2.0f;

		xui::layout::set_cursor( body_x - wx, body_y - wy );

		static constexpr const char* k_doc_tabs[] = { "Events", "Render API", "Client API", "Engine API", "Math", "Convar" };
		rendering::group_subtabs( k_doc_tabs, 6, this->m_doc_category );

		const float content_h = body_h - 26.0f;
		const float content_y = body_y + 26.0f;

		{
			// ── Tab 1..4: Lua API Reference ───
			struct doc_item
			{
				const char* name;
				const char* signature;
				const char* snippet;
				const char* desc;
			};

			static const std::vector<doc_item> k_events_docs = {
				{ "events.listen", "events.listen(\"on_draw\" | \"render\" | \"paint\", fn)", "events.listen(\"on_draw\", function()\n    local sw, sh = render.screen_size()\n    local text = \"femware\"\n    local tw, th = render.measure_text(text)\n    render.text(sw - tw - 16, 16, text, render.color(255, 95, 175, 255))\nend)", "Registers a callback invoked every frame to render 2D elements. The event name is one of \"on_draw\", \"render\", or \"paint\" (aliases). Re-running a script replaces all previously registered callbacks. The runtime is sandboxed: os, io, debug, package, load, dofile, loadfile, require, and collectgarbage are unavailable; each callback also runs under an instruction budget so runaway loops abort with an error instead of freezing the game." }
			};

			static const std::vector<doc_item> k_render_docs = {
				{ "render.text", "render.text(x, y, str, col, [centered])", "local white = render.color(255, 255, 255, 255)\nrender.text(100, 100, \"femware cs2\", white, false)", "Renders formatted text with the active cheat font. Optional centered flag centers text at (x, y)." },
				{ "render.line", "render.line(x1, y1, x2, y2, col, [thick])", "local c = render.color(255, 95, 175, 255)\nrender.line(100, 100, 250, 100, c, 2.0)", "Draws a 2D line between two screen positions with optional thickness." },
				{ "render.rect", "render.rect(x, y, w, h, col, [rounding], [thick])", "local col = render.color(255, 95, 175, 255)\nrender.rect(50, 50, 140, 60, col, 2.0, 1.0)", "Draws an outlined rectangle on screen with optional corner rounding." },
				{ "render.rect_filled", "render.rect_filled(x, y, w, h, col, [rounding])", "local bg = render.color(18, 14, 22, 220)\nrender.rect_filled(50, 50, 140, 60, bg, 3.0)", "Draws a solid filled rectangle on screen with optional corner rounding." },
				{ "render.gradient_rect", "render.gradient_rect(x, y, w, h, col1, col2, [horizontal], [radius])", "local c1 = render.color(255, 95, 175, 255)\nlocal c2 = render.color(140, 70, 240, 0)\nrender.gradient_rect(100, 100, 200, 30, c1, c2, true, 4.0)", "Draws a filled rectangle with a smooth linear two-color gradient (horizontal or vertical)." },
				{ "render.circle", "render.circle(cx, cy, r, col, [thick])", "render.circle(300, 300, 45.0, render.color(255, 95, 175, 255), 1.5)", "Draws an outlined circular shape." },
				{ "render.circle_filled", "render.circle_filled(cx, cy, r, col)", "render.circle_filled(300, 300, 8.0, render.color(255, 255, 255, 255))", "Draws a solid filled circle on screen." },
				{ "render.polyline", "render.polyline({ {x,y}, ... }, col, [closed], [thick])", "local pts = { {100,100}, {150,80}, {200,120}, {250,90} }\nrender.polyline(pts, render.color(255, 95, 175, 255), true, 2.0)", "Draws a connected path through an array of {x,y} points (or a flat x,y,x,y array). Pass closed=true to join the last point to the first." },
				{ "render.triangle", "render.triangle(x1,y1,x2,y2,x3,y3, col, [filled], [thick])", "render.triangle(400, 200, 460, 300, 340, 300, render.color(74, 222, 128, 255), true)", "Draws a triangle shape. Filled by default when [filled]=true, otherwise an outlined path." },
				{ "render.text_outlined", "render.text_outlined(x, y, str, col, [centered])", "render.text_outlined(100, 100, \"femware\", render.color(255, 255, 255, 255))", "Renders text with a crisp 1px black outline for readability on noisy backgrounds." },
				{ "render.text_shadowed", "render.text_shadowed(x, y, str, col, [centered])", "render.text_shadowed(100, 130, \"femware\", render.color(255, 255, 255, 255))", "Renders text with a soft drop shadow offset one pixel down-right." },
				{ "render.delta_time", "render.delta_time() -> seconds", "local dt = render.delta_time()\nlocal sw = render.screen_size()\nlocal x = ((x or 0) + 200 * dt) % sw\nrender.circle_filled(x, 100, 6.0, render.color(255, 95, 175, 255))", "Returns fractional seconds since the last frame — use it for frame-independent animation inside draw callbacks." },
				{ "render.color", "render.color(r, g, b, [a]) -> col", "local accent = render.color(255, 95, 175, 255)", "Constructs an RGBA color table compatible with all render functions." },
				{ "render.measure_text", "render.measure_text(str) -> w, h", "local w, h = render.measure_text(\"femware\")", "Calculates pixel width and height for the given text string." },
				{ "render.screen_size", "render.screen_size() -> w, h", "local sw, sh = render.screen_size()", "Returns display viewport dimensions in pixels." },
				{ "render.world_to_screen", "render.world_to_screen(x, y, z) -> sx, sy, vis", "local me = client.get_local()\nif me and me.is_alive then\n    local sx, sy, vis = render.world_to_screen(me.origin.x, me.origin.y, me.origin.z)\n    if vis then\n        render.circle_filled(sx, sy, 4.0, render.color(255, 95, 175, 255))\n    end\nend", "Projects 3D world space coordinates into 2D screen coordinates." }
			};

			static const std::vector<doc_item> k_client_docs = {
				{ "client.get_local", "client.get_local() -> table", "local me = client.get_local()\nif me and me.is_alive then\n    print(string.format(\"HP: %d Speed: %.0f\", me.health, me.speed))\nend", "Returns table with local player state: health, armor, team, is_alive, is_scoped, is_grounded, speed, origin {x,y,z}, velocity {x,y,z}." },
				{ "client.get_eye_pos", "client.get_eye_pos() -> x, y, z", "local ex, ey, ez = client.get_eye_pos()\nlocal sx, sy, vis = render.world_to_screen(ex, ey, ez)\nif vis then render.circle_filled(sx, sy, 4.0, render.color(74, 222, 128, 255)) end", "Returns the local player's camera/eye position in world space (origin + view offset)." },
				{ "client.get_players", "client.get_players([enemies_only]) -> table", "local players = client.get_players(true)\nfor i, p in ipairs(players) do\n    local sx, sy, vis = render.world_to_screen(p.origin.x, p.origin.y, p.origin.z)\n    if vis then\n        render.text(sx, sy, p.name .. \" [\" .. p.health .. \"]\", render.color(255, 255, 255, 255), true)\n    end\nend", "Returns an array of valid player tables: name, team, ping, steam_id, health, armor, is_local, is_alive, is_enemy, distance, origin {x,y,z}, screen {x,y,visible}. Pass true to only include enemies." },
				{ "client.get_weapon", "client.get_weapon() -> name, def_index", "local w, id = client.get_weapon()\nif w then\n    render.text(16, 48, \"Weapon: \" .. w, render.color(255, 255, 255, 255))\nend", "Returns the active weapon name (without the weapon_ prefix) and its item definition index. Both are nil when the local player is unarmed or invalid." },
				{ "client.get_view_angles", "client.get_view_angles() -> pitch, yaw, roll", "local p, y, r = client.get_view_angles()\nprint(string.format(\"%.1f %.1f %.1f\", p, y, r))", "Returns the local player's current view angles in degrees." },
				{ "client.is_key_down", "client.is_key_down(vk_code) -> bool", "local is_v_held = client.is_key_down(0x56) -- VK_V\nif is_v_held then\n    render.text(100, 100, \"V HELD\", render.color(74, 222, 128, 255))\nend", "Checks real-time physical key press state using Windows Virtual Key code." },
				{ "client.get_cursor_pos", "client.get_cursor_pos() -> mx, my", "local mx, my = client.get_cursor_pos()\nrender.circle(mx, my, 8.0, render.color(255, 255, 255, 200))", "Returns mouse cursor coordinates relative to the game client window." },
				{ "client.screen_size", "client.screen_size() -> w, h", "local sw, sh = client.screen_size()", "Returns display viewport width and height." },
				{ "client.log", "client.log(msg, [r, g, b])", "client.log(\"Script loaded successfully!\")", "Prints a message into the Lua Studio console." }
			};

			static const std::vector<doc_item> k_engine_docs = {
				{ "engine.is_in_game", "engine.is_in_game() -> bool", "if engine.is_in_game() then\n    render.text(16, 16, \"IN-GAME\", render.color(74, 222, 128, 255))\nend", "Returns true if the player is fully connected and spawned into a live match." },
				{ "engine.get_map_name", "engine.get_map_name() -> string", "local map = engine.get_map_name()\nrender.text(16, 32, \"Map: \" .. map, render.color(200, 200, 200, 255))", "Returns the active map name string (e.g. de_mirage, de_dust2)." },
				{ "engine.get_ping", "engine.get_ping() -> int", "local ping = engine.get_ping()\nprint(string.format(\"Ping: %d ms\", ping))", "Returns round-trip network latency in milliseconds." },
				{ "engine.get_fps", "engine.get_fps() -> number", "local fps = engine.get_fps()\nrender.text(16, 16, string.format(\"%.0f FPS\", fps), render.color(74, 222, 128, 255))", "Returns the current smoothed frame rate." },
				{ "engine.get_delta_time", "engine.get_delta_time() -> seconds", "local dt = engine.get_delta_time()", "Alias of render.delta_time — fractional seconds since the last rendered frame." },
				{ "engine.get_time", "engine.get_time() -> seconds", "local t = engine.get_time()\nprint(string.format(\"Up for %.1f s\", t))", "Returns fractional seconds elapsed since the process started." },
				{ "engine.is_key_down", "engine.is_key_down(vk_code) -> bool", "if engine.is_key_down(0x46) then -- VK_F\n    render.text(100, 100, \"F HELD\", render.color(74, 222, 128, 255))\nend", "Alias of client.is_key_down — checks real-time physical key press state using a Windows Virtual Key code." }
			};

			static const std::vector<doc_item> k_math_docs = {
				{ "math.clamp", "math.clamp(v, lo, hi) -> number", "local hp = math.clamp(hp, 0, 100)", "Clamps a value into the inclusive range [lo, hi]." },
				{ "math.lerp", "math.lerp(a, b, t) -> number", "local x = 100 + math.lerp(0, 200, 0.5)", "Linear interpolation between a and b by factor t. t=0 returns a, t=1 returns b." },
				{ "math.saturate", "math.saturate(t) -> number", "local a = math.saturate(alpha)", "Clamps a value into [0, 1]." },
				{ "math.pingpong", "math.pingpong(t, len) -> number", "local x = 100 + math.pingpong(engine.get_time(), 60)", "Oscillates between 0 and len as t grows, looping forever. Useful for animations." },
				{ "math.normalize_yaw", "math.normalize_yaw(yaw) -> number", "local y = math.normalize_yaw(450) -- 90", "Wraps a yaw angle into the range (-180, 180] degrees." },
				{ "math.angle_diff", "math.angle_diff(a, b) -> number", "local d = math.angle_diff(target_yaw, my_yaw)", "Returns the shortest signed angular difference between two yaws in degrees." },
				{ "math.distance", "math.distance(a, b) -> number", "local me = client.get_local()\nlocal p = client.get_players()[1]\nlocal d = math.distance(me.origin, p.origin)", "Distance between two points. Accepts two {x,y,z} tables or six raw coordinates." },
				{ "math.vector_to_angle", "math.vector_to_angle(x, y, z) -> pitch, yaw", "local p, y = math.vector_to_angle(vx, vy, vz)", "Converts a direction vector into pitch and yaw angles in degrees (FPS convention)." },
				{ "math.randomf", "math.randomf(min, max) -> number", "local n = math.randomf(0.0, 1.0)", "Returns a uniform random float in the inclusive range [min, max]." }
			};

			static const std::vector<doc_item> k_convar_docs = {
				{ "convar.get", "convar.get(name) -> value", "local sv = convar.get(\"sv_cheats\")\nlocal sens = convar.get(\"sensitivity\")\nprint(sv, sens)", "Reads the current value of a registered console variable. Returns a string, number, or boolean matching its type, or nil if the convar does not exist. Read-only — scripts cannot execute console commands." }
			};

			const auto* cur_items = &k_events_docs;
			if ( this->m_doc_category == 1 ) cur_items = &k_render_docs;
			else if ( this->m_doc_category == 2 ) cur_items = &k_client_docs;
			else if ( this->m_doc_category == 3 ) cur_items = &k_engine_docs;
			else if ( this->m_doc_category == 4 ) cur_items = &k_math_docs;
			else if ( this->m_doc_category == 5 ) cur_items = &k_convar_docs;

			if ( this->m_doc_item >= static_cast< int >( cur_items->size( ) ) )
			{
				this->m_doc_item = std::max( 0, static_cast< int >( cur_items->size( ) ) - 1 );
			}

			const float side_w = 200.0f;
			const float detail_w = body_w - side_w - pad;

			// Left list of functions in category
			xui::layout::set_cursor( body_x - wx, content_y - wy );
			if ( xui::begin_child( "##docs_item_list", side_w, content_h, true ) )
			{
				const auto avail_w = xui::layout::avail( ).first;
				for ( size_t i = 0; i < cur_items->size( ); ++i )
				{
					const auto& it = ( *cur_items )[ i ];
					const bool sel = ( this->m_doc_item == static_cast< int >( i ) );
					const auto row = xui::layout::item( avail_w, 22.0f );
					const bool hov = inp.in_rect( row );

					auto& cdl = xui::draw::current( );
					if ( sel )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_accent.alpha( 40 ), xdraw::corner_radius{ tokens::round_md } );
						cdl.rect_filled( row.x, row.y, 2.0f, row.h, tokens::col_accent );
					}
					else if ( hov )
					{
						cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated, xdraw::corner_radius{ tokens::round_md } );
					}

					cdl.text( row.x + 8.0f, row.y + 3.0f, it.name, sel ? tokens::col_accent : ( hov ? tokens::col_text : tokens::col_text_dim ) );

					if ( hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						this->m_doc_item = static_cast< int >( i );
					}
				}
				xui::end_child( );
			}

			// Right detail card
			xui::layout::set_cursor( body_x + side_w + pad - wx, content_y - wy );
			if ( xui::begin_child( "##docs_detail_panel", detail_w, content_h, false ) )
			{
				if ( this->m_doc_item >= 0 && this->m_doc_item < static_cast< int >( cur_items->size( ) ) )
				{
					const auto& chosen = ( *cur_items )[ this->m_doc_item ];
					const auto [rem_w, rem_h] = xui::layout::avail( );
					const auto card_rect = xui::layout::item( rem_w, rem_h );
					auto& cdl = xui::draw::current( );

					cdl.rect_filled( card_rect.x, card_rect.y, card_rect.w, card_rect.h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_md } );
					cdl.rect( card_rect.x, card_rect.y, card_rect.w, card_rect.h, tokens::col_border, 1.0f );
					cdl.rect_filled( card_rect.x, card_rect.y, card_rect.w, 2.0f, tokens::col_accent.alpha( 180 ) );

					float cy = card_rect.y + 6.0f;
					cdl.text( card_rect.x + 10.0f, cy, chosen.name, tokens::col_accent );
					cy += 16.0f;

					cdl.text( card_rect.x + 10.0f, cy, chosen.signature, tokens::col_text_dim );
					cy += 18.0f;

					cdl.rect_filled( card_rect.x + 8.0f, cy, card_rect.w - 16.0f, 1.0f, tokens::col_border );
					cy += 6.0f;

					cdl.text( card_rect.x + 10.0f, cy, chosen.desc, tokens::col_text );
					cy += 18.0f;

					const float snip_h = std::max( 40.0f, card_rect.y + card_rect.h - cy - 32.0f );
					cdl.rect_filled( card_rect.x + 8.0f, cy, card_rect.w - 16.0f, snip_h, tokens::col_dark, xdraw::corner_radius{ tokens::round_md } );
					cdl.rect( card_rect.x + 8.0f, cy, card_rect.w - 16.0f, snip_h, tokens::col_border, 1.0f );

					const auto [lua_w, lua_h] = xdraw::measure_text( "lua" );
					cdl.text( card_rect.x + card_rect.w - 16.0f - lua_w, cy + 3.0f, "lua", xdraw::color{ 127, 139, 155, 200 } );

					std::stringstream ss( chosen.snippet );
					std::string s_line;
					float snip_line_y = cy + 6.0f;
					while ( std::getline( ss, s_line ) && snip_line_y < cy + snip_h - 14.0f )
					{
						draw_highlighted_lua( cdl, card_rect.x + 12.0f, snip_line_y, s_line );
						snip_line_y += 15.0f;
					}

					// Action buttons: [Copy Snippet] [Insert to Studio]
					const float btn_y = card_rect.y + card_rect.h - 24.0f;
					const float b_w = ( card_rect.w - 24.0f ) * 0.5f;

					// Copy Snippet
					const xui::rect copy_btn{ card_rect.x + 8.0f, btn_y, b_w, 20.0f };
					const bool copy_hov = inp.in_rect( copy_btn );
					cdl.rect_filled( copy_btn.x, copy_btn.y, copy_btn.w, copy_btn.h, copy_hov ? tokens::col_elevated : tokens::col_card, xdraw::corner_radius{ tokens::round_md } );
					cdl.rect( copy_btn.x, copy_btn.y, copy_btn.w, copy_btn.h, copy_hov ? tokens::col_accent : tokens::col_border, 1.0f );
					const auto [ctw, cth] = xdraw::measure_text( "Copy Snippet" );
					cdl.text( std::floor( copy_btn.x + ( copy_btn.w - ctw ) * 0.5f ), std::floor( copy_btn.y + ( copy_btn.h - cth ) * 0.5f ), "Copy Snippet", copy_hov ? tokens::col_accent : tokens::col_text );

					if ( copy_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						if ( OpenClipboard( nullptr ) )
						{
							EmptyClipboard( );
							const std::string snip = chosen.snippet;
							const auto len = snip.size( ) + 1;
							const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
							if ( hMem )
							{
								memcpy( GlobalLock( hMem ), snip.c_str( ), len );
								GlobalUnlock( hMem );
								SetClipboardData( CF_TEXT, hMem );
							}
							CloseClipboard( );
							this->append_lua_log( "DOC", "Copied snippet to clipboard: " + std::string( chosen.name ), tokens::col_accent );
						}
					}

					// Insert to Lua Studio
					const xui::rect ins_btn{ card_rect.x + 8.0f + b_w + 8.0f, btn_y, b_w, 20.0f };
					const bool ins_hov = inp.in_rect( ins_btn );
					cdl.rect_filled( ins_btn.x, ins_btn.y, ins_btn.w, ins_btn.h, ins_hov ? tokens::col_accent.alpha( 80 ) : tokens::col_card, xdraw::corner_radius{ tokens::round_md } );
					cdl.rect( ins_btn.x, ins_btn.y, ins_btn.w, ins_btn.h, tokens::col_accent.alpha( ins_hov ? 255 : 120 ), 1.0f );
					const auto [itw, ith] = xdraw::measure_text( "+ Insert to Studio" );
					cdl.text( std::floor( ins_btn.x + ( ins_btn.w - itw ) * 0.5f ), std::floor( ins_btn.y + ( ins_btn.h - ith ) * 0.5f ), "+ Insert to Studio", ins_hov ? tokens::col_accent : tokens::col_text );

					if ( ins_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
					{
						std::stringstream ins_ss( chosen.snippet );
						std::string line;
						while ( std::getline( ins_ss, line ) )
						{
							this->m_lua_editor_lines.push_back( line );
						}
						this->m_lua_cursor_line = static_cast< int >( this->m_lua_editor_lines.size( ) ) - 1;
						this->append_lua_log( "DOC", "Inserted snippet: " + std::string( chosen.name ), tokens::col_accent );
						this->m_show_lua = true;
					}
				}
				xui::end_child( );
			}
		}

		xui::end_window( );
	}

	void menu::draw_players_inspector( float sw, float sh )
	{
		// Bots read steam_id == 0, which the players:: flag store treats as the
		// "no selection" sentinel (get() returns empty flags, set() is a no-op),
		// so every toggle on a bot row silently died. Derive a stable non-zero
		// key from the nickname and flag it with the top bit so it can never
		// collide with a real SteamID64 value.
		const auto player_flag_key = []( const player_match_entry& e ) -> std::uint64_t
			{
				if ( e.steam_id != 0 )
				{
					return e.steam_id;
				}

				const auto h = std::hash<std::string>{}( e.nickname ) & 0x3FFFFFFFFFFFFFFFull;
				return h | 0x8000000000000000ull;
			};

		const float dt = xdraw::delta_time( );
		this->m_players_last_refresh += dt;
		if ( this->m_players_last_refresh > 2.0f || this->m_match_players.empty( ) )
		{
			this->m_players_last_refresh = 0.0f;
			this->refresh_match_players( );
		}

		const auto reveal = xui::ease::out_cubic( this->m_open_anim );
		if ( !xui::begin_window( "##menu_players", this->m_players_x, this->m_players_y, this->m_players_w, this->m_players_h, false, 500.0f, 400.0f, reveal ) )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto wx = this->m_players_x;
		const auto wy = this->m_players_y;
		const auto ww = this->m_players_w;
		const auto wh = this->m_players_h;
		const auto& inp = xui::ctx( ).input;

		for ( int s_i = 5; s_i >= 1; --s_i )
		{
			const float spread = static_cast< float >( s_i ) * 3.0f;
			const std::uint8_t a = static_cast< std::uint8_t >( 6.0f * ( 6 - s_i ) * reveal );
			dl.rect_filled( wx - spread, wy - spread, ww + spread * 2.0f, wh + spread * 2.0f,
				xdraw::color{ 4, 3, 6, a }, xdraw::corner_radius{ tokens::round_md } );
		}
		dl.rect( wx - 1.0f, wy - 1.0f, ww + 2.0f, wh + 2.0f, tokens::col_accent.alpha( static_cast<std::uint8_t>( 45.0f * reveal ) ), xdraw::corner_radius{ tokens::round_lg + 2.0f }, 1.0f );

		dl.rect_filled( wx, wy, ww, wh, tokens::col_dark, xdraw::corner_radius{ tokens::round_lg } );

		{
			const auto ty = wy;
			dl.rect_filled( wx, ty, ww, tokens::title_bar_h, tokens::col_title_bar, xdraw::corner_radius{ tokens::round_lg } );
			dl.rect_filled( wx, ty + tokens::title_bar_h * 0.5f, ww, tokens::title_bar_h * 0.5f, tokens::col_title_bar );
			dl.rect_filled( wx, ty + tokens::title_bar_h - 1.0f, ww, 1.0f, tokens::col_border );

			const auto [bw, bh] = xdraw::measure_text( "PLAYERS" );
			const auto cy = std::floor( ty + ( tokens::title_bar_h - bh ) * 0.5f );

			dl.text( wx + 12.0f, cy, "PLAYERS", tokens::col_accent );

			// Close button [X]
			constexpr float cbtn_sz = 16.0f;
			const float cbtn_x = wx + ww - cbtn_sz - 10.0f;
			const float cbtn_y = ty + std::floor( ( tokens::title_bar_h - cbtn_sz ) * 0.5f );
			const auto cbtn_rect = xui::rect{ cbtn_x - 2.0f, cbtn_y - 2.0f, cbtn_sz + 4.0f, cbtn_sz + 4.0f };
			const bool cbtn_hov = inp.in_rect( cbtn_rect );
			if ( cbtn_hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_show_players = false;
			}
			if ( cbtn_hov )
			{
				dl.rect_filled( cbtn_rect.x, cbtn_rect.y, cbtn_rect.w, cbtn_rect.h, xdraw::color{ 239, 68, 68, 70 }, xdraw::corner_radius{ tokens::round_md } );
			}
			const float ccx = cbtn_x + cbtn_sz * 0.5f;
			const float ccy = cbtn_y + cbtn_sz * 0.5f;
			const auto cross_col = cbtn_hov ? xdraw::color{ 248, 113, 113, 255 } : tokens::col_text_dim;
			dl.line( ccx - 4.0f, ccy - 4.0f, ccx + 4.0f, ccy + 4.0f, cross_col, 1.4f );
			dl.line( ccx - 4.0f, ccy + 4.0f, ccx + 4.0f, ccy - 4.0f, cross_col, 1.4f );

			size_t ct_count = 0, t_count = 0;
			for ( const auto& p : this->m_match_players )
			{
				if ( p.team == 3 ) ++ct_count;
				else if ( p.team == 2 ) ++t_count;
			}

			char stats_buf[ 64 ];
			std::snprintf( stats_buf, sizeof( stats_buf ), "TOTAL: %zu  |  CT: %zu  |  T: %zu", this->m_match_players.size( ), ct_count, t_count );
			const auto [stw, sth] = xdraw::measure_text( stats_buf );
		dl.text( cbtn_x - stw - 14.0f, cy, stats_buf, tokens::col_text_dim );
	}

		// Inset accent top-edge line (follows the rounded frame)
		dl.rect_filled( wx + tokens::round_lg, wy + 1.0f, ww - tokens::round_lg * 2.0f, 2.0f,
			tokens::col_accent.alpha( static_cast< std::uint8_t >( tokens::col_accent.a * reveal ) ),
			xdraw::corner_radius{ 1.0f } );

		const float pad = 8.0f;
		const float body_x = wx + pad;
		const float body_y = wy + tokens::title_bar_h + pad;
		const float body_w = ww - pad * 2.0f;
		const float body_h = wh - tokens::title_bar_h - pad * 2.0f;

xui::layout::set_cursor( body_x - wx, body_y - wy );

		// ── Outer body: filter row on top, then list + mini profile side by side ──
		if ( xui::begin_child( "##players_table_view", body_w, body_h, false ) )
		{
			auto* view = xui::layout::current_window( );
			const float view_w = view->bounds.w;
			const float view_h = view->bounds.h;

			static constexpr const char* k_team_filters[] = { "All Players", "Counter-Terrorists", "Terrorists" };
			rendering::group_subtabs( k_team_filters, 3, this->m_players_team_filter );

			xui::layout::new_line( );
			xui::layout::spacing( 4.0f );
			xui::text_input( "##players_search", this->m_players_filter, 128, "Search nickname or Steam ID..." );
			xui::layout::same_line( );
			if ( xui::button( "REFRESH", 84.0f, 22.0f ) )
			{
				this->refresh_match_players( );
			}

			xui::layout::new_line( );
			xui::layout::spacing( 6.0f );

			const float split_top = view->cursor_y;
			const float list_w = std::floor( ( view_w - tokens::col_gap ) * 0.58f );
			const float info_w = view_w - list_w - tokens::col_gap;
			const float split_h = std::max( 60.0f, view_h - split_top );

			const auto filter_lower = detail::to_lower_workspaces( this->m_players_filter );

			// ── Left: compact player list ──
			xui::layout::set_cursor( 0.0f, split_top );
			if ( xui::begin_child( "##players_list", list_w, split_h, true ) )
			{
				auto* lwin = xui::layout::current_window( );
				const float lwi = lwin->bounds.w - 8.0f;
				auto& cdl = xui::draw::current( );

				if ( this->m_match_players.empty( ) )
				{
					const char* e1 = "NO ACTIVE MATCH";
					const char* e2 = "join a match and press REFRESH";
					const auto [e1w, e1h] = xdraw::measure_text( e1 );
					const auto [e2w, e2h] = xdraw::measure_text( e2 );
					cdl.text( std::floor( lwin->bounds.x + ( lwin->bounds.w - e1w ) * 0.5f ), lwin->bounds.y + 34.0f, e1, tokens::col_text_dim );
					cdl.text( std::floor( lwin->bounds.x + ( lwin->bounds.w - e2w ) * 0.5f ), lwin->bounds.y + 54.0f, e2, tokens::col_text_dim );
				}
				else
				{
					// Compact column header (HP + flag icons share fixed columns with rows)
					const auto hrow = xui::layout::item( lwi, 18.0f );
					auto& hdl = xui::draw::current( );
					const float hy = hrow.y + 2.0f;
					hdl.text( hrow.x + 4.0f, hy, "#", tokens::col_text_dim );
					hdl.text( hrow.x + 28.0f, hy, "PLAYER", tokens::col_text_dim );
					const float right = hrow.x + hrow.w - 6.0f;
					const float flags_x = right - 68.0f;
					const float hp_align = flags_x - 8.0f;
					const auto [hpw0, hph0] = xdraw::measure_text( "HP" );
					hdl.text( hp_align - hpw0, hy, "HP", tokens::col_text_dim );
					if ( this->m_textures.flag_shield.resource )
					{
						hdl.image( flags_x + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_shield.resource.Get( ), tokens::col_text_dim );
					}
					if ( this->m_textures.flag_eye.resource )
					{
						hdl.image( flags_x + 24.0f + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_eye.resource.Get( ), tokens::col_text_dim );
					}
					if ( this->m_textures.flag_eye_off.resource )
					{
						hdl.image( flags_x + 48.0f + 2.0f, hy, 14.0f, 14.0f, this->m_textures.flag_eye_off.resource.Get( ), tokens::col_text_dim );
					}

					constexpr float row_h = 26.0f;
					size_t display_idx = 0;
					for ( std::size_t i = 0; i < this->m_match_players.size( ); ++i )
					{
						const auto& p = this->m_match_players[ i ];

						if ( this->m_players_team_filter == 1 && p.team != 3 ) continue;
						if ( this->m_players_team_filter == 2 && p.team != 2 ) continue;

						if ( !filter_lower.empty( ) )
						{
							const auto name_low = detail::to_lower_workspaces( p.nickname );
							const auto id_str = std::to_string( p.steam_id );
							if ( name_low.find( filter_lower ) == std::string::npos && id_str.find( filter_lower ) == std::string::npos )
							{
								continue;
							}
						}

						++display_idx;
						const auto row = xui::layout::item( lwi, row_h );
						const bool hov = xui::ctx( ).input.in_rect( row );
						if ( hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
						{
							this->m_players_selected = static_cast< int >( i );
						}

						const auto is_sel = this->m_players_selected == static_cast< int >( i );
						const auto hover_anim = xui::anim::lerp( xui::fnv1a( "plyrow" ) + i, hov ? 1.0f : 0.0f, 14.0f );
						const auto sel_anim = xui::anim::lerp( xui::fnv1a( "plysel" ) + i, is_sel ? 1.0f : 0.0f, 10.0f );

						const auto row_bg = ( display_idx % 2 == 0 ) ? tokens::col_dark : tokens::col_card;
						cdl.rect_filled( row.x, row.y, row.w, row.h, row_bg );
						if ( sel_anim > 0.01f )
						{
							// Soft light pill, matches the dock-tab selection style.
							cdl.rect_filled( row.x, row.y, row.w, row.h, xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 34.0f * sel_anim ) } );
						}
						else if ( hover_anim > 0.01f )
						{
							cdl.rect_filled( row.x, row.y, row.w, row.h, tokens::col_elevated.alpha( static_cast< std::uint8_t >( 255.0f * hover_anim * 0.4f ) ) );
						}

						float rx = row.x + 4.0f;
						const float ry = row.y + std::floor( ( row_h - 14.0f ) * 0.5f );

						// Index
						cdl.text( rx, ry, std::to_string( display_idx ), tokens::col_text_dim );
						rx += 20.0f;

						// Tiny avatar (only when loaded; no placeholder box)
						constexpr float av_sz = 16.0f;
						const float av_y = row.y + std::floor( ( row_h - av_sz ) * 0.5f );
						if ( p.avatar_srv )
						{
							cdl.image( rx, av_y, av_sz, av_sz, p.avatar_srv, xdraw::corner_radius{ 4.0f }, xdraw::color{ 255, 255, 255, 255 } );
						}
						rx += av_sz + 4.0f;

						// Team dot
						const auto team_col = ( p.team == 3 ) ? xdraw::color{ 96, 165, 250, 255 } : ( p.team == 2 ? xdraw::color{ 251, 146, 60, 255 } : tokens::col_text_dim );
						cdl.rect_filled( rx, row.y + std::floor( ( row_h - 6.0f ) * 0.5f ), 6.0f, 6.0f, team_col, xdraw::corner_radius{ 3.0f } );
						rx += 12.0f;

						// Nickname
						std::string name_display = p.nickname;
						if ( p.is_local ) name_display += " (YOU)";
						else if ( p.is_bot ) name_display += " (BOT)";

						const auto name_col = p.is_local ? tokens::col_accent : ( p.is_alive ? tokens::col_text : tokens::col_text_dim );

						const float right = row.x + row.w - 6.0f;
						const float flags_x = right - 68.0f;
						const float hp_align = flags_x - 8.0f;
						const float name_space = hp_align - 44.0f - rx;
						while ( name_space > 12.0f && !name_display.empty( ) && xdraw::measure_text( name_display ).first > name_space )
						{
							name_display.pop_back( );
						}
						if ( name_display.size( ) < p.nickname.size( ) + 5 ) name_display += "...";
						cdl.text( rx, ry - 2.0f, name_display, xui::lerp( name_col, tokens::col_accent, sel_anim ) );

						// HP (right-aligned to its column, clear of flags)
						{
							std::string hp_str = ( !p.is_alive || p.health <= 0 ) ? "DEAD" : std::to_string( p.health );
							const auto hp_col = ( !p.is_alive || p.health <= 0 ) ? xdraw::color{ 248, 113, 113, 255 }
								: ( p.health > 50 ? xdraw::color{ 74, 222, 128, 255 } : ( p.health > 20 ? xdraw::color{ 251, 191, 36, 255 } : xdraw::color{ 248, 113, 113, 255 } ) );
							const auto [hw0, hh0] = xdraw::measure_text( hp_str );
							cdl.text( hp_align - hw0, ry, hp_str, hp_col );
						}

						// Flag toggles: lucide icons (shield = whitelist, eye = priority)
						auto p_flags = features::players::get( player_flag_key( p ) );
						const auto flag_icon = [ & ]( const textures::entry& ico, float bx, float by, bool on, const xdraw::color& on_col, std::uint32_t key ) -> bool
							{
								const xui::rect fr{ bx, by, 20.0f, 20.0f };
								const bool hov = xui::ctx( ).input.in_rect( fr );
								xui::anim::lerp( key, ( on || hov ) ? 1.0f : 0.0f, 12.0f );
								if ( ico.resource )
								{
									const auto col = on ? on_col : ( hov ? tokens::col_text : tokens::col_text_dim.alpha( 150 ) );
									cdl.image( bx + 2.0f, by + 3.0f, 16.0f, 16.0f, ico.resource.Get( ), col );
								}
								return hov && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( );
							};

						{
							const bool on = p_flags.whitelist( );
							if ( flag_icon( this->m_textures.flag_shield, flags_x, row.y + 3.0f, on, xdraw::color{ 74, 222, 128, 255 }, xui::fnv1a( "ply_wl" ) + i ) )
							{
								p_flags.set_whitelist( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
						{
							const bool on = p_flags.priority( );
							if ( flag_icon( this->m_textures.flag_eye, flags_x + 24.0f, row.y + 3.0f, on, xdraw::color{ 251, 191, 36, 255 }, xui::fnv1a( "ply_pr" ) + i ) )
							{
								p_flags.set_priority( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
						{
							const bool on = p_flags.no_visuals( );
							if ( flag_icon( this->m_textures.flag_eye_off, flags_x + 48.0f, row.y + 3.0f, on, xdraw::color{ 248, 113, 113, 255 }, xui::fnv1a( "ply_ne" ) + i ) )
							{
								p_flags.set_no_visuals( !on );
								features::players::set( player_flag_key( p ), p_flags );
							}
						}
					}
				}

				xui::end_child( );
			}

			// ── Right: mini profile ──
			xui::layout::set_cursor( list_w + tokens::col_gap, split_top );
			if ( xui::begin_child( "##players_info", info_w, split_h, false ) )
			{
				auto* iwin = xui::layout::current_window( );
				auto& pdl = xui::draw::current( );
				const float px = iwin->bounds.x;
				const float py = iwin->bounds.y;
				const float pw = iwin->bounds.w;

				if ( this->m_players_selected >= 0 && this->m_players_selected < static_cast< int >( this->m_match_players.size( ) ) )
				{
					const auto& sel = this->m_match_players[ static_cast< std::size_t >( this->m_players_selected ) ];
					auto sel_flags = features::players::get( player_flag_key( sel ) );

					// Mini avatar (no placeholder border box; dim silhouette fallback)
					constexpr float av = 40.0f;
					const float avx = px + 8.0f;
					const float avy = py + 8.0f;
					if ( sel.avatar_srv )
					{
						pdl.image( avx, avy, av, av, sel.avatar_srv, xdraw::corner_radius{ tokens::round_md }, xdraw::color{ 255, 255, 255, 255 } );
					}
					else if ( this->m_textures.model_mannequin.resource )
					{
						pdl.image( avx + 6.0f, avy + 6.0f, av - 12.0f, av - 12.0f, this->m_textures.model_mannequin.resource.Get( ), tokens::col_text_dim );
					}

					// Name + team
					const float nx = avx + av + 8.0f;
					std::string nname = sel.nickname;
					if ( sel.is_local ) nname += "  (YOU)";
					pdl.text( nx, avy - 1.0f, xui::truncate( nname, pw - ( nx - px ) - 8.0f ), sel.is_local ? tokens::col_accent : tokens::col_text );
					const auto tcol = ( sel.team == 3 ) ? xdraw::color{ 96, 165, 250, 255 } : ( sel.team == 2 ? xdraw::color{ 251, 146, 60, 255 } : tokens::col_text_dim );
					pdl.text( nx, avy + 16.0f, ( sel.team == 3 ) ? "CT" : ( sel.team == 2 ? "T" : "SPEC" ), tcol );

					// Health bar under the header block
					const float hbar_y = py + 8.0f + av + 8.0f;
					const float hbw = pw - 16.0f;
					const auto hp_col = ( sel.is_alive && sel.health > 0 ) ? xdraw::color{ 74, 222, 128, 255 } : xdraw::color{ 248, 113, 113, 255 };
					pdl.rect_filled( px + 8.0f, hbar_y, hbw, 3.0f, tokens::col_card, xdraw::corner_radius{ 1.5f } );
					const float hp_ratio = ( sel.is_alive && sel.health > 0 ) ? std::clamp( sel.health / 100.0f, 0.0f, 1.0f ) : 0.0f;
					pdl.rect_filled( px + 8.0f, hbar_y, hbw * hp_ratio, 3.0f, hp_col, xdraw::corner_radius{ 1.5f } );

					// Divider
					const float div_y = hbar_y + 8.0f;
					pdl.rect_filled( px + 8.0f, div_y, pw - 16.0f, 1.0f, tokens::col_border );

					// Minimal info lines
					float row_y = div_y + 12.0f;
					const float lx = px + 8.0f;
					const float vx = lx + 78.0f;
					const auto line = [ & ]( const char* label, const char* value, const xdraw::color& vcol )
						{
							pdl.text( lx, row_y, label, tokens::col_text_dim );
							pdl.text( vx, row_y, value, vcol );
							row_y += 18.0f;
						};

					{
						char b[ 32 ];
						std::snprintf( b, sizeof( b ), "%d", sel.health );
						const auto hcol = ( sel.is_alive && sel.health > 0 ) ? xdraw::color{ 74, 222, 128, 255 } : xdraw::color{ 248, 113, 113, 255 };
						line( "HEALTH", b, hcol );
						std::snprintf( b, sizeof( b ), "%d AP%s%s", sel.armor, sel.has_helmet ? " [H]" : "", sel.has_defuser ? " [DEF]" : "" );
						line( "ARMOR", b, xdraw::color{ 147, 197, 253, 255 } );
						std::string wep = sel.weapon.empty( ) ? "knife" : sel.weapon;
						if ( !wep.empty( ) && wep[ 0 ] >= 'a' && wep[ 0 ] <= 'z' )
						{
							wep[ 0 ] = static_cast< char >( std::toupper( static_cast< unsigned char >( wep[ 0 ] ) ) );
						}
						line( "WEAPON", wep.c_str( ), tokens::col_text );
						std::snprintf( b, sizeof( b ), "$%d", sel.money );
						line( "MONEY", b, xdraw::color{ 74, 222, 128, 255 } );
						std::snprintf( b, sizeof( b ), "%d ms", sel.ping );
						line( "PING", b, tokens::col_text );
					}

					// Steam ID + ghost copy link (no box)
					if ( sel.steam_id != 0 )
					{
						char sb[ 32 ];
						std::snprintf( sb, sizeof( sb ), "%llu", sel.steam_id );
						pdl.text( lx, row_y, "STEAM", tokens::col_text_dim );
						const auto sid_str = std::string( sb );
						pdl.text( vx, row_y, xui::truncate( sid_str, ( px + pw - 4.0f ) - vx - 40.0f ), tokens::col_text_dim );
						const auto [cw0, ch0] = xdraw::measure_text( "COPY" );
						const xui::rect cpb{ px + pw - 6.0f - cw0 - 2.0f, row_y - 1.0f, cw0 + 2.0f, 16.0f };
						const bool cph = inp.in_rect( cpb );
						pdl.text( cpb.x, cpb.y - 1.0f, "COPY", cph ? tokens::col_accent : tokens::col_text_dim );
						if ( cph && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
						{
							if ( OpenClipboard( nullptr ) )
							{
								EmptyClipboard( );
								const auto len = strlen( sb ) + 1;
								const auto hMem = GlobalAlloc( GMEM_MOVEABLE, len );
								if ( hMem )
								{
									memcpy( GlobalLock( hMem ), sb, len );
									GlobalUnlock( hMem );
									SetClipboardData( CF_TEXT, hMem );
								}
								CloseClipboard( );
							}
						}
						row_y += 18.0f;
					}

					// Divider
					pdl.rect_filled( px + 8.0f, row_y + 2.0f, pw - 16.0f, 1.0f, tokens::col_border );
					row_y += 10.0f;

					// Flag toggles: ghost icon rows (no filled boxes)
					const auto flag_row = [ & ]( const textures::entry& ico, const char* label, bool on, const xdraw::color& on_col, std::uint32_t key ) -> bool
						{
							const xui::rect fr{ px + 8.0f, row_y, pw - 16.0f, 22.0f };
							const bool hov = inp.in_rect( fr );
							xui::anim::lerp( key, ( on || hov ) ? 1.0f : 0.0f, 12.0f );
							if ( ico.resource )
							{
								const auto icol = on ? on_col : ( hov ? tokens::col_text : tokens::col_text_dim );
								pdl.image( fr.x + 4.0f, row_y + 3.0f, 16.0f, 16.0f, ico.resource.Get( ), icol );
							}
							pdl.text( fr.x + 26.0f, row_y + 3.0f, label, on ? tokens::col_text : ( hov ? tokens::col_text : tokens::col_text_dim ) );
							return hov && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( );
						};

					{
						const bool on = sel_flags.whitelist( );
						if ( flag_row( this->m_textures.flag_shield, "Whitelist (never target)", on, xdraw::color{ 74, 222, 128, 255 }, xui::fnv1a( "ply_sel_wl" ) ) )
						{
							sel_flags.set_whitelist( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}
					{
						const bool on = sel_flags.priority( );
						if ( flag_row( this->m_textures.flag_eye, "Priority target", on, xdraw::color{ 251, 191, 36, 255 }, xui::fnv1a( "ply_sel_pr" ) ) )
						{
							sel_flags.set_priority( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}
					{
						const bool on = sel_flags.no_visuals( );
						if ( flag_row( this->m_textures.flag_eye_off, "No ESP (hide from visuals)", on, xdraw::color{ 248, 113, 113, 255 }, xui::fnv1a( "ply_sel_ne" ) ) )
						{
							sel_flags.set_no_visuals( !on );
							features::players::set( player_flag_key( sel ), sel_flags );
						}
						row_y += 22.0f;
					}

					// Actions: ghost buttons (accent glow on hover, no heavy boxes)
					row_y += 4.0f;
					pdl.rect_filled( px + 8.0f, row_y + 2.0f, pw - 16.0f, 1.0f, tokens::col_border );
					row_y += 10.0f;
					const float abw = ( pw - 16.0f - 8.0f ) / 3.0f;
					const auto act = [ & ]( float bx, float bw2, const char* label, const xdraw::color& c, bool enabled ) -> bool
						{
							const xui::rect br{ bx, row_y, bw2, 24.0f };
							const bool bh = enabled && inp.in_rect( br );
							const auto [lw0, lh0] = xdraw::measure_text( label );
							const float ltx = std::floor( br.x + ( br.w - lw0 ) * 0.5f );
							const float lty = std::floor( br.y + ( br.h - lh0 ) * 0.5f );
							pdl.text( ltx, lty, label, !enabled ? tokens::col_text_dim.alpha( 90 ) : ( bh ? c : tokens::col_text_dim ) );
							if ( bh )
							{
								pdl.rect_filled( ltx, br.y + br.h - 3.0f, static_cast< float >( lw0 ), 1.0f, c.alpha( 200 ) );
							}
							return bh && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( );
						};

					const float bx0 = px + 8.0f;
					const bool can_profile = ( sel.steam_id != 0 );
					const bool can_spectate = !sel.is_local;
					const bool can_vote = !sel.is_local && !sel.is_bot;
					if ( act( bx0, abw, "PROFILE", tokens::col_accent, can_profile ) && can_profile )
					{
						steam::friends::open_profile( sel.steam_id );
					}
					if ( act( bx0 + abw + 4.0f, abw, "SPECTATE", tokens::col_accent, can_spectate ) && can_spectate )
					{
						const auto lcl = systems::g_local.get( );
						if ( lcl.is_valid( ) && !lcl.is_alive && lcl.controller )
						{
							const auto observer_pawn_handle = memory::read<std::uint32_t>( lcl.controller + SCHEMA( "CCSPlayerController", "m_hObserverPawn"_hash ) );
							if ( observer_pawn_handle )
							{
								const auto observer_pawn = systems::g_entities.lookup( observer_pawn_handle );
								if ( observer_pawn )
								{
									const auto observer_services = memory::read<std::uintptr_t>( observer_pawn + SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash ) );
									if ( observer_services )
									{
										for ( const auto& cached : systems::g_entities.get_by_type( systems::entities::type::player ) )
										{
											// Real players match on SteamID; bots (steam id 0)
											// match on the sanitized name, same source the
											// players list uses.
											const auto sid = memory::read<std::uint64_t>( cached.ptr + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
											if ( sel.steam_id != 0 )
											{
												if ( sid != sel.steam_id ) continue;
											}
											else
											{
												const auto ctrl_name_ptr = memory::read<std::uintptr_t>( cached.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
												const auto ctrl_name = ctrl_name_ptr ? memory::read_string( ctrl_name_ptr, 128 ) : std::string{};
												if ( ctrl_name != sel.nickname ) continue;
											}

											const auto target_handle = memory::read<std::uint32_t>( cached.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
											if ( target_handle )
											{
												memory::write<std::uint32_t>( observer_services + SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash ), target_handle );
											}
											break;
										}
									}
								}
							}
						}
					}
					if ( act( bx0 + ( abw + 4.0f ) * 2.0f, abw, "CALL VOTE", tokens::col_accent, can_vote ) && can_vote )
					{
						std::string kick_cmd = "callvote kick \"" + sel.nickname + "\"";
						memory::call<void>( PATTERN( patterns::engine_client_cmd ), addresses::globals::source2engine_to_client, 0, kick_cmd.c_str( ), 0x7ffef001 );
					}
				}
				else
				{
					const char* t2 = "click a row to inspect";
					const auto [t2w, t2h] = xdraw::measure_text( t2 );
					pdl.text( std::floor( px + ( pw - t2w ) * 0.5f ), py + 34.0f, t2, tokens::col_text_dim );
				}

				xui::end_child( );
			}

			xui::end_child( );
		}

		xui::end_window( );
	}


} // namespace rendering
