# Configurable bezel design and research

[Project entry point](README.md) · [CRT Appearance design](CRT-APPEARANCE.md) · [Baseline sample map](BASELINE.md) · [Inline editor geometry research](SSH-LOCAL-EDITOR-RESEARCH.md)

Research date: 2026-10-13. **New design document only.** No bezel implementation, asset creation, build, source edit or Git mutation was performed. This file is intentionally not linked into shared indexes yet because the independent identity task is editing those files.

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

Recommendation: begin with a small set of per-profile shape presets and separate curated finish-family variants, limited applicable adjustments and a neutral geometry fixture. Richer gradients/material/shape editing can be evaluated separately. Treat imported PNG/SVG skins as later optional work, with explicit metadata and proven import behavior. No implementation choice is finalized by this research.

