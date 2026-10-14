# CRT rendering plan and performance audit

[Project entry point](README.md) | [Implemented CRT controls](CRT-APPEARANCE.md) | [Bezel design](BEZEL-DESIGN.md)

## Status and decisions

Audit date: **2026-10-14**. Source checkpoint: **`2ecb6fa8d6650e64d62346f3ebb1e6c542891ab5`**, clean at the start of inspection. The initial audit changed no production code. The subsequently authorized hidden-tab rendering slice is implemented below; neutral bypass/resource keys, scanline geometry and screen-only phosphor tint are now implemented below. The earlier tint/scanline investigation JSON was checked against current source rather than treated as implementation evidence.

At the initial audit checkpoint, the built-in CRT shader was static and already skipped a clean frame. It still adds a full-size offscreen target and a full-screen pass whenever the retro toggle is enabled, including when both strengths are zero. Hidden tabs keep their controls and renderers. The original audited application path did not explicitly suspend their render threads; the implemented follow-up now gates unloaded controls. Composition presentation pacing can independently block or throttle them. **Source inspection and the limited process samples do not establish hidden-tab GPU utilization.**

Recommend adding **screen tint**, with separately named color, strength and contrast controls, and independent **scanline dark-band thickness and clear gap** in DIPs. The user approved screen-only tint as the later slice; a separate chrome option would require its own styling/composition design. Performance and alpha compatibility should be settled before adding temporal effects or additional buffers.

## Evidence conventions and limits

- **Source-proven** describes the named code path at this checkpoint, not an exhaustive claim about every platform/backend.
- **Measured** describes the isolated WARP shader probe or process samples below. Process CPU and private commit are not frame counts, GPU time or VRAM.
- **Proposed** describes future behavior, settings and acceptance targets. It is not implemented or an agreed performance promise.
- **Unresolved** requires instrumentation or interactive validation. Do not equate retained resources with continuous work, focus with visibility, or a Windows composition wait with an application visibility policy.

## Visibility and scheduling audit

