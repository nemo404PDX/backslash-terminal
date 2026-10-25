# Terminal status bars design

[Project entry point](README.md) | [Settings layout proposal](SETTINGS-LAYOUT-PROPOSAL.md) | [Bezel design](BEZEL-DESIGN.md) | [CRT appearance](CRT-APPEARANCE.md)

Status: **planned design, not implemented**, 2026-10-15. This document adds no runtime controls or settings keys. Preserve groups and current picker/reset work.

## Requested layout

Optional top and bottom status rows sit **inside the terminal screen/bezel**, visually integrated with the shell. They reserve actual layout space rather than cover terminal output: the top row shifts the terminal content downward; the bottom row reserves space below it. Either row can be disabled independently; disabled rows reclaim their space.

Each row supports at most two selectable items, left and right anchored, for four slots:

| Slot | Placement |
| --- | --- |
| TL | Top left |
| TR | Top right |
| BL | Bottom left |
| BR | Bottom right |

Candidate items are cwd/path, username, hostname, system time and folder item count. Slot contents, empty-slot behavior, defaults and duplicate-item policy remain decisions. This is application chrome styled to fit the terminal, not shell output or an injected prompt.

## Proposed first slice

**Proposal:** start with path, host, user and time; defer folder counts. Support independent row enablement and two slots per row, readable terminal-aligned styling and correct content sizing. Settings scope/inheritance, exact keys, localization and UI placement remain open; link with existing appearance/settings planning without claiming manifest fields exist.

Per-pane rows are a feasible first integration candidate. **Per-pane versus a future shared-bezel status area remains open.** A shared row would need an explicit focused-pane source and transitions, especially for mixed local/SSH/WSL splits. Do not silently display one pane's metadata as belonging to all panes.

## Metadata provenance and availability

Display only known values with explicit provenance/availability. Missing metadata should be unavailable/empty according to a defined UI policy, never fabricated from a title or another pane. Launch identity, profile label and live remote identity are different facts.

- **Path:** distinguish the profile's initial directory seed from an actual cwd report. Cooperative reports are advisory and may become stale. Local client cwd is not remote SSH cwd; WSL paths need their own context rather than Windows-path inference.
- **Host:** existing configured SSH alias/WSL distro/local launch identity can be labeled as such. They do not prove a live remote hostname, especially after nested SSH or shell transitions.
- **User:** local process/account identity does not establish the user inside WSL/SSH, sudo or a nested connection. Launch/configured username is not automatically current effective user. Use explicit known metadata or mark unavailable.
- **Time:** local machine time is a proposed initial source, clearly identified. Local versus remote clock, timezone, format and seconds/update cadence remain open; do not relabel local time as remote.
- **Folder item count:** later proposal only. Define files versus directories/hidden entries, currentness, errors and cost. For verified local context, bounded asynchronous enumeration is a research candidate. SSH/WSL counting needs explicit supported metadata/transport and authorization; no hidden interactive-shell commands or invisible probes. Avoid UI-thread I/O and unbounded network/directory scanning.

Do not query through the interactive shell invisibly, modify remote configuration or parse arbitrary terminal output into trusted metadata. Any later reporting integration is separate work with explicit capability/provenance boundaries.

## Source-backed integration research

Source inspected on 2026-10-15; these are seams, not runtime proof of status-bar feasibility.

- [TerminalPaneContent](../../backslash/src/cascadia/TerminalApp/TerminalPaneContent.cpp#L23) wraps its TermControl in a bezel and exposes root/minimum-size behavior. Investigate a layout container for top row / terminal / bottom row inside the screen aperture; preserve bezel sizing, focus and terminal ownership.
- [TermControl.xaml](../../backslash/src/cascadia/TerminalControl/TermControl.xaml) provides the terminal visual/control tree. Audit actual measured terminal area, renderer/swap-chain resizing, padding, scrollbar and pointer/selection/IME coordinate conversion. Reserving rows must resize the terminal grid/connection correctly, not just draw an overlay.
- [ControlCore initial directory](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp#L115), [WorkingDirectory](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp#L1554) and [ReportedWorkingDirectory](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp#L2482) show that initial and reported locations can be distinguished. Research update notifications and stale-report policy; no universal remote cwd guarantee follows.
- [TermControl metadata accessors](../../backslash/src/cascadia/TerminalControl/TermControl.cpp#L3785) expose launch-title prefix and directory accessors. [Connection identity design](CONNECTION-TITLES.md) documents conservative configured identity and nested-connection limits. Do not treat presentation strings as an authoritative host/user API.
- [Settings proposal](SETTINGS-LAYOUT-PROPOSAL.md), [bezel design](BEZEL-DESIGN.md) and [CRT appearance](CRT-APPEARANCE.md) provide planning context. Status rows require a separately authorized model/editor/rendering slice; none is implemented by this doc.

## Appearance, truncation and accessibility decisions

Choose whether rows share font/color/background and CRT effects with terminal text or use complementary chrome styling. Avoid claiming glow/scanlines apply to XAML text automatically. Evaluate readability, opacity, bezel clipping and high contrast before selecting an effects path; no new shader work is approved.

Long paths/identities require bounded slot widths, nonoverlap and a defined ellipsis strategy. Preserve the full value for tooltip and accessible name; do not persist truncated strings. Consider narrow windows, Unicode/RTL and whether both anchors share available space fairly.

Rows should not steal shell focus or intercept terminal keys. Decide whether items are read-only or provide explicit actions later. Screen readers need labels, provenance/unavailable state and predictable traversal; clock/count updates should not continually announce or disrupt reading. Respect reduced-motion/high-contrast preferences.

Resize must account for row height, DPI, font metrics, terminal minimum sizes and split panes. Confirm top offset affects mouse selection, cursor/IME/caret and scrollbar coordinates consistently. Define behavior when there is insufficient height, without covering output or collapsing the terminal unexpectedly.

## Future acceptance checklist (unrun)

1. Enable neither/top/bottom/both rows; verify at most two items each, left/right anchors, reclaimed disabled space and no terminal output underneath rows.
2. Resize, change DPI/font/opacity, split panes and use fullscreen/bezel modes; terminal grid and connection dimensions remain correct. Test cursor, pointer selection, scrolling and IME/caret alignment.
3. Local, WSL and SSH fixtures show only known path/user/host values with clear configured/live distinctions; missing/stale/nested metadata does not inherit another context.
4. Clock source/timezone is labeled and chosen explicitly; updates remain bounded with no continual accessibility announcements.
5. Long Unicode/RTL values never overlap; full metadata remains available through tooltip/accessibility. Keyboard focus stays in the terminal unless deliberately moved.
6. Verify appearance/effects/high contrast and opacity visually; styling stays inside the screen aperture without covering content.
7. Folder counts remain absent from the first proposed slice. Any later count implementation proves async/resource bounds, context validation and no invisible shell querying.
8. Once settings scope is chosen, test Save/Reset/reload, per-pane/focused-pane source changes and disabled no-update behavior. Metadata and timer cleanup survives pane/window closure.

Open: scope and shared-bezel source, defaults/slot duplication, metadata integrations, clock locality, truncation, effects path, minimum-height policy, item interaction and later folder-count semantics. No production implementation is authorized.
