# render

| Signature | Returns | Description |
| --- | --- | --- |
| `render.text(x, y, str, col, [centered])` | — | Draws text with the client font. `centered` centers it horizontally on `(x, y)`. |
| `render.text_outlined(x, y, str, col, [centered])` | — | Text with a crisp 1px outline. |
| `render.text_shadowed(x, y, str, col, [centered])` | — | Text with a soft drop shadow. |
| `render.line(x1, y1, x2, y2, col, [thick])` | — | Draws a 2D line with optional thickness. |
| `render.rect(x, y, w, h, col, [rounding], [thick])` | — | Outlined rectangle with optional corner rounding. |
| `render.rect_filled(x, y, w, h, col, [rounding])` | — | Filled rectangle with optional corner rounding. |
| `render.gradient_rect(x, y, w, h, col1, col2, [horizontal=true], [radius])` | — | Filled rectangle with a two-color linear gradient. |
| `render.circle(cx, cy, r, col, [thick])` | — | Outlined circle. |
| `render.circle_filled(cx, cy, r, col)` | — | Filled circle. |
| `render.polyline(points, col, [closed], [thick])` | — | Path through `{ {x,y}, ... }` or a flat `x,y,x,y,...` array. `[closed]` joins the last point to the first. |
| `render.triangle(x1,y1,x2,y2,x3,y3, col, [filled], [thick])` | — | Triangle; filled when `[filled]`, otherwise outlined. |
| `render.color(r, g, b, [a])` | col | Builds an RGBA color table. |
| `render.measure_text(str)` | w, h | Pixel width and height of a string. |
| `render.screen_size()` | w, h | Display viewport dimensions. |
| `render.delta_time()` | seconds | Time since the last rendered frame — for frame-independent animation. |
| `render.world_to_screen(x, y, z)` | sx, sy, vis | Projects world coordinates to screen space (accepts a `{x, y, z}` table). |

Every drawing function accepts either a color table from `render.color(...)` or a color table
built as `{r, g, b, [a]}`. Raw `r, g, b, [a]` arguments are also accepted.

```lua
local c1 = render.color(255, 95, 175, 255)
local c2 = render.color(140, 70, 240, 0)
render.gradient_rect(100, 100, 200, 30, c1, c2, true, 4.0)
```