# Clarified phosphor comparison: 13ebff74 versus 35414271

The user's clarified pair is the **last accepted build before pins, 13ebff74**, and the **latest pin/live-cwd build at the complaint, 35414271**. Read-only inspection found concrete differences in their independent saved appearance settings. Those differences explain different picker shortcut values and provide different inputs to rendering. Their production phosphor bytecode and packaged defaults are identical. This is evidence about the compared copies, not an attribution of who changed settings or a claim that identical bytecode rules out every native visual issue.

This follows the [initial audit](PHOSPHOR-REGRESSION-AUDIT-960a9f9.md), whose missing-build-identity conclusion is historical. CuratedDefaults **f86a758f** was delivered afterward and is excluded from this comparison. No user file was copied, reset or edited.

## Exact copies and module provenance

* Before pins: `artifacts/portable/Backslash-Dev-20261014-171049-545-Release-x64-13ebff74/WindowsTerminal.exe`, observed PID **33484**.
* Latest pins/live-cwd: `artifacts/portable/Backslash-Dev-20261015-101041-982-Release-x64-35414271/WindowsTerminal.exe`, observed PID **60184**.

Both were running during inspection, both have `.portable`, and each loaded its own adjacent Control and Settings.Model DLLs. The old Control SHA256 is `B1C4D4C1EFC0B5A7D6B312861EC3855349949A478C8DB6CA8F4E9A9187AB31B9`; the new one is `8DBDFA434AB60C356F3CE676EEA24B5EA629ECF5BA65922C7ECA56BD75C606EC`. No mixed module paths were found. The Backslash fallback settings file at `%LOCALAPPDATA%/Backslash/Terminal/settings.json` was absent. Portable settings resolve beside the model DLL, giving these copies independent `settings/settings.json` files. The older `D:/nemo-workspace/...8a1fbd77` copy is not this pair.

The saved-value snapshot is **2026-10-16T00:10:24.665059+00:00**. Both files remained stable during that read. Settings SHA256:

* Before pins: `FBD23903C75AE90B559C64E1A4A2CDCC41F9C4F65AF0E556F1CDCAD022A4A457`.
* Latest pins/live-cwd: `B53E5B78D57595FED1F99C06656238F237435F289A8C7F4D10625C8CF8F20BCB`.

## Saved appearance inputs

Both copies select PowerShell 7 GUID `{574e775e-4f2a-5b96-ac1e-a2962a402336}` as their startup default profile. This identifies a comparable saved profile; it does not establish the currently selected pane or Settings page.

| Scope/value | Before pins: 13ebff74 | Latest pins/live-cwd: 35414271 |
| --- | --- | --- |
| `profiles.defaults` scheme | Explicit **IBM 5153** | **Campbell** through model fallback |
| PowerShell 7 effective scheme | Profile override **VSCode Dark Modern** | **Campbell** |
| PowerShell 7 opacity | **86%** | **100%** |
| PowerShell 7 retro effects | On | On |
| PowerShell 7 glow | **29%** | **17%** |
| PowerShell 7 scanline intensity | **24%** | **9%** |
| PowerShell 7 thickness / gap | 4 / 4 DIP | 4 / 4 DIP |
| PowerShell 7 phosphor color / strength | **#00FF41 / 100%** | **#00FF41 / 100%** |
| Shared profile `{0f2058f2-5fc7-5bbb-9580-b3bdd4295cd0}` scheme | One Half Dark | One Half Dark |
| That shared profile's saved phosphor / strength | **#0DBC79 / 89%** | **#61D6D6 / 100%** |
| That shared profile's effective glow / scanlines / opacity | 2% / 16% / 86% | 17% / 9% / 100% |

For the PowerShell 7 profile, the saved focused and unfocused effective values match within each copy; neither has a saved custom shader/background image in this snapshot. Other GUIDs that are present in only one file were not compared as the same profile.

## Why the picker chips differ

The phosphor color picker gets its 16 shortcut chips from `Appearance.CurrentColorScheme` via `NullableColorPicker.ColorSchemeVM` and `ColorEntryAt(0..15)`. See [Appearances.xaml](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml) around line 563, [CurrentColorScheme](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.cpp) around line 1053, and [NullableColorPicker.xaml](../../backslash/src/cascadia/TerminalSettingsEditor/NullableColorPicker.xaml) around line 44. There is no separate fixed named phosphor-preset catalog. The saved independent phosphor RGB and the shortcut palette have different ownership.

Consequently, opening Defaults in the old copy gives IBM 5153 shortcuts, while its PowerShell 7 override gives VSCode Dark Modern shortcuts. The new saved configuration gives Campbell in both contexts. Exact observed literals:

| ANSI shortcut | IBM 5153 | VSCode Dark Modern | Campbell |
| --- | --- | --- | --- |
| black | #000000 | #000000 | #0C0C0C |
| red | #AA0000 | #CD3131 | #C50F1F |
| green | #00AA00 | #0DBC79 | #13A10E |
| yellow | #C47E00 | #E5E510 | #C19C00 |
| blue | #0000AA | #2472C8 | #0037DA |
| purple | #AA00AA | #BC3FBC | #881798 |
| cyan | #00AAAA | #11A8CD | #3A96DD |
| white | #AAAAAA | #E5E5E5 | #CCCCCC |
| brightBlack | #555555 | #666666 | #767676 |
| brightRed | #FF5555 | #F14C4C | #E74856 |
| brightGreen | #55FF55 | #23D18B | #16C60C |
| brightYellow | #FFFF55 | #F5F543 | #F9F1A5 |
| brightBlue | #5555FF | #3B8EEA | #3B78FF |
| brightPurple | #FF55FF | #D670D6 | #B4009E |
| brightCyan | #55FFFF | #29B8DB | #61D6D6 |
| brightWhite | #FFFFFF | #E5E5E5 | #F2F2F2 |

The different saved schemes directly account for different shortcut RGBs through this binding. Glow, scanline strength, opacity, scheme source colors and the explicit shared-profile tint also differ as rendering inputs. Native composited output was not captured in this follow-up, so this evidence does not assign all perceived output differences to one field.

## Shader comparison and limits

The accepted ZIP and latest pin/live-cwd ZIP contain the same four complete DXBC containers. Exact phosphor bytecode SHA256 is `EA9498337F61E5BE3B63C6F17F3FF5BC00554BF68AE8ABCA1B8816316595DE03`; packaged `defaults.json` SHA256 is `5ABE4FEFAF0D0D1CB160924BA55B26677E9C5C41EEC4B0AD3D2C57A0E3CCF504`. The earlier hue/opacity correction already existed in the accepted build. No shader, phosphor color, palette, default or settings migration is changed by this audit.

The earlier offscreen validation passed 2,000 shader RGB/strength/alpha cases, ten white endpoints, 170 monotonic gray levels and the actual-backend phosphor/opacity test. These remain useful regression evidence, but do not prove native hardware/compositor behavior. Unobserved state includes the actual selected pane/profile, current focus and unsaved Settings edits. If a discrepancy remains with equivalent inputs, those contexts and a user-controlled side-by-side visual are the next narrow evidence; the build IDs are now resolved.

Evidence: `artifacts/phosphor-regression-audit-20261015-960a9f9/clarified-pair-effective-appearances.json`, `active-process-modules.json`, `copy-resolution.json`, and the initial `exact-phosphor-bytecode-match.json`. The filtered snapshot omits profile commands, hostnames and credentials. Running copies/processes and their settings were left alone. No new build, branding UI, SSH/WSL change, commit or push was made.
