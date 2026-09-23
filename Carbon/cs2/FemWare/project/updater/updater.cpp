#include <pch/pch.hpp>

#include "updater.hpp"

#include <external/nlohmann/json.hpp>

#include <Windows.h>
#include <winhttp.h>
#include <ShlObj.h>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

// Parse a hex string like "0x23BEB20" → uintptr_t
static std::uintptr_t parse_hex( const std::string& s ) {
    if ( s.empty() ) return 0;

    std::uintptr_t value = 0;
    const char* begin = s.data();
    const char* end   = s.data() + s.size();

    if ( s.size() > 2 && s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) )
        begin += 2;

    const auto res = std::from_chars( begin, end, value, 16 );
    if ( res.ec != std::errc{} || res.ptr != end )
        return 0;

    return value;
}

// WinHTTP GET — returns body as string, empty on failure
static std::string winhttp_get( const wchar_t* host, const wchar_t* path ) {
    std::string result{};

    HINTERNET session = WinHttpOpen(
        L"Femware/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0 );
    if ( !session ) return result;

    HINTERNET conn = WinHttpConnect( session, host, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    if ( !conn ) { WinHttpCloseHandle( session ); return result; }

    HINTERNET req = WinHttpOpenRequest(
        conn, L"GET", path,
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE );

    if ( !req ) {
        WinHttpCloseHandle( conn );
        WinHttpCloseHandle( session );
        return result;
    }

    // 8-second timeout on all operations
    DWORD conn_timeout_ms = 8000;
    DWORD send_timeout_ms = 8000;
    DWORD recv_timeout_ms = 8000;
    WinHttpSetOption( req, WINHTTP_OPTION_CONNECT_TIMEOUT, &conn_timeout_ms, sizeof(DWORD) );
    WinHttpSetOption( req, WINHTTP_OPTION_SEND_TIMEOUT,    &send_timeout_ms, sizeof(DWORD) );
    WinHttpSetOption( req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &recv_timeout_ms, sizeof(DWORD) );

    if ( WinHttpSendRequest( req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                             WINHTTP_NO_REQUEST_DATA, 0, 0, 0 ) &&
         WinHttpReceiveResponse( req, nullptr ) ) {

        DWORD bytes_available = 0;
        while ( WinHttpQueryDataAvailable( req, &bytes_available ) && bytes_available > 0 ) {
            std::string chunk( bytes_available, '\0' );
            DWORD bytes_read = 0;
            if ( WinHttpReadData( req, chunk.data(), bytes_available, &bytes_read ) )
                result.append( chunk.data(), bytes_read );
        }
    }

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( conn );
    WinHttpCloseHandle( session );
    return result;
}

// Apply a parsed JSON offset object into carbon::offsets
static void apply_json( const json& j ) {
    if ( !j.is_object() )
        return;

    auto get = [ & ]( const char* dll, const char* key ) -> std::uintptr_t {
        if ( !j.contains( dll ) || !j[ dll ].is_object() )
            return 0;
        const auto& sec = j[ dll ];
        if ( !sec.contains( key ) )
            return 0;
        const auto& v = sec[ key ];
        if ( v.is_string() ) return parse_hex( v.get<std::string>() );
        if ( v.is_number_unsigned() ) return v.get<std::uintptr_t>();
        return 0;
    };

    // client.dll
    carbon::offsets::dwEntityList              = get( "client.dll", "dwEntityList" );
    carbon::offsets::dwLocalPlayerPawn         = get( "client.dll", "dwLocalPlayerPawn" );
    carbon::offsets::dwLocalPlayerController   = get( "client.dll", "dwLocalPlayerController" );
    carbon::offsets::dwViewMatrix              = get( "client.dll", "dwViewMatrix" );
    carbon::offsets::dwViewAngles              = get( "client.dll", "dwViewAngles" );
    carbon::offsets::dwGlowManager             = get( "client.dll", "dwGlowManager" );
    carbon::offsets::dwCSGOInput               = get( "client.dll", "dwCSGOInput" );
    carbon::offsets::dwGameRules               = get( "client.dll", "dwGameRules" );
    carbon::offsets::dwGlobalVars              = get( "client.dll", "dwGlobalVars" );
    carbon::offsets::dwWeaponC4                = get( "client.dll", "dwWeaponC4" );
    carbon::offsets::dwGameEntitySystem        = get( "client.dll", "dwGameEntitySystem" );
    carbon::offsets::dwPlantedC4               = get( "client.dll", "dwPlantedC4" );
    carbon::offsets::dwPrediction              = get( "client.dll", "dwPrediction" );
    carbon::offsets::dwSensitivity             = get( "client.dll", "dwSensitivity" );
    carbon::offsets::dwSensitivity_sensitivity = get( "client.dll", "dwSensitivity_sensitivity" );

    // engine2.dll
    carbon::offsets::dwBuildNumber                      = get( "engine2.dll", "dwBuildNumber" );
    carbon::offsets::dwNetworkGameClient                = get( "engine2.dll", "dwNetworkGameClient" );
    carbon::offsets::dwNetworkGameClient_localPlayer    = get( "engine2.dll", "dwNetworkGameClient_localPlayer" );
    carbon::offsets::dwNetworkGameClient_signOnState    = get( "engine2.dll", "dwNetworkGameClient_signOnState" );
    carbon::offsets::dwNetworkGameClient_maxClients     = get( "engine2.dll", "dwNetworkGameClient_maxClients" );
    carbon::offsets::dwWindowWidth                      = get( "engine2.dll", "dwWindowWidth" );
    carbon::offsets::dwWindowHeight                     = get( "engine2.dll", "dwWindowHeight" );

    // inputsystem.dll
    carbon::offsets::dwInputSystem = get( "inputsystem.dll", "dwInputSystem" );
}

// Current CS2 build number read directly from memory or engine2.dll export
static std::uint32_t read_build_number() {
    HMODULE engine = GetModuleHandleW( L"engine2.dll" );
    if ( !engine ) return 0;

    if ( carbon::offsets::dwBuildNumber > 0 ) {
        const auto val = *reinterpret_cast<const std::uint32_t*>(
            reinterpret_cast<std::uintptr_t>( engine ) + carbon::offsets::dwBuildNumber );
        if ( val > 0 ) return val;
    }

    using fn_t = std::uint32_t(*)();

    // Fallback: scan PE export "GetBuildNumber" if it exists
    if ( auto* fn = reinterpret_cast<fn_t>(
             GetProcAddress( engine, "GetBuildNumber" ) ) )
        return fn();

    return 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// carbon::updater implementation
// ---------------------------------------------------------------------------
namespace carbon::updater {

std::wstring cache_path() {
    wchar_t appdata[ MAX_PATH ]{};
    SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata );
    return std::wstring( appdata ) + L"\\femware\\offsets.json";
}

bool load_cached() {
    auto path = cache_path();

    if ( !std::filesystem::exists( path ) ) {
        wchar_t appdata[ MAX_PATH ]{};
        SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata );
        const std::wstring legacy_path = std::wstring( appdata ) + L"\\carbon\\offsets.json";
        if ( std::filesystem::exists( legacy_path ) ) {
            path = legacy_path;
        }
    }

    // Open the cache file
    std::ifstream f( path );
    if ( !f.is_open() ) return false;

    auto j = json::parse( f, nullptr, false );
    if ( j.is_discarded() ) return false;

    // Apply JSON first so dwBuildNumber is known for live_build check
    apply_json( j );

    // Check build number matches current game
    const std::uint32_t cached_build = j.value( "_build", 0u );
    const std::uint32_t live_build   = read_build_number();

    // If we have no build number or it doesn't match live build → cache is stale
    if ( cached_build == 0 || ( live_build != 0 && live_build != cached_build ) )
        return false;

    offsets::g_fetched_build = cached_build;
    offsets::g_source        = offsets::source::api_cached;
    return offsets::dwEntityList != 0;   // sanity check
}

bool fetch_live() {
    const std::string body = winhttp_get( L"raw.githubusercontent.com", L"/sezzyaep/CS2-OFFSETS/main/offsets.json" );
    if ( body.empty() ) return false;

    const auto j = json::parse( body, nullptr, false );
    if ( j.is_discarded() ) return false;

    apply_json( j );
    offsets::g_source = offsets::source::api_live;

    if ( offsets::dwEntityList != 0 ) {
        const auto live_build = read_build_number();
        if ( live_build != 0 ) {
            offsets::g_fetched_build = live_build;
        }
        save_cache();
        return true;
    }

    return false;
}

void save_cache() {
    const auto path = cache_path();

    // Ensure directory exists
    {
        wchar_t appdata[ MAX_PATH ]{};
        SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata );
        std::wstring dir = std::wstring( appdata ) + L"\\femware";
        CreateDirectoryW( dir.c_str(), nullptr );
    }

    json j;
    j[ "_build" ] = offsets::g_fetched_build;

    // client.dll
    j[ "client.dll" ][ "dwEntityList" ]              = offsets::dwEntityList;
    j[ "client.dll" ][ "dwLocalPlayerPawn" ]          = offsets::dwLocalPlayerPawn;
    j[ "client.dll" ][ "dwLocalPlayerController" ]    = offsets::dwLocalPlayerController;
    j[ "client.dll" ][ "dwViewMatrix" ]               = offsets::dwViewMatrix;
    j[ "client.dll" ][ "dwViewAngles" ]               = offsets::dwViewAngles;
    j[ "client.dll" ][ "dwGlowManager" ]              = offsets::dwGlowManager;
    j[ "client.dll" ][ "dwCSGOInput" ]                = offsets::dwCSGOInput;
    j[ "client.dll" ][ "dwGameRules" ]                = offsets::dwGameRules;
    j[ "client.dll" ][ "dwGlobalVars" ]               = offsets::dwGlobalVars;
    j[ "client.dll" ][ "dwWeaponC4" ]                 = offsets::dwWeaponC4;
    j[ "client.dll" ][ "dwGameEntitySystem" ]         = offsets::dwGameEntitySystem;
    j[ "client.dll" ][ "dwPlantedC4" ]                = offsets::dwPlantedC4;
    j[ "client.dll" ][ "dwPrediction" ]               = offsets::dwPrediction;
    j[ "client.dll" ][ "dwSensitivity" ]              = offsets::dwSensitivity;
    j[ "client.dll" ][ "dwSensitivity_sensitivity" ]  = offsets::dwSensitivity_sensitivity;

    // engine2.dll
    j[ "engine2.dll" ][ "dwBuildNumber" ]                   = offsets::dwBuildNumber;
    j[ "engine2.dll" ][ "dwNetworkGameClient" ]             = offsets::dwNetworkGameClient;
    j[ "engine2.dll" ][ "dwNetworkGameClient_localPlayer" ] = offsets::dwNetworkGameClient_localPlayer;
    j[ "engine2.dll" ][ "dwNetworkGameClient_signOnState" ] = offsets::dwNetworkGameClient_signOnState;
    j[ "engine2.dll" ][ "dwNetworkGameClient_maxClients" ]  = offsets::dwNetworkGameClient_maxClients;
    j[ "engine2.dll" ][ "dwWindowWidth" ]                   = offsets::dwWindowWidth;
    j[ "engine2.dll" ][ "dwWindowHeight" ]                  = offsets::dwWindowHeight;

    // inputsystem.dll
    j[ "inputsystem.dll" ][ "dwInputSystem" ] = offsets::dwInputSystem;

    std::ofstream f( path );
    if ( f.is_open() )
        f << j.dump( 2 );
}

bool initialize() {
    // ── Priority 1: cached offsets (instant, no network) ─────────────────
    if ( load_cached() )
        return true;

    // ── Priority 2: live API fetch ────────────────────────────────────────
    if ( fetch_live() )
        return true;

    // ── Priority 3: pattern scan fallback ────────────────────────────────
    // The existing addresses::globals system will run normally.
    // We mark the source so callers know.
    offsets::g_source = offsets::source::pattern_scan;
    return false;   // caller should not treat this as fatal
}

} // namespace carbon::updater