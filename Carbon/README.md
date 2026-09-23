## Requirements

- Visual Studio 2022 or Newer
- The **Desktop development with C++** workload
- The **C++ Clang tools for Windows** and **vcpkg** components
- An x64 processor with AVX2 support
- Internet access for the first dependency restore

FreeType is declared in `vcpkg.json` and is restored automatically by Visual Studio/MSBuild.

## Documentation

- [Lua Scripting API](cs2/FemWare/LUA_API.md) — the full `render`, `client`, `engine`, and `events` binding reference (also browsable in-game under Lua Studio → Documentation).
- [Feature Changelog](cs2/FemWare/CHANGELOG.md) — a 1:1 inventory of all features and subsystems.

## Build

Open a **Developer PowerShell for Visual Studio** at the repository root and run:

```powershell
msbuild cs2\FemWare\femware.vcxproj /m /p:Configuration=Ship /p:Platform=x64
```

The Ship build is written to `cs2\bin\cs2.dll`. Use `Configuration=Development` to produce `cs2\bin\femware-dev.dll` with development diagnostics.

If you use a standalone vcpkg installation instead of Visual Studio's bundled copy, set `VCPKG_ROOT` to its directory before building.
