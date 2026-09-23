# entity

Player data mirrors the table shape produced by `client.get_players`; the two share the same
ordering and field set.

| Signature | Returns | Description |
| --- | --- | --- |
| `entity.get_local()` | table or nil | Same table as `client.get_local()`. |
| `entity.get_players()` | array | Same list as `client.get_players()`. |
| `entity.get_player(index)` | table or nil | A single player table by 1-based index into the player list. |

```lua
local enemy = entity.get_player(1)
if enemy and enemy.is_alive then
    client.log(enemy.name .. " has " .. enemy.health .. " HP")
end
```