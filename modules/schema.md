# schema

Read-only access to the Source 2 schema table. Used for diagnostics.

| Signature | Returns | Description |
| --- | --- | --- |
| `schema.get(class_name, field_name)` | int or nil | Byte offset of a schema field within a class, or nil when the class or field is not present. |

```lua
local health_off = schema.get("C_BaseEntity", "m_iHealth")
if health_off then
    client.log("m_iHealth is at +" .. health_off)
end
```