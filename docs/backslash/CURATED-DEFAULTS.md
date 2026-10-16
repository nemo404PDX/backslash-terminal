# Curated Backslash defaults

Implemented at the user's request, validated **2026-10-16 UTC**, uncommitted on master atop `960a9f976f08cd7d4c8dcff263c61b71afedb101`. These are fallbacks and inbox defaults; they do not overwrite an existing portable's explicit settings or force-enable effects. [Historical phosphor](PHOSPHOR-COLOR-REFERENCE-dc027cae.md) and [bezel](BEZEL-COLOR-REFERENCE-dc027cae.md) backups retain their original dc027cae values. The preceding [color audit](PHOSPHOR-REGRESSION-AUDIT-960a9f9.md) remains anchored before this separately authorized defaults change.

| Setting | New default | Notes |
| --- | --- | --- |
| Initial terminal grid | **80 columns x 28 rows** | Explicit CLI/settings/restored size stays authoritative |
| Window placement | **Center on launch** | Existing native nearest-monitor work-area centering; explicit placement wins |
| Font size | **16 points** | Font family unchanged; no font installation |
| Cursor | **Filled box** (`filledBox`) | Explicit appearance overrides remain |
| Scrollbar | **Hidden** | `scrollbarState: hidden` |
| Dark scanline thickness | **4 DIP** | Valid range unchanged |
| Clear scanline gap | **4 DIP** | Valid range unchanged |
| Retro glow intensity | **15%** (`0.15`) | Used only when built-in retro effects are enabled |
| Scanline intensity | **10%** (`0.10`) | Used only when built-in retro effects are enabled |
| Retro effects toggle | **Off** | Kept off by default |
| Bezel | **Off** | Kept off by default |
| Bezel branding badge | **Omitted** (`none`) | Used if bezel is later enabled |
| Center aperture in fullscreen | **Off** | Does not disable ordinary window centering |

Phosphor default **#00FF41**, tint intensity **0**, every existing bezel RGB/gradient/badge color, ANSI schemes, and the hue/opacity shader calculation are unchanged. Glow/scanline strengths and geometry are intentional default changes requested in this slice; they can change rendered output when effects are enabled without changing any color literal.

## Implementation and precedence

`MTSMSettings.h`, `ControlProperties.h` and renderer initial constants agree on the requested fallbacks. The inbox `defaults.json` has the new global grid/centering and built-in profile appearance values; the build compresses it into resources. Font family strings are unchanged. Schema default annotations agree, including focused/unfocused CRT values and legacy font-size metadata. No user settings migration/reset is added.

`TerminalWindow::CenterOnLaunch` delegates eligibility to the small tested `LaunchPositionPolicy`: a configured `initialPosition` (including a partial/zero coordinate), command-line `--pos`, restored position or tab tear-out bounds suppresses default centering. An ordinary launch without AppArgs can center. AppHost's existing monitor selection, DPI/size calculation, taskbar-aware work area and native centering math remain unchanged. Special quake/fullscreen/maximized launch handling retains its existing paths.

Known existing copy inspected read-only: `D:/nemo-workspace/Backslash-Dev-0.0.1.0-CRT-x64-8a1fbd77`. It explicitly saves **JetBrains Mono 17**, **glow 25%**, **scanline intensity 85%**, and **retro effects on** in `profiles.defaults`. Those values continue to win if that settings file is used with a new build. Its filled-box cursor and hidden scrollbar already match the request. It has no explicit global grid/centering override in the inspected file. This does not establish that directory as the user's currently compared build, and it is not evidence of a reset. Settings/state hashes stayed unchanged. Nothing in that copy, an installed Terminal or a running installation was replaced.

To use new defaults in an existing configuration, the user can explicitly reset only the relevant overrides in Settings. This task performs no such reset. A freshly extracted independent portable folder receives the curated defaults without copying another folder's settings.

## Verification

