#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/settings.hpp>
#include <utilities/security/security.hpp>
#include <utilities/steam/steam.hpp>
#include <external/config.hpp>
#include <core/systems/systems.hpp>
#include <core/resources/workspace.hpp>

#include "../../rendering.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "menu.workspaces.detail.hpp"

namespace rendering {

	void menu::init_lua_studio( )
	{
		if ( this->m_lua_initialized )
			return;

		this->m_lua_initialized = true;

		systems::g_lua.initialize( );

		std::error_code ec{};
		const auto scripts_dir = workspace::scripts( );


		this->refresh_lua_scripts( );

		if ( !this->m_lua_scripts.empty( ) )
		{
			this->m_lua_selected_script = 0;
			this->load_lua_script( this->m_lua_scripts[ 0 ].path );
		}

		this->append_lua_log( "SYSTEM", "Lua runtime loaded. Ready.", tokens::col_accent );
	}

	void menu::refresh_lua_scripts( )
	{
		this->m_lua_scripts.clear( );

		std::error_code ec{};
		const auto scripts_dir = workspace::scripts( );
		if ( !std::filesystem::exists( scripts_dir, ec ) )
			return;

		for ( const auto& entry : std::filesystem::directory_iterator( scripts_dir, ec ) )
		{
			if ( entry.is_regular_file( ) && entry.path( ).extension( ) == ".lua" )
			{
				this->m_lua_scripts.push_back( { entry.path( ).filename( ).string( ), entry.path( ).string( ), false } );
			}
		}
	}

	void menu::load_lua_script( const std::string& path )
	{
		this->m_lua_editor_lines.clear( );
		std::ifstream file( path );
		if ( file.is_open( ) )
		{
			std::string line;
			while ( std::getline( file, line ) )
			{
				this->m_lua_editor_lines.push_back( line );
			}
		}

		if ( this->m_lua_editor_lines.empty( ) )
		{
			this->m_lua_editor_lines.push_back( "-- Empty script" );
		}

		this->m_lua_cursor_line = 0;
		if ( !this->m_lua_editor_lines.empty( ) )
		{
			this->m_lua_line_edit_buf = this->m_lua_editor_lines[ 0 ];
		}
		else
		{
			this->m_lua_line_edit_buf.clear( );
		}

		const auto fname = std::filesystem::path( path ).filename( ).string( );
		this->append_lua_log( "LOAD", "Loaded script: " + fname + " (" + std::to_string( this->m_lua_editor_lines.size( ) ) + " lines)", xdraw::color{ 74, 222, 128, 255 } );
	}

	void menu::save_lua_script( const std::string& path )
	{
		std::ofstream file( path );
		if ( file.is_open( ) )
		{
			for ( const auto& line : this->m_lua_editor_lines )
			{
				file << line << "\n";
			}
			const auto fname = std::filesystem::path( path ).filename( ).string( );
			this->append_lua_log( "SAVE", "Saved script to disk: " + fname, xdraw::color{ 74, 222, 128, 255 } );
		}
		else
		{
			this->append_lua_log( "ERROR", "Failed to write file: " + path, xdraw::color{ 239, 68, 68, 255 } );
		}
	}

