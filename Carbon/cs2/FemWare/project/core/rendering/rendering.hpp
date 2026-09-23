#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include <core/rendering/fw_logo.hpp>

struct ID3D11RenderTargetView;
struct ID3D11Texture2D;

namespace rendering {

	class context
	{
	public:
		bool initialize( IDXGISwapChain* swap_chain );
		void shutdown( );
		void on_present( IDXGISwapChain* swap_chain );
		void on_resize_buffers( );
		void on_resize_buffers_post( IDXGISwapChain* swap_chain );

		[[nodiscard]] HWND get_window( ) const { return this->m_window; }
		[[nodiscard]] ID3D11Device* get_device( ) const { return this->m_device; }
		[[nodiscard]] ID3D11DeviceContext* get_context( ) const { return this->m_context; }
		[[nodiscard]] bool is_initialized( ) const { return this->m_initialized; }
		[[nodiscard]] bool ui_assets_ready( ) const { return this->m_ui_assets_ready; }
        ID3D11RenderTargetView* get_rtv( ) const { return this->m_rtv; }
		[[nodiscard]] HWND get_overlay_window( ) const { return this->m_overlay_window; }
		[[nodiscard]] bool overlay_active( ) const { return this->m_overlay_initialized && this->m_overlay_window && IsWindow( this->m_overlay_window ); }

		static bool guarded_setup_streamproof_overlay( context* ctx );
		static bool guarded_update_streamproof_overlay( context* ctx );
		static bool guarded_render_streamproof_overlay( context* ctx );

	private:
		void create_rtv( IDXGISwapChain* swap_chain );
		void setup_zdraw( HWND window );
		bool try_bind_ui_assets( );
		void render_visuals( );

		bool setup_streamproof_overlay( );
		bool create_overlay_resources( int w, int h );
		void release_overlay_resources( );
		void destroy_streamproof_overlay( );
		bool update_streamproof_overlay( );
		bool render_streamproof_overlay( );
		bool force_streamproof_windowed( );
		void restore_streamproof_windowed( );

		ID3D11Device* m_device{ nullptr };
		ID3D11DeviceContext* m_context{ nullptr };
		ID3D11RenderTargetView* m_rtv{ nullptr };
		HWND m_window{ nullptr };
		bool m_initialized{ false };
		bool m_ui_assets_ready{ false };
		int m_game_w{ 0 };
		int m_game_h{ 0 };
		IDXGISwapChain* m_swap_chain{ nullptr };
		bool m_overlay_forced_windowed{ false };

		HWND m_overlay_window{ nullptr };
		ID3D11Texture2D* m_overlay_texture{ nullptr };
		ID3D11Texture2D* m_overlay_staging{ nullptr };
		ID3D11RenderTargetView* m_overlay_rtv{ nullptr };
		HDC m_overlay_mem_dc{ nullptr };
		HBITMAP m_overlay_dib{ nullptr };
		HGDIOBJ m_overlay_old_bmp{ nullptr };
		void* m_overlay_bits{ nullptr };
		bool m_overlay_initialized{ false };
		int m_overlay_w{ 0 };
		int m_overlay_h{ 0 };
	};

    class menu
    {
    public:
        void initialize_graphics( );

        void draw( );
        void shutdown( ) const;

        void toggle( ) { this->m_open = !this->m_open; }
		[[nodiscard]] bool is_open( ) const { return this->m_open; }
		void set_draw_overlay_ctx( bool overlay ) { this->m_draw_on_overlay = overlay; }
		void apply_saved_cursor( );
		void apply_theme_preset( int preset );
		void apply_custom_accent( xdraw::color col );

        enum class master_mode : int
        {
            main, lua, players, count
        };

        enum class tab : int
        {
            ragebot, legitbot, player, world, misc, config, count
        };

        struct textures
        {
            struct entry
            {
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> resource{};
                int width{};
                int height{};
            };

