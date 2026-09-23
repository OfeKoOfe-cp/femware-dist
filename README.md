# FemWare Lua API

Reference for the Lua 5.4 runtime embedded in the FemWare client. Scripts run in the in-game
**Lua Editor** tab and draw through the same renderer as the client menu.

## Quick start

1. Open the in-game menu and switch to the **Lua Editor** tab.
2. Write a script in the editor, or load one from disk.
3. Click **Run**. The script stays live until you re-run it.
4. Click **Docs** to open this reference in your browser.

```lua
events.listen("on_draw", function()
    local sw, sh = render.screen_size()
    local me = client.get_local()
    local msg = me and ("HP: " .. me.health) or "no local player"
    render.text(16, 16, msg, render.color(255, 255, 255, 255))
end)
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

- [Getting started — callbacks](getting-started/callbacks.md)
- [Getting started — scripting rules](getting-started/scripting-rules.md)
- [events](modules/events.md)
- [render](modules/render.md)
- [client](modules/client.md)
- [entity](modules/entity.md)
- [engine](modules/engine.md)
- [globals](modules/globals.md)
- [schema](modules/schema.md)
- [convar](modules/convar.md)
- [math](modules/math.md)
- [Cookbook — ESP skeleton](cookbook/esp-skeleton.md)
- [Cookbook — player tags](cookbook/player-tags.md)

## Releases

Signed payloads are published on the [releases page](https://github.com/OfeKoOfe-cp/femware-dist/releases).