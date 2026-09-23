# loadstring

| Signature | Returns | Description |
| --- | --- | --- |
| `loadstring(source_or_url, [chunk_name])` | func or nil, err | Compiles a script and returns its function. When the first argument is an `http(s)://` url it fetches the raw script first and compiles the fetched body. `chunk_name` is used in error messages. |

```lua
local fn, err = loadstring("https://raw.githubusercontent.com/user/repo/main/script.lua")
if fn then
    fn()
else
    client.log("load failed: " .. tostring(err))
end
```
