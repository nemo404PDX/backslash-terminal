# Badge composed-render fix and aperture anchors

**Latest delivery:** [exact current user assets](#exact-user-asset-delivery-2026-10-16). This supersedes the prior artifact that staged older artwork. Tests were skipped at the user’s request for the latest build.

Validated 2026-10-16 UTC, uncommitted on master atop `602009965b8861778291c8176bbad146195f001e`. This supersedes the badge delivery **c69d5ac7**, which decoded the SVG but painted a gray square. The preceding uncommitted badge slice, original/working SVGs, authoring assets, backups and previous packages are preserved. No user settings, SSH/WSL behavior, palette/default value, shader, or automatic window-sizing behavior is changed.

## Reproduced cause and narrow correction

The exact production `RetroBezel` control reproduced the report in a real native XAML island. `ImageOpened` fired and its layout box was the intended **115.94x22 DIP**, yet its painted content was a flat **34x33-pixel square**, with **zero white glyph pixels** at the test monitor's 150% scale. Its `SvgImageSource` retained default **infinite raster dimensions**. Thus the earlier `SetSourceAsync` success checks established valid decoding but did not establish composed image sizing or pixels.

`BezelBadgeSvg.h` now sets finite raster dimensions **527x100**, matching the unchanged SVG viewBox, before supplying the stream. That same composed control then painted a **174x33-pixel** wordmark/slash with **1,082 pure-white pixels** in the initial before/after probe. Image layout, stream handling/lifetime, file resolution, fills, paths and transforms were held constant for this comparison. No source artwork was edited to mask the defect. The SVG's backing remains intact and the separate rectangular plate remains removed.

The confirmed problem is the source's default raster sizing for the percentage-sized SVG in composition. A successful decoder result alone did not catch it. This report does not infer an unobserved texture-allocation size inside XAML.

Before/after native composed evidence: `artifacts/badge-composed-regression-20261016/baseline-composed/style-0-bottom-left.png` and `fixed-initial-composed/style-0-bottom-left.png`, with their full pixel buffers and logs. The pre-fix capture visibly shows the same square; the corrected one shows the supplied artwork.

## Approved placement correction

The user's annotated placement clarification excludes the entire left/right side rails. Horizontal anchoring and width limits now use **the screen-opening span**, with **12 DIP inside each opening boundary**, rather than the outer frame edges. Left/right choices align to that allowed span; symmetric centered positions retain the same midpoint. Vertical centering in the selected casing band remains unchanged.

For a medium bezel, the side inset is 80 DIP and the opening begins 6 DIP outside the screen rectangle. The left anchor is therefore `80 - 6 + 12 = 86 DIP` from the frame edge; the right edge is symmetrically inset. Slim/Medium/Wide use their own opening bounds. When space is narrow, the logo scales uniformly to fit this allowed span or collapses if no span remains. It cannot spill into a side rail to retain its previous size. Extra filler and branding-off behavior remain independent.

`BadgeTests.cpp` now checks explicit left/right anchors, unchanged centers, side-rail exclusion, narrow-span scaling/collapse, all casing sizes/fillers/placements, and 100/125/150/200% sizing math. Existing serialization/inheritance/legacy behavior remains covered.

## Composed and actual-application validation

`backslash/tools/backslash/Test-BezelBadgeComposition.ps1 -EvidenceDirectory <existing-directory>` builds the native `BadgeComposedTests.cpp` fixture using existing tools. It hosts the **production RetroBezel**, not an SVG-only decoder, in live-control, wide-preview and narrow scrolling-preview containers, using the production preview layout helper. The screen in this component fixture is a placeholder, so text rendering is not confused with badge glyphs.

**162/162 composed cases passed:** three styles x six positions x three bezel thicknesses x three contexts. They capture native `RenderTargetBitmap` pixels and require a wide painted silhouette, white lettering/logo pixels, dark backing, correct image-transform coordinates, finite source raster dimensions, side-rail exclusion and a fully visible badge after narrow-preview scrolling. No `ImageFailed` event occurred. Comparisons inspect the badge region so unrelated scrollbar animations cannot contaminate the result. The old gray square fails the glyph/aspect requirements.

Evidence: `artifacts/badge-composed-regression-20261016/badge-composed-87958d58/composed-results.json` and 162 PNGs. These are real composed XAML-control captures, not synthetic SVG drawings or desktop screenshots.

The **final actual portable application** was also launched from fresh owned copies. Captures show all three styles in a running terminal, with the corrected bottom-left anchor. Scoped accessibility actions opened Settings, Defaults, and the exact profile `AppearanceNavigator`; the resulting **Defaults > Appearance** preview was captured and visually inspected with the wordmark/slash clearly present and properly placed. These are full actual-application captures, separate from the component fixture. Capture initially failed for a hidden window; the successful checks used a visible owned window. No user window was selected, captured, resized or closed.

Actual captures: `artifacts/badge-composed-regression-20261016/owned-profile-preview-capture/actual-application-solid.png`, `actual-application-outline.png`, `actual-application-currentColor.png`, and `actual-settings-preview.png`. Accessibility/action records and natural-exit results are alongside them. All owned sessions exited naturally, zero forced stops. The first generic Appearance navigation matched the global page; the recorded final preview uses the profile-specific automation id and contains `PreviewViewport`. The original supplied screenshot was inspected by the parent; its optional local Library transfer was blocked by the helper's Windows metadata support, so no local-source screenshot inspection is claimed.

## Build, package and limits

Final build: **`20261015-194956-750-Release-x64-96d27508`**, Release/x64, **0 errors / 134 warnings**, **1m26.89s**. Settings DLL build: **0 errors / 8 warnings**. Full adjacent-resource TAEF: **237/237 Settings** and **34/34 Control**, with no failed/blocked/skipped/not-run cases. Compiled CRT ABI/reflection/WARP checks passed, including 2,000 tint cases. Portable isolation and exact packaged SVG hashes passed.

Final owned runtime checks passed **11 startups**, **seven live placement/style changes**, and explicit usable-grid resizes to **40x16, 100x30 and 80x28**, with zero forced stops. Each ordinary startup retained usable 80x28. The accepted live-toggle behavior remains: fixed outer bounds, usable grid reallocates, while launch/reopen reserves the configured grid. No automatic resize code was added.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-194956-750-Release-x64-96d27508/WindowsTerminal.exe`.

Clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-BadgeRenderFix-x64-96d27508.zip`. Final SHA256, source inventory/checkpoint and test summary are in `badge-render-fix-summary.json` under the final build run. Extract the entire ZIP into a new folder and keep `.portable`; no fixture settings are included. Earlier c69/a955 builds remain preserved as evidence and are not this final delivery.

Actual capture/render inspection was performed at the host's current 150% scale. The four-scale unit math does not establish native multi-monitor DPI transitions. Native all-position/style/thickness composition is tested in the shared-control fixtures; full application interaction for every combination, keyboard/reset/Cancel/accessibility coverage, arbitrary badge recoloring and other hardware/display combinations remain outside this verification. CurrentColor remains explicitly white. No full product visual acceptance is implied.

No commit/push, install/registration, security change or source-artwork modification occurred. The pre-fix uncommitted slice was first archived in `artifacts/badge-composed-regression-20261016/preserved-badge-work-before.zip` and `.patch`; the final source checkpoint retains the complete working slice for review.

## Approved dropdown presentation order

The badge position list is explicitly displayed as **Omitted, Top left, Top center, Top right, Bottom left, Bottom center, Bottom right**. The editor reorders the existing localized enum entries after building its value map. It retains enum identities `0, 5, 2, 6, 3, 1, 4` in that display order and the existing canonical JSON tokens; saved positions and the legacy top/bottom aliases keep their meanings. This changes presentation only. The delivered opening-relative **12 DIP** placement and finite **527x100** SVG raster viewport are unchanged; the proposed fixed-maximum margin was cancelled before any edits.

Final approved delivery: **`20261015-203449-237-Release-x64-d669fc48`**, Release/x64, **0 errors / 134 warnings**, **52.41 seconds**. Full adjacent-resource TAEF passed **237/237 Settings** and **34/34 Control**, with no failed/blocked/skipped/not-run cases. Stable numeric/JSON position assertions were added to the existing badge roundtrip regression. CRT ABI/reflection/WARP checks passed, including the 2,000 tint cases. Isolation/completeness and exact approved-asset hashes passed.

The actual native Settings page was exercised in a fresh owned portable copy. UI Automation exposed the exact seven-item order above, retained the existing `bottomLeft` selection, selected each item and invoked Save; all seven canonical tokens were read back from that copy's settings. Page controls were reacquired after each Save. `dropdown-order-results.json` and the expanded native accessibility tree record these checks; the popup capture was taken while its owner was outside the viewport and does not establish a visible popup screenshot. Actual terminal and profile Appearance preview captures are separate. No user settings or windows were used.

**162/162 composed control cases** passed against the approved assets, with zero image failures. The fixture now waits for a newly assigned style's `ImageOpened` before pixel assertions and retains diagnostic PNGs before assertions. Pixel/aspect/backing/anchor thresholds are unchanged. The final owned runtime checks passed **11 startups**, **seven live changes**, and **three explicit usable-grid resizes**, with zero forced stops. The native host remains at 150%; other DPI/monitor transitions and full keyboard/reset/Cancel/accessibility combinations remain manual.

During this run the working cutout SVG and its Affinity source were edited concurrently. They were left untouched and excluded from this badge commit. The earlier failed white-pixel checks used that edited cutout, whose holes expose casing instead of the previously committed white lettering. The final portable is a separate copy assembled with the exact three approved SVG blobs from `602009965b8861778291c8176bbad146195f001e`; all other runtime files are byte-identical to the fresh build's portable. The original build/portable, concurrent artwork and prior deliverables are preserved. The final isolation run uses the established check with only its source-asset comparison redirected to those approved blobs; the validation copy is retained as evidence.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-203449-237-Release-x64-d669fc48-ApprovedBadgeAssets/WindowsTerminal.exe`.

Clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-BadgeFinal-x64-d669fc48.zip`. Extract the whole archive into a fresh folder and keep `.portable`. It contains no fixture settings, test logs, object files or authoring assets. SHA256, source inventory/checkpoint and publication results are recorded in `badge-final-delivery-summary.json` and `badge-publication-results.json` under the final build run. Validation evidence is under `artifacts/badge-final-order-20261016-918732c7`, with the approved-asset composed matrix in `badge-composed-approved-assets-34a9a5fa` and the successful native Settings check in `owned-order-check4`.

The final approval covers the accumulated badge integration/render fix, tests/documentation and presentation ordering, with ordinary pushes to both existing remotes. The opening-relative geometry, source artwork, shader/defaults and accepted live-sizing behavior were not changed by this ordering work.

## Exact user-asset delivery, 2026-10-16

This delivery supersedes the previous `ApprovedBadgeAssets` artifact. All three current user SVG exports are used directly and copied byte-for-byte into the build/portable; none is derived from another style or replaced with an older committed file. Their exact hashes and layer/fill semantics are recorded in [the authoritative asset record](BACKSLASH-BADGE-ASSET.md#authoritative-user-exports-2026-10-16). The normal and CurrentColor files each retain separate backing/letter regions. Outline retains transparent letter holes. CurrentColor white applies to both authored regions; no independent badge opacity/tint controls are introduced.

Fresh Release/x64 build: **`20261015-211157-321-Release-x64-fc122494`**, **0 errors / 134 warnings**, **00:00:15.65**. **Tests and visual validation were skipped at the user's explicit request.** The owned in-progress composed fixture was stopped; partial output is not a passing matrix. The previously recorded test/visual results are historical and are not claimed for these updated assets. Only necessary build/package integrity checks were performed: each runtime SVG matches the current supplied file's SHA256, the fresh ZIP includes `.portable`, and it contains no fixture settings or authoring sources.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-211157-321-Release-x64-fc122494/WindowsTerminal.exe`.

Clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-UserBadgeAssets-x64-fc122494.zip`. SHA256: `17EDB9FA8C08C39D3B5B03FE1FAAEE3C375120404D54C60F8DBE7192462AF4D7`. Extract the complete ZIP into a fresh folder and keep `.portable`. Prior packages and the user's working portable copies/settings remain unchanged. Snapshot, input structure/hashes and build/package records are under `artifacts/badge-user-assets-20261016-4a730a72` and this build run. The finite 527x100 raster viewport, dropdown saved identities/order, opening-relative 12 DIP placement, defaults and live-sizing behavior remain unchanged.