	void menu::execute_lua_buffer( )
	{
		const auto start_time = std::chrono::high_resolution_clock::now( );
		const auto active_name = ( this->m_lua_selected_script >= 0 && this->m_lua_selected_script < static_cast< int >( this->m_lua_scripts.size( ) ) )
			? this->m_lua_scripts[ this->m_lua_selected_script ].name : "buffer";

		this->append_lua_log( "EXEC", "Compiling & linking: " + active_name + "...", tokens::col_accent );

		int scope_depth = 0;
		int repeat_depth = 0;
		int paren_depth = 0;
		int brace_depth = 0;
		int bracket_depth = 0;
		int callbacks_registered = 0;
		int functions_compiled = 0;
		int render_calls = 0;
		bool in_multiline_comment = false;
		bool in_multiline_string = false;
		size_t line_num = 0;
		size_t total_chars = 0;

		for ( const auto& raw_line : this->m_lua_editor_lines )
		{
			++line_num;
			total_chars += raw_line.size( ) + 1;

			size_t i = 0;
			const size_t len = raw_line.size( );

			while ( i < len )
			{
				if ( in_multiline_comment )
				{
					if ( i + 1 < len && raw_line[ i ] == ']' && raw_line[ i + 1 ] == ']' )
					{
						in_multiline_comment = false;
						i += 2;
					}
					else
					{
						++i;
					}
					continue;
				}

				if ( in_multiline_string )
				{
					if ( i + 1 < len && raw_line[ i ] == ']' && raw_line[ i + 1 ] == ']' )
					{
						in_multiline_string = false;
						i += 2;
					}
					else
					{
						++i;
					}
					continue;
				}

				// Check multiline comment start
				if ( i + 3 < len && raw_line[ i ] == '-' && raw_line[ i + 1 ] == '-' && raw_line[ i + 2 ] == '[' && raw_line[ i + 3 ] == '[' )
				{
					in_multiline_comment = true;
					i += 4;
					continue;
				}

				// Check single-line comment
				if ( i + 1 < len && raw_line[ i ] == '-' && raw_line[ i + 1 ] == '-' )
				{
					break;
				}

				// Check multiline string start
				if ( i + 1 < len && raw_line[ i ] == '[' && raw_line[ i + 1 ] == '[' )
				{
					in_multiline_string = true;
					i += 2;
					continue;
				}

				// String literal skipping
				if ( raw_line[ i ] == '"' || raw_line[ i ] == '\'' )
				{
					const char quote = raw_line[ i++ ];
					while ( i < len && raw_line[ i ] != quote )
					{
						if ( raw_line[ i ] == '\\' && i + 1 < len )
						{
							i += 2;
						}
						else
						{
							++i;
						}
					}
					if ( i < len && raw_line[ i ] == quote )
					{
						++i;
					}
					continue;
				}

				// Delimiters
				if ( raw_line[ i ] == '(' ) { ++paren_depth; ++i; continue; }
				if ( raw_line[ i ] == ')' ) { --paren_depth; ++i; continue; }
				if ( raw_line[ i ] == '{' ) { ++brace_depth; ++i; continue; }
				if ( raw_line[ i ] == '}' ) { --brace_depth; ++i; continue; }
				if ( raw_line[ i ] == '[' ) { ++bracket_depth; ++i; continue; }
				if ( raw_line[ i ] == ']' ) { --bracket_depth; ++i; continue; }

				// Identifiers and Keywords
				if ( std::isalpha( static_cast<unsigned char>( raw_line[ i ] ) ) || raw_line[ i ] == '_' )
				{
					size_t start_token = i;
					while ( i < len && ( std::isalnum( static_cast<unsigned char>( raw_line[ i ] ) ) || raw_line[ i ] == '_' || raw_line[ i ] == '.' ) )
					{
						++i;
					}
					const std::string_view token{ raw_line.data( ) + start_token, i - start_token };

					if ( token == "function" )
					{
						++scope_depth;
						++functions_compiled;
					}
					else if ( token == "then" || token == "do" )
					{
						++scope_depth;
					}
					else if ( token == "repeat" )
					{
						++repeat_depth;
					}
					else if ( token == "until" )
					{
						--repeat_depth;
						if ( repeat_depth < 0 )
						{
							this->append_lua_log( "SYNTAX ERROR", "Line " + std::to_string( line_num ) + ": Unexpected 'until' without matching 'repeat'.", xdraw::color{ 248, 113, 113, 255 } );
							return;
						}
					}
					else if ( token == "end" )
					{
						--scope_depth;
						if ( scope_depth < 0 )
						{
							this->append_lua_log( "SYNTAX ERROR", "Line " + std::to_string( line_num ) + ": Unexpected 'end' without matching block statement.", xdraw::color{ 248, 113, 113, 255 } );
							return;
						}
					}
					else if ( token == "events.listen" )
					{
						++callbacks_registered;
					}
					else if ( token.starts_with( "render." ) )
					{
						++render_calls;
					}
					continue;
				}

				++i;
			}
		}

		if ( scope_depth > 0 )
		{
			this->append_lua_log( "SYNTAX ERROR", "Missing 'end': " + std::to_string( scope_depth ) + " unclosed code block(s) detected.", xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		if ( repeat_depth > 0 )
		{
			this->append_lua_log( "SYNTAX ERROR", "Missing 'until': " + std::to_string( repeat_depth ) + " unclosed 'repeat' block(s).", xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		if ( paren_depth != 0 )
		{
			this->append_lua_log( "SYNTAX ERROR", "Unbalanced parentheses ( ) count: " + std::to_string( paren_depth ), xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		if ( brace_depth != 0 )
		{
			this->append_lua_log( "SYNTAX ERROR", "Unbalanced table braces { } count: " + std::to_string( brace_depth ), xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		if ( bracket_depth != 0 )
		{
			this->append_lua_log( "SYNTAX ERROR", "Unbalanced brackets [ ] count: " + std::to_string( bracket_depth ), xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		if ( this->m_lua_selected_script >= 0 && this->m_lua_selected_script < static_cast< int >( this->m_lua_scripts.size( ) ) )
		{
			this->save_lua_script( this->m_lua_scripts[ this->m_lua_selected_script ].path );
		}

		std::string full_code{};
		for ( const auto& l : this->m_lua_editor_lines )
		{
			full_code += l;
			full_code += '\n';
		}

		std::string err{};
		if ( !systems::g_lua.execute_buffer( full_code, err ) )
		{
			this->append_lua_log( "RUNTIME ERROR", err, xdraw::color{ 248, 113, 113, 255 } );
			return;
		}

		const auto end_time = std::chrono::high_resolution_clock::now( );
		const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( end_time - start_time ).count( );

		std::string msg = "Lua VM running in " + std::to_string( elapsed_us ) + " µs (" + std::to_string( this->m_lua_editor_lines.size( ) ) + " lines, ";
		msg += std::to_string( total_chars ) + " bytes, " + std::to_string( functions_compiled ) + " funcs, ";
		msg += std::to_string( render_calls ) + " render calls, ";
		msg += std::to_string( callbacks_registered ) + " callbacks). Script active!";
		this->append_lua_log( "SUCCESS", msg, xdraw::color{ 74, 222, 128, 255 } );
	}

	void menu::append_lua_log( std::string_view tag, std::string_view msg, xdraw::color col )
	{
		const auto now = std::chrono::system_clock::now( );
		const auto in_time_t = std::chrono::system_clock::to_time_t( now );
		std::tm buf{};
		localtime_s( &buf, &in_time_t );

		char time_str[ 16 ];
		std::strftime( time_str, sizeof( time_str ), "%H:%M:%S", &buf );

	std::string formatted = std::string( msg );
	this->m_lua_logs.push_back( { time_str, std::string( tag ), std::move( formatted ), col } );

		if ( this->m_lua_logs.size( ) > 80 )
		{
			this->m_lua_logs.erase( this->m_lua_logs.begin( ) );
		}
	}

	void menu::refresh_match_players( )
	{
		const auto keep_idx = this->m_players_selected;
		std::uint64_t keep_selected_id{};
		std::string keep_name;
		if ( keep_idx >= 0 && keep_idx < static_cast< int >( this->m_match_players.size( ) ) )
		{
			const auto& sel = this->m_match_players[ static_cast< std::size_t >( keep_idx ) ];
			keep_selected_id = sel.steam_id;
			keep_name = sel.nickname;
		}
		this->m_players_selected = -1;

		this->m_match_players.clear( );

		const auto local = systems::g_local.get( );
		const auto local_ctrl = local.controller;

		for ( const auto& player : systems::g_entities.get_by_type( systems::entities::type::player ) )
		{
			const auto ctrl = player.ptr;
			if ( !ctrl ) continue;

			player_match_entry e{};
			e.is_local = ( ctrl == local_ctrl );
			e.team = memory::read<int>( ctrl + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
			e.ping = memory::read<int>( ctrl + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
			e.steam_id = memory::read<std::uint64_t>( ctrl + SCHEMA( "CBasePlayerController", "m_steamID"_hash ) );
			e.is_alive = memory::read<bool>( ctrl + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) );
			e.is_bot = ( e.steam_id == 0 ) || memory::read<bool>( ctrl + SCHEMA( "CBasePlayerController", "m_bControllingBot"_hash ) );

			const auto name_ptr = memory::read<std::uintptr_t>( ctrl + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
			if ( name_ptr )
			{
				e.nickname = memory::read_string( name_ptr, 128 );
			}

			if ( e.nickname.empty( ) )
			{
				e.nickname = e.is_local ? "Local Player" : ( e.is_bot ? "Bot" : "Player" );
			}

			// Read money
			const auto money_services = memory::read<std::uintptr_t>( ctrl + SCHEMA( "CCSPlayerController", "m_pInGameMoneyServices"_hash ) );
			if ( money_services )
			{
				e.money = memory::read<int>( money_services + SCHEMA( "CCSPlayerController_InGameMoneyServices", "m_iAccount"_hash ) );
			}

			// Read pawn-specific live stats: Health, Armor, Weapon
			const auto pawn_handle = memory::read<std::uint32_t>( ctrl + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
			if ( pawn_handle )
			{
				const auto pawn = systems::g_entities.lookup( pawn_handle );
				if ( pawn )
				{
					e.health = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
					e.armor = memory::read<int>( pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) );

					const auto item_services = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pItemServices"_hash ) );
					if ( item_services )
					{
						e.has_helmet = memory::read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasHelmet"_hash ) );
						e.has_defuser = memory::read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasDefuser"_hash ) );
					}

					const auto weapon_services = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
					if ( weapon_services )
					{
						const auto active_wep_handle = memory::read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
						if ( active_wep_handle )
						{
							const auto wep_ptr = systems::g_entities.lookup( active_wep_handle );
							if ( wep_ptr )
							{
								const auto vdata = memory::read<std::uintptr_t>( wep_ptr + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
								if ( vdata )
								{
									const auto wep_name_ptr = memory::read<const char*>( vdata + SCHEMA( "CCSWeaponBaseVData", "m_szName"_hash ) );
									if ( wep_name_ptr )
									{
										e.weapon = memory::read_string( reinterpret_cast< std::uintptr_t >( wep_name_ptr ), 64 );
										if ( e.weapon.starts_with( "weapon_" ) )
										{
											e.weapon.erase( 0, 7 );
										}
									}
								}
							}
						}
					}
				}
			}

			if ( e.weapon.empty( ) )
			{
				e.weapon = e.is_alive ? "knife" : "none";
			}

			if ( e.steam_id != 0 )
			{
				e.avatar_srv = detail::s_ws_avatar_cache.get( e.steam_id );
			}

			this->m_match_players.push_back( std::move( e ) );
		}

		// Re-anchor the selection on the same steam id so the info panel
		// survives periodic mid-match refreshes. Bots (and the local player in
		// some setups) read steam_id == 0, which collides with the
		// "no selection" sentinel, so when the id lookup fails we fall back to
		// the same list slot as long as it still holds the same bot row —
		// otherwise the selection would silently vanish on every 2s refresh.
		int next_sel = -1;
		if ( keep_selected_id != 0 )
		{
			for ( auto idx = 0u; idx < this->m_match_players.size( ); ++idx )
			{
				if ( this->m_match_players[ idx ].steam_id == keep_selected_id )
				{
					next_sel = static_cast< int >( idx );
					break;
				}
			}
		}
		if ( next_sel < 0 && keep_idx >= 0 && keep_idx < static_cast< int >( this->m_match_players.size( ) ) )
		{
			const auto& still = this->m_match_players[ static_cast< std::size_t >( keep_idx ) ];
			if ( still.steam_id == 0 || keep_name.empty( ) || still.nickname == keep_name )
			{
				next_sel = keep_idx;
			}
		}
		this->m_players_selected = next_sel;
	}


} // namespace rendering