* Release/x64 build **0 errors / 134 warnings**, **1:34.57**; build `20261015-165707-949-Release-x64-f86a758f`.
* **233/233 Settings tests passed**, including three new packaged-default/adapter, explicit override serialization/inheritance and native-centering precedence cases. Tests load inbox resources without touching user state or contacting SSH/WSL.
* **34/34 Control tests passed**, including the actual phosphor/opacity WARP probe and prior CRT/resource regressions. Geometry fixtures explicitly retain their original 12-point test grid; the real settings adapter is independently checked for 16-point product defaults.
* **3/3 owned native launch scenarios passed**: normal centered launch, settings coordinates, and CLI coordinates overriding settings. Every resulting console measured **80x28**; all sessions exited naturally (**0 forced stops**). Win32/DWM bounds were read only for the owned process. The centered window measured `(304,156)-(1348,911)` against work area `(0,0)-(1653,1067)` in the probe's coordinate space, within a 2-unit center tolerance. Explicit-placement checks used DWM visible frame bounds. No screenshot/color interpretation or user-window manipulation is claimed.
* Portable isolation and **9/9 owned pin runtime regressions passed**, **0 forced stops**. Packaged default JSON was checked directly. The new DLL still contains the accepted exact phosphor bytecode SHA256 `EA9498337F61E5BE3B63C6F17F3FF5BC00554BF68AE8ABCA1B8816316595DE03`.
* Frozen references' JSON hashes, 9 C++-derived bezel materials, 16 inbox schemes and SVG XML were verified. Historical values were not rewritten to the new defaults.

Native Settings/appearance clicks, screenshot color acceptance, multi-monitor hardware combinations and DPI/accessibility inspection remain unrun. Restore/tear-out precedence has unit-policy coverage; the three live cases do not claim exhaustive native restore/tear-out validation.

## New isolated delivery

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-165707-949-Release-x64-f86a758f/WindowsTerminal.exe`.

Complete clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-CuratedDefaults-x64-f86a758f.zip`. SHA256: **`ABBC0B1A66E0675817B9947144C168076D6DFFE6033040C23DA9D1DDAF096DD4`**. Extract the entire ZIP into a new folder; retain `.portable`. No test settings are included. Previous ZIPs and source checkpoints remain preserved.

Evidence: `artifacts/build/20261015-165707-949-Release-x64-f86a758f/curated-validation`, `curated-window-smoke/window-placement-results.json`, `pin-portable-smoke.json`, `portable-isolation.log`, `source-checkpoint.zip`, `source-inventory.json` and `curated-defaults-delivery-summary.json`. The separate regression-audit evidence remains under `artifacts/phosphor-regression-audit-20261015-960a9f9`.

Acceptance checklist: launch the new copy with no settings, confirm centered 80x28/font 16/filled box/hidden scrollbar; verify retro and bezel start off; enable retro and inspect 4/4 DIP bands with 15% glow/10% scanlines; enable bezel and confirm badge omitted/center aperture off; confirm explicit font/strength/placement overrides and CLI position still win. No commit/push, install/registration, new palette or user-settings reset occurred.

## Bezel sizing acceptance (2026-10-16)

The user confirmed that **80 columns x 28 rows is the usable terminal area**, with bezel space added around it. They tested the thickest bezel and accepted thinner options. Keep 80x28 unchanged; no further sizing work is needed.

An additional owned native launch check of a fresh ZIP extraction measured **80x28 with bezel off and on**. Medium/badge-none increased the outer window from **1044x755 to 1204x915** (+160 width/+160 height in the Win32 probe coordinate space), preserving the center. Both sessions exited naturally, zero forced stops. This verifies initial ordinary-window sizing; it does not claim automatic outer resizing on a live toggle. Exact bounds and source reasoning are recorded in [the accepted invariant](BEZEL-DESIGN.md#accepted-usable-grid-invariant-2026-10-16), with evidence in `bezel-grid-smoke/bezel-grid-results.json` under this build run. The package, product source, colors and defaults were not changed by this check.

## Accepted live-toggle distinction (2026-10-16 follow-up)

The user explicitly accepted leaving existing live behavior unchanged: toggling a bezel keeps outer window bounds fixed and reallocates usable rows/columns; launch/reopen reserves configured usable 80x28 cells. This is distinct from the initial-launch invariant above. No automatic window resizing is added. The separate [badge implementation and owned live-toggle observation](BEZEL-BADGE-IMPLEMENTATION.md#accepted-live-toggle-behavior-and-manual-checks) records measurements and remaining visual checks. Curated defaults and the previously delivered ZIP remain unchanged.
