# json

| Signature | Returns | Description |
| --- | --- | --- |
| `json.encode(value)` | string | Serializes a Lua value (table, string, number, boolean) to JSON. |
| `json.decode(text)` | value or nil | Parses JSON into a Lua value; `nil` on invalid input. |

```lua
local text = json.encode({ hp = 100, items = { "knife", "awp" } })
local data = json.decode(text)
client.log(data.items[2])
```
