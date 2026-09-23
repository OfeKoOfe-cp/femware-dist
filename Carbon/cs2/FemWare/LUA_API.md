# Velocity — Lua Scripting API Reference

The Lua runtime exposes the global tables `render`, `client`, `engine`, `events`, `convar`, and
`math` (extended). All of them are available to scripts loaded in the **Lua Studio** tab.
Re-running a script replaces every previously registered callback.

## Sandbox & Stability

- The standard library is **sandboxed**: `os`, `io`, `debug`, `package`, `load`, `dofile`,
  `loadfile`, `require`, and `collectgarbage` are stripped. Scripts cannot touch the disk,
  start processes, or inspect the host.
- Every callback runs under an **instruction budget**. A runaway loop (e.g. `while true do end`)
  aborts with a `script aborted: exceeded the N-instruction budget` error instead of freezing
  the render loop.
- A callback that raises **8 consecutive errors** is automatically unregistered and logged as
  disabled; re-run the script to re-enable it.
- Garbage collection is tuned for a per-frame render loop with a bounded incremental step.

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
| `render.polyline(points, col, [closed], [thick])` | — | Draws a path through `{ {x,y}, ... }` (or a flat `x,y,x,y,...` array). `[closed]` joins the last point to the first. |
| `render.triangle(x1,y1,x2,y2,x3,y3, col, [filled], [thick])` | — | Draws a triangle; filled when `[filled]` is true, otherwise an outlined path. |
| `render.text_outlined(x, y, str, col, [centered])` | — | Text with a crisp 1px black outline. |
| `render.text_shadowed(x, y, str, col, [centered])` | — | Text with a soft one-pixel drop shadow. |
| `render.color(r, g, b, [a])` | col | Builds an RGBA color table. |
| `render.measure_text(str)` | w, h | Measures the pixel width and height of a string. |
| `render.screen_size()` | w, h | Returns the display viewport dimensions. |
| `render.delta_time()` | seconds | Fractional seconds since the last rendered frame — for frame-independent animation. |
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
| `client.get_local()` | table or nil | Local state: `health`, `armor`, `team`, `is_alive`, `is_scoped`, `is_grounded`, `speed`, `origin {x,y,z}`, `velocity {x,y,z}`. |
| `client.get_eye_pos()` | x, y, z | Local camera/eye position in world space (origin + view offset). |
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
| `engine.get_fps()` | number | Smoothed frame rate. |
| `engine.get_delta_time()` | seconds | Alias of `render.delta_time`. |
| `engine.get_time()` | seconds | Fractional seconds since process start. |
| `engine.is_key_down(vk_code)` | bool | Alias of `client.is_key_down`. |

```lua
if engine.is_in_game() then
    local map = engine.get_map_name()
    render.text(16, 16, "IN-GAME  " .. map, render.color(74, 222, 128, 255))
end
```

---

## convar

| Signature | Returns | Description |
| --- | --- | --- |
| `convar.get(name)` | string \| number \| boolean \| nil | Reads the current value of any registered console variable (typed). Returns nil for unknown or unsupported types. **Read-only** — scripts cannot execute console commands. |

```lua
local sens = convar.get("sensitivity")   -- number
local sv   = convar.get("sv_cheats")     -- boolean
local map  = convar.get("map")           -- string
```

---

## math

`math` is the standard Lua math library extended with cheat conveniences (standard functions
like `floor`, `ceil`, `abs`, `sqrt`, `min`, `max`, `rad`, `deg`, `pi`, and `random` remain available).

| Signature | Returns | Description |
| --- | --- | --- |
| `math.clamp(v, lo, hi)` | number | Clamps `v` into `[lo, hi]`. |
| `math.lerp(a, b, t)` | number | Linear interpolation: `a + (b - a) * t`. |
| `math.saturate(t)` | number | Clamp into `[0, 1]`. |
| `math.pingpong(t, len)` | number | Oscillates between `0` and `len` as `t` grows. |
| `math.normalize_yaw(yaw)` | number | Wraps a yaw into `(-180, 180]`. |
| `math.angle_diff(a, b)` | number | Shortest signed angular difference in degrees, `(-180, 180]`. |
| `math.distance(a, b)` | number | Distance between two `{x,y,z}` tables or two sets of `(x,y,z)` coordinates. |
| `math.vector_to_angle(x, y, z)` | pitch, yaw | Converts a direction vector to FPS-convention angles in degrees. |
| `math.randomf(min, max)` | number | Uniform random float in `[min, max]`. |

```lua
local me = client.get_local()
if me and me.is_alive then
    local p = client.get_players()[1]
    if p then
        local d = math.distance(me.origin, p.origin)
        local ang_p, ang_y = math.vector_to_angle(
            p.origin.x - me.origin.x, p.origin.y - me.origin.y, p.origin.z - me.origin.z)
        print(string.format("player at %.0f units, yaw %.1f", d, ang_y))
    end
end
```