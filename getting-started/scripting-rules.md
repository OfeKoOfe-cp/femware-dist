# Scripting rules

- Re-running a script replaces every previously registered callback.
- A callback that raises errors repeatedly is removed automatically; re-run the script to
  re-enable it.
- World positions are meters; angles are degrees.
- Colors are accepted by every drawing function in either form:

```lua
render.rect_filled(10, 10, 100, 40, 255, 95, 175, 255)                 -- r, g, b, [a]
render.rect_filled(10, 10, 100, 40, render.color(255, 95, 175, 255))    -- color table
```

A color table may also use array indices: `{255, 95, 175, 255}`.