            entry logo{};
            entry user{};
            entry tabs[ static_cast< int >( tab::count ) ]{};
            entry search{};
            entry settings{};
            entry cfg_folder_on{};
            entry cfg_folder_off{};
            entry cfg_cloud_on{};
            entry cfg_cloud_off{};
            entry cfg_plus{};
			entry flag_shield{};
			entry flag_eye{};
			entry flag_eye_off{};
			entry intro_splash{};
			entry cat_overlay{};
			entry dock_main{};
			entry dock_skins{};
			entry dock_models{};
			entry model_mannequin{};
			entry dock_lua{};
			entry dock_console{};
			entry dock_docs{};
			entry dock_players{};
			entry dock_save{};
			entry dock_power{};
			entry lua_icon{};
			entry lua_docs{};
			entry lua_copy{};
			entry lua_clear{};
			entry lua_play{};
			entry lua_reload{};
			entry lua_minus{};
			entry fw_logo_static{};
			xdraw::gif_image fw_logo_glow{};
			xdraw::gif_image fw_logo_spin{};
			xdraw::gif_image fw_logo_spin_glow{};
        } m_textures{};

    private:
		bool draw_intro( );
        void draw_status_bar( float x, float y, float w, float h );
        void draw_linoria_tab_bar( float x, float y, float w );
        void draw_linoria_subtab_bar( float x, float y, float w );
        void draw_ambience( float sw, float sh );
        void try_load_user_avatar( );
        void sync_theme_style( ) const;
        void draw_search_results( float x, float y, float w, float h );
        void rebuild_search_index( );
        void close_search( );
        void activate_search_result( std::size_t index );

        void draw_ragebot( float group_w ) const;
        void draw_legitbot( float group_w ) const;
        void draw_player( float group_w ) const;
        void draw_world( float group_w ) const;
        void draw_skins( float group_w ) const;
        void draw_skin_changer( float sw, float sh );
        void draw_misc( float group_w ) const;
        void draw_config( float group_w );

        bool m_open{ true };
        bool m_config_modal_open{};
        bool m_config_cloud_refresh_pending{};
        bool m_config_advanced_open{};
        bool m_last_open{ true };
		bool m_draw_on_overlay{ false };
        float m_open_anim{ 1.0f };
        std::uint8_t m_saved_relative_mouse{};
		bool m_has_saved_cursor{};
		int m_saved_cursor_x{};
		int m_saved_cursor_y{};

        float m_x{ 100.0f };
        float m_y{ 100.0f };
        float m_w{ 600.0f };  // LinoriaLib portrait aspect ratio: taller than width
        float m_h{ 720.0f };
        float m_body_x{};
        float m_body_y{};
        float m_body_w{};
        float m_body_h{};

        int m_tab{};
        int m_subtab{};
        int m_subtab_pill_tab{ -1 };
        float m_subtab_pill_x{ -1.0f };
        float m_tab_indicator_x{ -1.0f };
        float m_tab_indicator_w{ 0.0f };
        float m_subtab_indicator_x{ -1.0f };
        float m_subtab_indicator_w{ 0.0f };
		int m_connector_subtab_tab{ -1 };
		float m_connector_subtab_x{ -1.0f };
		float m_connector_tab_x{ -1.0f };
		float m_intro_elapsed{};
		float m_intro_assets_ready_at{ -1.0f };
		float m_intro_bar_phase{};
		bool m_intro_base_graphics_ready{};
		bool m_intro_finished{};
		bool m_search_open{};
		float m_user_avatar_retry_delay{};
		std::string m_search_query{};
		std::vector<std::size_t> m_search_visible_indices{};

		struct search_entry
		{
			std::string name{};
			std::string category{};
			std::string name_lower{};
			std::string category_lower{};
			int tab{};
			int subtab{};
			int bind_key{};
		};
		std::vector<search_entry> m_search_entries{};

		// Master Shell & Workspaces (Supports opening multiple windows at once)
		master_mode m_master_mode{ master_mode::main };
		bool m_show_main{ true };
		bool m_show_skins{ false };
		bool m_show_models{ false };
		bool m_show_lua{ false };
		bool m_show_lua_console{ false };
		bool m_show_docs{ false };
		bool m_show_players{ false };
		float m_master_indicator_x{ -1.0f };
		float m_master_indicator_w{ 0.0f };

		// Skin Changer Studio State
		float m_skins_x{ 100.0f };
		float m_skins_y{ 40.0f };
		float m_skins_w{ 1160.0f };
		float m_skins_h{ 720.0f };

		// Custom Models Studio State
		float m_models_x{ 320.0f };
		float m_models_y{ 60.0f };
		float m_models_w{ 980.0f };
		float m_models_h{ 620.0f };
		int m_models_selected{ -1 };
		std::string m_models_search{};

		// Lua Studio State
		float m_lua_x{ 620.0f };
		float m_lua_y{ 60.0f };
		float m_lua_w{ 920.0f };
		float m_lua_h{ 600.0f };

