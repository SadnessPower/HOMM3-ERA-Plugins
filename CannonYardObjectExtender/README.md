# CannonYardObjectExtender

Standalone Win32 ERA plugin project in the `CannonYardObjectExtender` folder. The project, plugin filename, registration name and C++ class remain `CannonYardObjectExtender`.

It registers the Cannon Yard as War Machine Factory, object type 106, subtype 1, through the RMG Object Extender API. It is not a Map Editor plugin.

## Build

Open `CannonYardObjectExtender.vcxproj` directly in Visual Studio with the v143 C++ toolset and a Windows 10 SDK installed. Select `Release | Win32` or `Debug | Win32`.

Alternatively, from a Visual Studio developer shell:

```powershell
msbuild .\CannonYardObjectExtender.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32
```

The Release plugin is produced at `Release\CannonYardObjectExtender.era`. Intermediate files are placed in `build\Win32\Release` (or the corresponding Debug directory).

All non-system build dependencies are included beside the project: `H3API.hpp`, `patcher_x86_commented.hpp`, `era.h`, `era.cpp` and `ObjectExtenderAPI.hpp`. No source files, libraries, property sheets or paths from the `H3ERA_Plugins_new` repository are required. SDK headers retain their original attribution notices.

## Runtime

The plugin still requires the normal game-side ERA/patcher services and `RMG_CustomizeObjectProperties.era` exposing its Object Extender API. Install `CannonYardObjectExtender.era` in the intended mod's `EraPlugins` folder. Cannon Yard generation settings and DEF/MSK assets remain separate mod resources; moving this source project does not rename or move those resources.
