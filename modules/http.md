# http

| Signature | Returns | Description |
| --- | --- | --- |
| `http.get(url, [timeout_s])` | body or nil, err | Blocking `GET` of an `http(s)` url. Returns the response body, or `nil` plus the error. `timeout_s` defaults to 5. |

```lua
local body, err = http.get("https://raw.githubusercontent.com/user/repo/main/data.json")
if body then
    local data = json.decode(body)
    client.log(tostring(data.hello))
end
```
