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

Animate_scan/Animate_breathe are band/fade animation examples, not frame-history persistence. BackgroundImage's t0/t1 alpha composition is not a full bezel, inline image protocol or curvature/input mapping. Nop/Invert/Grayscale are comparison fixtures (Grayscale uses arithmetic RGB average); Broken/Error are diagnostics, not a guaranteed automatic fallback. See [complete sample map](BASELINE.md) for precise paths and limits. No new effect controls or runtime tests have been implemented.
