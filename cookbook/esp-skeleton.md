# ESP skeleton

Draws a box and a health bar above every enemy.

```lua
events.listen("on_draw", function()
    for _, p in ipairs(client.get_players(true)) do
        if p.is_alive and p.screen.visible then
            local bx, by = p.screen.x - 15, p.screen.y - 40
            local bw, bh = 30, 40

            render.rect(bx, by, bw, bh, render.color(255, 255, 255, 220))
            render.rect_filled(bx - 4, by, 3, bh * (p.health / 100), render.color(74, 222, 128, 255))
        end
    end
end)
```