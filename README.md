# FemWare Lua API

Reference for the Lua 5.4 runtime embedded in the FemWare.

## Global functions

### `loadstring(source_or_url, [chunk_name]) -> fn or nil, err`

Luau-style `loadstring`. Compiles a script and returns its function. If the first argument is an
`http(s)://` url it fetches the raw script first and compiles the fetched body, so raw links can
be executed directly:

```lua
local fn, err = loadstring("https://raw.githubusercontent.com/user/repo/main/something.lua")
if fn then
    fn()
else
    client.log("load failed: " .. tostring(err))
end
```

## Modules

| Module | Purpose |
| --- | --- |
| `render` | Overlay drawing — text, shapes, gradients, world-to-screen |
| `client` | Local player, eye position, player list, weapon, cursor, input, clipboard |
| `engine` | In-game state, map, ping, fps, time |
| `entity` | Indexed access to the player list |
| `events` | Per-frame render callbacks |
| `globals` | Server curtime, tick count, screen size |
| `schema` | Read-only Source 2 schema field offsets |
| `convar` | Read-only typed console variables |
| `math` | Standard library extended with convenience functions |

## Reference

- [events](modules/events.md)
- [render](modules/render.md)
- [client](modules/client.md)
- [entity](modules/entity.md)
- [engine](modules/engine.md)
- [globals](modules/globals.md)
- [schema](modules/schema.md)
- [convar](modules/convar.md)
- [math](modules/math.md)

## Releases

Signed payloads are published on the [releases page](https://github.com/OfeKoOfe-cp/femware-dist/releases).
