# Supplied SVG bezel badge

**Superseded delivery:** c69d5ac7 painted a gray square despite decoder success. See [the composed-render fix and approved aperture anchors](BEZEL-BADGE-RENDER-FIX.md) for the corrected portable, actual-application captures and new pixel regressions. The initial delivery record below remains historical.

Implemented 2026-10-16 UTC, uncommitted on master atop `602009965b8861778291c8176bbad146195f001e`. This replaces the temporary text label and rectangular plate with the user's SVG artwork in the existing shared live-pane/Appearance-preview bezel. No new per-pane bezel architecture is introduced. Branding remains **off by default**; all prior casing/palette/phosphor/grid defaults remain unchanged.

## Placement, artwork and extra space

Settings > Profiles (or Defaults) > Appearance > Bezel:

* **Backslash badge:** Omitted; top left/center/right; bottom left/center/right.
* **Badge artwork:** Solid backing (default, `backslash-badge.svg`); Cutout backing (`backslash-badge-outline.svg`); CurrentColor (white, `backslash-badge-currentcolor.svg`).
* **Extra bezel space:** None (default), Top or Bottom. This adds 28 DIP independently of whether or where the logo is shown. The existing fullscreen centering option balances this extra space on the opposite side.

The old rectangular badge Border, background, seam and inset-plate paint are removed. The casing's existing gradient continues behind the image. Each SVG's intentional backing, compound cutouts, separate lettering/logo path, transforms and opacity are retained. The static artwork files and authoring backups are unchanged. CurrentColor is set to **white inside the SVG stream**; no configurable badge color or inherited XAML Foreground behavior is claimed.

JSON example (inside a profile or `profiles.defaults`):

```json
{
  "experimental.retroBezelEnabled": true,
  "experimental.retroBezelBadge": "bottomRight",
  "experimental.retroBezelBadgeStyle": "solid",
  "experimental.retroBezelFiller": "none"
}
```

New canonical badge tokens are `none`, `topLeft`, `topCenter`, `topRight`, `bottomLeft`, `bottomCenter`, `bottomRight`. Old `top`/`bottom` tokens still load: they map to centered placement and retain their old top/bottom 28 DIP filler when that layer does not explicitly specify filler. Save writes canonical center tokens plus the preserved filler. Explicit filler, including `none` or null/reset, wins over that legacy mapping. Turning branding off leaves filler intact; reset/copy/inheritance and Save/Cancel use the normal profile settings model. Enum identities Bottom=1 / Top=2 are retained; the editor exposes seven distinct choices without alias duplicates.

## Layout and renderer

`BezelBadgeLayout.h` places a proportional **527:100** image, at most **22 DIP high** (115.94 DIP wide), 12 DIP inside left/right corners and vertically centered between the outer rim and the selected casing shoulder. It reduces both dimensions together when space is narrow, with a 4 DIP vertical band safety margin; impossible/tiny/nonfinite bounds collapse the decoration. Frame SizeChanged recalculates placement. A noninteractive Canvas contributes no desired size, so the image cannot enlarge or clip the usable terminal grid through measurement. The image and its canvas disable hit testing; the image is marked raw decorative accessibility content. No new input handlers or focus targets are introduced.

`BezelBadgeSvg.h` loads the three explicitly packaged `Assets/branding` files beside the executable, caches their UTF-8 data, and supplies a stream to native `SvgImageSource.SetSourceAsync`. A load owns its source rather than a RetroBezel pointer; late completion cannot reattach to another style or dereference a destroyed pane. Assets are never loaded while branding is initially off. Packaging copies only the three working SVGs, not `.afdesign` authoring files or exact-source backups. Source decode failures are logged without replacing terminal content.

## Validation and fresh delivery

Release/x64 build `20261015-181808-788-Release-x64-c69d5ac7`: **0 errors / 134 warnings**, **3m34.86s**. Settings DLL: 0 errors / 9 warnings. Focused **4/4 badge tests** and **237/237 full Settings tests** passed. Coverage includes every placement/style save/load combination, legacy mapping, independent filler/off behavior, copy/inheritance/null reset, vertical centering, narrow/invalid bounds and sizing math at 100/125/150/200% scales. The existing curated-default, CRT, bezel, profile and source regressions also passed.

**34/34 Control tests** passed. Compiled shader ABI/reflection and WARP output passed, including 2,000 phosphor cases, ten white endpoints and 170 monotonic gray levels. No shader/palette/tint value is changed here. Portable isolation and exact packaged SVG hashes passed. Native offscreen **XAML SvgImageSource decoded all three styles successfully**, using the actual shared stream/asset helper and the application's existing Windows target manifest. This establishes native SVG decoding, not a live screenshot match or arbitrary recoloring.

Owned runtime checks: **11 startup cases**, all at usable **80x28**, covering branding off, all six positions, all three styles, Slim/Medium/Wide, independent filler and legacy top. **Seven live placement/style changes** retained the grid. Explicit VT resizes to **40x16, 100x30 and 80x28** produced those usable grids. All sessions exited naturally, **zero forced stops**. Mathematical bounds cover narrow previews; native Settings scrolling/monitor-DPI gestures and composed badge pixels remain manual. Hit-test exclusion is established by the implementation's image/canvas flags, not a native pointer-interaction test.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-181808-788-Release-x64-c69d5ac7/WindowsTerminal.exe`.

Clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-Badge-x64-c69d5ac7.zip`. Final SHA256 and inventory are recorded in `badge-delivery-summary.json` under the build run. The portable is fresh and contains no test settings; user/running copies and previous artifacts were not overwritten. No commit/push, registration, installation, SSH/WSL change or window-resize redesign occurred.

Evidence: `badge-validation/` (settings/Control/CRT logs and native XAML probe), `badge-runtime-smoke/badge-runtime-results.json`, `bezel-live-toggle-observation/live-toggle-results.json`, `portable-isolation.log`, source checkpoint/inventory and final summary under this build run. Reproduce the decoder with `backslash/tools/backslash/Test-BezelBadgeSvg.ps1 -EvidenceDirectory <existing-evidence-directory>`; it uses existing SDK/C++ tools and creates a fresh owned probe directory, without installation or desktop interaction.

## Accepted live-toggle behavior and manual checks

The user explicitly chose to **leave live sizing as it is**: toggling the bezel keeps the outer window fixed and allows usable rows/columns to change. Initial launch/reopening reserves the configured 80x28 usable grid. The owned ordinary-window observation measured: off launch **1044x755 / 80x28**, live on **same bounds / 67x21**, live off **same bounds / 80x28**; reopening with bezel on **1204x915 / 80x28**, live off **same bounds / 92x34**, live on **same bounds / 80x28**. These are fixture measurements in Win32 probe coordinates, not universal cell counts. Both sessions exited naturally. No automatic resize code or further sizing proposal is added; this avoids imposing resizing on maximized/snapped/split-pane situations.

User checklist in a fresh extraction: enable bezel, select each position/style, Save and reopen; verify the supplied backing and continuous casing gradient with no separate plate; resize/narrow the Appearance page and inspect DPI/corners; compare branding off with filler top/bottom retained; reset/inherit and Cancel a separate edit; verify selection/scrolling/pointer focus remains with the terminal. CurrentColor should look white in this version. Native badge screenshot acceptance, keyboard/accessibility interaction, hardware rendering and multi-monitor/DPI transitions remain unverified because no supported desktop interaction tool is exposed.
