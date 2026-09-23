# convar

| Signature | Returns | Description |
| --- | --- | --- |
| `convar.get(name)` | string \| number \| boolean \| nil | Current value of a registered console variable (typed). Nil for unknown or unsupported types. **Read-only** — scripts cannot execute console commands. |

```lua
local sens = convar.get("sensitivity")   -- number
local sv   = convar.get("sv_cheats")     -- boolean
local map  = convar.get("map")           -- string
```