# math

`math` is the standard Lua math library extended with convenience functions. Standard functions
like `floor`, `ceil`, `abs`, `sqrt`, `min`, `max`, `rad`, `deg`, `pi`, and `random` remain
available.

| Signature | Returns | Description |
| --- | --- | --- |
| `math.clamp(v, lo, hi)` | number | Clamps `v` into `[lo, hi]`. |
| `math.lerp(a, b, t)` | number | Linear interpolation: `a + (b - a) * t`. |
| `math.saturate(t)` | number | Clamps into `[0, 1]`. |
| `math.pingpong(t, len)` | number | Oscillates between `0` and `len` as `t` grows. |
| `math.normalize_yaw(yaw)` | number | Wraps a yaw into `(-180, 180]`. |
| `math.angle_diff(a, b)` | number | Shortest signed angular difference in degrees, `(-180, 180]`. |
| `math.distance(a, b)` | number | Distance between two `{x,y,z}` tables or two `(x,y,z)` coordinate sets. |
| `math.vector_to_angle(x, y, z)` | pitch, yaw | Converts a direction vector to angles in degrees. |
| `math.randomf(min, max)` | number | Uniform random float in `[min, max]`. |

```lua
local me = client.get_local()
if me and me.is_alive then
    local p = client.get_players()[1]
    if p then
        local d = math.distance(me.origin, p.origin)
        local ang_p, ang_y = math.vector_to_angle(
            p.origin.x - me.origin.x, p.origin.y - me.origin.y, p.origin.z - me.origin.z)
        print(string.format("player at %.0f units, yaw %.1f", d, ang_y))
    end
end
```