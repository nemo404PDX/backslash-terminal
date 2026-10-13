# Baseline capability and research record

[Project entry point](README.md) · [Source provenance](../../UPSTREAM-SOURCE.md)

Verified against local stable v1.25.2733.0 source and official sources on 2026-10-13. Runtime behavior has not been tested: no build/install was performed. Current upstream documentation and main-branch issues are research references, not evidence that Backslash additions are shipped.

| Area | Evidence | Implication |
| --- | --- | --- |
| Mixed-shell panes | [Microsoft panes guide](https://learn.microsoft.com/en-us/windows/terminal/panes); [Pane.cpp](../../src/cascadia/TerminalApp/Pane.cpp) | Reuse existing splits and profile launch paths. |
| Layout/startup restoration | [Microsoft startup guide](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/startup); [GlobalAppSettings.cpp](../../src/cascadia/TerminalSettingsModel/GlobalAppSettings.cpp), [MTSMSettings.h](../../src/cascadia/TerminalSettingsModel/MTSMSettings.h), [TerminalWindow.cpp](../../src/cascadia/TerminalApp/TerminalWindow.cpp), [TerminalPage.cpp](../../src/cascadia/TerminalApp/TerminalPage.cpp) | startupActions and persisted layout machinery exist. This baseline also marks legacy persistedWindowLayout spelling for fixup; use actual baseline settings behavior rather than copying old docs blindly. Restore layout is not running-process persistence. |
| Pinned tabs | [Draft PR #20776](https://github.com/microsoft/terminal/pull/20776) | Draft upstream work to track/review; no shipped claim or code incorporation. |
| Tab groups | [Open issue #8267](https://github.com/microsoft/terminal/issues/8267) | Feature request to track; design work remains. |
| Sixel inline output | [SixelParser.cpp](../../src/terminal/adapter/SixelParser.cpp); [upstream 1.22 release](https://github.com/microsoft/terminal/releases/tag/v1.22.10352.0) | Sixel arrived in the 1.22 series and is in this baseline; prioritize reuse. |
| Kitty graphics | [Open issue #8389](https://github.com/microsoft/terminal/issues/8389) | Separate protocol request; do not claim support from the issue. |
| Retro/shaders/backgrounds | [Microsoft appearance guide](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-appearance); [IAppearanceConfig.idl](../../src/cascadia/TerminalSettingsModel/IAppearanceConfig.idl), [MTSMSettings.h](../../src/cascadia/TerminalSettingsModel/MTSMSettings.h), [BackendD3D.cpp](../../src/renderer/atlas/BackendD3D.cpp) | Existing hooks are a starting point, not a complete configurable CRT system. |
| Persisted ANSI scheme | [ColorScheme.cpp](../../src/cascadia/TerminalSettingsModel/ColorScheme.cpp) | 16 ANSI entries plus foreground/background/cursor/selection roles. Purple maps to ANSI magenta; aliases exist. |
| Extended palette | [colorTable.cpp](../../src/types/colorTable.cpp) | Already builds indexed cube/grayscale entries; add persistence/editor semantics rather than new rendering. |
| Live palette changes/reset | [OutputStateMachineEngine.cpp](../../src/terminal/parser/OutputStateMachineEngine.cpp), [adaptDispatch.cpp](../../src/terminal/adapter/adaptDispatch.cpp) | Existing OSC set/reset paths need compatibility coverage. |
| Other palette models | [Kitty configuration](https://sw.kovidgoyal.net/kitty/conf/#color-scheme), [Konsole scheme source](https://github.com/KDE/konsole/blob/master/src/colorscheme/ColorScheme.cpp) | Different slot/role systems; no universal Unix palette. |
| CRT inspiration | [cool-retro-term source](https://github.com/Swordfish90/cool-retro-term) | Inspiration only; review licenses before code/assets reuse. |

Recheck upstream issue/PR state before future implementation. Full settings/model/renderer call-path audit, runtime compatibility checks, release prerequisite validation and benchmarking remain future work.


## SSH and command-editor architecture evidence

See [shell design protocol research](SHELL-PRODUCTIVITY.md) for RFC4254 shell/exec status limits, pinned OSC133 A/B/C/D dispatch and optional-status inspection, local/remote completion context, and Kitty bootstrap provenance. These findings do not authorize host queries or integration installation.

Detailed source findings and the preferred local-compose rendering lifecycle are in [SSH local editor research](SSH-LOCAL-EDITOR-RESEARCH.md). Prior overlay/pill exploration is superseded; it is not active roadmap scope.
