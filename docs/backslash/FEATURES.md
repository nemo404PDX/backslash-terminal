# Feature scope

[Project entry point](README.md) · [Roadmap](ROADMAP.md)

All Backslash additions below are requested or proposed, not implemented.

## Requested: named saved workspaces

Save named tab/pane layouts mixing WSL, PowerShell, Visual Studio developer shells and SSH. Make them understandable to reopen and edit, including a layout with an agent terminal on one side and an editor, preview or build output elsewhere. Include pinned tabs and useful tab grouping.

Reuse upstream splits and layout/startup actions; do not confuse reopening a layout with keeping processes running. A newly launched shell does not restore process memory, an SSH connection or an editor's unsaved buffers. Persistent detach/reattach is a separate architecture decision.

Acceptance direction: save and reopen a mixed-shell layout; preserve pane geometry and profile references; identify missing profiles, unavailable hosts and machine-specific paths clearly. Pin/group operations should work with keyboard navigation and restored layouts. Exact workspace schema, naming rules, portable path handling and pin/group interactions remain open.

## Requested: SSH bookmarks

Provide a friendly saved SSH catalog inspired by the user's Panic Prompt experience: name, host, user, port, key reference and useful groups. Integrate bookmark launch with panes and workspaces. Decide whether to reuse OpenSSH configuration aliases or store normalized connection metadata, and specify import/export behavior before implementation.

Proposed safeguards: never export SSH passwords or private-key bytes in workspace/bookmark files; store references instead. Show local/WSL/remote machine identity clearly in each pane. Handle key-reference portability and host-key prompts through the SSH client rather than bypassing them. Confirmation policy for executing saved commands, metadata privacy and credential integration remain open.

## Requested: palettes and profiles

Shared palette editing and per-profile choices must be understandable. [Palette design](PALETTE.md) defines agreed independent copy/fill behavior. Renderer features must not be restricted merely because a profile runs Windows, WSL or SSH.

## Requested: inline images

Images must be terminal output content, not only backgrounds. Reuse Sixel support first; choose an initial demo and verify scrollback, resize, alternate-screen behavior, clipping and resource limits. Kitty graphics is a separate proposed protocol expansion, not present because its upstream issue exists. Protocol selection, remote transport limitations and application compatibility remain open.

## Requested: CRT Appearance controls

Add a configurable profile Appearance section with sliders/fields, live preview, saved controls, presets and fast reset/comparison. Include intentional glitch/effects, old-screen overlay/frame treatment, and actual terminal-view curvature/warping alongside background image/transparency. This scope goes beyond the upstream retro toggle. See [CRT design](CRT-APPEARANCE.md).

## Requested: curated terminal fonts

Add a Get more fonts modal/catalog in profile font/Appearance settings, prioritizing labeled ligature support and separately verified Nerd Font/Powerline coverage. Catalog entries link official sources and metadata; explicit Install fetches fonts. No fonts are bundled, downloaded or installed now. See [font catalog design](FONTS.md) for representative code/prompt previews, proposed install/apply UX, integrity/license requirements and open installation scope.

## Requested: optional shell productivity setup

Offer explicit opt-in setup for command suggestions, completion and searchable history adapted to the actual shell and Windows/WSL/SSH target. The user's preferred successful-only suggestion mode includes only confirmed-successful executions and excludes failed/unknown outcomes. History/suggestion behavior must be configurable; storage and suggestion-source controls are proposed independent choices. Older successful matches remain accessible after a newer typo fails. In the requested shared-persistent storage mode, history is shared across tabs/windows and app restarts, updated as commands complete rather than only at shell exit. Default autosuggestions respect verified target machine/connection, user and compatible shell; cross-connection sharing is explicit opt-in. Cross-directory reuse within that scope and deliberate origin-labeled All connections search remain available; no exact retention duration is selected. Full searchable history retains per-execution success/failure/unknown markers and All/Successful/Failed/Unknown filters. Suggestion exclusion does not silently delete history. Preserve normal local shell-native suggestions; client-owned inline editing/completion over SSH2 is the preferred direction to investigate without per-host suggestion-plugin setup. Compose locally and submit on Enter; preserve native providers as a choice. The pill proposal is retired, and feasibility must be disclosed rather than silently substituting it. SSH recall must offer a local connection-scoped browser without remote autosuggestion plugins, with manual Unknown entries and safe copy-only fallback when input context is uncertain. See [shell productivity design](SHELL-PRODUCTIVITY.md) for strict eligibility fixtures, custom-engine versus provider options, setup targets and privacy.

## Proposed: editor integration and safe file updates

Reuse an existing terminal editor initially; do not assume a new built-in editor. Treat a rich embedded editor/preview pane as a later integration decision. If an editor or preview observes external file changes, reload automatically only when the buffer is unmodified; otherwise present conflict/diff choices without overwriting edits.

## Open choices

Workspace storage/versioning, SSH config interoperability, editor/preview integration, pin/group UX, graphics protocol expansion and persistent session architecture need explicit decisions. Accessibility, identity and compatibility should be checked in each small feature slice.

