# Player tags

Labels every visible player with their name and a short tag.

```lua
events.listen("on_draw", function()
    for _, p in ipairs(client.get_players()) do
        if p.is_alive and p.screen.visible then
            local tag
            if p.is_local then
                tag = "you"
            elseif p.is_enemy then
                tag = "enemy " .. p.health
            else
                tag = "mate"
            end
            render.text(p.screen.x, p.screen.y - 48, p.name .. " (" .. tag .. ")", render.color(255, 255, 255, 255), true)
        end
    end
end)
```