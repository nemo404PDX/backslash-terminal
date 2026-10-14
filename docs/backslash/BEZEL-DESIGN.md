# Configurable bezel design and research

[Project entry point](README.md) · [CRT Appearance design](CRT-APPEARANCE.md) · [Baseline sample map](BASELINE.md) · [Inline editor geometry research](SSH-LOCAL-EDITOR-RESEARCH.md)

Research date: 2026-10-13. A limited first bezel slice is now implemented; its scope and evidence are recorded below. The remaining sections retain historical research and future options, not a promise that those features have shipped. No commit or push was made for this slice.

## Implemented first slice (2026-10-13)

The frame is disabled by default. Settings → Profiles → a profile (or Defaults) → Appearance now has a **Retro bezel** group with an enable toggle, three bounded frame widths, nine selectable casing finishes, an optional top/bottom/omitted badge, and fullscreen aperture centering. Each control uses the existing local-override/inheritance/reset convention; Save and Cancel use the existing editor settings copy. The preview uses the same frame helper and existing 100 ms debounced refresh as live appearance changes. Width is a small preset choice, not a free geometry editor; there is no yellowing slider.

| Profile JSON property | Default | Supported values |
| --- | --- | --- |
| `experimental.retroBezelEnabled` | `false` | Boolean |
| `experimental.retroBezelSize` | `"medium"` | `"slim"` (18 DIP), `"medium"` (28 DIP), `"wide"` (40 DIP) |
| `experimental.retroBezelColor` | `"offWhite"` | `"offWhite"` (#E8E8DD), `"cream"` (#E5DABD), `"lightYellow"` (#D9C796), `"agedYellow"` (#C4AB68), `"graphite"`, `"charcoal"`, `"nearBlack"`, `"slateBlue"`, `"mutedOlive" |
| `experimental.retroBezelBadge` | `"bottom"` | `"none"`, `"bottom"`, `"top"` |
| `experimental.retroBezelCenterFullscreen` | `true` | Boolean; effective only with an enabled frame in fullscreen |

These are **profile geometry settings**, like text padding/font metrics, rather than focused/unfocused appearance settings. Focus changes never resize the aperture. Profile defaults, explicit local false, copying, duplication, null reset and override-source lookup participate in the normal profile model. Invalid enum strings or numeric enum inputs are rejected. Bezel fields under `unfocusedAppearance` are unsupported. New English labels use existing resource fallback; translations are future work.

### Rendering/layout boundary

- A shared XAML `RetroBezel` wraps the existing TermControl in `TerminalPaneContent`; the Appearance preview wraps its own TermControl with the same helper. The pane root remains stable during settings reload. Pane focus notifications read the nested terminal control's focus state, preserving mouse/keyboard provenance. The renderer/shaders and terminal input coordinate code remain unchanged.
- The casing is a **hollow** procedural Border ring with simple diagonal material shading, a bright/dark outer bevel and a recessed aperture bevel. The follow-up correction below supersedes the original outer-radius/rectangular-opening geometry. No whole-image stretching, artwork import, clip mask, time-based redraw or screen warp is introduced. Decoration is non-hit-testable.
- The whole TermControl, including scrollbar/search/selection overlays, sits inside the aperture. Frame insets are external to TermControl. Existing `padding` remains the SwapChainPanel margin used by text and input/accessibility coordinates. Colors are frame-only; no ANSI/default-role palette or CRT intensity setting is changed. The hollow center avoids painting plastic beneath a translucent terminal background.
- Initial window dimensions, VT requested row/column dimensions, pane minimum dimensions and pane resize snapping reserve the frame's fixed DIP insets. The terminal's normal layout/SwapChainPanel size change drives row/column resizing; the existing two-by-two cell minimum remains. Frame sizes do not change font size.
- A badge adds a fixed **28 DIP chin** on its chosen side. In fullscreen with centering enabled, an additional 28 DIP casing inset on the opposite side centers the aperture within each pane, reducing available rows. With centering off it fills the pane asymmetrically. This is pane-local centering; window chrome is outside that contract.
- The badge is simple original `BACKSLASH` text in a small strip, using a normal system font. It is temporary, not the final selected logo/icon. User-added raster concept references in `branding/concepts/concept-c-green.png` and `concept-a-b-c.png` are now locally verified and visually inspected; a production badge asset remains unselected. No manufacturer logos or GPL source were copied. Narrow badges use text ellipsis; that visual behavior still needs manual inspection.

Texture import, wear, richer shapes/materials, radius controls, custom skins, curvature, glass reflection, additional CRT effects and SSH work remain future scope.

### Verification actually performed

Final evidence: `artifacts/build/20261013-152835-951-Release-x64-afe9d435`.

- Repository `tools/backslash/Build.ps1`, Release/x64: **0 errors, 134 warnings**, elapsed **00:00:28.77**. Existing packaging/PRI/link warning categories remain. Earlier attempts are preserved under `20261013-151735-953-Release-x64-a96a0efb` (helper visibility error), `20261013-152030-463-Release-x64-0bc97a66` (nested x:Bind code-generation error), and `20261013-152227-773-Release-x64-3923110e` (successful intermediate before the hollow-ring change), and `20261013-152642-440-Release-x64-b0f3fa1b` (successful intermediate before preserving nested pane focus state).
- Settings DLL compiled successfully. TAEF used `bin/x64/Release/UnitTests_SettingsModel/TE.exe /InProc`, with **resources.pri beside the host**. All **12 ProfileTests passed** and all **7 Backslash tests passed** (17 distinct tests; two bezel tests occur in both selections), with zero failed/blocked/skipped/not-run tests in those selections. New coverage includes disabled defaults, all enum storage, local false, inheritance/override source, copy, null/clear reset, invalid input retaining prior values, duplication, shade independence from text/CRT settings, fixed DIP sizing/fullscreen balancing, and focus-independent control geometry propagation. The complete settings suite was not run.
- `tools/backslash/Test-RetroShader.ps1 -Runtime`: compiled shader ABI/reflection checks and WARP pixel-output regression cases passed. Shader source and intensity implementation are unchanged.
- Fresh `New-PortableDistribution.ps1` assembly and `Test-PortableIsolation.ps1` passed. Earlier artifacts were preserved.
- Hidden isolated startup smoke ran **7 cases**: disabled, medium/bottom with CRT enabled, slim/top/cream, wide/omitted/aged, fullscreen bottom/top centered, and fullscreen bottom without balancing. Each profile shell executed. All four windowed cases started at **80 columns × 24 rows** and changed to **90 × 30** after a VT size request; fullscreen cases retained a usable grid. See `bezel-portable-smoke.json` and case settings/shell evidence. This establishes startup/grid behavior, not a visual appearance check.
- **2 existing Windows Terminal settings/state files retained identical SHA256 hashes**. Only processes whose executable path exactly matched the fresh portable executable were stopped. Smoke settings/state were moved to the evidence directory, leaving the delivery folder clean. No installed settings were written.
- Clean ZIP: `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-Bezel-x64-afe9d435.zip`; **258 entries**, `.portable` included, smoke settings excluded. SHA256: **`CB4BABA191C73896C852946E7E3A353113E48473004412889F9E96AAFFAC6713`**.
- Source whitespace checking passed with `core.whitespace=cr-at-eol`. Three editor files' original LF endings were restored after compilation; their normalized logical sources are identical, recorded in `source-line-ending-restoration.json`. Other original source line endings were retained. No installation/registration, certificate trust, security setting, commit or push was performed.

**Runnable executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-152835-951-Release-x64-afe9d435\WindowsTerminal.exe`.

### Visual and interaction verification still unrun

No screenshots or native UI interaction checks were performed. This session exposes no `node_repl` for the supported Computer Use skill; its CUA tool has native computer APIs disabled. A shell startup smoke is not substituted for desktop visual testing. In particular, live control Save/Cancel/reset, keyboard/accessibility navigation, mouse selection/cursor/hyperlinks, pane split/move/zoom, small-window clipping, background transparency/acrylic/image appearance, and multi-monitor/fractional DPI remain manual checks. Hardware-GPU performance and idle redraw measurements are also unrun.

Use the fresh portable directory or extract the ZIP into a new directory; keep `.portable` beside the executable. Only that copy's adjacent settings should be edited.

1. Open a test profile's Appearance page. Confirm bezel off by default, then enable it. Try all four swatches and three widths; terminal ANSI output/colors and CRT sliders should stay independent. Confirm preview changes and Save/Cancel behavior.
2. Switch bottom → top → omitted; confirm the temporary Backslash strip stays in the frame and no edge text is covered. Change terminal padding separately. Drag a selection and position the cursor near all four edges.
3. Resize, split/close/move/zoom panes, and change font size. Confirm usable rows/columns, intact corners, aligned scrollbar/search/selection, and no clipping. Try a narrow pane for badge ellipsis.
4. Toggle fullscreen (Alt+Enter); compare centering on/off with top/bottom badges. Opposite-side outer space should keep the aperture centered and consume only the documented space.
5. Set bezel defaults, override a profile, clear each override, duplicate it, Save/relaunch, and check values. Test keyboard controls/reset and available DPI scales (100/125/150/200%).

### Changed source files

- Shared frame: `src/cascadia/inc/RetroBezel.h`, `RetroBezelGeometry.h`.
- Model/schema: `MTSMSettings.h`, `Profile.idl`, `TerminalSettingsSerializationHelpers.h`, `EnumMappings.idl/.h/.cpp`; `doc/cascadia/profiles.schema.json`.
- Adapter/sizing: `TerminalControl/IControlSettings.idl`, `TermControl.cpp`, `inc/ControlProperties.h`, `TerminalSettingsAppAdapterLib/TerminalSettings.cpp`.
- Pane/fullscreen/focus: `TerminalApp/TerminalPaneContent.idl/.h/.cpp`, `TerminalPage.cpp`, `Pane.cpp`.
- Editor/preview: `TerminalSettingsEditor/Appearances.idl/.h/.cpp/.xaml`, `Profiles_Appearance.h/.cpp`, English `Resources.resw`.
- Tests: `UnitTests_SettingsModel/ProfileTests.cpp`, `TerminalSettingsTests.cpp`; this design document.

## Historical research and future design options


## Requested direction and proposed first scope

The user's current priority is a small set of frame presets with limited meaningful adjustments, saved per profile; it does not need to be fully customizable. Color, thickness and corner roundness are examples to evaluate, and each preset may expose only applicable controls. Specific preset designs/control lists remain proposed, not all accepted requirements. Custom imported frames are later optional work. The user's recollection of cool-retro-term presets/exact controls is uncertain; the source observations below are separate verified research, not confirmation of that recollection.

Recommend constrained preset geometry first. A full vector editor is outside the first scope. A rounded rectangular frame/aperture with bounded parameters is one proposed preset family, not the final selected design. Vector paths or procedural distance-field shading are implementation candidates, not chosen APIs. Offer live preview, neutral/no-bezel reset and reusable presets saved per profile. Decide per-profile inheritance/overrides and preset versioning before implementation; richer import/export can follow later.

| Proposed adjustment (not all first-scope controls) | Intended purpose | Constraint/open question |
| --- | --- | --- |
| Border thickness | Linked uniform or separate left/top/right/bottom values | Display DIP units; keep usable aperture at minimum size |
| Corner radius | Rounded outer/inner geometry | Bound by actual dimensions; define whether radii link |
| Limited bulge/taper | Slight cabinet/screen opening shape variation | Constrained range without self-intersection or hidden text |
| Base color | Color picker for the frame | Alpha/contrast and backdrop interaction |
| Gradient stops/direction | Reusable material/color variation | Number/type of stops and coordinate anchoring remain open |
| Bevel/highlight/shadow | Light/shaded dimensional appearance | Predictable scale and no dependence on animated redraw |
| Screen aperture | Explicit visible screen opening | Distinct from artwork transparency and border decoration |
| Text-safe padding | Space between aperture and text | Preserve readability/cursor edges independently of border cuts |

Use only a small meaningful subset for initial presets (potentially color/thickness/roundness). Gradient stops, bulge/taper and material effects are research options, not requirements to ship in the first slice. These are proposals to evaluate, not a promised final settings API. A preset can describe shape/material parameters without bundling a raster image. Do not force screen curvature or animation just because a bezel is enabled.

## Preferred UX: shape, finish family and coordinated variant

Choose frame shape separately, then a finish/material family, then one of a few curated coordinated variants. Proposed families include retro plastic, metallic and painted; these examples and the exact catalog are not approved final designs. A variant coordinates base, edge, highlight, shadow and accent roles so users do not need to edit every color independently.

Allow only limited optional brightness/accent tweaks where useful, saved per profile alongside the chosen preset/variant. Each shape/family can expose its applicable controls; avoid a full vector or material editor. The user's game-customization analogy is a design inspiration, not a verified claim about a particular game.

Bezel material palettes are distinct from ANSI terminal text palettes. Coordinated frame/text presets may be offered, but must not couple individual edits or silently recolor command output. Copying/customizing a finish should preserve understandable independent ownership; exact override/copy semantics remain to design consistently with the project's palette principles.

A material appearance can require gradients, shading and roughness/highlight distinctions beyond recoloring. Choosing a “metallic” palette alone does not establish realistic metal rendering. First prove readable, coherent curated finishes at bounded cost, then decide which limited material differences are worthwhile; no physical-material simulation is promised.

## Proposed plastic variants: yellowing swatches, separate wear

The user's latest clarification is to select plastic yellowing through palette swatches across a spectrum, with proposed examples fresh off-white, cream, lightly yellowed and aged yellow. This does not need a yellowing slider. Exact swatch colors/names remain future catalog design, not implemented controls.

Treat physical wear as a separate possible visual property rather than infer scratches, dirt or surface damage from a yellowed palette. Its control, appearance and implementation remain undecided; no wear slider or automatic wear progression is selected.

These choices belong to proposed future bezel/finish design, outside the active CRT intensity implementation. They preserve the shape → finish family → coordinated variant model and independent material versus ANSI text palettes.

## Proposed retro-computer themes and Backslash badge

IBM, Commodore, Atari and possibly Sinclair are inspiration references for coordinated retro-computer palettes, not verified selected models or a finalized catalog. A theme might coordinate casing and optional terminal colors, such as blue with light gray; exact colors/materials and named variants remain proposals. Do not claim historical accuracy without model-specific research.

Use original Backslash artwork/logo/badge, with an optional Backslash name styled to fit the theme. Do not reproduce other manufacturers' logos or imply their endorsement. Exact badge assets/typeface rights remain pending.

A configurable badge lip/chin may be placed at the bottom, at the top or omitted. Its geometry, depth and allowed size remain design questions. Keep it separate from screen text padding and from wear/yellowing choices; plastic yellowing stays a swatch selection, with no slider required.

Applying casing/finish alone must not overwrite the terminal palette. Offer any full paired casing-and-terminal theme as an explicit choice with clear preview/scope; ANSI/default-role palette behavior remains independent afterward. No automatic coupling of individual material and terminal color edits is implied.

## Proposed fullscreen center-display layout

An optional fullscreen center-display layout can balance opposite-side spacing so the terminal aperture stays centered despite an asymmetric badge chin. For example, a bottom chin may be counterbalanced by outer space above, rather than shifting text upward within its aperture. The exact balancing policy is proposed, not implemented; it may trade some display area for visual symmetry and must show that tradeoff.

Distinguish three measurements:
- **Outer layout margin:** space around the entire frame/display composition, including opposite-side balancing.
- **Bezel inset/aperture:** frame material thickness and the opening available to the screen.
- **Text padding:** readable safe space inside the opening, independent of frame decoration.

Preserve useful terminal rows/columns and a minimum aperture. Do not silently consume excessive text space to center a chin, alter font size unexpectedly, or confuse decorative layout with remote scrolling. Define center-display versus fill-window behavior, extreme aspect ratios, minimum sizes and top/bottom/omitted chin transitions. Actual pane geometry changes must follow the established resize policy; local suggestion-only view shifts remain a different operation.

Future fixtures: casing-only selection leaves ANSI/default-role colors intact; explicit paired theme applies only its previewed scope; original Backslash badge remains legible; bottom/top/no chin center the aperture under the selected policy; spacing measurements remain independent; fullscreen/resize/DPI preserve readable rows/columns and aligned input/selection. These themes/layout options are future bezel design, outside the active CRT implementation.

## Source-backed cool-retro-term architecture

Inspected official master sources, not a pinned import into Backslash. These are architecture references, not copied code or runtime results.

| Actual upstream path | Finding and implication |
| --- | --- |
| [app/qml/TerminalFrame.qml](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/TerminalFrame.qml) | Root is ShaderEffect. It supplies frameSize, screenCurvature, screenRadius, viewportSize, frameColor, frameShininess and ambientLight; the frame is generated parametrically. |
| [app/shaders/terminal_frame.frag](https://github.com/Swordfish90/cool-retro-term/blob/master/app/shaders/terminal_frame.frag) | distortCoordinates, roundedRectSdfPixels and main derive frame/aperture geometry and procedural material/noise/glass shading. This frame path is not an imported SVG/PNG or nine-slice image. |
| [app/qml/ShaderTerminal.qml](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/ShaderTerminal.qml), [app/shaders/terminal_dynamic.frag](https://github.com/Swordfish90/cool-retro-term/blob/master/app/shaders/terminal_dynamic.frag) | terminalFrameLoader renders an RGBA frameSource which the dynamic shader composites; frame generation and final display composition are separate pieces. |
| [app/shaders/terminal_static.frag](https://github.com/Swordfish90/cool-retro-term/blob/master/app/shaders/terminal_static.frag) | Screen content warping is separate from the decorative frame shader. A bezel does not inherently implement the screen/input distortion contract. |
| [app/qml/TerminalWindow.qml](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/TerminalWindow.qml) | normalizedWindowScale is 1024 divided by mean window dimension, feeding frame/curvature scaling. This is their design policy, not a selected Backslash scale formula. |
| [app/qml/ApplicationSettings.qml](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/ApplicationSettings.qml) | margin, frameSize and screenRadius are distinct saved settings, reinforcing separation between content space, frame thickness and shape. |
| [app/qml/PreprocessedTerminal.qml](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/PreprocessedTerminal.qml) | Margin/DPI handling and correctDistortion coordinate conversion accompany mouse/wheel events. Appearance and interaction mapping must agree. |

The inspected files carry GPLv3-or-later notices. They may inform architecture research, but no source/shader/artwork was copied. Do not assume their code is MIT-compatible; component-level licensing review is required before any proposed reuse. Backslash's upstream MIT notices remain intact.

## Procedural shapes versus custom artwork skins

| Path | Strength | Main limitation |
| --- | --- | --- |
| Constrained procedural/vector bezel | Editable shape/color/gradient; adapts cleanly to sizes; presets can be compact | Needs explicit aperture/layout/shading geometry and coordinate model |
| RGBA PNG skin | Easiest first textured artwork import with predictable raster appearance | Needs metadata and nine-slice/tile/minimum-size policies; large texture and DPI quality limits |
| Optional static SVG skin | Scalable shapes at declared supported subset | SVG does not solve aspect deformation, aperture semantics or artwork layout by itself; complexity/feature validation needed |

Custom imported skins are later optional work, separate from the initial per-profile preset scope. Do not turn “vector bezel” into a requirement for unrestricted SVG authoring or copying third-party designs.

## Proposed skin metadata: independent units and semantics

Keep these fields conceptually distinct; exact schema remains open:

1. Source image dimensions and **source-pixel nine-slice cuts**: which asset regions form corners/edges/center.
2. **Displayed DIP border thickness**: how thick the frame appears regardless of source resolution.
3. Explicit aperture insets/shape: where screen content is allowed.
4. Text-safe padding inside the aperture: where text/cursor/selection must remain readable.
5. Minimum size and aspect policy: clamp, constrain, crop or letterbox behavior must be designed.
6. Optional ornaments: separate anchors for logos/knobs/details that should not stretch with an edge.
7. Asset/license attribution, preset version and validated local source reference.

Preserve corners, stretch/tile only permitted edge regions, and treat ornaments separately. Avoid arbitrary whole-image aspect stretching. At small sizes, enforce a defined minimum instead of overlapping cuts or collapsing the screen. Never infer the aperture from alpha: transparency can represent glass, decoration, antialiasing or holes unrelated to usable text space.

The proposed storage model distinguishes source pixels from layout DIPs. Convert carefully to the selected backend's API units and test at multiple DPI levels; these proposed fields are not claimed to match every Windows API parameter directly.

## Exploratory hybrid: dimensional shading and material textures

A procedural frame, pixel shaders and texture layers can be combined in a proposed hybrid design: adjustable shape/aperture; bevel lighting; optional tiled material texture such as metal grain, plastic or wear; highlights/shadows; and a separate glass-reflection layer. A convincing 3D-ish appearance need not require a full 3D scene, mesh viewer or physically based material editor. This is a future design option, not an implemented Backslash capability or finalized first-slice requirement.

Finish presets may bundle coordinated palette roles, a local texture reference and limited shading parameters. Keep that complexity behind curated variants rather than expose an unrestricted material editor. Validate readability and cost before choosing particular material families; recoloring plus a texture is not automatically realistic metal.

Use tiling/appropriate UV coordinates so grain/texture scale remains consistent with resize and DPI instead of stretching the entire grain image with the window. Define whether scale follows DIPs, pixels or material-relative units and convert deliberately. Authored scratches/logos/knobs may require separately anchored layers rather than repeating or deforming with edge regions. Specify aperture masking, clipping and alpha composition for each layer.

Frame material shading is distinct from terminal-content postprocessing such as scanlines/glow/curvature. Glass reflections should be separately controlled so they do not obscure text, become input targets or imply the screen is warped. A neutral frame and no-reflection/texture variant must remain usable.

Source context: cool-retro-term's procedural frame/aperture shader and separate compositing path demonstrate architectural separation; the Windows Terminal BackgroundImage sample demonstrates terminal t0/image t1 alpha composition, while Retro/atlas shaders already provide sampled glow/scanlines. Those facts do not establish a complete Backslash frame-material/glass compositor. Current shader hooks do not automatically provide arbitrary layered materials, texture metadata, aperture layout or shared hit-testing transforms. No GPL shader code was copied.

Future tests: consistent texture scale at multiple aspect ratios/DPIs; no repeating artifacts at nine-slice joins; anchored details stay anchored; glass/highlights preserve readable text and cursor/selection; neutral disable works; bounded resource use and no unnecessary continuous redraw for static materials. Exact layer order, lighting model, UV policy and allowed assets remain open.

## Windows primitives: relevant, not selected integration

- [CompositionNineGridBrush](https://learn.microsoft.com/en-us/uwp/api/windows.ui.composition.compositionninegridbrush) supports nine-grid stretching of a composition source, inset scales and a hollow center. Its examples separate asset insets and visual scale. Candidate for image or procedural-border composition; it does not supply a terminal aperture/input model.
- [Image.NineGrid](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.image.ninegrid) documents preserving corner detail and stretching edges differently. It uses Thickness/layout measurements; verify decoded-asset/DPI conversion rather than assuming raw source pixels and display DIPs are interchangeable.
- [SvgImageSource](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.media.imaging.svgimagesource) supports secure static SVG, without animation/interaction, and has asynchronous loading/failure handling. SVG support is a bounded render path, not an editable bezel schema.
- [Direct2D SVG support](https://learn.microsoft.com/en-us/windows/win32/direct2d/svg-support) lists supported shapes/gradients/attributes and limitations. Unsupported entities can be ignored, so validate/reject outside the chosen subset rather than accept silent visual corruption as a successful import.

These Microsoft Learn pages were actually fetched through the installed connector. No Windows API prototype was run. Backend selection and compatibility with the existing terminal XAML/composition/render path remain engineering decisions.

## Frame geometry, screen curvature and input are separate

A decorative bezel defines outer material and screen aperture. Screen curvature changes where terminal content appears. Border radius is not the same as screen warping; either feature must work with the other disabled.

Proposed coordinate model: pane layout in DIPs → aperture/text-safe rectangle → terminal grid/cell coordinates → optional screen warp → final presentation pixels. Pointer/selection conversion must follow the inverse matching model. Decorative overlay is non-hit-testable; it must not steal terminal input or hide cursor/selection. Deliberately interactive controls, if ever introduced, need a separate explicit contract.

When curvature is enabled, share parameters/transforms between rendering and input, including resize/font/DPI changes, near-edge cells and text-safe clipping. Coordinate this with the preferred local inline editor's view shift and selection/IME/accessibility mapping; do not apply a second unrelated warp to its preview. A neutral skin with no curvature must remain a valid readable baseline.

Changing border/aperture may legitimately change available content layout and terminal rows/columns; define that size policy explicitly. Merely shifting local inline composition to reveal a suggestion must still obey its separate rule of no fake remote layout input. Do not confuse decorative clipping, local presentation offset, actual pane resize and PTY geometry.

## Asset handling and quality requirements

Treat local custom artwork as data: cap file bytes, decoded dimensions, vector path/gradient complexity and decode time; allow no script, external fetch or arbitrary active content. Proposed SVG importer should restrict external references even if a library offers some reference support. Validate against the selected renderer's actual subset.

Decode/cache outside interactive rendering work where possible; refresh/rasterize at appropriate DPI/size and invalidate deterministically on file/preset changes. Fail imports clearly, keep the prior usable preset and release obsolete resources. Preserve asset license notices; no license rights are inferred from a file being user supplied.

Require readable text, accessible labeled sliders/color controls, neutral disable/reset and fast preset comparison. Keep decorative layers non-hit-testable, bound GPU/memory work, avoid resize jitter, seams, soft raster edges and obscured active input. Static bezel settings should not create unnecessary perpetual animation. Performance thresholds and supported size/DPI ranges remain to specify.

## Staged validation plan — no tests performed yet

| Stage | Proof | Acceptance evidence |
| --- | --- | --- |
| 1: limited per-profile presets | Neutral frame plus a small proposed set with applicable color/thickness/roundness adjustments, without curvature | No self-intersections; defined minimum aperture; stable extreme aspect ratios; correct text-safe padding |
| 2: DPI/layout | Resize, pane splits, font zoom, fractional/multi-monitor DPI | Smooth geometry/consistent DIP thickness; no corner seams or blurry unexpected raster scaling; stable saved values |
| 3: later optional artwork | Explicit source cuts, displayed border, aperture/padding and ornaments | Corners/details preserved; no alpha-derived aperture guesses; min-size/aspect policy predictable |
| 4: interaction | Mouse/copy/selection/cursor/hyperlinks/IME/accessibility; local composition expansion | Overlay does not intercept; coordinate mappings match content; active input stays visible |
| 5: optional curvature | Same shared warp/inverse mapping with bezel on/off | Near-edge click/selection aligns; neutral no-warp mode remains correct; no duplicate transforms |
| 6: robustness/performance | Malformed/oversized SVG/PNG, unsupported features, failed loads, repeated preset/file changes | No external activity; bounded resource use; errors retain previous usable state; cache/DPI refresh correct |
| 7: persistence/variants | Save/reload shape + family + variant + limited per-profile tweaks; later duplicate/import/export under chosen schema | Coordinated roles round-trip; ANSI colors remain independent; no implied live link; later missing local assets handled clearly |

## Open choices

Specific shape presets, retro-computer-inspired finish/palette variants, original Backslash badge/chin assets and applicable limited brightness/accent/control ranges; fullscreen center-display balancing/minimum-aperture policy; per-profile inheritance/overrides; inner/outer radius relationship; gradient types/stop limits; bezel versus aperture clipping representation; reserved content sizing; PNG nine-slice tiling policy; ornaments/minimum aspect behavior; optional SVG subset and sanitization; hybrid texture/shading/glass layer order, UV scale and anchored-detail policy; backend/renderer layer; resource/performance limits; preset schema/import/export/licensing UI; warp/input mapping and interaction with inline editor composition.

Recommendation: begin with a small set of per-profile shape presets and separate curated finish-family variants, limited applicable adjustments and a neutral geometry fixture. Richer gradients/material/shape editing can be evaluated separately. Treat imported PNG/SVG skins as later optional work, with explicit metadata and proven import behavior. The historical research did not finalize an implementation; the shipped first-slice choices are documented above.


## Corner correction following user references (2026-10-13)

The three supplied reference images were materialized through the supported Library transfer flow and inspected as pixels: `libfile_5ff414f797188191834b40df038b06b9` (annotated current corner), `libfile_9d4faa332f488191b34b7f32817d4368` (CRT recess), and `libfile_1259c2fe12b881918785bdf6f01ba719` (flipped boxed comparison). The last clarification concerns the **two boundaries of the recessed transition**: its outer shoulder is sharp/square, while the inner screen opening is rounded. It is not solely an application-window silhouette change. Lighting direction in a flipped crop is not a fixed lighting specification.

The shared helper now draws a hollow material path with a square external silhouette. The recess is a separate hollow path with a square outer shoulder and a 6 DIP rounded inner boundary; the original light/dark bevel shading remains. A 6 DIP inner clearance contains the entire arc outside the rectangular terminal control, preserving all corner cells, scrollbar/overlays, input coordinates, and the existing preset/window/grid sizing. A non-hit-testable Canvas isolates these paths from desired-size calculation, avoiding resize feedback. There is no terminal content clip, renderer warp, new shader, texture, or glass effect. Textured plastic, richer sloping materials, and glass refinements from the reference remain future work.

The corner constants/clearance are covered by `BackslashBezelSquareOuterRoundedInner`; the existing model/adapter/bezel and CRT tests remain applicable. Final fresh-build and startup/grid-resize evidence is recorded in SSH-PROFILES.md. Desktop visual verification of the resulting shoulder/rounded opening, selection, small panes, DPI, and transparency remains manual because the supported native automation runtime is unavailable.

### Center aperture regression correction

The user screenshot `libfile_7273be7555888191bdf864d9ee61a1d1` was materialized and inspected. Its wallpaper strip matches a concrete source defect: centering reserved the opposite-side 28 DIP as a margin outside the hollow casing, while TerminalPage's host is transparent. That area had no material paint. The correction keeps the casing at full pane size and adds balancing space to its own inset; content/minimum/grid dimensions retain the same total reservation. No opaque fill is added beneath the terminal, so terminal transparency is preserved.

Centering now takes the existing fullscreen state as a separate input and applies only there. It cannot enter/exit fullscreen, change focus mode, or alter tab visibility. The original tab policy is retained verbatim in a small shared pure helper for regression tests: focus hides tabs; true fullscreen uses showTabsFullscreen; ordinary/maximized mode uses the existing titlebar/multiple/always-show choices. The screenshot alone cannot establish what caused fullscreen/tab state before capture, and interactive reproduction is unavailable. The diagnosed unpainted strip is fixed; disappearance of tabs due specifically to toggling this control is not independently reproduced.

`BackslashBezelCenteringModeAndTabs` covers repeated center on/off in windowed, maximized, fullscreen and exiting fullscreen, top/bottom/omitted badges, disabled geometry, unchanged launch/tab preferences, and the actual production tab policy. Fresh startup/grid tests cover mode combinations and hot-reload toggles. Visual tab-row and wallpaper coverage checks, interactive Alt+Enter exit, and pointer/DPI behavior remain manual.

Final integrated validation: `artifacts/build/20261013-155957-526-Release-x64-96cd752b` (0 errors / 134 warnings), all 158 settings tests, shader ABI/WARP regressions, portable isolation, and 13 startup cases with 52 repeated center hot-reloads passed. Windowed/maximized grids remain stable; fullscreen grids return consistently when center toggles, with both fullscreen tab preferences tested. Four installed/user-portable settings/state hashes remain unchanged. Clean ZIP/executable paths and SHA256 are recorded in [SSH-PROFILES.md](SSH-PROFILES.md). Live visual tab/window-background checks and fullscreen exit remain unrun.

### Opaque black surround correction

The supplied screenshot `libfile_f01403fd68e081919a9602dc39ed78c1` was inspected as pixels. Its arrows indicate the exposed inside top/left strips and rounded corner between the bevel and rectangular terminal; tabs are visible. The rounded recess intentionally reserves a 6 DIP safety gutter outside the TermControl, but that hollow gutter previously exposed the transparent host or window background image.

The shared live/preview helper now places a non-hit-testable opaque-black hollow Border below the colored casing/recess. Its BorderThickness is exactly the same inset as the TermControl margin, and its Background is null. Thus the gutter/corner gaps are black, colored material remains above it, and the rectangular screen—including padding/scrollbar/overlays—has no black backing beneath it. The backing has fixed alpha 255 and XAML opacity 1; it does not use the TermControl background brush. Inspection confirms terminal opacity/acrylic changes the TermControl RootGrid background brush, not the surrounding frame's container opacity. Centering remains inside the full-size casing; fullscreen/tab policy and grid dimensions are unchanged.

`BackslashBezelOpaqueSurroundTransparentScreen` checks the screen-hole boundaries, gutter/corners and fixed-black color across all widths/badges, centering/mode combinations, opacity 0/0.25/0.6/1 and acrylic on/off. Build, full settings/shader regressions, isolated transparency startup/hot-reload cases, and fresh delivery evidence are recorded below after validation. Actual visual black-versus-transparent composition and acrylic appearance remain manual; native Computer Use is unavailable. This changes the frame surround only, with no global titlebar/theme opacity override.

### Badge plate vertical centering

The user clarification and parent inspection of `libfile_f0930181a1988191be36f14f1767059b` specify the horizontal red line as the vertical midpoint of the flat chin. The whole 22 DIP badge plate is now centered between the 2 DIP outer rim and the square inner-recess shoulder (`sideInset - 6 DIP clearance - 3 DIP recess`). Its offset from the outer edge is 5.5/9.5/15.5 DIP for slim/medium/wide with a badge. Top badges use the analogous top distance. Fullscreen balancing changes the opposite edge only, so plate centering stays correct. Existing horizontal margins/stretch are unchanged.

`BackslashBezelBadgeVerticalChinCenter` verifies equal flat-space clearance and plate midpoint across sizes, top/bottom, fullscreen/centering combinations and 100/125/150/200% mathematical DPI scaling. Native rendered/pixel-rounded badge inspection remains manual. The crop itself was interpreted from the parent's pixel inspection; this executor did not independently materialize that second image.

Final follow-up evidence: `artifacts/build/20261013-162825-371-Release-x64-f3e3a085`, 0 build errors / 134 warnings, all 161 settings tests and 11 focused tests passed, shader/isolation checks passed, and 19 startup cases with 76 centering hot-reloads passed. The fresh runnable copy/ZIP (also containing the automatic built-in SSH selection fix) is listed in [SSH-PROFILES.md](SSH-PROFILES.md). ZIP SHA256: `0CD83CE7C44387C7DD471EB81244C4DCF3AC43CFBC8C1FC150A29DBEF48AE590`. Four existing settings/state files retained identical hashes. Actual composited transparency, rendered badge centering and native interaction remain manual.

### Subdued inset badge material

The user/parent-inspected reference `libfile_3bae91bd79f88191a6492dddcf7228ba` identifies the pale cream strip as too conspicuous. The prior fill mixed 28% white into the casing. It is replaced by an opaque computed equivalent of a 22% neutral #707070 tint over each casing swatch. A 1 DIP seam uses a dark upper/left edge and a softer lower/right edge; no glow, thick contour, new slider or renderer blend mode is introduced. The plate color derives only from the selected casing, never terminal opacity/acrylic. BACKSLASH changes from weight 600 to 700; font family, size and letter spacing remain unchanged.

The whole 22 DIP plate, corner radius, horizontal placement and vertically centered chin position are retained. Border space is compensated by reducing internal padding so text insets/available content space stay at the original 10 DIP horizontal and 2 DIP vertical. The opaque black screen gutter, transparent screen cutout and square-outer/rounded-inner aperture geometry remain unchanged. The new shared color helper preserves the exact four casing swatches and their fallback.

`BackslashBezelBadgeMaterialSwatchesAndDpi` checks all swatches for subdued casing-aware face colors, dark/softer seam ordering, opaque alpha, unchanged text insets at 100/125/150/200% mathematical scaling, and bold weight. Existing badge-center, mode/tab, surround/transparency and profile tests continue to cover geometry. Actual native compositing, pixel rounding and aesthetic review remain manual because node_repl/native Computer Use is unavailable. The reference was interpreted from the parent's inspected pixels; no new screenshot or font asset was generated.

#### Badge material delivery and evidence (2026-10-14 UTC)

Source remains uncommitted atop `79cf9d34`. The prior source, SSH/profile behavior, main README, screenshot guide and user branding assets are preserved. A future outlined SVG can replace the lettering child independently of the plate; none has been supplied and no SVG import/font dependency is added now.

- Fresh build: `artifacts/build/20261013-170939-466-Release-x64-ef8de854`, Release/x64, **0 errors / 134 warnings**, **1m22.63s**. Settings DLL: 0 errors / 8 warnings. Existing warning categories remain.
- TAEF host adjacent to DLL/resources.pri: **12 focused tests passed; all 162 settings tests passed**, no failures/blocked/skipped/not-run tests. Tests cover all four swatches, subdued face/seam ordering, fixed opaque alpha, the original text insets and 100/125/150/200% mathematical DPI scaling, plus prior badge-center, aperture/surround, modes/tabs, SSH persistence/restoration and WSL behavior.
- Existing shader ABI/reflection and WARP output tests passed; shader source unchanged. Local fake SSH capture remains offline. Portable isolation checks passed.
- **19 isolated hidden startup cases and 76 centering hot-reloads passed**, including all widths/swatches, top/bottom/off and windowed/maximized/fullscreen, opaque/translucent/acrylic settings. This establishes successful startup/grid/reload behavior, not native plate appearance or rendered DPI quality. Four existing installed/user-portable settings/state hashes remained unchanged; only 19 exact-path owned processes were stopped. Smoke files are retained in evidence, and delivery has no settings folder.
- No screenshot, new font, SVG import, SSH connection, user-instance launch, registration, commit or push occurred.

**Executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-170939-466-Release-x64-ef8de854\WindowsTerminal.exe`.

**ZIP:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-BadgeMaterial-x64-ef8de854.zip` (258 entries, `.portable` included, smoke settings excluded).

SHA256: **`DD077711E3244029648D5FC45B5880AC23FFE2E81486244AB7851AF35259068B`**.

Changed files in this slice: `src/cascadia/inc/RetroBezel.h`, new `BezelBadgeMaterial.h`, `UnitTests_SettingsModel/ProfileSourceTests.cpp`, and this document. Actual native aesthetics, seam pixel rounding, wordmark readability in narrow panes and fractional DPI remain manual because node_repl/native Computer Use is unavailable; CUA native APIs are disabled. Inspect the plate with all plastic swatches and top/bottom/off, then compare transparent/acrylic screen settings to confirm casing/plate stay independent.

### Small outer-recess corner rounding (superseding the square shoulder)

The user's green-check reference (`libfile_2b091efdff888191acfdc1f17e91e7f3`) and current red-arrow reference (`libfile_39892016c93c81919796423ca3bf4ae6`), as inspected by the parent, supersede the earlier square **dark-band shoulder**. The recessed band now has a small 3 DIP rounded outer boundary on all four corners and retains its larger 6 DIP inner opening. The external casing/app outline stays square. Existing dark bevel shading remains, so the band reads as a continuous gap without new controls.

A shared clamp limits each arc to half its rectangle's dimensions; when a tiny opening clamps the inner radius, the outer radius scales down to half the inner radius, retaining their visual order. Empty/pre-layout rectangles generate empty decoration rather than invalid arcs. The black opaque surround, rectangular transparent terminal cutout, safety gutter, badge dimensions/centering/material, tabs and SSH/WSL behavior remain unchanged. The straight recess bounds are unchanged, so the badge's flat-chin midpoint is unchanged.

`BackslashBezelRecessCornerRadii` covers normal/narrow/tiny/empty/negative dimensions, rotated symmetry, all preset widths/badge positions and 100/125/150/200% mathematical scaling, plus square casing and screen-safe inner bounds. The actual native corner appearance and pixel rounding remain manual; node_repl/native Computer Use is unavailable. References were interpreted from the parent's inspected pixels. This is a single refinement; duplicate retry requests must not repeat the mutation.

### Thicker DPI-aware presets

The user requests thicker versions of all three presets, including Slim. Shared geometry changes **Slim 12 → 18 DIP, Medium 20 → 28 DIP, Wide 32 → 40 DIP**. The moderate 6/8/8 DIP increases leave distinct 10/12 DIP steps, and Slim now has at least 7 DIP of flat casing outside the inner recess/rim. Preset names and serialized enum values are unchanged. The 28 DIP badge chin and 22 DIP plate remain; the derived vertical offsets become 8.5/13.5/19.5 DIP on the badge side.

These are logical XAML DIPs, scaled once by the existing layout/sizing path. Initial-window and VT-size-request code already uses this shared helper with DPI scale, so terminal font metrics and user text padding are not altered. A fixed-size/fullscreen pane naturally has less terminal area with a thicker frame. Tests cover original font size, new widths, inner-recess clearance, badge fit and mathematical 100/125/150/200% scaling; runtime initial-grid/VT resize/hot-reload checks and native visual limitations are recorded with the new delivery. This width change accompanies the small outer-recess corner refinement.

### Rounded recess and thicker presets: verified delivery

Evidence: `artifacts/build/20261013-175202-052-Release-x64-09c88028`. The earlier corner-only `0f49d0ab` and geometry-only `f356ccc0` successful build directories are preserved. Checkpoint remains `79cf9d34`; all changes remain uncommitted.

- Final Release/x64 application build: **0 errors, 134 warnings**, **16.87 seconds**. Settings DLL build: **0 errors, 8 warnings**, **23.39 seconds**. Existing packaging/resource warnings remain.
- Adjacent-resource TAEF host: `bin/x64/Release/UnitTests_SettingsModel/TE.exe`, DLL and `resources.pri` together. Focused ProfileSourceTests **14/14** and full settings suite **164/164** passed, with no failed/blocked/skipped/not-run tests. Includes new radius/clamp/symmetry and thicker-preset layout/font preservation coverage, existing shell capture/restore/WSL, badge/gutter, fullscreen tab policy, CRT/profile regressions.
- Shader ABI/reflection and WARP pixel-output regressions passed. Fresh portable assembly and isolation checks passed.
- **19 hidden isolated startup cases and 76 repeated centering hot-reloads passed**. All widths/swatches, badge positions, windowed/maximized/fullscreen, transparent/acrylic, tabs and CRT cases started. Windowed grids remain 80x24 initially and 90x30 after VT requests; windowed/maximized center toggles preserve the grid, and fullscreen toggles repeat without a collapse. Modern Ubuntu-24.04 appears exactly once; no utility distro profile or WSL/SSH execution occurred.
- **4 installed/user-portable settings/state hashes unchanged**; only 19 exact-path test processes stopped. Smoke settings were preserved in evidence, leaving the delivery without a settings directory. Preservation audit confirms **59 existing source/docs/assets plus two previous ZIPs unchanged** outside the six scoped files, including FILE-TRANSFER-DESIGN.md, README/screenshot guidance, SSH/WSL source, badge material, original user concept images and licenses.

**Executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-175202-052-Release-x64-09c88028\WindowsTerminal.exe`.

**ZIP:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-RecessPresets-x64-09c88028.zip` (258 entries; `.portable` included, smoke settings excluded).

SHA256: **`43EF225AC2736BF571565CEC63E9CB7A6F7D1AF8F2683BB1BC52461898304750`**.

Changed files for this refinement: `src/cascadia/inc/RetroBezel.h`, `RetroBezelGeometry.h`, `UnitTests_SettingsModel/ProfileSourceTests.cpp`, `ProfileTests.cpp`, `TerminalSettingsEditor/Resources/en-US/Resources.resw`, and this document. The width labels now read 18/28/40 DIP. Source diff/status/preservation and smoke evidence are in the build directory. `source-checkpoint.zip` preserves the scoped working-tree source/docs/assets, including new untracked files; `final-source.patch` records the tracked diff without staging.

**Manual checklist:** compare Slim/Medium/Wide at 100/125/150/200% DPI; inspect the small outer dark-band corner and larger inner opening on all four corners; confirm continuous shading/opaque gutter and unchanged rectangular terminal transparency; inspect badge top/bottom/off, all four swatches, narrow/split panes and fullscreen tabs/centering. Native aesthetics, pixel rounding and interactive settings/accessibility checks remain unrun because node_repl/native Computer Use is unavailable (CUA native APIs disabled). The startup/grid checks do not establish rendered appearance. No installation, registration, security changes, commits, pushes, user-instance launch or remote connection occurred.

### Additional understated casing finishes

Five finishes append to the original four plastic shades. The original names, numeric values 0-3, default off-white, RGB values and light-plastic shading/badge path remain unchanged. New persisted strings and adapter/selector values are:

| Finish | JSON value | Numeric value | Base RGB |
| --- | --- | --- | --- |
| Graphite gray | `"graphite"` | 4 | #666B6C |
| Charcoal | `"charcoal"` | 5 | #414648 |
| Near black | `"nearBlack"` | 6 | #222629 |
| Muted slate blue | `"slateBlue"` | 7 | #566572 |
| Muted olive | `"mutedOlive"` | 8 | #626A55 |

The existing enum-backed selector obtains all nine choices from the model map and sorts by numeric value, so new choices follow the originals. English fallback labels call it **Casing finish**. Schema, enum mapping, copying, inheritance, null/reset and duplication retain normal settings behavior; no slider is added.

The five darker finishes use restrained neutral upper/outer highlights and darker lower edges, plus a shadow/light recessed band that separates from the black gutter. Their inset badge face is slightly darker than the casing, with a dark upper seam, subtle lighter lower seam and warm light lettering (#EEEFE5). The small wordmark's actual opaque plate/lettering pairs have calculated sRGB contrast of approximately 4.86/7.61/11.45/5.30/5.04 respectively; tests require at least 4.5. Every casing, recess, seam, plate and wordmark layer stays opaque. Original light badge lettering remains #2A2D2B. This checks color math, not native font rasterization or accessibility conformance.

Geometry remains square casing, 3 DIP outer recess/6 DIP inner opening, 6 DIP screen safety gutter, 18/28/40 DIP widths, and the existing 22 DIP centered plate/28 DIP chin. Terminal ANSI colors, transparency/acrylic, CRT settings, profiles and SSH/WSL behavior are independent. New model/adapter and material tests cover all nine serialized values, enum-map order, inheritance/copy/reset/duplicates, the five darker color contrasts and original shade fallback. Final build/package/startup evidence is appended after validation; native visual limits remain explicit.

### Five additional finishes: verified portable delivery

Evidence: `artifacts/build/20261013-181436-771-Release-x64-b6705194`. Checkpoint remains `79cf9d34`; all changes remain uncommitted. Previous geometry/material packages, source snapshots, FILE-TRANSFER design, branding assets, README and screenshot guidance are preserved.

- Final Release/x64 application build: **0 errors, 135 warnings**, **3m56.10s**. Final settings DLL: **0 errors, 8 warnings**, **26.55 seconds**. Existing packaging/resource/link warning categories remain. The first test compile's logs are preserved; its casing-color adapter assertions were corrected to verify independent terminal colors/opacity, matching the wrapper's direct profile-color ownership.
- Adjacent-resource TAEF (`TE.exe`, test DLL and `resources.pri` together): focused ProfileSourceTests **15/15**, full settings suite **166/166**, with no failed/blocked/skipped/not-run tests. New coverage includes all nine stable enum/string mappings and copying/inheritance/null reset/duplication, focused/unfocused terminal color and opacity independence, darker opaque materials and badge contrast. Existing SSH argument capture/restore/WSL, bezel corner/width/gutter/badge and fullscreen tab/CRT regressions passed.
- Static schema/model/UI consistency passed: nine ordered numeric values, serialized strings and English fallback labels; off-white remains the default. The selector sorts by numeric value, retaining the original four first. Shader ABI/reflection and WARP pixel-output regressions passed. Fresh portable assembly and isolation checks passed.
- **34 isolated hidden startup cases and 136 centering hot-reload samples passed**, including the original 19 regressions plus each new finish in opaque windowed/bottom, transparent maximized/top and acrylic fullscreen/no-badge modes. All widths, fullscreen tabs and modern WSL discovery remain covered. **Ten palette reload samples** cycle through all nine finishes and back to off-white, preserving the 90x30 terminal grid. Windowed starts remain 80x24 and VT requests reach 90x30; center toggles repeat without collapse.
- **4 existing installed/user-portable settings/state hashes unchanged**. Only 34 exact-path owned processes stopped; smoke settings moved into evidence, leaving delivery clean. Preservation audit confirms **55 existing source/docs/assets plus four previous artifacts unchanged** outside ten scoped files; the previous light-plastic shading path and complete geometry helper are retained.

**Executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-181436-771-Release-x64-b6705194\WindowsTerminal.exe`.

**ZIP:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-DarkPalettes-x64-b6705194.zip` (258 entries, `.portable` included, smoke settings excluded).

SHA256: **`AA5BE5C8E61DED9C1703AF804FF137C7A893117DD1D62B4C7E76C21765104D66`**.

Changed files for this slice: `TerminalSettingsModel/Profile.idl`, `TerminalSettingsSerializationHelpers.h`, `inc/BezelBadgeMaterial.h`, `RetroBezel.h`, `TerminalSettingsEditor/Resources/en-US/Resources.resw`, `doc/cascadia/profiles.schema.json`, `UnitTests_SettingsModel/ProfileTests.cpp`, `ProfileSourceTests.cpp`, `TerminalSettingsTests.cpp`, and this document. Source diff/status/preservation and the full uncommitted source checkpoint are retained in evidence.

**Manual checklist:** choose all nine finishes and switch light-to-dark-to-light in the settings preview and a live pane; inspect top/bottom/omitted badge, readable lettering and inset seam, dark recess separation/continuous 3/6 DIP corners and opaque black gutter with opaque/transparent/acrylic terminal screens; compare Slim/Medium/Wide at 100/125/150/200% DPI and narrow/split/fullscreen panes. Native XAML appearance, font antialiasing, fractional-DPI pixel quality and interactive accessibility checks remain unrun because node_repl/native Computer Use is unavailable (CUA native APIs disabled). Mathematical color contrast and startup/grid checks do not establish native appearance. No new slider, group implementation, user settings edit, SSH connection, installation/registration, security change, commit or push occurred.
