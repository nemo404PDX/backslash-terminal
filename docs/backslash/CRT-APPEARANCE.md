# CRT appearance

In Settings > Profiles, choose a profile and open Appearance. Enable **Retro terminal effects** to use the built-in effects. They are off by default. Changes use the normal preview, inheritance, reset and Save/discard controls; disabling effects keeps their saved values.

## Controls

- **Glow intensity** controls the halo around bright terminal content. Zero disables glow.
- **Scanline intensity**, **thickness** and **gap** control the band pattern. Choose **Black** or **White** scanlines. [Scanline color](SCANLINE-COLOR.md) describes the JSON option and blending behavior.
- **Screen phosphor color** and **Screen tint intensity** control terminal tint. Zero tint intensity keeps the untinted colors. Scheme-preserving phosphor and legacy behavior have different color/background rules; see [phosphor modes and opacity](PHOSPHOR-SCHEME-TONES.md).
- **Bezel and badges** are separate appearance controls. They do not recolor the tabs or terminal text. See [bezel controls](BEZEL-DESIGN.md) and [badge colors](CURRENTCOLOR-BADGE-CONTROLS.md).

Profile settings inherit from Defaults unless explicitly overridden. Reset clears a local override. Explicit zero remains an override; turning effects off and on does not reset the chosen values.

## Restrictions

A custom pixel shader takes precedence over the built-in effects, including when that shader fails to load. The Appearance preview does not implement unfocused appearance; inspect actual terminal focus changes when configuring unfocused values.

Large effects or strong tint can reduce readability and color distinctions. Test your font, cursor, selection and backgrounds before relying on an appearance. Broader hardware performance, DPI and accessibility coverage is incomplete. Curvature, phosphor afterimage persistence and CRT open/close animations remain proposed features.

For source architecture, shader ABI, reproducible fixtures and historical results, see the [CRT implementation record](../dev/FEATURE-RECORDS.md#crt-implementation-and-validation-history) and [rendering design](CRT-RENDERING-PLAN.md). These records do not establish that every scenario is validated in a downloaded version.
