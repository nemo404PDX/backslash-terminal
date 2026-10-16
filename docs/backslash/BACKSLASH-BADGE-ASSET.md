# Backslash badge SVG inspection and conservative cleanup

**Current assets:** the user re-exported all styles. See [the authoritative export record](#authoritative-user-exports-2026-10-16); the earlier cleanup account below is historical.

Completed 2026-10-16 UTC. The supplied solid-backed and cutout-backed artwork was inspected, backed up byte-for-byte, then conservatively cleaned with the user's explicit authorization. A separate `currentColor` duplicate of the solid-backed artwork was created. This work prepares assets and records integration constraints; product code and the delivered portable ZIP are unchanged.

## Working assets and backups

| Asset | Supplied bytes | Working bytes | Result |
| --- | ---: | ---: | --- |
| [backslash-badge.svg](../images/backslash-badge.svg) | 19,813 | **19,411** | Solid dark backing, white lettering/logo; 402 bytes removed (2.03%) |
| [backslash-badge-currentcolor.svg](../images/backslash-badge-currentcolor.svg) | Derived duplicate | **19,425** | Solid dark backing; lettering and slash/logo inherit `currentColor` |
| [backslash-badge-outline.svg](../images/backslash-badge-outline.svg) | 33,734 | **33,332** | Filled backing with letter cutouts plus separate white lettering/logo; 402 bytes removed (1.19%) |

Exact originals are preserved in [docs/asset-backups](../asset-backups/README.md), outside runtime asset candidates. Working SHA256 values:

* Solid: `817D98BE673A969BA36C1FD7CBCCFF66FB95B25D0A25549DB251E335FF72A691`.
* CurrentColor: `3D8DECE0462CB3E7114209762A470FF57F0CDCF27EA53F10E8A29AE7532920F0`.
* Cutout: `43A25EF96FE5EFF8232D821AA7B57D0B382B55C23EB217E649D5669F45920262`.

`backslash-badge.afdesign` and `backslash-badge-outline.afdesign` remain untouched authoring files.

## Structure preserved

All three working SVGs retain `viewBox="0 0 527 100"`, width/height `100%`, aspect ratio **5.27:1**, three groups and two filled paths. Text and the slash mark are already path geometry: there is no live text/font dependency. Group IDs and transforms are unchanged:

| Group | Transform | Component |
| --- | --- | --- |
| `badge` | `matrix(2.11562,0,0,2.11562,-210.341,-185.892)` | Common parent |
| `backer` | `matrix(1.30253,0,0,0.712199,-19.8012,34.8411)` | Dark backing path |
| `letters` | `matrix(1.5384,0,0,1.5384,-92.3637,-44.4099)` | Lettering and slash/logo path |

Every `d` string is exact. The solid backing path has 5,176 characters; the cutout backing has 19,097 characters and 17 move commands. Both lettering paths have the same 13,840 characters and 16 move commands. The outline variant is a **filled compound path with letter holes**, not stroke-only lettering. Its `evenodd` fill rule and component separation are retained.

Cleanup removed the XML declaration and SVG 1.1 public DOCTYPE, unused namespace declarations, `version`/`xml:space`, indentation, and unused clip/stroke presentation properties. Equivalent white paint was shortened. The supplied DOCTYPE identified a W3C DTD, but no artwork resource depends on it. There are no image elements, external asset references, scripts, animation or `foreignObject`. No geometry was simplified, rounded, merged or flattened.

In the currentColor duplicate, `backer` explicitly remains `#000`, `letters` has `fill="currentColor"`, and its child path has no hardcoded fill overriding that inheritance. Both the name and slash mark therefore recolor together. Without a supplied SVG `color`, currentColor defaults to black; a host must set a suitable color explicitly.

## Native render verification

The installed Direct2D SVG renderer with WARP rendered at **1054x200** onto a transparent target. Comparisons use complete BGRA buffers, not PNG compression or a visual approximation:

* Supplied solid versus cleaned solid: **0 differing pixels**, no antialiasing tolerance.
* Supplied solid versus currentColor with white: **0 differing pixels**.
* CurrentColor at green `#00FF41` and amber `#FFB000`: **0 alpha differences**, **0 errors across 89,306 opaque dark-backing samples**, and **0 errors across 49,838 opaque lettering/logo samples** for each color. The actual SVG root `color` was set through Direct2D's DOM.
* Supplied cutout versus cleaned cutout: **0 differing pixels**, no antialiasing tolerance. Path data, transforms and evenodd rule also match structurally.
* With the separate lettering hidden in memory, the solid backing stays opaque at all **49,838** solid-letter sample positions. The cutout backing is transparent at **49,833/49,838**, with the remaining five antialiased positions and **zero opaque backing samples**. This confirms the exported holes instead of inferring them from the name.

Evidence: [solid/currentColor validation](../../artifacts/badge-svg-inspection-20261016/cleanup-render-validation.json), [cutout validation](../../artifacts/badge-svg-inspection-20261016/outline-render-validation.json), native probe sources/build logs and before/after PNG/BGRA files in `artifacts/badge-svg-inspection-20261016`. [Green preview](../../artifacts/badge-svg-inspection-20261016/currentcolor-green.png), [amber preview](../../artifacts/badge-svg-inspection-20261016/currentcolor-amber.png), [cutout backing alone](../../artifacts/badge-svg-inspection-20261016/outline-backing-only.png). These are offscreen asset tests; no live XAML bezel render or Settings interaction is claimed.

## Independent fills and opacity

Separate backing and lettering groups allow later DOM paint/opacity changes without altering paths. With the solid version, fading or hiding lettering exposes its opaque dark backing. With the cutout version, the holes expose the underlying bezel, so semitransparent lettering blends with the bezel there. Fading the common parent affects both components. Preserve the compound backing and evenodd rule to retain this distinction. The cutout variant keeps its supplied white lettering; no additional cutout-currentColor file was requested or created.

## Integration constraints and design context

A future implementation can explicitly package the selected working SVG as Content in an `Assets/branding` location and load it with `Image`/`SvgImageSource`, preserving 5.27:1 with uniform sizing. Keep backup and `.afdesign` authoring inputs out of runtime asset globs. Microsoft's [SvgImageSource API](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.media.imaging.svgimagesource?view=winrt-26100) describes URI/stream loading; [Direct2D SVG support](https://learn.microsoft.com/en-us/windows/win32/direct2d/svg-support) lists supported path/group/paint/transform attributes. This simple path-only artwork decoded successfully in the native probe.

Current `RetroBezel.h` uses a `TextBlock` inside a rectangular badge `Border`. Replacing that label alone would leave the existing plate. Dynamic color also needs explicit SVG color/paint supplied through an in-memory SVG stream or native DOM; setting XAML `Foreground` does not establish inheritance across an external image boundary. The probe verifies native DOM coloring, not that future XAML binding.

The agreed design context is a continuous casing gradient with **no separate recessed rectangular plate**. The SVG's intentional backing is a distinct artwork component and remains preserved. Proposed logo placement is top-left/center/right, bottom-left/center/right, or off; optional extra bezel filler is independent of logo visibility. These are future UI/layout requirements, not implemented modes or an approval to change geometry/defaults. The existing delivered badge-none default and all casing values remain unchanged.

## Implemented follow-up

The later [badge integration slice](BEZEL-BADGE-IMPLEMENTATION.md) packages these exact working files and replaces the temporary rectangular plate with an SVG image, six placements and separate filler controls. CurrentColor is explicitly white in this version. The inspection/cleanup results and source/backup hashes above remain unchanged; the earlier integration discussion records the preparation stage rather than current implementation status.

## Rendering correction

The initial c69 integration decoded the files but composed a square. [The rendering fix](BEZEL-BADGE-RENDER-FIX.md) supplies an explicit finite source raster viewport matching this unchanged 527x100 viewBox and adds painted-pixel regression checks. Original, working and backup SVG hashes above remain unchanged. Placement now uses the screen-opening span, excluding side rails.

## Authoritative user exports, 2026-10-16

These current files supersede the historical cleanup description above. They are packaged byte-for-byte as supplied: no optimization, regeneration, layer replacement, copied style derivation, or fill rewriting is applied to the files. The authoring sources are preserved exactly. The original backups and previous commits/packages remain available.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| `backslash-badge.svg` | 33580 | `77905E1C478398CAC4F3705CFAA14D9147B70CB7DC3E2CBF72B24795C30CBDEB` |
| `backslash-badge-outline.svg` | 19574 | `1AF2CBFCB2B2B7F5DB32AEBB5132C093D88AE5F56E216A25B47B9078EBC092AC` |
| `backslash-badge-currentcolor.svg` | 33607 | `73DF39D8FD75A81E61DC32879901F0A8338976F88FB6A4C05691F406B0C4D3ED` |
| `backslash-badge.afdesign` | 51459 | `73FA0E93F0A6ABE318FE1523B1D52248ACDE497B37B7621238D84C12A8CB16ED` |
| `backslash-badge-outline.afdesign` | 69167 | `7018453B3A3196521BBE889BAA898A5A5A8758AD8CEF4E2DAEACF715B3B33938` |

All three SVGs retain the 527x100 viewBox. The normal file has an evenodd compound backing with letter-shaped holes plus a separate white letter/slash path. Outline contains only that compound backing, so its letter-shaped holes expose the bezel. The currentColor file has both regions: its root `fill="currentColor"` reaches the backing, and the letter path explicitly uses `fill:currentColor`. Its backing and letter path data hashes match those in the normal file; the user authored the fill assignments.

The current runtime example supplies `color="#FFFFFF"` in memory without changing the saved SVG. Thus **both CurrentColor backing and letter faces resolve to white**. The host does not inherit XAML Foreground into this external SVG, and there is no separate badge tint, backing-opacity or letter-opacity setting. The file keeps separate regions for a future host that controls their paint/opacity, but this build does not add such controls or alter bezel opacity. No older outline/CurrentColor file is substituted to create another appearance.

The composed test now distinguishes Outline's exposed casing from the visible authored letter faces in the normal/CurrentColor styles. Tests and visual checks were explicitly skipped/stopped at the user's request for this delivery; no fresh test or visual pass is claimed. Validation for this build is limited to build success and exact package-file integrity. Historical test results above concern their recorded assets/builds.