		struct lua_script_entry
		{
			std::string name{};
			std::string path{};
			bool auto_load{};
		};
		std::vector<lua_script_entry> m_lua_scripts{};
		int m_lua_selected_script{ -1 };
		std::vector<std::string> m_lua_editor_lines{};
		int m_lua_cursor_line{ 0 };
		std::string m_lua_new_script_name{ "custom_esp.lua" };
		std::string m_lua_line_edit_buf{};
		int m_lua_sel_start{ -1 };
		int m_lua_sel_end{ -1 };
		bool m_lua_sel_dragging{ false };

		struct lua_log_entry
		{
			std::string time{};
			std::string text{};
			xdraw::color col{ 255, 255, 255, 255 };
		};
		std::vector<lua_log_entry> m_lua_logs{};
		bool m_lua_initialized{};

		// Dedicated Console Workspace State
		float m_console_x{ 620.0f };
		float m_console_y{ 440.0f };
		float m_console_w{ 720.0f };
		float m_console_h{ 300.0f };
		std::string m_console_input_buf{};

		// Dedicated Documentation Workspace State
		float m_docs_x{ 580.0f };
		float m_docs_y{ 80.0f };
		float m_docs_w{ 780.0f };
		float m_docs_h{ 560.0f };
		int m_doc_category{ 0 };
		int m_doc_item{ 0 };

		// Players Workspace State
		float m_players_x{ 620.0f };
		float m_players_y{ 60.0f };
		float m_players_w{ 880.0f };
		float m_players_h{ 580.0f };

		struct player_match_entry
		{
			std::uint64_t steam_id{};
			std::string nickname{};
			int ping{};
			int health{ 100 };
			int armor{ 0 };
			bool has_helmet{ false };
			bool has_defuser{ false };
			int money{ 0 };
			std::string weapon{};
			bool is_alive{ true };
			int team{ 0 };
			bool is_local{};
			bool is_bot{};
			ID3D11ShaderResourceView* avatar_srv{ nullptr };
		};
		std::vector<player_match_entry> m_match_players{};
		float m_players_last_refresh{ 0.0f };
		std::string m_players_filter{};
		int m_players_team_filter{ 0 };
		int m_players_selected{ -1 };
		float m_players_selected_refresh{ 0.0f };

		struct snow_flake
		{
			float x{};
			float y{};
			float vx{};
			float vy{};
			float size{};
			std::uint8_t alpha{ 255 };
		};
		std::vector<snow_flake> m_snow{};
		float m_snow_time{ 0.0f };

		void draw_top_master_bar( float sw, float sh );
		void draw_lua_studio( float sw, float sh );
		void draw_lua_console( float sw, float sh );
		void draw_documentation( float sw, float sh );
		void draw_players_inspector( float sw, float sh );
		void draw_models_studio( float sw, float sh );

		void init_lua_studio( );
		void refresh_lua_scripts( );
		void load_lua_script( const std::string& path );
		void save_lua_script( const std::string& path );
		void execute_lua_buffer( );
	public:
		void append_lua_log( std::string_view tag, std::string_view msg, xdraw::color col );
	private:

		void refresh_match_players( );


        static constexpr const char* k_tab_names[ static_cast< int >( tab::count ) ]
        {
            "rage", "legit", "player", "world", "misc", "config"
        };

        static constexpr auto k_max_subtabs{ 6 };

        struct subtab_info
        {
            const char* names[ k_max_subtabs ]{};
            int count{};
        };

        static constexpr subtab_info k_subtab_defs[ static_cast< int >( tab::count ) ]
        {
            { { "pistol", "smg", "rifle", "shotgun", "sniper", "lmg" }, 6 },
            { { "pistol", "smg", "rifle", "shotgun", "sniper", "lmg" }, 6 },
            { { "enemies", "allies", "local" },                         3 },
            { { "esp", "scene", "view", "weather", "cosmetics" },       5 },
            { { "movement", "effects", "hud", "general" },              4 },
            { { "configs", "settings" },                                2 }
        };
    };

	class widgets
	{
	public:
		void draw( );

		static inline std::string s_map_name{};

	private:
		void watermark( xdraw::draw_list& draw_list );
		void keybinds( xdraw::draw_list& draw_list );
		void media_player( xdraw::draw_list& draw_list );
		void audio_visualizer_widget( xdraw::draw_list& draw_list );
		void fw_logo_widget( xdraw::draw_list& draw_list );
	};

