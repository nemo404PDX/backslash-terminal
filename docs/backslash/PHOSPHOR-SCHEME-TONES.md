# Scheme-sensitive phosphor

`experimental.retroTerminalSchemePhosphor` defaults to false. Missing settings preserve legacy rendering; no user settings are migrated or rewritten. Appearance exposes **Preserve scheme tones in phosphor**. It inherits/resets with the other appearance settings, including unfocused appearance and setup transfer.

The existing palette-derived picker and swatches remain. The scheme supplies shortcut colors for an independent phosphor RGB value. Changing schemes changes source tones and available swatches, without automatically replacing an explicit chosen hue.

## Color mapping and contrast

Both modes retain the existing linear-light Rec.709 luminance ramp. Full strength maps source luminance from black to the selected RGB endpoint, removes source hue, and preserves tonal ordering. Partial strength blends original colors. Selecting black retains the existing grayscale fallback. Dark/saturated endpoints reduce available brightness; equal-luminance source hues merge inherently.

Scheme mode bypasses the legacy cell contrast guard and endpoint-forcing selection/cursor policies. Normal selection/cursor rendering and the independent adjust-indistinguishable-colors setting remain. Exact source black stays black. Legacy mode retains its optional contrast protection and existing effect calculations.

For gray #444444 and #CCCCCC with orange #FF8800, zero-glow mapping produces #442000 and #CC6C00. Legacy protection can replace the first color with black because projected contrast is about 3.93:1. Scheme mode retains both source tones.

## Background ownership and opacity

With effective scheme phosphor enabled (built-in effects on, positive tint strength, no custom shader) and no background picture, Atlas owns the default background at every opacity, including 100%. Default RGB remains straight until its alpha is premultiplied exactly once. Explicit, reversed, concealed-cell backgrounds and foreground glyph opacity retain their existing rules. An alpha-capable target/grayscale antialiasing remains stable through 100-99-100 transitions.

The same single CRT pass processes background and text, retaining scanline phase, DPI and glow interaction. Opacity changes invalidate cached background pixels and clear colors. Glow can add bounded coverage; the opacity setting is input background coverage, not a promise that every final effect pixel has exactly that alpha. At zero opacity distant background is transparent while foreground text/halos remain visible.

The XAML solid fill beneath the renderer becomes transparent to avoid double composition. Padding outside the SwapChainPanel remains a separate uniform mapped brush, as it was outside the CRT aperture previously. Right/bottom fallback inside Atlas uses the same byte premultiplication as cell textures, avoiding low-alpha seams. The packed-color helper now multiplies channels independently; old packed division spilled channel bits at fractional alpha. All RGB values retain exact alpha-0/255 behavior.

For acrylic, Atlas supplies the owned colored/spatial foreground once. The underlying AcrylicBrush retains BackgroundSource, TintColor, FallbackColor, brush opacity, nullable luminosity and fallback settings; its TintOpacity becomes zero. Published outside-UI brush clones retain the requested tint opacity and source scheme color. Zero-owned-tint acrylic has the same material properties as the existing zero-tint path. External blur/noise/fallback are retained, and desktop pixels are never captured or phosphor-mapped. Windows automatically derives luminosity when TintLuminosityOpacity is null, so higher-opacity material brightness can differ from legacy; this is a new composition policy, not pixel-identical acrylic rendering.

At zero tint strength mapping is identity and renderer ownership returns to the existing division. Effects-off and custom-shader precedence retain their existing semantics.

## Pictures and scope

Profile/window pictures retain the existing XAML decoder, URI loading, stretch/alignment and animated GIF support. An owned offscreen image visual supplies shared GPU capture; the compiled phosphor shader maps cached pixels. Static capture stops after a frame; animated capture continues while visible presentations need it. Image opacity applies once. Source/layout/decode changes invalidate cached captures.

Window images use an auto-sized centered Image in a sized common host. Each pane crops actual common image coordinates, accounting for letterbox/overflow offsets. Mixed legacy/scheme panes share a source with identity/mapped presentations. Original pixels remain visible until a presentation is ready and return on failure; original pixels are removed beneath ready presentations to avoid double images. Disjoint rectangular clips preserve the original Grid/ImageBrush filtering outside pane apertures.

Image recoloring is tint mapping, not a claim that background pictures participate in Atlas's text CRT blur/scanlines. Pictures already keep Atlas default cells transparent at all opacities, so they do not cross the former opaque/default-background threshold. Existing image and terminal opacity controls remain distinct. Hidden/unloaded presentations release capture/GPU resources and recover on refresh. Bezel, badge, scrollbar, overlays and surrounding UI are outside the terminal-content mapping.

## Validation and limits

Production-shader WARP tests cover CPU transfer agreement, alpha, legacy behavior, scheme light-band ordering and 0/25/50/75/90/100 scanline sweeps. Native Atlas tests cover cell-role assignment, indexed/truecolor/reverse/selection/cursor, opacity, glow, alpha bounds and fractional DPI. Native 100-99-100 sweeps measured a maximum first-tick change of 3/255, with exact round-trip restoration. Channel tests cover fractional arithmetic and every RGB value at alpha 0/255. Settings tests cover defaults, inheritance, reset/copy, focused/unfocused projection and setup transfer.

Owned hidden-XAML/GPU fixtures cover shared animation, PNG embedded alpha, opacity 0/1/50/99/100%, initial failure/retry/post-ready loss/recovery, static layout changes and image crop geometry. Rectangle-mask comparisons against the original brush have zero per-channel error across four stretch modes and five opacity values. Acrylic property checks cover source/luminosity/fallback/opacity combinations; they are property evidence, not desktop pixels.

Actual full-app visual QA, physical monitor DPI transitions, real hardware removal and driver memory/timing measurements remain separate and pending. Synthetic readbacks and source/numeric checks are not screenshots of the application.

## Startup cleanup regression

On the tested Windows XAML runtime, clearing an already-empty UIElementCollection during every LayoutUpdated invalidates layout repeatedly. This caused the first portable candidate to fail at startup with stowed HRESULT 0x88000FA8. Disabled padding and window-image cleanup now clear only nonempty collections. The persistent tools/backslash/Test-XamlCleanup.ps1 runner extracts both actual production cleanup bodies and verifies that an owned, never-visible XAML host settles; reverting the guards fails that regression.

An approved debugger test reproduced the old candidate failure and saved an owned-process dump. Isolated copies of the corrected build opened a real terminal window and exited cleanly with both a default effects-off profile and orange scheme phosphor at 99% acrylic. These smoke tests used temporary settings and cmd.exe /d, not the user's profiles. They verify startup, not complete application color/interaction visual QA.