| Scenario | Source-proven behavior | Implication / unresolved check |
| --- | --- | --- |
| Selected tab to hidden tab | [`TabManagement.cpp:942`](../../src/cascadia/TerminalApp/TabManagement.cpp#L942) unfocuses tabs and replaces `_tabContent` children with selected content. [`Tab.cpp:344`](../../src/cascadia/TerminalApp/Tab.cpp#L344) stores focus state; it does not tear down a renderer. | Visual-tree removal retains tab/control/core ownership. No explicit hidden-tab render suspension was found in this switching path. Measure already-rendered tabs, not just cold startup tabs. |
| Visible unfocused split | [`TermControl.cpp:2418`](../../src/cascadia/TerminalControl/TermControl.cpp#L2418) applies unfocused appearance; [`ControlCore.cpp:2540`](../../src/cascadia/TerminalControl/ControlCore.cpp#L2540) changes cursor visibility and sends focus information. [`renderer.cpp:1589`](../../src/renderer/base/renderer.cpp#L1589) manages cursor-blink inhibition. | A visible split must continue rendering output, selection and its unfocused appearance. Never suspend solely because it lacks focus. |
| Minimized window | [`IslandWindow.cpp:561`](../../src/cascadia/WindowsTerminal/IslandWindow.cpp#L561) reports minimize/restore visibility; [`TerminalWindow.cpp:832`](../../src/cascadia/TerminalApp/TerminalWindow.cpp#L832) forwards it; [`TerminalPage.cpp:4096`](../../src/cascadia/TerminalApp/TerminalPage.cpp#L4096) visits terminal panes. [`ControlCore.cpp:2501`](../../src/cascadia/TerminalControl/ControlCore.cpp#L2501) only forwards to initialized ConPTY `ShowHide`. | This callback does not itself suspend Atlas. Settings previews are not terminal panes in this traversal. Minimize/restore rendering behavior remains unmeasured. |
| Covered / occluded window | [`AtlasEngine.r.cpp:448`](../../src/renderer/atlas/AtlasEngine.r.cpp#L448) waits on shutdown or frame-latency completion; without a pending presentation it uses an 8 ms shutdown wait. [`:478`](../../src/renderer/atlas/AtlasEngine.r.cpp#L478) presents and records the wait state. | No explicit `DXGI_STATUS_OCCLUDED` policy was found in this Atlas path. Full cover and partial cover need composition-specific measurements; do not infer HWND swap-chain behavior. |
| Static clean content | [`AtlasEngine.cpp:119`](../../src/renderer/atlas/AtlasEngine.cpp#L119) returns `S_FALSE` when rows/cursor are clean and continuous redraw is false. [`renderer.cpp:473`](../../src/renderer/base/renderer.cpp#L473) respects that result. | Built-in CRT does not require animation. Output, cursor blink, resizing and settings can still invalidate it. |
| Animated custom shader | [`BackendD3D.cpp:381`](../../src/renderer/atlas/BackendD3D.cpp#L381) resets continuous redraw. [`:416`](../../src/renderer/atlas/BackendD3D.cpp#L416) compiles and reflects a custom shader; the first variable in the first constant buffer is treated as Time. Failed overall reflection conservatively enables continuous redraw. Failed buffer/variable description leaves it disabled. [`renderer.cpp:488`](../../src/renderer/base/renderer.cpp#L488) requests another frame. | Preserve the published ABI. Static built-in controls must not begin using Time. A visibility gate must also cover animated custom shaders; test successful, missing and reordered reflection data. |

The frame-latency waitable object is a presentation pacing mechanism, as described by [Microsoft's DXGI documentation](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject). It does not prove that invisible controls allocate nothing or submit no work. The empty-dirty-rectangle path explicitly flushes the device context to avoid queued GPU commands accumulating; retain that safeguard.

`Renderer::_disablePainting` is private; [`renderer.cpp:56`](../../src/renderer/base/renderer.cpp#L56) enables rendering, and [`:110`](../../src/renderer/base/renderer.cpp#L110) teardown wakes and joins the thread. Actual control transfer uses [`ControlCore::Detach:278`](../../src/cascadia/TerminalControl/ControlCore.cpp#L278). Do not turn every tab switch into detach/recreate. A future suspension API needs a wakeable state transition, coalesced invalidation and a full redraw on resume, with no UI-thread indefinite wait on presentation.

## Resources, compilation and cleanup

Each initialized core owns a Renderer and Atlas engine ([`ControlCore.cpp:155`](../../src/cascadia/TerminalControl/ControlCore.cpp#L155), [`:392`](../../src/cascadia/TerminalControl/ControlCore.cpp#L392)). Atlas creates its own D3D device/context ([`AtlasEngine.r.cpp:204`](../../src/renderer/atlas/AtlasEngine.r.cpp#L204)); a cross-control device cache was not found in this path. Automatic WARP selects Direct2D, so an HLSL app benchmark must explicitly select `direct3d11`, rather than assume software rendering exercises this backend.

The composition swap chain has three BGRA8 buffers ([`AtlasEngine.r.cpp:333`](../../src/renderer/atlas/AtlasEngine.r.cpp#L333)). An enabled postprocess adds one BGRA8 `W × H` render-target/SRV texture ([`BackendD3D.cpp:547`](../../src/renderer/atlas/BackendD3D.cpp#L547)). Approximate raw surfaces, excluding alignment, glyph atlases, driver allocations, other renderer resources and sharing:

| Physical target size | Three swap-chain buffers, `12WH` | Extra postprocess target, `4WH` |
| --- | ---: | ---: |
| 1920 × 1080 | 23.73 MiB | 7.91 MiB |
| 3840 × 2160 | 94.92 MiB | 31.64 MiB |

These are byte estimates, not measured committed VRAM. Multiply by actual retained initialized target sizes, not tab count alone. Hidden tabs can retain old dimensions, and split panes have smaller targets.

The built-in shader uses embedded bytecode. A custom path compiles from disk per shader recreation, with no application-level bytecode cache evident in the audited path. Custom shader failure retains custom precedence and does not enable the built-in shader. [`BackendD3D.cpp:251`](../../src/renderer/atlas/BackendD3D.cpp#L251) recreates shader, buffers, sampler and custom resources on a **misc generation** change. That generation includes more than shader selection: default colors and selection colors also change it ([`AtlasEngine.cpp:328`](../../src/renderer/atlas/AtlasEngine.cpp#L328), [`:658`](../../src/renderer/atlas/AtlasEngine.cpp#L658)). Thus a qualifying color update can rebuild unchanged shader resources. Glow/scanline strength changes use outer state instead ([`AtlasEngine.api.cpp:427`](../../src/renderer/atlas/AtlasEngine.api.cpp#L427)); preserve that inexpensive constant-update pattern.

[`BackendD3D::ReleaseResources:204`](../../src/renderer/atlas/BackendD3D.cpp#L204) clears target views, not every custom texture or shader. Shader recreation resets the full custom resource set; target recreation releases its old texture before allocation. Swap-chain destruction clears state and flushes, while core destruction tears down the render thread and then releases owned resources. **`Close()` is not the same as destruction**: [`TermControl.cpp:2651`](../../src/cascadia/TerminalControl/TermControl.cpp#L2651) stops UI timers and closes interactivity; [`ControlCore.cpp:1856`](../../src/cascadia/TerminalControl/ControlCore.cpp#L1856) closes the connection; [`:269`](../../src/cascadia/TerminalControl/ControlCore.cpp#L269) performs renderer teardown on destruction. TermControl contains a retained-instance warning for the last-window-tab path on Windows 10. This is a source warning, not a measured leak on this machine. Include close-with-retained-reference tests.

Each painted frame runs the full-target postprocess draw ([`BackendD3D.cpp:2332`](../../src/renderer/atlas/BackendD3D.cpp#L2332)), even when text invalidation is partial. Nonzero glow samples a 13 × 13 neighborhood plus the base sample. Tint and scanlines can be arithmetic on that base sample; they should not introduce another pass or texture. Optimizing glow or restricting the postprocess to dirty regions requires measured benefit and correct expanded blur damage, scrolling and edge handling.

## Settings preview lifetime

[`Profiles_Appearance.cpp:43`](../../src/cascadia/TerminalSettingsEditor/Profiles_Appearance.cpp#L43) retains a real TermControl with forced cursor visibility and disabled input; input disable does not disable rendering. [`PreviewConnection.cpp:31`](../../src/cascadia/TerminalSettingsEditor/PreviewConnection.cpp#L31) emits static sample content; its Close/Resize methods are no-ops.

The preview has a 100 ms trailing debounce ([`Profiles_Appearance.cpp:167`](../../src/cascadia/TerminalSettingsEditor/Profiles_Appearance.cpp#L167)). [`OnNavigatedFrom:88`](../../src/cascadia/TerminalSettingsEditor/Profiles_Appearance.cpp#L88) revokes observers but does not explicitly stop the preview or reset its pending updater. [`ThrottledFunc.h:140`](../../src/cascadia/WinRTUtils/inc/ThrottledFunc.h#L140) queues through a weak reference to the throttler; the preview callback itself captures the page's raw `this`. This avoids a simple dispatcher strong-reference cycle, but is not proof of complete navigation cancellation. [`SettingsPaneContent.cpp:48`](../../src/cascadia/TerminalApp/SettingsPaneContent.cpp#L48) has an empty Close method. Page/frame lifetime determines eventual destruction; no explicit appearance-page cache setting was found in the inspected path.

Proposed: suspend on hidden settings tab or navigation away, resume only while the preview is shown, cancel pending updates on leaving, and explicitly clean up on final close. Test rapid slider/navigation transitions and retained pages without relying solely on their destructors. Current preview displays focused/default appearance; unfocused preview remains a separate unimplemented feature.

## Isolated measurements

Local evidence is under `artifacts/audit/crt-rendering-079f931b/` (the suffix is an audit label, not a source commit). These ignored artifacts are retained locally, not included in repository documentation publication.

### WARP shader and alpha probe: passed

`RetroShaderAlphaAudit.cpp/.exe`, `alpha-audit-build.log` and `alpha-audit-runtime.log` contain a local-copy extension of the existing `tools/backslash/RetroShaderTests.cpp`. It compiles current production HLSL and runs D3D11 WARP against a 32 × 32 float texture. Existing neutral/default/reset pixel tests passed; reflection confirmed external b0 = 32 bytes and built-in b1 = 16 bytes. Compiled built-in Time was unused.

For a uniform premultiplied input `RGBA = (0.1, 0.1, 0.1, 0.4)`:

| Case | Measured output alpha |
| --- | ---: |
| Both strengths zero | 0.40000001 |
| Glow zero, scanlines 0.5, dark row | 0.20000000 |
| Glow 0.3, scanlines zero, sampled even row | 0.99988329 |

Glow output RGB was approximately 0.24997082 per channel. This confirms current float4 glow/scanline operations change alpha. The test is not a hardware timing, UNORM clipping or actual acrylic/compositor opacity test.

### Limited process baseline

Eight single samples used a new copy of `artifacts/portable/Backslash-Dev-20261013-232956-608-Release-x64-974bfbf6`, with Direct3D11 and software rendering explicitly configured, bezel off, 120 × 40 cells, local `pwsh -NoProfile` fixtures and the cursor hidden by VT. The custom fixture only modulates RGB with Time. Each case requested one or eight tabs through startup actions, waited for shell markers, warmed for three seconds, then sampled host CPU for approximately eight seconds. No terminal build was repeated for this documentation task; the copied executable's SHA-256 matched the prior portable executable.

| Configuration | Initialized shell markers | Host CPU seconds / ~8 s | Equivalent logical cores | Host private commit |
| --- | ---: | ---: | ---: | ---: |
| Off, 1 tab | 1 | 0.00 | 0.00 | 53.05 MiB |
| Off, 8 tabs | 8 | 0.00 | 0.00 | 91.43 MiB |
| Enabled, both strengths zero, 1 tab | 1 | 0.00 | 0.00 | 67.55 MiB |
| Enabled, both strengths zero, 8 tabs | 8 | 0.00 | 0.00 | 205.98 MiB |
| Default built-in strengths, 1 tab | 1 | 0.00 | 0.00 | 68.00 MiB |
| Default built-in strengths, 8 tabs | 8 | 0.00 | 0.00 | 177.90 MiB |
| Time-dependent custom shader, 1 tab | 1 | 62.78 | 7.85 | 60.58 MiB |
| Time-dependent custom shader, 8 tabs | 8 | 188.06 | 23.50 | 147.14 MiB |

Equivalent logical cores means aggregate process CPU seconds divided by elapsed seconds, not Task Manager's whole-machine percentage. WARP is multithreaded; hardware rendering normally moves much of this work to the GPU. The static CPU result is zero **recorded processor time in these samples**, not proof of zero submissions. Private commit varies by initialization/allocation timing; the zero/default eight-tab difference is not evidence that zero strengths intrinsically cost more than defaults.

All requested shells initialized, but a shell marker does not prove its renderer completed warmup. Launch requested `-WindowStyle Hidden`; `MainWindowHandle` was zero, and actual composition visibility was not established. These are not controlled selected-versus-hidden-tab or minimized-window measurements. The animated cases demonstrate substantial aggregate CPU work in this configured software-rendering launch and support a visibility/scheduling investigation; they do not attribute the work to particular hidden tabs or measure hardware GPU load.

Evidence: `process-baseline-v2/process-baseline.json`, per-case settings/markers, `process-isolation-result.json`, saved harness/log and shader fixture. The first harness attempt failed on PowerShell scalar/array handling before sampling; its evidence was preserved, the harness was corrected, and all eight cases completed. Only exact-path processes from the new audit copy were stopped. Four known installed/user-portable settings/state hashes were unchanged, and the pre-existing portable terminal PID remained present. All fixture shell PIDs had exited at final verification. No user's working portable settings, SSH/WSL configuration or installed Terminal was changed.

## Proposed screen tint

The current GPU texture contains terminal text, cell backgrounds, cursor and selection. It excludes the separate XAML background image, acrylic backdrop, overlaid controls/search/scrollbar, bezel/badge and window chrome ([`TermControl.xaml:1252`](../../src/cascadia/TerminalControl/TermControl.xaml#L1252), [`:1268`](../../src/cascadia/TerminalControl/TermControl.xaml#L1268), [`RetroBezel.h`](../../src/cascadia/inc/RetroBezel.h)). A whole-screen/chrome tint cannot be promised through this texture alone. Avoid a colored transparent overlay: it alters transparency/contrast and leaves the underlying hue transformation undefined.

Proposed controls: color, strength 0–100% (default 0), and contrast with neutral/reset 1.0 and a finite bounded range to choose visually. At strength zero, return the incoming **post-CRT RGBA exactly**, without conversions or contrast changes. Contrast belongs to the tinted result, so its value has no effect when tint strength is zero. At 100%, a monochrome mode deliberately removes ANSI hue distinctions; explain that visibly in the UI and check selection/cursor/dim-text readability.

Implement independently: safely recover straight RGB from positive premultiplied alpha, calculate defined linear-light luminance, map it between theme-aware background and chosen phosphor/foreground endpoints, apply contrast around a documented pivot, interpolate RGB by strength, convert back and premultiply while retaining input alpha. Preserve transparent black at alpha zero, avoid NaNs near zero, and define gamut clipping. Dark/light themes need appropriate endpoint direction; saturated blue needs luminance/readability checks, not just multiplication by a blue color.

Tint preserving its input alpha **does not fix the existing CRT alpha behavior** demonstrated above. Correcting glow/scanline alpha to preserve the original texture is a separately reviewable compatibility change, with transparency fixtures and an explicit migration/default decision. Do not silently change legacy enabled appearance while adding tint.

Use the existing built-in postprocess and constant-upload path; allow tint alone with glow/scanlines inactive. Define the effective activation predicate from all active effects, while preserving custom-shader precedence and the external b0 ABI. No new image, history buffer, Time dependency or animation is needed for static tint. [Microsoft's shader contract](https://github.com/microsoft/terminal/blob/main/samples/PixelShaders/README.md) documents the custom path and b0 inputs. [cool-retro-term's shader](https://github.com/Swordfish90/cool-retro-term/blob/master/app/shaders/terminal_dynamic.frag) offers a conceptual luminance/chroma comparison; no GPL code or assets were copied.

## Proposed scanline thickness and gap

Current [`custom_shader_ps.hlsl:62`](../../src/renderer/atlas/custom_shader_ps.hlsl#L62) alternates equal bright/dark one-DIP bands using `floor(pixelY / scale)`, giving a two-DIP total period. Retain the existing intensity control and distinguish **dark-band thickness** from **clear gap**:

| Proposed value | Default | Proposed range / behavior |
| --- | ---: | --- |
| Thickness | 1 DIP | 0.5–8 DIP, positive |
| Clear gap | 1 DIP | 0–16 DIP; zero means uniform dimming |
| Existing intensity | 0.5 | 0–1; zero bypasses modulation |

Physical period is `(thickness + gap) × DPI/96`. Start the clear interval first to retain the present integer-scale phase. Use periodic pixel-interval coverage or equivalent filtered integration, including wraparound and subpixel bands; a floor-only mask aliases at 125%/150%. Verify averaged attenuation, not only individual rows. Preserve exact current default rows at 100%; fractional-scale appearance may improve, so review that compatibility explicitly.

The two reserved built-in b1 floats at offsets 8/12 can hold thickness/gap while keeping b1 at 16 bytes and current offsets 0/4 unchanged ([`BackendD3D.h:62`](../../src/renderer/atlas/BackendD3D.h#L62)). Tint parameters may require a separate aligned built-in-only buffer or an explicitly versioned b1 extension; choose that in the tint slice. Never repurpose external b0. Apply independent finite validation/clamps rather than the existing intensity 0–1 clamp.

Both features require inherited appearance fields through model/IDL/JSON/schema, editor reset/copy/duplicate, adapter, ControlCore, Atlas state and constant upload. Use the existing seams recorded in the investigation: `MTSMSettings.h`, `AppearanceConfig.*`, `IAppearanceConfig.idl`, `Appearances.*`, `TerminalSettings.cpp`, `ControlCore.cpp`, `AtlasEngine.api.cpp`, `common.h` and `BackendD3D.*`. Test explicit zero, null/reset, focused/unfocused application, invalid input retaining prior values, save/cancel/reload and custom-path precedence. No live preview should save profile changes by itself.

## Implemented follow-up: hidden controls and opt-in counters (2026-10-14)

This authorized slice stops after visibility/lifetime handling. It adds **no tint, scanline geometry, neutral-effect bypass, history implementation or shader ABI change**. The agreed progressive successful-history behavior is now specified in [SHELL-PRODUCTIVITY.md](SHELL-PRODUCTIVITY.md).

TermControl schedules a weak dispatcher callback on XAML Loaded/Unloaded and checks the **settled `IsLoaded()` state**. An early prototype treated Unloaded as an immediate permanent hide; the real split fixture exposed that this suspended the existing pane during reparenting. The final reconciliation handles that transaction correctly. [Microsoft defines IsLoaded](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.frameworkelement.isloaded?view=winrt-26100) as tree membership/readiness, and [Unloaded](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.frameworkelement.unloaded?view=winrt-26100) as removal from the main tree. This is a tree-lifetime policy, not a focus or desktop-occlusion heuristic.

Renderer suspension waits on a resume event **or shutdown**. The connection and parser remain active; invalidations accumulate/coalesce. Resume requests a full viewport/content redraw under the terminal lock, including invalidations consumed by a frame interrupted before Present. A hide may allow an already in-flight presentation to finish; after the worker's `suspended` acknowledgment, no further hidden frame starts. Close also suspends retained cores, and late load callbacks cannot revive closed/detached controls. No UI callback joins a render thread or waits for presentation.

Enable developer counters only by setting process-local `BACKSLASH_RENDER_DIAGNOSTICS` to an **absolute writable JSONL file path** before launching an isolated copy. Invalid/relative paths leave diagnostics disabled. Counters are off by default; disabled hot paths perform no logging, timer sampling or diagnostic atomic increments. Opt-in snapshots append at lifecycle transitions/destruction and at most once per second while painting. Stop/remove the opt-in after the bounded run; the log grows with run duration. It records PID/renderer ID, visibility/suspension, parsed UTF-16 character count, paint requests/attempts/prepared frames/clean skips, swap-chain Present1 attempts, postprocess draws, shader compile/recreation calls and owned offscreen texture create/release/nominal bytes. It records no commands, output text or credentials. Counts are cumulative; snapshot fields are individually atomic rather than a transaction. Prepared frames include attached render engines and are not the same as GPU submissions. Texture bytes cover the owned custom offscreen target, not driver residency, glyph atlases or actual VRAM.

Final source build: `artifacts/build/20261014-010956-848-Release-x64-dea1af72`, Release/x64: **0 errors, 134 warnings**, 28.33 seconds incrementally after the earlier full successful slice build (0 errors/134 warnings, 1m15.49s). Existing PRI/packaging warnings remain. Earlier failed prototype/build logs were preserved. No commit, push, registration, installation or user settings change occurred.

Verification:

- Standalone actual-production visibility helper: **14 assertions plus diagnostic JSON roundtrip passed**, including hidden acknowledgment/no frames, independent output progression, one coalesced resume invalidation, 500 rapid load/unload cycles and shutdown while hidden. Diagnostics default-off and relative-path rejection passed. Run [Test-RenderVisibility.ps1](../../tools/backslash/Test-RenderVisibility.ps1) in a fresh evidence directory.
- New Core test confirmed hidden output remains in the real terminal buffer, unfocusing a visible core does not suspend it, and close prevents later resume. **Focused 1/1 and full Control 30/30 passed**. The initial `/InProc` full invocation blocked ten tests requiring a second host; rerunning with supported default TAEF hosting passed all 30. Host/test DLL/resources.pri were adjacent.
- **Full Settings 170/170 and focused SSH/WSL/bezel 19/19 passed**; compiled CRT b0/b1 ABI and WARP pixel tests passed; portable isolation passed. These suites validated unchanged shared Core/renderer/model/shader code in the prior `2e7d4926` build; the final source change only reconciled TermControl reparent events and was verified through the fresh app below.
- Final app: all eight animated renderers were explicitly shown through this isolated copy's `focus-tab` CLI, then seven were hidden. Every renderer had already presented/drawn before hiding. Across all **seven final hidden intervals**, presents and postprocess draws were unchanged, while parsed output increased by **599–627 characters** per renderer. Returning each tab requested a full redraw and continued animation. Evidence: `render-visibility-final-hidden/warm-hidden-output-8/verified-hidden-intervals.json` and JSONL counters.
- Final app: both visible split panes, including the unfocused pane, remained loaded and animated with continuing output. Observed **1,685/1,800 presents and postprocess draws**, and **502/492 parsed characters** at their sampled snapshots. Evidence: `render-visibility-smoke/visible-unfocused-splits-2/diagnostics.jsonl`.
- Four known installed/user-portable settings/state hashes were unchanged; the pre-existing terminal PID remained present. Only exact-path processes from newly copied test artifacts were stopped. Settings and diagnostic fixtures stayed in smoke copies, outside the pristine delivery directory.

Process comparison, all single approximately eight-second host CPU samples with configured Direct3D11/WARP:

| Case | Logical-core equivalents | Meaning |
| --- | ---: | --- |
| Initial published-baseline quiet animated, 1 / 8 tabs | 7.85 / 23.50 | Original short samples, no per-renderer visibility proof |
| Initial visibility implementation quiet animated, 1 / 8 tabs | 6.71 / 6.96 | Same quiet fixture shape; indicates reduced aggregate WARP work, not a hardware GPU result |
| Final revision, 8 tabs with ongoing output | 8.20 | Different output workload; counters prove seven warmed hidden intervals submit no work |
| Final revision, two visible animated splits with output | 8.21 | Both visible renderers continue drawing |
| Quiet static eight tabs, before / initial implementation | 0.00 / 0.00 | No recorded CPU time in these short samples; does not prove zero GPU work |

CPU figures exclude shell children and count multithreaded WARP work. They are not Task Manager whole-machine percentages, statistically repeated hardware benchmarks, GPU time or memory proofs. Retained offscreen targets are intentional in this slice; no eviction/device sharing optimization or general memory reduction is claimed.

Pristine executable: `artifacts/portable/Backslash-Dev-20261014-010956-848-Release-x64-dea1af72/WindowsTerminal.exe`. ZIP and source-checkpoint hashes are in the final build's delivery manifest. Copy the distribution to a fresh folder for manual checking; keep existing working settings untouched.

Remaining checks: actual GPU timing/VRAM, minimized and covered windows, screenshots/fractional-DPI visuals, dedicated Settings preview navigation/close cycles, and cross-window move/attach visuals. IsLoaded is not an occlusion/minimize signal; those windows may remain loaded and are not claimed suspended here. The generic preview control receives the same tree reconciliation, but preview-specific lifetime coverage remains unrun. Native UI automation was unavailable. No unresolved failing test remains in the verified slice; at that checkpoint, the next item was effective-zero bypass/resource invalidation, now completed below.

## Implemented follow-up: neutral bypass and resource keys (2026-10-14)

Built-in CRT is now inactive when its toggle is off or **both strengths are exactly zero**. That state uses the same ordinary drawing path as CRT off, with no postprocess shader/target/pass. No near-zero threshold was introduced. Saved strengths, default 0.3/0.5 enabled appearance, HLSL math and legacy alpha behavior remain unchanged. A custom path always takes precedence, including neutral strengths and compilation failure; failure never enables built-in CRT.

Backend resource keys are separate: selected mode/custom source identity, auxiliary-image identity, and physical target dimensions. Ordinary color/selection and positive strength changes update constants/invalidation without recompiling the selected program or recreating its target. Vertex shader, b0 buffer and sampler are retained across compatible active program changes on the same device; built-in b1 remains 16 bytes and external b0 remains 32 bytes. A reset of only views at unchanged dimensions reuses the owned texture. A resize releases the old owned target before allocating the new size. Returning to neutral releases the owned offscreen target and postprocess resources. Device/swap-chain changes still follow existing ownership rules; no cross-device/global shader or texture cache was added.

Custom primary-source and image path/size/write-time stamps are rechecked during relevant miscellaneous-setting updates. A missing primary source becoming available recovers; a same-path primary source edit recompiles without reloading an unchanged image or target. Image failure suppresses the pass/continuous scheduling while retaining the compiled program; recovering the image reuses that program. Source includes still use the existing D3D standard include handler. **No new file watcher is provided**: included-file-only edits or edits preserving root size/write time are not independently detected by these keys. Changing/reselecting the primary path or disabling/re-enabling the custom shader forces fresh compilation; do not describe this as general dependency-aware hot reload. Existing internal debug shader hot reload is preserved; its diagnostic lambda capture was corrected.

Opt-in diagnostics now also report auxiliary-image load attempts and owned-image releases. Resource byte counts remain nominal owned offscreen bytes, not GPU residency. An already submitted GPU operation can outlive a logical COM release; no driver-memory or hidden-target eviction improvement is claimed.

### Final validation and measured comparison

Final application build: `artifacts/build/20261014-021339-767-Release-x64-b202bb33`, Release/x64: **0 errors, 134 warnings**, 56.19 seconds. Existing warnings remain. Fresh pristine portable: `artifacts/portable/Backslash-Dev-20261014-021339-767-Release-x64-b202bb33/WindowsTerminal.exe`. ZIP/source-checkpoint hashes are in `neutral-resource-delivery.json` in that build directory.

- **Focused actual-backend WARP test 1/1; full Control 31/31; Settings 170/170; focused SSH/WSL/bezel 19/19; visibility concurrency/JSON, compiled CRT ABI/pixels and portable isolation: passed.** TAEF hosts/test DLL/resources.pri were adjacent. The WARP backend test runs on an MTA worker like production WIC loading; an early STA fixture failure and an early fixture generation-synchronization error were diagnosed and corrected, with logs retained.
- The actual BackendD3D draws to an **unattached composition buffer** and reads BGRA8 pixels back; it never presents, attaches a XAML surface or creates a window. Neutral output was **bit-identical to CRT off** for opaque and partial-alpha background/glyph fixtures, with zero target creation, zero program recreation and zero postprocess draws. Active color/strength updates and same-size view resets reused the target/program; resize replaced only the size-dependent target; neutral transition released it. Custom/image failure/recovery, missing-source recovery, primary-source edits, animation detection and custom precedence passed.
- Controlled full backend sequence cumulative counters: `{"shaderCompiles":4,"shaderRecreates":7,"textureCreates":5,"textureReleases":4,"offscreenBytes":547200,"shaderImageLoads":3,"shaderImageReleases":2,"postprocessDraws":10}`. The final active target is 360 × 380 × 4 = 547,200 nominal bytes; these cumulative totals include explicit resize, deactivate/reactivate, missing-image and source-edit transitions and are not idle overhead.
- Final app visibility regressions again verified **seven warmed hidden renderers with unchanged presents/postprocess draws and increasing parsed output**, plus both visible split panes (including the unfocused pane) continuing to draw. Evidence is under `render-visibility-final-hidden/` and `render-visibility-smoke/` in the final build.

Before/after comparison used separately copied prior visibility build `dea1af72` and the final build, identical 80 × 25 local WARP fixtures with 35 OSC11 background-color changes plus output, and approximately eight-second observation windows. Each last sampled host snapshot had 999 parsed UTF-16 characters. Counters are cumulative from construction:

| Version / mode | Custom compile attempts | Program recreation calls | Owned target creations | Postprocess draws | Retained target bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| before / neutral | 0 | 32 | 32 | 32 | 2,912,000 |
| before / active | 0 | 37 | 37 | 37 | 2,912,000 |
| before / custom | 37 | 37 | 37 | 1456 | 2,912,000 |
| after / neutral | 0 | 0 | 0 | 0 | 0 |
| after / active | 0 | 1 | 1 | 37 | 2,912,000 |
| after / custom | 1 | 1 | 1 | 1916 | 2,912,000 |

The larger custom draw count after reuse is not a frame-rate guarantee: compilation no longer interrupts as many frames, and composition/WARP scheduling was not held to a hardware frame budget. The active cases retain their postprocess; the neutral case is the draw bypass. These tests measure calls/nominal allocations and pixel correctness, **not hardware GPU timing or VRAM**. Original/source-portable settings and pre-existing terminal processes were preserved; only exact-path test-copy processes were stopped. One initial comparison setup encountered an already-existing copied settings directory and was preserved; the fresh corrected comparison overwrote only its own fixture copy.

Minimize/occlusion policy, hardware profiling, native screenshots/DPI visuals, dedicated preview-close cycles and cross-window move visuals remain unrun/unchanged. No tint, scanline geometry, autocomplete implementation, commit/push, registration or installation was added. This completed the second bounded slice; scanline dimensions are now implemented below, with screen-only tint remaining next.

## Implemented follow-up: scanline thickness and clear gap (2026-10-14)

Settings > Profiles > select a profile > Appearance now has **Dark scanline thickness (DIP)** and **Clear gap between scanlines (DIP)** alongside intensity. Sliders use quarter-DIP steps and numeric labels, inherited-value/override source and individual reset controls. A DIP is a device-independent pixel; system DPI scales it. No tint control was added.

| JSON key | Default | Finite range |
| --- | ---: | --- |
| `experimental.retroTerminalScanlineThickness` | 1 DIP | 0.5–8 DIP |
| `experimental.retroTerminalScanlineGap` | 1 DIP | 0–16 DIP |

Both values support focused/unfocused appearance, defaults/parent inheritance, explicit zero gap, copy/duplicate, save/reload, null/reset and finite clamping. Nonnumeric/nonfinite values are rejected without replacing the prior property. They remain outside appearance telemetry like the existing intensities. The default appearance uses the existing 100 ms debounced live preview; unfocused preview remains unimplemented, while unfocused actual-control propagation is tested.

The existing one-pass shader integrates a periodic dark mask over each physical pixel, with clear space first and period `(thickness + gap) × DPI/96`. Integration includes period wrap and bands smaller than a pixel. Gap zero is uniform dimming. Work occurs only with nonzero scanline intensity; zero remains neutral/bypass eligible regardless of dimensions. The transform still multiplies **RGBA**, preserving the established CRT alpha semantics; it does not introduce an alpha correction. No Time use, animation request, new pass or texture was added.

**Compatibility:** default 1/1-DIP output is identical to the previous pattern at 100% and 200% DPI. At fractional DPI, the former hard floor pattern is replaced by partial pixel coverage; some formerly fully bright/dark rows become intermediate. This intentional difference avoids point-sampled aliasing. It is not a claim that all monitors/content are visually shimmer-free: native DPI/screenshot/scrolling checks remain unrun. Pixel-space phase stays anchored to the target, not time.

External custom b0 stays **32 bytes** (offsets 0/4/8/16), with custom shader precedence unchanged. Built-in b1 stays **16 bytes**: glow offset 0, intensity offset 4, thickness offset 8 and gap offset 12 use the former reserved floats. Dimension updates change outer settings/invalidation and upload constants, without changing the program/image selection key. A custom shader does not receive built-in dimensions or effects.

### Validation and delivery

Final build: `artifacts/build/20261014-024136-380-Release-x64-bb0cc9a5`, Release/x64: **0 errors, 135 warnings**, 4m38.91s after interface changes regenerated dependent WinRT code. Existing warning classes remain. Early missing-interface and strict standalone-fixture compile errors were corrected; failed evidence is preserved. The pristine portable and source-checkpoint paths/hashes are in `scanline-delivery.json` there.

- **Settings 171/171, focused SSH/WSL/bezel 19/19, Control 31/31, actual backend WARP reuse/neutral/custom tests, visibility concurrency/JSON and portable isolation passed.** Model coverage includes defaults, inherited source, serialization/copy/reload, explicit zero gap, reset/null, finite clamps and invalid values retaining prior state; duplication and focused/unfocused adapter/reset tests were extended.
- Production HLSL WARP matrix: six dimension pairs × five scales (0.75/1/1.25/1.5/2) × all 32 rows, checking independently computed periodic pixel coverage for RGB and alpha, plus zero-intensity identity. It includes default, thin/subpixel, thick, large gap, extrema and zero gap. All legacy integer-DPI rows passed, as did b0/b1 reflection/offset checks and unused Time. At 125% DPI, a row with 75% dark coverage produced RGB 0.0625 from source 0.25 at full intensity. Logs: `retro-shader-console.log` and `scanline-shader-corrected/shader-console.log`.
- Actual backend live dimension changes reused the target/program in the WARP readback test. A separate isolated app changed its own fixture settings live while output continued. Three dimension changes retained **one program recreation, one owned target, no compile**; zero intensity released the target, and later dimension changes did not restart draws. The program recreation count of two after disabling reflects a release/reset call, not a second allocation.

| Live stage | Program recreation/reset calls | Target creations | Retained nominal bytes | Cumulative postprocess draws |
| --- | ---: | ---: | ---: | ---: |
| default | 1 | 1 | 2,917,600 | 16 |
| thickness-0.5-gap-3 | 1 | 1 | 2,917,600 | 39 |
| thickness-4-gap-0 | 1 | 1 | 2,917,600 | 62 |
| thickness-8-gap-16 | 1 | 1 | 2,917,600 | 85 |
| zero-intensity-dimension-change | 2 | 1 | 0 | 87 |

Evidence: `scanline-live-v2/live-stage-results.json` and JSONL counters. The initial live harness hit an unsupported ordered-dictionary Clone method; its run was preserved and the corrected fresh harness completed. This affected test bookkeeping, not production code.

- The final app again passed **seven warmed hidden-renderer intervals with zero new presents/draws and continuing output**, plus both visible animated split panes including the unfocused pane. No minimize/occlusion policy changed.
- Known installed/user-portable settings/state and existing terminal processes were preserved; only exact-path fixture-copy processes were stopped. Runtime settings and logs stayed outside the pristine delivery directory. No commit/push, registration, installation or remote execution occurred.

Manual checklist: copy the portable to a fresh folder; adjust thickness/gap/intensity and reset each while checking default preview and real tabs; inspect 100/125/150/200% DPI, thin bands, thick bands, gap zero, transparent backgrounds, cursor/selection and scrolling; verify save/cancel/reload/duplicate and actual unfocused appearance. Confirm custom shaders keep precedence and zero intensity with glow zero remains ordinary rendering. Hardware GPU/VRAM, native UI accessibility/screenshot checks and dedicated preview-close/move validation remain unrun. This completed the third approved slice; screen-only tint is implemented below.

## Implemented follow-up: screen-only phosphor tint (2026-10-14)

Settings > Profiles > select a profile > Appearance adds **Screen phosphor color** and **Screen tint intensity**, including color swatch/accessible name, a 0-100% slider with numeric value, inherited source and independent reset. Enable Retro terminal effects. Color defaults to `#00FF41`; intensity defaults to zero and finite values clamp to [0, 1]. Values persist as `experimental.retroTerminalTintColor` (RGB hex, alpha canonicalized to 255) and `experimental.retroTerminalTintIntensity`. Both support defaults/parent inheritance, focused/unfocused appearance, null/reset, copy/duplicate and save/reload. Invalid color/numeric input keeps the prior property; nonfinite strength is rejected. As with the other CRT controls, neither property is added to appearance telemetry.

Tint is independent of glow/scanline strengths within the existing master toggle. Zero tint leaves the incoming **post-CRT RGBA exactly unchanged**, with no conversion or clipping. With all three strengths zero, the existing neutral bypass remains active regardless of tint color and scanline dimensions. A nonempty custom shader path retains precedence, including compile/image failures; built-in tint never supplies a fallback.

### Color and alpha contract

The Direct3D11 postprocess samples encoded sRGB premultiplied pixels. After existing glow and scanline processing, tint recovers straight RGB from positive alpha, clips it to [0, 1], converts sRGB to linear light and calculates luminance `Y = 0.2126 R + 0.7152 G + 0.0722 B`. Full tint maps that luminance along a **black -> chosen phosphor -> white** linear-light ramp: below the chosen color's luminance it scales that color; above it, it interpolates toward white. Intermediate intensity blends original and mapped RGB in linear light before encoding/premultiplying. This keeps the input pixel's linear-light luminance at ordinary positive alpha, without needing a separate light/dark theme threshold or an extra contrast control. It intentionally differs from the initial endpoint/contrast proposal: black and white remain black and white, brighter highlights whiten when a chosen hue cannot carry the required luminance in gamut, and black/white phosphor gives grayscale. Full intensity removes ANSI hue distinctions, so use lower intensity when their colors are meaningful.

Tint returns the stage's original alpha unchanged. Alpha zero returns transparent black; the zero-intensity path also retains otherwise invalid hidden RGB exactly. A denominator floor of 1e-6 avoids unsafe near-zero division; below that alpha, RGB may darken while remaining finite and bounded, below the precision of the production BGRA8 surface. The separate legacy glow/scanline float4 operations remain unchanged: tint preserves **their result's alpha**, rather than correcting it back to pre-CRT alpha. No alpha-migration or overall opacity change was introduced. Luminance preservation describes these surface pixels, not a guarantee about their final contrast over a separately colored image/acrylic backdrop.

### Exact surface coverage

- Tinted: pixels drawn into the terminal GPU surface, including text/colored glyphs, terminal-rendered inline image slices, rendered cell/default backgrounds where present, cursor and selection. This uses the existing Direct3D11 CRT path; an automatic software fallback to Direct2D does not acquire a new postprocess implementation.
- Outside that surface: the separate XAML profile `backgroundImage` (including desktop wallpaper), a window-level background image, solid/acrylic backdrop behind transparent terminal pixels, XAML search/scrollbar/tooltips/overlays, bezel/badge/casing, tab bar and window chrome. Those retain their colors. A fully transparent default-background pixel has no color for tint to change; its visible backdrop remains outside the pass.
- A custom shader and its `experimental.pixelShaderImagePath` retain their own rendering contract because custom mode overrides every built-in effect.

External b0 stays **32 bytes** at offsets 0/4/8/16 and custom precedence is unchanged. Private built-in b1 grows from 16 to **32 bytes**: existing glow/intensity/thickness/gap stay at offsets 0/4/8/12, and RGB phosphor/intensity occupy float4 at offset 16. Only the built-in shader receives this buffer. Constants update through outer settings without changing shader/image keys; tint introduces no pass, texture, image or Time dependency and no continuous redraw request. An active built-in pass is reused across tint-only and combined CRT updates.

The default appearance uses the existing 100 ms debounced preview and shared settings adapter/control update path. Color picker and slider changes, color swatch refresh and inherited reset notifications are wired and compiled. Focused/unfocused/reset propagation is covered by adapter tests and real-app live updates; native Settings preview visuals, save/cancel interaction, accessibility and preview-close/navigation checks remain unrun. Unfocused preview remains unimplemented; unfocused actual-control settings are propagated.

### Validation and delivery

Final Release/x64 build: `artifacts/build/20261014-032714-166-Release-x64-b187e644`, **0 errors / 135 warnings**, 4m43.15s. Standalone fixtures now enforce the production shader warning policy; an initial Windows min/max macro collision and FXC negative-pow warning were corrected, with prior failed evidence preserved.

- **Control 31/31; Settings 172/172; focused SSH/WSL/bezel 19/19; actual BackendD3D WARP, visibility helper and portable isolation passed.** Model fixtures cover default/off, inherited source, explicit zero, null/reset, copy/serialization/reload, opaque RGB normalization, finite validation and duplicate behavior. Focused/unfocused adapter tests cover color/strength inheritance and reset.
- Production HLSL WARP checks passed **1,200 tint cases** across dark/light/gray/saturated RGB, six phosphor choices, strengths 0/0.25/0.5/1 and alpha 0/1e-8/1e-6/0.4/1. CPU-double reference checks cover RGB, luminance preservation/readability, finite/gamut behavior and exact zero/alpha identity. Twenty-seven combined glow/scanline/DPI configurations check all 32 rows against their untinted post-CRT result. The earlier scanline matrix/default integer-DPI tests remain passing. Reflection confirms b0/b1 layout and unused Time. Evidence: `retro-shader-console.log`.
- Actual backend BGRA8 readback proves tint preserves every post-scanline pixel alpha while changing RGB; strength/color/tint-only transitions reuse the active target/program and remain static. Custom/image error/recovery and legacy neutral/resource fixtures also pass. Evidence: `postprocess-focused-warp.log`.
- A pristine portable was assembled without installation. A separate own-copy app changed tint/color/combined CRT settings live during local output, keeping **one program recreation, one target creation and zero shader compiles**. All-zero strength releases the target; a later color/dimension change stays bypassed. Recreation count two after disabling includes reset/release, not another allocation.

| Live stage | Program recreation/reset calls | Target creations | Retained nominal bytes | Cumulative postprocess draws |
| --- | ---: | ---: | ---: | ---: |
| default | 1 | 1 | 2,917,600 | 22 |
| tint-#FFB000-strength-1-scan-0-glow-0 | 1 | 1 | 2,917,600 | 37 |
| tint-#4060FF-strength-0.4-scan-0.5-glow-0 | 1 | 1 | 2,917,600 | 60 |
| tint-#FFFFFF-strength-0.75-scan-0.5-glow-0.3 | 1 | 1 | 2,917,600 | 83 |
| zero-intensity-color-and-dimension-change | 2 | 1 | 0 | 85 |

Evidence: `tint-live/live-stage-results.json` and JSONL diagnostics. Tint-only traffic tests verified **seven warmed hidden-renderer intervals** with zero presents/draw delta and continuing parser output, plus **both visible splits**, including the unfocused pane. Evidence: `tint-visibility/warm-hidden-output-8/verified-hidden-intervals.json` and `tint-visibility/process-after.json`.

Known installed/user-portable settings/state hashes and existing external terminal processes were preserved. Concurrent own-copy processes are excluded only when their executable path under this build run is confirmed; a departing null-path snapshot is tied to its confirmed own PID in the other fixture's snapshot. No commit/push, install/registration, remote execution or modification of existing user windows occurred. Pristine artifact, SHA-256, source checkpoint and changed-file inventory are recorded in `tint-delivery.json` in the build run.

Manual checklist: extract/copy to a fresh folder; enable Retro terminal effects with glow/scanlines zero, choose green/amber/blue and compare tint 0/25/50/100% on dark and light schemes; inspect dim/saturated ANSI text, selection, cursor and inline images; compare a transparent/image/acrylic background against the unchanged bezel/tabs; combine glow/scanline dimensions at 100/125/150/200% DPI; check preview, inherited reset, save/cancel/reload/duplicate and actual unfocused appearance. Confirm custom shaders retain precedence and all-zero strengths restore normal rendering. Native UI/screenshot/accessibility/preview-close/move/minimize/occlusion checks and hardware GPU time/VRAM profiling remain unrun. No continuous animation or new visibility policy was added.

## Prioritized bounded slices

1. **Visibility and lifetime diagnostics, then suspension.** Add narrowly scoped test counters for paint requests/submissions, postprocess draws, compile/create/release operations and live texture bytes. Establish warm hidden-tab, minimize and preview behavior first. Suspend rendering for explicit hidden/minimized state with safe wake/resume; continue parsing shell output and coalesce dirty state. Include animated custom shaders and settings preview. Focus alone is never the gate. Covered-window suspension depends on a reliable measured composition signal.
2. **Effective-zero bypass and keyed resource invalidation.** When all built-in effects are neutral and no custom shader is selected, skip the postprocess allocation and draw. Changes to strengths only update constants/invalidation. Separate shader selection/compile keys, auxiliary-image keys, target-size keys and ordinary color constants so unchanged shader/texture resources survive theme/selection changes. Verify disable/re-enable, resize and failure recovery.
3. **Scanline geometry.** Implement finite inherited DIP settings, filtered periodic coverage, b1 reserved slots and WARP/DPI tests. Keep it static and use one pass.
4. **Tint, after scope and alpha decisions.** Implement color/strength/contrast without new textures or temporal work. Test the complete transparent pipeline and accessibility/readability, then decide whether an alpha-correct CRT compatibility slice is required.
5. **Measured follow-ups only.** Consider a bounded bytecode cache keyed by source/include identity, compile flags and shader target if redundant compilation remains significant after keyed invalidation. Device shader objects stay device-specific. Bound entries/bytes and invalidation; never cache stale file edits. Cross-tab device/texture sharing, hidden-target eviction and separable bloom are larger lifetime/quality changes, justified only by measured memory or frame cost.

## Proposed performance acceptance and user validation

Use a fresh portable copy and fixtures, hardware Direct3D11 plus separate WARP correctness coverage, 100/125/150/200% DPI, 1080p and 4K targets. Record adapter, backend, scale, target bytes, cursor state and output rate. Run at least three 30-second samples after warmup; capture medians/p95 and compare equal content. The numbers below are initial proposed gates to calibrate against measured hardware baselines.

| Case | Proposed acceptance |
| --- | --- |
| 1, 8 and 16 tabs: cold, then each shown once and hidden | After explicit suspension acknowledgment, zero hidden postprocess draws/presents until visible again; resume shows accumulated output correctly. Distinguish parser CPU from rendering. Compare host CPU to no-effect baseline; provisional idle overhead under 0.5% of one logical core. |
| Static clean selected tab, cursor hidden; off / all neutral / scanline / tint / glow | No timer-driven frames after settling. All-neutral allocation/draw counts equal off. Cursor-visible case allows configured blink invalidations rather than demanding zero frames. |
| Four visible splits, some unfocused, with continuous output | Every visible pane updates; no focus-based suppression. Track frame and input latency. Tint/scanline without glow should add no pass or texture beyond the shared postprocess; provisional added GPU cost ≤1 ms at 1080p, ≤2 ms at 4K on the named target hardware. |
| Animated custom shader, static custom shader, failed reflection and failed compile | Visible animation is presentation-paced, not a busy loop. Hidden animation submits zero frames after acknowledgment. Static shaders remain event-driven; custom failure/precedence remains compatible. |
| Minimized/restored; fully covered/partly covered | Confirm actual window state. Minimize stops rendering and restore redraws once without stale content/deadlock. Characterize occlusion before asserting a covered-window gate. |
| Appearance preview: open, hidden settings tab, navigate away, close | Counters stop when inactive; pending slider work cannot revive a closed preview. No retained renderer/texture growth across 20 warmed navigation/close cycles. Save/cancel semantics and preview dimensions remain correct. |
| Slider burst, profile/theme/focus changes, repeated resize | Constants coalesce through preview debounce; no compile on strength-only/color-only change after invalidation refactor, no duplicate size textures, no stale bands. Surface bytes match live dimensions; old resource counts return to expected baseline. |
| Twenty warmed tab-close cycles; close with a retained control reference; 16 hidden initialized tabs | Exact live renderer/texture counts are bounded by the declared retention policy. No per-cycle growth after caches warm; provisional process private-commit tolerance within max(8 MiB, 5%) of warmed baseline. Report GPU resident/committed memory separately; driver cache variation is not by itself a leak. |

Functional fixtures must cover default pattern identity; thickness/gap independently of intensity; scales 1/1.25/1.5/2; subpixel average and wrap; gap zero; tint zero exact RGBA identity; contrast neutral/reset/extremes; tint endpoints and blue/dim ANSI; alpha 0/near-zero/fractional/1; cursor/selection; background image/acrylic; and b0/b1 reflection. Re-run settings, shader, bezel/preview and portable-isolation regressions in their supported resource-adjacent hosts when implementation begins.

Interactive checks remain unrun here: native UI automation is unavailable, and no visibility changes were applied to the user's windows. A later operator should use the isolated copy to warm each tab, compare hidden tabs against visible splits, minimize/restore and cover/uncover it, open/leave/close Settings, inspect fractional-DPI scanlines and transparent tint, and confirm screenshots/readability. Hardware GPU timing, GPU memory, per-renderer submission counts and close-cycle resource proofs remain required. No profiler installation, app registration, remote execution or user settings change is needed for this plan.