	class fonts
	{
	public:
		enum class size : std::uint8_t
		{
			petite,
			normal,
			big,
			count
		};

		struct family_t
		{
			std::array<xdraw::font*, static_cast< std::size_t >( size::count )> sizes{ };

			xdraw::font* operator[]( size size ) const { return this->sizes[ static_cast< std::size_t >( size ) ]; }
			xdraw::font*& operator[]( size size ) { return this->sizes[ static_cast< std::size_t >( size ) ]; }
		};

		void initialize( );

		family_t inter_medium{};
		family_t inter_bold{};
		family_t smallest_pixel7{};

	private:
		void load_family( family_t& family, std::span<const std::byte> data, const std::array<float, static_cast< std::size_t >( size::count )>& sizes );
	};

	// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	//  LinoriaLib-style group header â€” call first inside xui::begin_child()
	// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	inline void group_header( const char* label )
	{
		auto& dl = xui::draw::current( );
		const auto [tw, th] = xdraw::measure_text( label );
		const auto avail_w = xui::layout::item_width( );
		const auto r = xui::layout::item( avail_w, th + 4.0f );

		// Rounded accent tick
		dl.rect_filled( r.x, r.y + 2.0f, 2.5f, th - 2.0f, tokens::col_accent, xdraw::corner_radius{ 1.25f } );

		// Title label
		dl.text( r.x + 7.0f, r.y, label, tokens::col_text );

		// Trailing subtle line to right edge
		const float line_x = r.x + 11.0f + tw;
		if ( line_x < r.right( ) )
		{
			dl.rect_filled( line_x, r.y + std::floor( th * 0.5f ), r.right( ) - line_x, 1.0f, tokens::col_line );
		}
	}

	inline bool group_subtabs( const char* const* tab_names, int tab_count, int& active_tab )
	{
		auto& dl = xui::draw::current( );
		const auto& inp = xui::ctx( ).input;
		const auto avail_w = xui::layout::item_width( );
		constexpr float tab_h = 20.0f;

		const auto abs = xui::layout::item( avail_w, tab_h );
		dl.rect_filled( abs.x, abs.y, abs.w, tab_h, tokens::col_card, xdraw::corner_radius{ 6.0f } );
		dl.rect( abs.x, abs.y, abs.w, tab_h, tokens::col_border, xdraw::corner_radius{ 6.0f }, 1.0f );

		const float btn_w = abs.w / static_cast<float>( tab_count );
		bool changed = false;

		for ( int i = 0; i < tab_count; ++i )
		{
			const float bx = abs.x + btn_w * static_cast<float>( i );
			const xui::rect btn{ bx, abs.y, btn_w, tab_h };
			const bool is_active = ( active_tab == i );
			const bool hovered = inp.in_rect( btn );

			if ( hovered && inp.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				active_tab = i;
				changed = true;
			}

			const auto act_a = xui::anim::lerp( xui::fnv1a( "grp_tab_act" ) + i, is_active ? 1.0f : 0.0f, 14.0f );

			if ( act_a > 0.01f )
			{
				// Soft light pill, same as the dock tabs (not an accent fill).
				dl.rect_filled( bx + 2.0f, abs.y + 2.0f, btn_w - 4.0f, tab_h - 4.0f,
					xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 34.0f * act_a ) }, xdraw::corner_radius{ 5.0f } );
			}
			else if ( hovered )
			{
				dl.rect_filled( bx + 2.0f, abs.y + 2.0f, btn_w - 4.0f, tab_h - 4.0f,
					tokens::col_elevated, xdraw::corner_radius{ 5.0f } );
			}

			const auto [tw, th] = xdraw::measure_text( tab_names[ i ] );
			const float tx = std::floor( bx + ( btn_w - tw ) * 0.5f );
			const float ty = std::floor( abs.y + ( tab_h - th ) * 0.5f );
			dl.text( tx, ty, tab_names[ i ], xui::lerp( tokens::col_text_dim, tokens::col_accent, act_a ) );
		}

		xui::layout::spacing( 4.0f );
		return changed;
	}


	inline context g_context{};
	inline menu g_menu{};
	inline widgets g_widgets{};
	inline fonts g_fonts{};

	inline std::uint64_t s_last_kill_time{ 0 };
	inline std::uint64_t s_last_shot_time{ 0 };

} // namespace rendering
