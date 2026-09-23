# Callbacks

The script body executes once when you click **Run**. Everything that must happen every frame is
done from a callback registered with `events.listen`.

```lua
events.listen("on_draw", function()
    -- runs once per rendered frame
end)
```

`events.listen` and `events.register` are aliases. The event name accepts `"render"` and
`"paint"` as aliases for `"on_draw"`.

Re-running the script replaces every previously registered callback — there is no way to keep
stateful handlers across runs, so register what you need each time.