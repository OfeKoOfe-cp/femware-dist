# entity

Player data mirrors the table shape produced by `client.get_players`; the two share the same
ordering and field set.

| Signature | Returns | Description |
| --- | --- | --- |
| `entity.get_local()` | table or nil | Same table as `client.get_local()`. |
| `entity.get_players()` | array | Same list as `client.get_players()`. |
| `entity.get_player(index)` | table or nil | A single player table by 1-based index into the player list. |
| `entity.read_int(address, class_name, field_name)` | int or nil, err | Reads a schema field as an integer. Offsets resolve at runtime, so it survives game updates; an unknown or renamed field returns `nil` plus an error. |
| `entity.read_float(address, class_name, field_name)` | number or nil, err | Reads a schema field as a float. |
| `entity.read_bool(address, class_name, field_name)` | bool or nil, err | Reads a schema field as a boolean. |
| `entity.read_vec(address, class_name, field_name)` | x, y, z or nil, err | Reads a schema field as a 3-component vector. |
| `entity.get_bone_pos(address, bone)` | x, y, z or nil, err | World position of a bone. `bone` is a name (`"head"`, `"neck"`, `"pelvis"`, `"left_foot"`, ...) or a raw bone id; an unknown bone returns `nil` plus an error. |

`address` is the `addr` field from a player table (`client.get_local()`, `client.get_players()`).

```lua
local enemy = entity.get_player(1)
if enemy and enemy.is_alive then
    local hp, err = entity.read_int(enemy.addr, "C_BaseEntity", "m_iHealth")
    if hp == nil then
        client.log("read failed: " .. tostring(err))
    else
        client.log(enemy.name .. " HP " .. hp)
    end
end
```