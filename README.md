# HOMM3 ERA Plugins

Six independent C++ plugin projects for Heroes of Might and Magic III ERA.

| Folder | Project | Release output |
| --- | --- | --- |
| [ACM_Leadership](ACM_Leadership) | `ACM_Leadership.vcxproj` | `Release/ACM_Leadership.dll` |
| [CannonYardObjectExtender](CannonYardObjectExtender) | `CannonYardObjectExtender.vcxproj` | `Release/CannonYardObjectExtender.era` |
| [CombatHintDialog](CombatHintDialog) | `CombatHintDialog.vcxproj` | `Release/CombatHintDialog.era` |
| [Hero Reserve Movement](<Hero Reserve Movement>) | `HeroReserveMovement.vcxproj` | `bin/Release/HeroMovementCalculationForReserveSlots.era` |
| [DisableDelays](DisableDelays) | `DisableDelays.vcxproj` | `Release/DisableDelays.era` |
| [HumanAI](HumanAI) | `HumanAI.vcxproj` | `Release/HumanAI.era` |

## Build and install

Open the individual `.vcxproj` or its existing `.sln` in Visual Studio and select `Release | Win32` (some solutions name Win32 `x86`). ERA is a 32-bit game; do not deploy x64 builds.

ACM Leadership and Cannon Yard use the v143 C++ toolset and Windows 10 SDK. The other four Release projects retain their existing v141_xp toolset and Windows 7 SDK requirements. The ERA, Patcher_x86 and H3API files needed by each project are included locally; SDK attribution notices are preserved. Runtime mod requirements remain unchanged.

Build outputs stay inside the source project: building does not automatically overwrite a game installation. Copy the resulting `.era` or `.dll` into the intended mod's `EraPlugins` directory after closing the game.

`ACM_Leadership/tests/LeadershipRulesTests.vcxproj` provides standalone Leadership tests. Its optional native tests are described in the project's README.

`CombatHintDialog/GenerateSkillsJson.ps1` accepts `-GameRoot` for the installed game directory, or explicit `-CreatureScript` and `-OutputPath` parameters. When this repository is directly inside the game root, its default game-directory lookup remains valid.

## Repository notes

Visual Studio caches, build outputs and user-specific debugger settings are kept locally but ignored by Git. Runtime plugin names and source behavior were not renamed.

DisableDelays previously contained its own Git metadata. That history was preserved outside this repository during consolidation; its existing source and attribution files remain here. The previous source remote was https://github.com/SadnessPower/Disable-AI-Turn-Delay_Source.git.

No new redistribution license is assigned here. Preserve existing source and SDK attribution and check the relevant permissions before publishing.
