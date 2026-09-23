# engine

| Signature | Returns | Description |
| --- | --- | --- |
| `engine.is_in_game()` | bool | True when fully connected and spawned into a live match (sign-on state ≥ 6). |
| `engine.get_map_name()` | string | Current map name, e.g. `de_mirage`. |
| `engine.get_ping()` | int | Round-trip network latency in milliseconds. |
| `engine.get_fps()` | number | Smoothed frame rate. |
| `engine.get_delta_time()` | seconds | Alias of `render.delta_time`. |
| `engine.get_time()` | seconds | Time since process start. |
| `engine.is_key_down(vk_code)` | bool | Alias of `client.is_key_down`. |

```lua
if engine.is_in_game() then
    local map = engine.get_map_name()
    render.text(16, 16, "IN-GAME  " .. map, render.color(74, 222, 128, 255))
end
```