# events

| Signature | Description |
| --- | --- |
| `events.listen("on_draw", fn)` | Registers a per-frame render callback. `"render"` and `"paint"` are accepted aliases. |
| `events.register("on_draw", fn)` | Alias of `events.listen`. |

Re-running the script replaces every previously registered callback.

```lua
events.listen("on_draw", function()
    local sw, sh = render.screen_size()
    local tw, th = render.measure_text("femware")
    render.text(sw - tw - 16, 16, "femware", render.color(255, 95, 175, 255))
end)
```