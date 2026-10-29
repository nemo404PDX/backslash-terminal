# CurrentColor badge: Lettering and Backing

In a profile or Defaults > Appearance > Bezel, choose **CurrentColor** artwork to show **Lettering** and **Backing**. Each uses the existing color picker with **None (transparent)**, plus the normal inherited-value/reset control. Defaults are white letter faces (`#FFFFFF`) and black backing (`#000000`). Either region, or both regions, may be None. Solid and Outline continue to use their exact supplied artwork and ignore these two choices.

The existing profile settings pipeline handles inheritance, copy, null/reset, Save and discard. These are independent profile-level values, like the other bezel choices. Literal `"none"` is an explicit transparent override; omitted values inherit; JSON null clears that layer's override. Reset can therefore restore an inherited color or inherited None rather than forcing a global default.

```json
{
  "experimental.retroBezelBadgeStyle": "currentColor",
  "experimental.retroBezelBadgeLettering": "#FFFFFF",
  "experimental.retroBezelBadgeBacking": "none"
}
```

Each color is `#RRGGBB`, with `none` as the only extra option. Invalid manually entered strings fall back to the corresponding readable default at rendering/picker time; arbitrary input is never inserted as SVG markup. No alpha slider, opacity setting, automatic color inheritance, new badge mode, or global bezel-opacity change is introduced.

At runtime the app parses an in-memory copy of the user's CurrentColor SVG with DTD/external resolution disabled. It selects the existing `letters` and `backer` groups and overrides only the descendant paths' inline fill paint with the selected RGB or `none`. Path data, transforms, fill rules, group IDs, viewBox, source SVG bytes and Affinity source bytes are preserved. In-memory serialization is derived runtime data. A missing identified group is logged as a load failure rather than silently painting the wrong region.

The normal export already has a backing cut out under its separate letter faces, so None lettering exposes the casing through those openings. None backing leaves the chosen letter/slash faces. Both None yields no visible badge. Switching style or editing either paint supplies a new image source; an obsolete asynchronous load still owns only its old source and cannot reattach to the latest image.

Live pane and profile Appearance preview read the same settings and share the same renderer. Position enum values/order, 527x100 raster viewport, 22 DIP maximum badge height, opening-relative 12 DIP horizontal placement, extra filler and accepted live-sizing behavior remain unchanged. The two [appended finishes](BEZEL-FINISH-EXTENSIONS.md) are separate from these controls.

Validation for this delivery is build success and package/source-asset integrity only. No broad or focused test suite, rendering matrix, UI loop or native screenshot check was run. Interactive picker/None combinations, inheritance/reset/Save/discard behavior, runtime DOM decoding and preview appearance remain unverified in this build.

## Build for review

Fresh Release/x64 run: **`20261015-213130-815-Release-x64-6a5e300d`**, **0 errors / 134 warnings**, **00:02:55.22**. No test suite, rendering matrix, application UI loop or screenshot check was run. Package integrity confirms the three runtime SVGs match the untouched supplied source files; both Affinity source hashes remain unchanged. The full fresh archive contains `.portable` and excludes fixture settings and authoring sources.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-213130-815-Release-x64-6a5e300d/WindowsTerminal.exe`.

ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-CurrentColorControls-x64-6a5e300d.zip`. SHA256: `E073D2101586DE433475154F030AE130AA4FBA31B3E57D4F7CE24FA2A29347A8`.
