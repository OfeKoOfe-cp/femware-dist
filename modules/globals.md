# globals

Game state snapshots from the client's global variables. Each call reads live memory.

| Signature | Returns | Description |
| --- | --- | --- |
| `globals.get_curtime()` | seconds | Server curtime in seconds (`0` when not in a match). |
| `globals.get_tickcount()` | int | Current server tick (`0` when not in a match). |
| `globals.get_screen_size()` | w, h | Display viewport dimensions (alias of `render.screen_size`). |

```lua
local t = globals.get_tickcount()
client.log("tick " .. t)
```