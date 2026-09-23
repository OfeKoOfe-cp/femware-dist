# entity

Player data mirrors the table shape produced by `client.get_players`; the two share the same
ordering and field set.

| Signature | Returns | Description |
| --- | --- | --- |
| `entity.get_local()` | table or nil | Same table as `client.get_local()`. |
| `entity.get_players()` | array | Same list as `client.get_players()`. |
| `entity.get_player(index)` | table or nil | A single player table by 1-based index into the player list. |
| `entity.read_int(address, class_name, field_name)` | int | Reads a schema field as an integer. Resolves the offset at runtime, so it survives game updates. |
| `entity.read_float(address, class_name, field_name)` | number | Reads a schema field as a float. |
| `entity.read_bool(address, class_name, field_name)` | bool | Reads a schema field as a boolean. |
| `entity.read_vec(address, class_name, field_name)` | x, y, z | Reads a schema field as a 3-component vector. |
| `entity.get_bone_pos(address, bone)` | x, y, z | World position of a bone. `bone` is a name (`"head"`, `"neck"`, `"pelvis"`, `"left_foot"`, ...) or a raw bone id. |

`address` is the `addr` field from a player table (`client.get_local()`, `client.get_players()`).

```lua
local enemy = entity.get_player(1)
if enemy and enemy.is_alive then
    local hp = entity.read_int(enemy.addr, "C_BaseEntity", "m_iHealth")
    local hx, hy, hz = entity.get_bone_pos(enemy.addr, "head")
    client.log(enemy.name .. " HP " .. hp .. " head " .. string.format("%.0f %.0f %.0f", hx, hy, hz))
end
```