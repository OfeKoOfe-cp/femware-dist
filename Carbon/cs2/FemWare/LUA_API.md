# Velocity — Lua Scripting API Reference

The Lua runtime exposes four global tables: `render`, `client`, `engine`, and `events`.
All of them are available to scripts loaded in the **Lua Studio** tab. Re-running a script
replaces every previously registered callback.

Colors are accepted by every render function in either form:

```lua
render.rect_filled(10, 10, 100, 40, 255, 95, 175, 255)      -- r, g, b, [a]
render.rect_filled(10, 10, 100, 40, render.color(255, 95, 175, 255)) -- color table
```

---

## Events

| Signature | Description |
| --- | --- |
| `events.listen("on_draw", fn)` | Registers a per-frame render callback. The event name may also be `"render"` or `"paint"` (aliases). |
| `events.register("on_draw", fn)` | Alias of `events.listen`. |

```lua
events.listen("on_draw", function()
    local sw, sh = render.screen_size()
    local tw, th = render.measure_text("femware")
    render.text(sw - tw - 16, 16, "femware", render.color(255, 95, 175, 255))
end)
```

---

## render

| Signature | Returns | Description |
| --- | --- | --- |
| `render.text(x, y, str, col, [centered])` | — | Draws text with the active cheat font. `centered` centers it horizontally on `(x, y)`. |
| `render.line(x1, y1, x2, y2, col, [thick])` | — | Draws a 2D line with optional thickness. |
| `render.rect(x, y, w, h, col, [rounding], [thick])` | — | Draws an outlined rectangle with optional corner rounding. |
| `render.rect_filled(x, y, w, h, col, [rounding])` | — | Draws a filled rectangle with optional corner rounding. |
| `render.gradient_rect(x, y, w, h, col1, col2, [horizontal=true], [radius])` | — | Draws a filled rectangle with a two-color linear gradient. |
| `render.circle(cx, cy, r, col, [thick])` | — | Draws an outlined circle. |
| `render.circle_filled(cx, cy, r, col)` | — | Draws a filled circle. |
| `render.color(r, g, b, [a])` | col | Builds an RGBA color table. |
| `render.measure_text(str)` | w, h | Measures the pixel width and height of a string. |
| `render.screen_size()` | w, h | Returns the display viewport dimensions. |
| `render.world_to_screen(x, y, z)` | sx, sy, vis | Projects world coordinates to screen space (also accepts a `{x, y, z}` table). |

```lua
local c1 = render.color(255, 95, 175, 255)
local c2 = render.color(140, 70, 240, 0)
render.gradient_rect(100, 100, 200, 30, c1, c2, true, 4.0)
```

---

## client

| Signature | Returns | Description |
| --- | --- | --- |
| `client.get_local()` | table or nil | Local state: `health`, `armor`, `team`, `is_alive`, `is_scoped`, `speed`, `origin {x,y,z}`, `velocity {x,y,z}`. |
| `client.get_players([enemies_only])` | array | Player list with `name`, `team`, `ping`, `steam_id`, `health`, `armor`, `is_local`, `is_alive`, `is_enemy`, `distance`, `origin {x,y,z}`, `screen {x,y,visible}`. |
| `client.get_weapon()` | name, def_index | Active weapon name (no `weapon_` prefix) and its item definition index; both nil when unarmed. |
| `client.is_key_down(vk_code)` | bool | Real-time key state for a Windows Virtual Key code. |
| `client.get_view_angles()` | pitch, yaw, roll | Current local view angles in degrees. |
| `client.get_cursor_pos()` | mx, my | Mouse cursor position relative to the game window. |
| `client.screen_size()` | w, h | Display viewport dimensions. |
| `client.log(msg, [r, g, b])` | — | Prints a message to the Lua console. |

```lua
local me = client.get_local()
if me and me.is_alive then
    print(string.format("HP: %d Speed: %.0f", me.health, me.speed))
end

local w, id = client.get_weapon()
if w then
    render.text(16, 48, "Weapon: " .. w, render.color(255, 255, 255, 255))
end
```

---

## engine

| Signature | Returns | Description |
| --- | --- | --- |
| `engine.is_in_game()` | bool | True when fully connected and spawned into a live match (sign-on state ≥ 6). |
| `engine.get_map_name()` | string | Current map name, e.g. `de_mirage`. |
| `engine.get_ping()` | int | Round-trip network latency in milliseconds. |
| `engine.get_time()` | seconds | Fractional seconds since process start. |
| `engine.is_key_down(vk_code)` | bool | Alias of `client.is_key_down`. |

```lua
if engine.is_in_game() then
    local map = engine.get_map_name()
    render.text(16, 16, "IN-GAME  " .. map, render.color(74, 222, 128, 255))
end
```