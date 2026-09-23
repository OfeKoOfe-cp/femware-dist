# spawn

| Signature | Returns | Description |
| --- | --- | --- |
| `spawn(fn)` | — | Runs `fn` on a coroutine so it can suspend with `client.wait`. |
| `client.wait(ms)` | — | Suspends the running coroutine for `ms` milliseconds; only valid inside a `spawn`ed function. |

```lua
spawn(function()
    while true do
        client.log("tick")
        client.wait(1000)
    end
end)
```
