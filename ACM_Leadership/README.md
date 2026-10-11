# ACM Leadership

Standalone Win32 plugin for Advanced Classes Mod using H3API, ERA and Patcher_x86.
Build `ACM_Leadership.vcxproj` with Visual Studio/MSBuild v143, Release|Win32.
All SDK headers and ERA implementation are local: no dependency on H3ERA_Plugins_new.

## Behavior

- Basic/Advanced/Expert/Master/Grandmaster Leadership grants native morale +1/+2/+3/+4/+5.
- Expert/Master/Grandmaster reduces the army's effective faction count by 1/2/3, never below one for a nonempty army.
- Native specialty scaling, artifacts, terrain rules, morale clamping and undead penalties are retained.
- Native popup modifier rows and totals use the actual mastery. The localized explanation is placed directly below the native introduction, without an empty line, for Good/Neutral/Bad morale. Formatting uses current ERA translations and native localized strings, not hardcoded displayed sentences; unrelated text is preserved.
- Morale popups containing that localized Leadership explanation use the game's medium font to measure the visible translated sentence, excluding color tags. Vanilla adds borders, rounds the size, and controls height, centering and icon placement; larger native widths are retained. Long translations wrap at vanilla's normal 578-pixel text-width limit. Unrelated dialogs are unaffected.
- Master/Grandmaster's existing extra-attack damage and Luck scripts are unchanged.
- Ten translation keys under `AC_Misc` in `advanced_classes.json` hold new explanations, modifier rows and all five Leadership descriptions. ERM skill-description callers read those keys rather than the removed ERT entries.

## Compatibility

The alignment rule was reconstructed from daemon_n's `Combat_LeadershipDecreasesFactionsNum.dll`: subtract the skill-level-minus-two bonus and clamp the faction count to at least one.
That old DLL is disabled as `.dll.disabled` in ACM's EraPlugins folder (rename it back to recover it). As a defensive compatibility measure, this plugin also retires that exact DLL's patcher instance if another enabled mod supplies it; it does not rewrite captured executable bytes.

Angelic Alliance adjustments are derived from the installed native/new_towns morale calculation, not a duplicate list of town affiliations. A scoped description wrapper preserves the original alliance flag: vanilla reuses its stack slot for the morale result. Thus the extension does not rely on the misleading later value used by the existing new_towns description hook.

Old saves may already contain the former ERM temporary Leadership bonus. It cannot safely be distinguished from legitimate temporary morale bonuses, so the plugin does not blindly subtract existing saved bonuses. The old post-battle additions are removed for future battles.

## Hook sites

- 004E3C48: Leadership table load (LoHook).
- 0044AB30: native army morale, chained with other plugins (HiHook).
- 004DCBA6: actual mastery label and explained-morale accumulator (LoHook).
- 004DCCC7: localized alignment explanation in the native hero description (LoHook).
- 0044B630: preserve original description arguments with nested-call scope (HiHook).
- 0044B97A / 0044B9C8: native alignment-row branch decisions (LoHooks).
- 004F6C00: relocate only the new explanation in native morale dialogs (HiHook).
- 004F658A / 004F663F: minimum trial/final text width inside vanilla dialog sizing (LoHooks, scoped to matching Leadership morale text).

No fixed HD Mod DLL address or replacement dialog is used. Ordinary dialogs without the Leadership explanation are forwarded unchanged.

## Tests

`tests/LeadershipRulesTests.vcxproj` runs standalone rule/text tests. Synthetic text fixtures test formatting independently of any game's wording or translation.
`/p:AcmNativeTests=true` additionally enables game-thread native-function tests on OnAfterCreateWindow if `ACM_LEADERSHIP_TEST_LOG` names an output log. Normal Release builds omit that testing code.
