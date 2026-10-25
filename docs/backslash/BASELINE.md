# Baseline capability and research record

[Project entry point](README.md) · [Source provenance](UPSTREAM-SOURCE.md)

Verified against local stable v1.25.2733.0 source and official sources on 2026-10-13. Runtime behavior has not been tested: no build/install was performed. Current upstream documentation and main-branch issues are research references, not evidence that Backslash additions are shipped.

| Area | Evidence | Implication |
| --- | --- | --- |
| Mixed-shell panes | [Microsoft panes guide](https://learn.microsoft.com/en-us/windows/terminal/panes); [Pane.cpp](../../backslash/src/cascadia/TerminalApp/Pane.cpp) | Reuse existing splits and profile launch paths. |
| Layout/startup restoration | [Microsoft startup guide](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/startup); [GlobalAppSettings.cpp](../../backslash/src/cascadia/TerminalSettingsModel/GlobalAppSettings.cpp), [MTSMSettings.h](../../backslash/src/cascadia/TerminalSettingsModel/MTSMSettings.h), [TerminalWindow.cpp](../../backslash/src/cascadia/TerminalApp/TerminalWindow.cpp), [TerminalPage.cpp](../../backslash/src/cascadia/TerminalApp/TerminalPage.cpp) | startupActions and persisted layout machinery exist. This baseline also marks legacy persistedWindowLayout spelling for fixup; use actual baseline settings behavior rather than copying old docs blindly. Restore layout is not running-process persistence. |
| Pinned tabs | [Draft PR #20776](https://github.com/microsoft/terminal/pull/20776) | Draft upstream work to track/review; no shipped claim or code incorporation. |
| Tab groups | [Open issue #8267](https://github.com/microsoft/terminal/issues/8267) | Feature request to track; design work remains. |
| Sixel inline output | [SixelParser.cpp](../../backslash/src/terminal/adapter/SixelParser.cpp); [upstream 1.22 release](https://github.com/microsoft/terminal/releases/tag/v1.22.10352.0) | Sixel arrived in the 1.22 series and is in this baseline; prioritize reuse. |
| Kitty graphics | [Open issue #8389](https://github.com/microsoft/terminal/issues/8389) | Separate protocol request; do not claim support from the issue. |
| Retro/shaders/backgrounds | [Microsoft appearance guide](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-appearance); [IAppearanceConfig.idl](../../backslash/src/cascadia/TerminalSettingsModel/IAppearanceConfig.idl), [MTSMSettings.h](../../backslash/src/cascadia/TerminalSettingsModel/MTSMSettings.h), [BackendD3D.cpp](../../backslash/src/renderer/atlas/BackendD3D.cpp) | Existing hooks are a starting point, not a complete configurable CRT system. |
| Persisted ANSI scheme | [ColorScheme.cpp](../../backslash/src/cascadia/TerminalSettingsModel/ColorScheme.cpp) | 16 ANSI entries plus foreground/background/cursor/selection roles. Purple maps to ANSI magenta; aliases exist. |
| Extended palette | [colorTable.cpp](../../backslash/src/types/colorTable.cpp) | Already builds indexed cube/grayscale entries; add persistence/editor semantics rather than new rendering. |
| Live palette changes/reset | [OutputStateMachineEngine.cpp](../../backslash/src/terminal/parser/OutputStateMachineEngine.cpp), [adaptDispatch.cpp](../../backslash/src/terminal/adapter/adaptDispatch.cpp) | Existing OSC set/reset paths need compatibility coverage. |
| Other palette models | [Kitty configuration](https://sw.kovidgoyal.net/kitty/conf/#color-scheme), [Konsole scheme source](https://github.com/KDE/konsole/blob/master/src/colorscheme/ColorScheme.cpp) | Different slot/role systems; no universal Unix palette. |
| CRT inspiration | [cool-retro-term source](https://github.com/Swordfish90/cool-retro-term) | Inspiration only; review licenses before code/assets reuse. |

Recheck upstream issue/PR state before future implementation. Full settings/model/renderer call-path audit, runtime compatibility checks, release prerequisite validation and benchmarking remain future work.


## SSH and command-editor architecture evidence

See [shell design protocol research](SHELL-PRODUCTIVITY.md) for RFC4254 shell/exec status limits, pinned OSC133 A/B/C/D dispatch and optional-status inspection, local/remote completion context, and Kitty bootstrap provenance. These findings do not authorize host queries or integration installation.

Detailed source findings and the preferred local-compose rendering lifecycle are in [SSH local editor research](SSH-LOCAL-EDITOR-RESEARCH.md). Prior overlay/pill exploration is superseded; it is not active roadmap scope.

## Pinned samples and production reuse map

Source inspection only; none of these fixtures was run. Samples show reusable mechanisms, not finished Backslash controls or runtime certification.

| Actual path/symbol | Existing behavior | Reuse and limits |
| --- | --- | --- |
| [samples/PixelShaders/Retro.hlsl](../../backslash/samples/PixelShaders/Retro.hlsl): Gaussian2D, Blur, SquareWave, Scanline, main | 13×13 sampled blur/glow; hardcoded 0.3 glow gain, sigma 2×scale and 0.5 scanline factor; period scales with DPI | Parameterize existing glow/scanlines first, not a new glow renderer. |
| [atlas custom_shader_ps.hlsl](../../backslash/src/renderer/atlas/custom_shader_ps.hlsl): same functions | Production built-in retro shader implements the sampled glow/scanlines | Existing implementation to preserve/extend under explicit controls. Simple glow is single-pass; higher-quality bloom might warrant more passes. |
| [BackendD3D.cpp](../../backslash/src/renderer/atlas/BackendD3D.cpp): _recreateCustomShader, _executeCustomShader, RequiresContinuousRedraw | Loads/compiles shader, uploads constants/textures and executes the production hook | Existing shader ABI/settings/precedence and error path need compatibility checks before new parameters. |
| [Animate_scan.hlsl](../../backslash/samples/PixelShaders/Animate_scan.hlsl), [Animate_breathe.hlsl](../../backslash/samples/PixelShaders/Animate_breathe.hlsl): main | Moving inverted band; cosine background fade | Time-animation fixtures, not phosphor afterimage/history persistence. |
| [Nop.hlsl](../../backslash/samples/PixelShaders/Nop.hlsl), [Invert.hlsl](../../backslash/samples/PixelShaders/Invert.hlsl), [Grayscale.hlsl](../../backslash/samples/PixelShaders/Grayscale.hlsl): main | Neutral texture output, RGB inversion, arithmetic RGB average | Neutral/comparison fixtures; average grayscale is not perceptual luminance. |
| [Outlines.hlsl](../../backslash/samples/PixelShaders/Outlines.hlsl), [Rasterbars.hlsl](../../backslash/samples/PixelShaders/Rasterbars.hlsl): main | Neighbor-alpha sampling; static color bars/drop shadow | Composition examples, not temporal persistence or finished bezel geometry. |
| [BackgroundImage.hlsl](../../backslash/samples/PixelShaders/BackgroundImage.hlsl): t0/t1, main | Terminal t0 plus image t1 alpha composition | Background-texture example; not inline output graphics, a full bezel, or warped input alignment. |
| [Broken.hlsl](../../backslash/samples/PixelShaders/Broken.hlsl), [Error.hlsl](../../backslash/samples/PixelShaders/Error.hlsl) | Invalid HLSL compile fixture; visible error-stripe fixture | Diagnostic examples, not evidence that production automatically loads Error.hlsl as fallback. |
| [EchoCon.cpp](../../backslash/samples/ConPTY/EchoCon/EchoCon/EchoCon.cpp): CreatePseudoConsoleAndPipes | Native pipes/pseudoconsole/process walkthrough; launches ping localhost | Does not demonstrate an SSH editor or persistent-shell ownership. |
| [MiniTerm Terminal.cs](../../backslash/samples/ConPTY/MiniTerm/MiniTerm/Terminal.cs): ForwardCtrlC | Managed input/output and Ctrl-C forwarding demo | Not production resizing/full keyboard/IME or local composition. |
| [GUIConsole MainWindow.xaml.cs](../../backslash/samples/ConPTY/GUIConsole/GUIConsole.WPF/MainWindow.xaml.cs): CopyConsoleToWindow, Window_KeyDown | Raw VT text appended to TextBlock; key name strings sent | Explicit skeleton, not terminal parsing/rendering or a reusable complete editor. |
| [ReadConsoleInputStream.cs](../../backslash/samples/ReadConsoleInputStream/ReadConsoleInputStream.cs) | Console INPUT_RECORD/ReadConsoleInputW handling | Useful console event example; not a TSF/IME solution for the Backslash control. |
| [ColorTool](../../backslash/src/tools/ColorTool/README.md), [ansi-color](../../backslash/src/tools/ansi-color/README.md) | Scheme parsing/conversion and SGR/color table diagnostics | Palette fixtures, not the proposed palette generation algorithm. |
| [vtapp](../../backslash/src/tools/vtapp/Program.cs), [test-unicode.py](../../backslash/src/tools/vttests/test-unicode.py) | Terminal/Unicode output examples | Useful font checks; not certification of version-specific ligature/patched glyph coverage. |
| [SixelParser.cpp](../../backslash/src/terminal/adapter/SixelParser.cpp); [BackendD3D.cpp](../../backslash/src/renderer/atlas/BackendD3D.cpp) and [BackendD2D.cpp](../../backslash/src/renderer/atlas/BackendD2D.cpp): _drawBitmap | Inline image parser and atlas bitmap rendering | Reuse actual output-content path; _drawBitmap is a renderer symbol, not a SixelParser method. |

[Shader sample README](../../backslash/samples/PixelShaders/README.md) describes Time as seconds since enabling. The inspected atlas runtime instead derives it from queryPerfCount modulo frequency×1000: a wrapping roughly 1000-second time value, not a reset-on-enable clock. Shader reflection checks use of the first time constant and sets continuous-redraw need; if reflection cannot be created it conservatively requests continuous redraw. Tests must cover wrap/discontinuity, reduced motion and idle redraw, not rely on sample wording alone.

Existing OSC633 completion flow remains DoVsCodeAction → Terminal::InvokeCompletions → ControlCore completion event → TerminalPage::_ControlCompletionsChangedHandler/_OpenSuggestions → Command::ParsePowerShellMenuComplete → DEL+text input. It is not a client-owned editor. Marked scrollback extraction is not persistent command storage. Production SshHostGenerator/ConptyConnection remain the initial SSH transport seams; ConPTY samples do not weaken the [independent audit gates](SSH-LOCAL-EDITOR-AUDIT.md).
