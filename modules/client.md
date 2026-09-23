# client

| Signature | Returns | Description |
| --- | --- | --- |
| `client.get_local()` | table or nil | Local state: `health`, `armor`, `team`, `is_alive`, `is_scoped`, `is_grounded`, `speed`, `origin {x,y,z}`, `velocity {x,y,z}`, `addr`. |
| `client.get_eye_pos()` | x, y, z | Local eye position in world space. |
| `client.get_players([enemies_only])` | array | Player list with `name`, `team`, `ping`, `steam_id`, `health`, `armor`, `is_local`, `is_alive`, `is_enemy`, `distance`, `origin {x,y,z}`, `screen {x,y,visible}`, `addr`. |
| `client.get_weapon()` | name, def_index | Active weapon name (no `weapon_` prefix) and item definition index; nil when unarmed. |
| `client.is_key_down(vk_code)` | bool | Real-time state of a Windows Virtual Key code. |
| `client.get_view_angles()` | pitch, yaw, roll | Current local view angles in degrees. |
| `client.get_cursor_pos()` | mx, my | Mouse cursor position relative to the game window. |
| `client.screen_size()` | w, h | Display viewport dimensions. |
| `client.log(msg, [r, g, b])` | — | Prints a message to the Lua console. |
| `client.set_clipboard(text)` | — | Copies a string to the Windows clipboard. |
| `client.wait(ms)` | — | Suspends the running coroutine; only valid inside `spawn`. |
| `client.set_bind(vk_code, fn)` | — | Runs `fn` once per key press; `nil` clears the bind. |
| `client.set_data(key, value)` | — | Persists a JSON value for the current script. |
| `client.get_data(key)` | value or nil | Reads a value written by `client.set_data`. |

```lua
local me = client.get_local()
if me and me.is_alive then
    client.log(string.format("HP: %d Speed: %.0f", me.health, me.speed))
end

local w, id = client.get_weapon()
if w then
    render.text(16, 48, "Weapon: " .. w, render.color(255, 255, 255, 255))
end
```