# Configurable CRT Appearance

[Project entry point](README.md) · [Roadmap](ROADMAP.md)

## Requested product scope

Provide an optional **per-profile Appearance section** with sliders/fields and live preview. The user requests richer CRT controls inspired by [cool-retro-term](https://github.com/Swordfish90/cool-retro-term), configurable intentional glitch/effects, an old-screen overlay/frame, and actual terminal-view curvature/warping. Place this beside background image and transparency settings so their interaction is understandable.

This is a substantial requested feature, not merely a theme preset or the existing simple retro toggle. The first implementation slice exposes the existing built-in glow and scanline strengths; the other proposed controls remain unimplemented.

## Proposed controls to evaluate

| Control | Proposed purpose | Design/engineering question |
| --- | --- | --- |
| Curvature | Warp the terminal view like a curved screen | Inverse coordinate mapping, clipping, cursor and selection alignment |
| Scanlines | Adjustable line pattern/strength | DPI, font readability and moiré |
| Glow/bloom | Phosphor glow around bright content | Existing sampled glow parameters, contrast and performance; extra passes only if quality requires |
| Phosphor persistence | Decaying afterimage | History buffers, lifecycle and memory limits |
| Flicker | Controlled temporal variation | Reduced motion and bounded frequency |
| Noise/glitch intensity | Deliberate visual distortion | Stable hit-testing; never induce real UI instability |
| Vignette | Darken screen edges | Readability and contrast |
| Bezel/overlay | Old-screen frame or texture treatment | UI layering, content area and asset rights |

The exact controls, ranges, algorithms and preset set remain open. Requested outcomes must guide evaluation; this table is not an accepted implementation API.

## Reuse and architectural limits

The baseline already exposes `experimental.retroTerminalEffect`, `experimental.pixelShaderPath` and `experimental.pixelShaderImagePath`. [Microsoft's appearance documentation](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-appearance) describes the toggle and shader override. Reuse those hooks where suitable; verify precedence and migration for the new section.

Do not promise everything fits one stateless shader. Persistence may require history buffers and time-aware decay. Simple sampled glow already exists in one pass; higher-quality bloom may justify additional passes. Overlays may need composition/UI work. Curvature requires coordinated rendering and inverse mouse hit-testing, selection and cursor placement; merely warping the picture is insufficient. Decide how inline images, scrollbars, background images, transparency and previews participate.

cool-retro-term is visual inspiration. Its Qt/QML and GPL components require a component-level license compatibility review before copying any code/assets into this MIT-based source tree. No code has been copied and no licensing conclusion has been made.

## Required quality bar

- Neutral zero-effect default/preset; disabling effects returns an undistorted readable view.
- Text stays readable; controls have keyboard access, labels and accessible values.
- Reduced-motion/disable-effects path removes temporal effects.
- Bound GPU load, animation rate and memory; avoid continuous redraw when unnecessary.
- No resize jitter, flickering geometry or stale history buffers.
- Cursor, selection and mouse targets align with curved content, including edge cells.
- Save controls per profile; changes preview promptly without modifying other profiles.
- Fast reset and preset comparisons with clear save/cancel behavior.
- Intentional visual glitch is a controlled optional effect. Crashes, input lag, unstable sizing and accidental rendering corruption are defects, not aesthetic behavior.

Before implementation, specify performance thresholds and supported GPU/DPI/resize scenarios. Prototype curvature and hit-testing together; prototype persistence separately. A static shader preview alone cannot demonstrate the quality bar.


## Concrete first slice: parameterize existing retro effects

Reuse [Retro.hlsl](../../samples/PixelShaders/Retro.hlsl) and [production custom_shader_ps.hlsl](../../src/renderer/atlas/custom_shader_ps.hlsl): Gaussian2D/Blur already sample a 13×13 neighborhood, main adds glow at hardcoded gain 0.3, sigma is 2×scale and Scanline/SquareWave use hardcoded strength 0.5 with DPI-scaled period. Do not invent glow from scratch.

The first proposed vertical slice exposes selected existing glow/scanline parameters through profile controls, saved settings, live preview, neutral/reset comparison and measured GPU/readability/DPI/resize checks. “Persistence” in this slice means saved control values; phosphor afterimage history remains separate later rendering work. Preserve shader ABI/override precedence and verify zero strength disables each effect cleanly.

Production [BackendD3D.cpp](../../src/renderer/atlas/BackendD3D.cpp) provides _recreateCustomShader/_executeCustomShader/RequiresContinuousRedraw. Time use already participates in redraw detection; sample README's elapsed-since-enable description differs from this runtime's performance-counter modulo 1000 seconds. Verify animation wrap, reflection failures, reduced motion and idle behavior before claiming scheduling solved.

Animate_scan/Animate_breathe are band/fade animation examples, not frame-history persistence. BackgroundImage's t0/t1 alpha composition is not a full bezel, inline image protocol or curvature/input mapping. Nop/Invert/Grayscale are comparison fixtures (Grayscale uses arithmetic RGB average); Broken/Error are diagnostics, not a guaranteed automatic fallback. See [complete sample map](BASELINE.md) for precise paths and limits. The glow/scanline implementation and focused evidence are recorded below; the other effects remain proposals.


## Implemented glow and scanline slice (2026-10-13)

Controls: **Settings → Profiles → select a profile → Appearance**, immediately below the existing **Retro terminal effects** toggle in the Text group. Glow and scanline sliders use 0–100% with a percentage label and the standard inherited-value/reset control. The toggle remains off by default. Turning it off preserves both saved intensities. Both focused and unfocused appearance support the JSON settings and reset/inheritance path.

| JSON setting | Default | Range |
| --- | --- | --- |
| `experimental.retroTerminalGlowIntensity` | `0.3` | `0..1` |
| `experimental.retroTerminalScanlineIntensity` | `0.5` | `0..1` |

Finite out-of-range values clamp in both JSON loading and projected setters. Non-numeric JSON and non-finite values are rejected; failed validation retains the previous value. Explicit zero is a local override and round-trips. Null/reset clears the override and restores inheritance. The separate intensity macro participates in copy and JSON serialization, without adding intensity keys to appearance settings telemetry.

The production shader is `src/renderer/atlas/custom_shader_ps.hlsl`; sample shaders are unchanged. Default enabled effects retain the original gains and Gaussian/DPI behavior. Zero glow skips the 169-sample blur; zero scanline skips modulation. Both zero retain the shader pass and its base texture sample, but bypass effect work. There is no new continuous redraw request. Existing custom time/reflection behavior remains. A custom shader path still takes precedence, including compilation failure: it does not fall back to the built-in retro shader.

External shader **b0 remains 32 bytes**, with time/scale/resolution/background at offsets 0/4/8/16. Built-in intensities use separate **b1, 16 bytes**, with glow/scanline/padding at offsets 0/4/8. C++ size assertions and compiled reflection tests cover these boundaries. Parameter updates change the outer renderer settings generation, invalidate the frame, and upload b1 without changing misc shader-selection generation or recreating/compiling the shader.

Default-appearance projected-property changes use the existing 100 ms debounced preview refresh. **Unfocused preview is not implemented**: the preview subscription and `CreateForPreview` use the default appearance. Unfocused values propagate through the application adapter and ControlCore update path for actual terminal focus changes; focused/unfocused propagation and reset are covered by model/adapter tests, not an interactive focus test.

### Verification and deliverable

- Final build evidence: `artifacts/build/20261013-144052-896-Release-x64-ed9f7260`. `Build.ps1`, Release/x64: **0 errors, 135 warnings**, 2m55s. Warning count matches the preserved identity baseline; existing PRI263/packaging/link warnings remain.
- Focused settings DLL build succeeded. Direct TAEF `/name:*Backslash* /InProc`: **4 passed, 0 failed/blocked/skipped/not run** (`settings-focused-tests-filtered.log`). The three new tests cover defaults, explicit-zero/custom-path round-trip, local copy, inheritance/override source, clear/null reset, off/on value preservation, telemetry exclusion, finite clamping, nonnumeric/nonfinite rejection and retaining values after rejection, plus focused/unfocused adapter propagation. The fourth is the existing disabled-external-fragments identity test.
- Compiled shader ABI checks and WARP pixel-output tests passed in `artifacts/build/20261013-143844-146-Release-x64-ed0a8dd0/retro-shader-test.log` (same shader source, before restoration of original line endings). Reflection confirms 32/16-byte buffers; pixel cases verify both-zero neutrality, scanline 0.5/1 output, zero scanline neutrality, default gains, linear glow scaling and reset using the same shader object. Reproduce with `tools/backslash/Test-RetroShader.ps1 -Runtime -EvidenceDirectory <fresh-existing-directory>` using the installed SDK/C++ toolchain.
- Fresh portable assembly and `Test-PortableIsolation.ps1` passed, preserving the existing identity checks and required files/notices. Runnable folder: `artifacts/portable/Backslash-Dev-20261013-144052-896-Release-x64-ed9f7260`; run its `WindowsTerminal.exe` by full path.
- Hidden smoke launch loaded the preseeded isolated CRT profile and executed its PowerShell test shell. **Two existing Windows Terminal settings/state files retained identical SHA256 hashes**; only the process whose executable path matched the fresh artifact was stopped. Settings/shell smoke data was then moved to the build evidence directory, leaving the delivery folder clean. No adjacent state file was observed during this short smoke. See `crt-portable-smoke.json`; no UI was inspected.
- Clean ZIP: `artifacts/portable/Backslash-Dev-0.0.1.0-CRT-x64-ed9f7260.zip`, **258 entries**, `.portable` included, smoke settings excluded. SHA256: `186524A89ADD98BCCAD492B6AD558CEC51242C8C4E99955B3B01DA443C996F78`.
- The preserved identity baseline ZIP still hashes to `E8D9B2F3456B5EA4BF268B53EAFD5575FA83AE47AB112E959C4765CF12FB899F`. The CRT-only diff passed whitespace checking with `core.whitespace=cr-at-eol`, preserving original source encodings/line endings.
- Retained iteration evidence: an initial adapter test compile used dot access on a `com_ptr` and was corrected. A composite TAEF filter was misparsed and started an unfiltered run, which aborted at `ProfileTests::DuplicateProfileTest` with exit `0xC0000409`; its host-layout cause was diagnosed in the follow-up below; full-suite coverage is not claimed. The corrected name-filtered run passed all four requested tests. Earlier build attempts and evidence remain preserved.

### Source scope

- Model: `MTSMSettings.h`, `IAppearanceConfig.idl`, `AppearanceConfig.h/.cpp`, `CascadiaSettings.cpp` profile duplication; schema: `doc/cascadia/profiles.schema.json` (profile and unfocused).
- Adapter/control: `IControlAppearance.idl`, `ControlProperties.h`, `TerminalSettingsAppAdapterLib/TerminalSettings.cpp`, `ControlCore.cpp`.
- Editor: `Appearances.idl/.h/.cpp/.xaml`, English `Resources.resw` labels and accessible slider names. Existing resource fallback supplies English; new translations are not provided.
- Renderer: `common.h`, `AtlasEngine.h/.api.cpp`, `BackendD3D.h/.cpp`, `custom_shader_ps.hlsl`.
- Tests: `SerializationTests.cpp`, `TerminalSettingsTests.cpp`, extended `ProfileTests.cpp`, new `tools/backslash/RetroShaderTests.cpp` and `Test-RetroShader.ps1`.

No new curvature, bezel, history/persistence, flicker, glitch, vignette, preset or reduced-motion controls are implemented. Hardware-GPU performance, DPI/resize/readability, visual UI, keyboard/reset interaction, live slider preview, interactive focus switching, idle redraw measurement and custom compilation failure runtime checks remain unrun. WARP pixel tests establish shader output behavior, not performance or interactive UI readiness. No installation, registration, certificate trust, system integration, commit or push was performed. Existing identity changes, baseline portable artifacts and `BEZEL-DESIGN.md` were preserved.


## Validation follow-up: DuplicateProfileTest abort

The same pre-fix test DLL reproduced `0xC0000409` when hosted by `packages/Microsoft.Taef.10.100.251104001/build/Binaries/x64/TE.exe /InProc`, and passed `DuplicateProfileTest` individually when hosted by `bin/x64/Release/UnitTests_SettingsModel/TE.exe /InProc`. Both host executables have identical SHA256 (`B729C665A51A1E36DC24152ECDE4E4A7EFB618B75DC7387D8C6EDB103EF952C1`). The package-host folder has no `resources.pri`; the local-host folder does. This isolates the abort to test hosting/resource setup without changing identity or CRT code, so a separate baseline checkout was unnecessary.

`build/scripts/Run-Tests.ps1:31` explicitly requires the host executable beside each test DLL's `resources.pri`. `DuplicateProfile` first obtains the localized `CopySuffix` through `RS_`; `ScopedResourceLoader` uses `ResourceManager::Current().MainResourceMap()`, and `LibraryResources.cpp` fail-fasts on resource initialization/string lookup exceptions. The observed host-layout failure matches this path. No debugger was installed or used to recover the underlying resource HRESULT.

Diagnostic evidence is retained under `artifacts/build/20261013-144052-896-Release-x64-ed9f7260`: `duplicate-profile-individual.log`, `duplicate-profile-local-host.log`, and `duplicate-profile-host-comparison.json`. Use the local host for resource-dependent tests; changing only the working directory is not the repository's required host layout.

Inspection also found a separate CRT regression: the separate intensity macro had not been included in `CascadiaSettings::DuplicateProfile`, so duplicating a profile omitted its local intensity overrides. This is fixed by applying `MTSM_RETRO_INTENSITY_SETTINGS` to the existing duplication macro, retaining its default-origin rules. `DuplicateProfileTest` now sets glow to explicit zero and scanline to 0.75 and verifies both copied values and complete profile JSON equality.

Latest corrected-build evidence: **`artifacts/build/20261013-145014-247-Release-x64-8a1fbd77`**.

- Release/x64 build: **0 errors, 135 warnings**, 36.72 seconds.
- Test DLL build: succeeded. Local TAEF host: **all 10 ProfileTests passed**, plus **all 4 Backslash CRT/identity tests passed**, with no failures/blocked/skipped/not-run tests in those selections. See `profile-tests-local-host.log` and `settings-focused-tests-local-host.log`. The entire model suite was not run.
- Fresh assembly, static portable identity checks, and hidden isolated smoke passed. The profile shell ran; two existing Terminal settings/state hashes stayed unchanged; the owned app process was stopped. No adjacent state file was observed during the short smoke. Smoke settings and marker were retained under build evidence and removed from the delivery folder by a checked move.
- Latest clean ZIP: `artifacts/portable/Backslash-Dev-0.0.1.0-CRT-x64-8a1fbd77.zip`; SHA256 **`AA3F2CE07BBC5B416531517E0E0602B0300BEA118A5E23AF6780A008E29A324A`**. Earlier deliveries remain preserved; this one supersedes the initial CRT ZIP.
- Extracted runnable executable: **`D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-145014-247-Release-x64-8a1fbd77\WindowsTerminal.exe`**. The delivery folder is clean and contains `.portable`; its next launch will create adjacent settings.

### Manual visual checklist — all unrun

Use the latest clean portable folder or extract its ZIP into a new directory. Launch its executable by full path. Keep `.portable` beside it and edit only that copy's adjacent settings.

1. Open Settings → Profiles → a test profile → Appearance. In the Text group, find the retro toggle and glow/scanline sliders directly below it. Verify readable labels and 30%/50% initial values; toggle is off by default.
2. Enable retro effects with no custom shader path. Verify glow and alternating scanlines in both the preview and the saved profile's actual terminal. Move each slider and check that the default-appearance preview updates after its brief debounce.
3. Set glow to 0% and scanlines to 50%: scanlines remain, blur/glow disappears. Set glow to 30% and scanlines to 0%: glow remains, line modulation disappears. Set both to 0%: compare readable neutral output with the retro toggle off.
4. Save explicit zero values, close/reopen Settings and relaunch the portable copy. Verify zero remains saved. Turn the retro toggle off/on and verify saved intensities are preserved. Cancel a separate unsaved change and verify it is not retained.
5. In profile defaults set glow 55% and scanlines 25%. On the test profile clear both overrides: verify inheritance indicators and 55%/25% values. Override glow with 0%, then use its reset control and verify it returns to inherited 55%.
6. Tab to both sliders and use arrow/Home/End keys. Verify percentage labels and accessible names, plus keyboard access to each reset control.
7. Duplicate the test profile after explicitly setting glow 0% and scanlines 75%. Verify the duplicate retains both values after save/reload and can reset them independently.
8. Add an unfocused appearance with glow/scanlines 0% while the focused appearance has visible effects. Save, focus the actual terminal, then another window: verify actual terminal focus transitions apply the values. **Do not use the preview to judge unfocused appearance; that preview is not implemented.**
9. In this copy's `settings/settings.json`, set `experimental.pixelShaderPath` to `D:\codex-workspace\backslash-terminal\samples\PixelShaders\Nop.hlsl` for the test profile. With retro enabled, changing intensities must not add built-in effects. Then use a nonexistent shader path to trigger compile/load failure: verify it does not activate built-in retro effects. Restore an empty shader path afterward.
10. Check text, cursor, selection, background image/transparency and scrolling at 0/default/full intensity. Resize the window and test available DPI scales (e.g. 100%, 125%, 150%, 200%): look for stale bands, redraw artifacts or unreadable text. This is visual coverage only; hardware GPU timing and idle redraw measurements remain separate unrun checks.

No remaining implementation/build blocker was identified. Native UI automation is unavailable in this session, so every checklist item above remains for manual validation. The earlier WARP evidence remains applicable: this follow-up changes model duplication and its test, not shader/runtime rendering code.
