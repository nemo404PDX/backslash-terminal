# Configurable CRT Appearance

[Project entry point](README.md) · [Roadmap](ROADMAP.md)

## Requested product scope

Provide an optional **per-profile Appearance section** with sliders/fields and live preview. The user requests richer CRT controls inspired by [cool-retro-term](https://github.com/Swordfish90/cool-retro-term), configurable intentional glitch/effects, an old-screen overlay/frame, and actual terminal-view curvature/warping. Place this beside background image and transparency settings so their interaction is understandable.

This is a substantial requested feature, not merely a theme preset or the existing simple retro toggle. None of these new controls is implemented.

## Proposed controls to evaluate

| Control | Proposed purpose | Design/engineering question |
| --- | --- | --- |
| Curvature | Warp the terminal view like a curved screen | Inverse coordinate mapping, clipping, cursor and selection alignment |
| Scanlines | Adjustable line pattern/strength | DPI, font readability and moiré |
| Glow/bloom | Phosphor glow around bright content | Blur passes, contrast and performance |
| Phosphor persistence | Decaying afterimage | History buffers, lifecycle and memory limits |
| Flicker | Controlled temporal variation | Reduced motion and bounded frequency |
| Noise/glitch intensity | Deliberate visual distortion | Stable hit-testing; never induce real UI instability |
| Vignette | Darken screen edges | Readability and contrast |
| Bezel/overlay | Old-screen frame or texture treatment | UI layering, content area and asset rights |

The exact controls, ranges, algorithms and preset set remain open. Requested outcomes must guide evaluation; this table is not an accepted implementation API.

## Reuse and architectural limits

The baseline already exposes `experimental.retroTerminalEffect`, `experimental.pixelShaderPath` and `experimental.pixelShaderImagePath`. [Microsoft's appearance documentation](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-appearance) describes the toggle and shader override. Reuse those hooks where suitable; verify precedence and migration for the new section.

Do not promise everything fits one stateless shader. Persistence may require history buffers and time-aware decay. Bloom may need additional passes. Overlays may need composition/UI work. Curvature requires coordinated rendering and inverse mouse hit-testing, selection and cursor placement; merely warping the picture is insufficient. Decide how inline images, scrollbars, background images, transparency and previews participate.

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

