# FemWare Lua API

Reference for the Lua 5.4 runtime embedded in the FemWare.

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
| `spawn` | Coroutine timers (`spawn` + `client.wait`) |
| `http` | Blocking HTTP GET |
| `json` | JSON encode/decode |

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
- [loadstring](modules/loadstring.md)
- [spawn](modules/spawn.md)
- [http](modules/http.md)
- [json](modules/json.md)

## Releases

Signed payloads are published on the [releases page](https://github.com/OfeKoOfe-cp/femware-dist/releases).
