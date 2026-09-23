# FemWare

An internal Counter-Strike 2 client for the modern Source 2 SDK. Hardware-rendered menu, a
sandboxed Lua scripting runtime, and an entirely local feature set — no cloud config, no
telemetry, no external dependencies at runtime.

## Layout

- `Carbon/` — primary workspace
  - `cs2/FemWare/` — client source tree
  - `cs2/FemWare/LUA_API.md` — Lua scripting reference
  - `cs2/FemWare/CHANGELOG.md` — feature history
- `loader/` — signed loader and payload pipeline
- `webradar/` — browser-based radar companion

## Documentation

| Document | Contents |
| --- | --- |
| [README](Carbon/README.md) | Requirements and build instructions |
| [Lua Scripting API](Carbon/cs2/FemWare/LUA_API.md) | Full reference for `render`, `client`, `engine`, `events`, `entity`, `globals`, `schema`, `convar`, and `math` |
| [Feature Changelog](Carbon/cs2/FemWare/CHANGELOG.md) | 1:1 inventory of features and subsystems |

## Scripting

FemWare ships a sandboxed Lua 5.4 runtime in the Lua Studio tab. Scripts draw through the same
geometry pipeline as the menu, run under per-callback instruction budgets, and cannot touch the
disk or the host process.

```lua
events.listen("on_draw", function()
    local me = client.get_local()
    local msg = me and ("HP: " .. me.health) or "no local player"
    render.text(16, 16, msg, render.color(255, 255, 255, 255))
end)
```

Lua Studio's **Docs** button opens this repository; the complete binding reference lives in
[LUA_API.md](Carbon/cs2/FemWare/LUA_API.md).

## Build

See [Carbon/README.md](Carbon/README.md) for toolchain requirements and the Ship build command.