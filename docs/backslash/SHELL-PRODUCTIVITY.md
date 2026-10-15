# Optional shell productivity setup

[Project entry point](README.md) · [Feature scope](FEATURES.md) · [Roadmap](ROADMAP.md)

## Requested behavior

Provide optional download/install setup for typing suggestions accepted with Tab/right-arrow where supported, previous-command suggestions and searchable history. In the user's preferred successful-only mode, automatic history suggestions must include ONLY previously confirmed successful command executions. Failed/nonexistent commands and unknown outcomes are excluded by default. A recent typo that fails must not suppress access to older confirmed-successful matches.

Browse multiple matching history entries, retain older matches, and explicitly dismiss/exclude a bad suggestion without silently erasing history. Keep suggestion exclusion separate from history deletion, with understandable persistence and undo/reset behavior to design. Keybindings must be configurable and match the actual shell's capabilities; Tab completion and right-arrow acceptance may conflict with existing bindings.

## Agreed: progressive successful-history suggestions

This is intended behavior, not an implemented autocomplete feature. Successful command matches persist across sessions and restarts within the approved verified host/connection, user and compatible-shell boundary. Directory changes within that boundary do not hide eligible older matches. Broader cross-connection search/reuse remains explicit and origin-labeled.

The query is the actual text the user has typed, including arguments. Every additional character progressively narrows eligible prefix matches; backspacing broadens them again. Displayed ghost text is not part of that query until accepted. For example, `git ` can match several successful commands; typing `git commit -` narrows to successful executions with that full prefix rather than continuing to suggest unrelated `git` subcommands.

Offer the most recently confirmed-successful matching execution first. Configurable modified-arrow bindings cycle through all eligible matches while preserving the user's actual typed prefix and query; cycling changes only the offered ghost suffix, not the prefix or eligibility. Typing narrows the candidate set, and backspacing broadens it. Right arrow at the end of the editing buffer accepts the remaining suffix. Escape dismisses the suggestion without changing the typed buffer; acceptance does not execute, and Enter executes/submits the reviewed command once. Refresh or reject stale candidates whenever the actual buffer, cursor, target or input mode changes. Existing text after the cursor requires the current-buffer edit rules below, not blind suffix concatenation.

A newer failed execution must never displace an older working match. Eligibility and recency come from confirmed successful executions; a later failure of the same command text does not remove its earlier eligible success or make the failure count as a new successful recency event. Keep failures searchable in full history, with per-execution status. Unknown, incomplete and unavailable outcomes remain distinct from success and are excluded from success-only suggestions.

Ordinary Up/Down retains the shell's history and multiline navigation. Suggestion cycling uses separate configurable modified-arrow bindings in an integrated, verified command-editing mode. Alt+Up/Down is only a candidate pending an audit of shell, terminal, application and accessibility conflicts; no default binding is selected here. Shift+arrow combinations may conflict with selection and must not be assigned without the same audit. Acceptance and dismissal bindings also require provider-compatible ownership checks. Never steal keys from password prompts, interactive/full-screen applications, selection, unverified command editing or pass-through input modes. Preserve their native input behavior and use the safe manual browser/copy fallback when ownership or buffer state is unknown. No autocomplete code, provider installation, shell hook or host change is authorized by this documentation update.

Acceptance fixtures:

1. An older successful `git commit -m "working"` survives restart and appears from another eligible session. A newer failed `git commti` does not replace it.
2. `git ` offers the newest eligible matching success; adding `commit -` narrows the full command/argument prefix. Deleting those characters restores the broader candidate set.
3. Ghost suffix changes and configured modified-arrow cycling leave the actual typed prefix unchanged. A match visible only because ghost text was mistakenly added to the query is a defect.
4. Right arrow at the end accepts only the missing current suffix; Escape dismisses without editing the buffer, and Enter submits once. Cycling/highlighting alone neither edits the typed prefix nor executes.
5. A command text with older success and newer failure remains eligible from the successful execution, ordered by successful recency. All/Failed history still shows the failure; Unknown remains a separate filter/status.
6. Host/user/shell and input-mode changes invalidate candidates. Cross-directory recall inside the approved scope works; password/full-screen/unknown modes retain safe native input or copy-only fallback.
7. Ordinary Up/Down retains native history/multiline behavior. Cycling bindings remain configurable and conflict-audited; Alt+Up/Down is a candidate only, and Shift+arrow selection is preserved. Other application/input modes never receive suggestion interception.

## Shell/session boundary

Completion, autosuggestions and history providers belong to the shell or its integration, while the terminal displays their output and delivers input. Adapt setup to the actual shell/session; do not promise identical tools or key behavior in every profile.

Candidates for later research include PowerShell PSReadLine predictors/history, fish autosuggestions, zsh autosuggestions and optional history tools. These are candidates, not selected integrations, and their exact versions, licensing, capabilities and setup commands have not been validated here. No packages have been downloaded/installed and no shell configuration has been changed.

Integrating existing providers first is recommended. Whether to add a new cross-shell history/suggestion service is open and requires a separate architecture decision, especially for shell hooks, storage, remote sessions and keybinding ownership.

## Explicit opt-in setup

Before running anything, show intended provider/version, official source, dependencies, installation target and proposed configuration changes. Preserve user edits and existing configuration, detect conflicts and provide rollback/uninstall design. Obtain explicit opt-in for concrete steps; showing a setup option is not permission to install.

Distinguish local Windows, a named WSL distribution and a remote SSH host. The selected terminal profile alone is insufficient evidence of the target's package manager or filesystem. Never silently install on a remote host. Remote setup requires an explicit target and authorization.

## History quality and privacy

History is local-only by default. Keep full diagnostic history separate from suggestion eligibility: in successful-only mode, diagnostic retention of failures must never leak those entries back into automatic suggestions. Do not transmit command history to remote services or synchronize across targets silently. Define secret detection/redaction, retention, storage permissions, export behavior and destructive-command safeguards before implementation.

In successful-only mode, confirmed success is a hard eligibility filter, not a ranking signal. Rank only eligible successful executions, using recency/frequency as possible signals. Require reliable shell-native completion/exit status associated with the actual command execution and session, not terminal-text heuristics. Imported history without status, running/incomplete executions, and unavailable status are unknown and excluded by default. Nonzero exit does not prove a typo; nevertheless failed executions are excluded from automatic history suggestions in successful-only mode. Exit 0 does not prove semantic correctness, desirability or safety. Users can dismiss unwanted successful entries. Preserve access to older eligible matches regardless of ranking. A suggestion should not execute merely because it is highlighted or accepted into the editing buffer.

Open: provider-specific strict eligibility support, execution/status correlation, indexed search, ranking transparency, multiline commands, duplicates, sensitive-history handling, unsupported sessions, keybinding collisions and opt-in local/remote setup boundaries. If a provider cannot support a requested behavior, expose that limitation rather than imply the feature is universally available.

## Successful-only mode acceptance examples

1. Fixture: older execution `git status` is confirmed successful (exit 0); newer `git statsu` fails (nonzero). Typing `git sta` offers `git status`; `git statsu` is absent from automatic candidates even if more recent.
2. Fixture: several same-prefix successful commands exist plus failed and imported status-unknown entries. All eligible successful matches remain browseable; failed and unknown entries never appear in automatic suggestions. Diagnostic history may still expose those failures separately.
3. Explicitly dismiss an unwanted successful candidate. It is excluded from suggestions without silently deleting diagnostic history; restore controls remain a design question. Acceptance edits the command buffer rather than silently executing it.
4. A legitimate nonzero tool result remains in diagnostic history, but is excluded from automatic suggestions by default. A successful destructive command is not intrinsically safe and may be dismissed; define safeguards before implementation.
5. Setup previews the exact Windows/WSL/SSH target and preserves existing configuration. Cancel performs no install/config change; unsupported/offline/error states do not damage profiles. A provider that cannot enforce confirmed-success-only eligibility must not be presented as satisfying the requirement.
6. Command history stays within approved storage/target boundaries; no silent uploads or cross-target transfer occur.

## Reliable success semantics: design work required

Bind status to a completed command execution using reliable native shell integration, including shell/session identity. Define how pipelines (last-stage versus pipefail/all-stage status), compound commands, background/long-running jobs, interruptions/cancellation, disconnected SSH sessions and tools with meaningful nonzero statuses are represented. These are open semantics, not permission to weaken the default: in successful-only mode, only reliably confirmed successful executions qualify; failures and unknown/incomplete outcomes do not. A previous successful execution of the same command text may remain eligible even if a later execution fails, provided its eligibility comes from the earlier confirmed-success execution.


## Open option: a Backslash command-suggestion engine

A custom IntelliSense/autocomplete-like engine remains an explicit option alongside integrating existing providers. Use “command suggestions” as the umbrella, while keeping history suggestions, command/path/argument completion, and richer command-aware help separate capabilities. None inherently requires AI.

A proposed direction for evaluation is a Backslash history-first engine with shell-specific adapters. The user's preferred SSH direction is client-owned normal inline LOCAL editing/completion, with native shell behavior supported; prior pill/overlay exploration is superseded; local Windows custom UI remains undecided. This is a proposal, not an accepted architecture. A bounded history-ranking prototype could be small; robust cross-shell UI and integration is a substantially larger effort.

Terminal byte streams alone do not reliably expose the editable command buffer, shell history or trustworthy exit status, especially over SSH. Prove adapter integration, input ownership, command-buffer synchronization and keybinding conflicts with a small proof of concept before promising a universal completion UI. Account for multiline editing, interactive full-screen programs and unsupported/remote shells. Decide integrate-versus-own-engine only after comparing capabilities, maintenance, privacy and interaction correctness.

## Requested: full history status markers and filters

Provide a searchable history panel with per-execution success/failure markers (icon and/or line color), All/Successful/Failed filters, and an explicit Unknown category/filter for unrecorded outcomes. Keep failures inspectable rather than deleting them. Exact visual layout is not selected; marker and filtering behavior is requested. Optional exit-code detail is proposed.

Provide icon plus text/label accessibility so status does not rely on palette color alone. Keep full history display separate from the selected automatic suggestion mode's eligibility; successful-only mode retains strict success-only eligibility. Unknown is not success.

Repeated identical command text can have different outcomes on different executions. Preserve per-execution records with reliable outcome association; never collapse success/failure globally by command string. Derive suggestion candidates from confirmed successful executions, with grouping/deduplication presentation to design.

Successful-only acceptance fixtures: run the same command text once successfully, once with failure and once without recorded status. All shows three execution records with accurate distinct markers; Successful, Failed and Unknown each return the matching record. In successful-only mode, Failed/Unknown do not enter autosuggestions, while the successful execution may supply an eligible candidate. Search works within each filter. Status remains understandable without color, and filtering does not delete records.

## Requested: persistent shared history across sessions

History must be shared across Backslash tabs/windows and survive application restarts. Commands from other sessions should become available promptly as executions complete; history must not remain siloed per terminal session or merge only when a shell exits.

A proposed implementation is a local central history store with concurrent-safe ingestion as commands complete, stable per-execution IDs to prevent duplicate ingestion, safe status updates and promptly refreshed window views. This is a design proposal, not a selected storage engine/schema. Shared persistent history data is separate from settings: settings control history behavior and the UI browses the history store.

Preserve session, shell, host/SSH target and working directory/project provenance where reliably available. Session/host/shell/project/directory metadata are grouping and filtering aids, not blanket exclusions from discovery. Explicit cross-directory and all-history search/reuse must reach relevant commands from other directory trees. Autosuggestion defaults respect the agreed verified machine/connection, user and compatible-shell boundary described below; offer explicit opt-in cross-connection sharing with visible origin and the selected eligibility filter. Context grouping must never block an explicit broader search. Clearly show environment differences before reuse rather than silently treating a foreign command as compatible.

Keep strict confirmed-success eligibility when successful-only mode is selected. Running, interrupted, crashed and unknown executions must never become successful implicitly. Importing old shell history does not establish success unless reliable outcome metadata is available. Identical command text can differ in outcome and context across hosts; retain execution records rather than assign one global status to the string.

Remote capture requires actual shell integration and a user-visible scope. Do not claim all SSH commands can be discovered from terminal output text. The shared store is local by default; no cloud synchronization is needed or authorized. Define privacy opt-out/private sessions, retention, storage permissions, export and clean uninstall/retention choices. Secret capture/redaction is not a guaranteed safeguard until tested against the actual shell handling; avoid promising complete secret exclusion.

Shared-persistent storage with successful-only suggestions acceptance fixtures: concurrent tabs/windows ingest completed commands without lost records or duplicate IDs; an older successful command from another eligible session becomes browseable promptly without waiting for shell exit; views refresh safely while queries/filters run; records and provenance survive restart; interrupted/crashed/unknown executions remain ineligible; the same text with success on one host and failure on another retains separate outcomes and respects suggestion scope; imported unknown history stays out of autosuggestions; private-session/opt-out and retention/uninstall behavior follow the selected policy. Define crash recovery, status transitions, locking/transactions and refresh latency before implementation.

## Requested: cross-directory discovery and older history

Commands must remain discoverable and reusable across directory trees, and persist into tomorrow and subsequent restarts. Directory/session grouping should help navigation rather than hide relevant commands. An explicit broader search overrides context narrowing; broader automatic suggestions, when enabled, show their source context and obey the selected eligibility mode; successful-only still excludes failed/unknown executions.

Sensible date grouping is proposed for browsing older history. Configurable retention and explicit archive/export options are open design choices; no exact duration has been selected. The user's browser-history comparison does not establish a fixed 30-day policy. Define archive discovery/search, export privacy and reversible retention controls before implementation. Do not silently perform irreversible pruning or make older records undiscoverable by default.

Acceptance examples: a successful command executed under directory tree A is found through explicit search while working in tree B, with its origin shown; the command remains accessible tomorrow after restart; context filters can be cleared deliberately; an enabled broader suggestion mode shows origin and follows the selected eligibility filter; successful-only includes only confirmed-successful executions. Test date grouping and any selected archive/retention behavior, including access to older records, clear policy changes and no silent irreversible pruning.
## Requested: configurability without imposing one history preference

History behavior must be selectable so other users are not forced into the original user's preference. The original user's preferred mode is successful-only suggestions with Backslash shared persistent history; its strict confirmed-success filter remains mandatory whenever that suggestion mode is selected. Failed and unknown executions are excluded there. This preference is not an unchangeable universal restriction.

A proposed UI model has two independent controls; labels/options are design proposals, not a selected schema:

- History storage: Shell-managed / Backslash shared persistent / Backslash session-only.
- Suggestion source: Successful-only / All recorded / Off.

All recorded intentionally permits broader candidates from recorded history, including failures/unknown outcomes, with outcome/origin visible and applicable safeguards; it must not be mislabeled successful-only. Off disables Backslash suggestions, without claiming it disables a shell's own providers. Which combinations are available and how shell-managed imports obtain reliable status need design; absent status remains unknown and ineligible in successful-only mode.

Shell-native history persists independently. Disabling Backslash capture does not disable shell history, third-party providers or imply privacy. Shell append/overwrite/save-on-exit/incremental history options differ by shell; do not present them as a universal “rewrite history” switch.

Mode changes must not silently delete existing data. Retention changes, explicit purge and archive/export are separate actions with their own clear scope. Define capture behavior, query access to previously stored data and migration on storage mode changes without implicit erasure.

Acceptance: switch modes without losing existing execution records; successful-only excludes failed/unknown, All recorded visibly distinguishes their outcomes, and Off disables Backslash suggestions. Shell-managed/session-only modes do not promise shared persistent Backslash capture. Existing shell history/provider behavior remains separately explained; purge/retention never run as a side effect of changing a mode.

## Agreed: machine/connection boundary for suggestions

By default, autosuggestions are scoped to the verified target machine/connection, user and compatible shell. Remote commands must not automatically become suggestions on another machine. Directory grouping is distinct from this boundary: cross-directory reuse within the eligible machine/user/shell scope remains available.

Cross-connection suggestion sharing requires explicit opt-in, preserves the selected successful-only/All recorded eligibility mode, and labels origin. Full-history All connections search is a deliberate broader action with clearly labeled origin; discovery does not silently enable automatic cross-machine suggestions. Keep manual reuse distinct from automatic scope.

Nested SSH, WSL and containers can change the actual execution target. Use conservative provenance when target identity is unknown; do not infer trusted host identity from prompt text. Require reliable integration/connection metadata before mixing an execution into a verified target's autosuggestion scope. Exact scope keys, aliases, host identity changes and nested-target behavior remain implementation design questions.

Acceptance fixtures: a remote-host command is not automatically suggested locally or on another host; commands from another directory on the same verified target/user/compatible shell remain discoverable and eligible; All connections search explicitly exposes origin-labeled records; cross-connection sharing is disabled until opted in; unknown/nested target executions are not silently attributed to the outer/local host.

## Requested: SSH recall without remote autosuggestion plugins

Backslash should retain access to previous commands associated with SSH targets even when the remote machine has no autosuggestion plugin. Provide a local connection-scoped saved-command/history browser independent of remote suggestion plugins. This local Backslash data is distinct from remote shell-owned history and retains the default verified machine/connection, user and compatible-shell boundaries.

A selected command can be copied or intentionally inserted into an identified active remote command line for review. Selection/insertion must never automatically execute it. Guard insertion into password prompts, full-screen applications and unknown input contexts; use a safe copy-only fallback when a shell prompt/editing context cannot be identified reliably. Exact context identification and insertion handshake require design/testing; a plausible-looking prompt alone is not trusted identity or input-context evidence.

When the remote shell owns editing, reliable automatic capture of its edited/executed commands requires shell integration. A client-owned editor could know the text it intentionally submits, but reliable confirmed completion/exit status remains a separate integration problem. Without it, offer manually saved or explicitly recorded commands labeled Unknown/unverified; these can be browsed/copied but are excluded from successful-only autosuggestions. Do not claim raw keystroke capture reconstructs executed commands: line editing, pasted text, cancellation and application input make that unreliable. Remembering a manual entry is not proof it ran successfully.

A proposed richer tier is lightweight, explicitly opt-in remote shell integration. Preview its target, source, dependencies and shell configuration changes; never silently modify/install on a host. Inline ghost suggestions require an explicit input-ownership choice: coordinate remote-shell editing through integration, or evaluate a client-owned editor with proper handoff. SSH does not inherently require an overlay. The fallback browser alone does not establish ghost suggestions or confirmed success; a client-owned editor is a separate candidate, and success evidence remains separately required.

Acceptance: an SSH host with no suggestion plugin supports connection-scoped manual recall/copy; saved entries retain target/user origin and Unknown status; successful-only excludes them; explicit insertion occurs only in a reliably identified command line and never executes; password/full-screen/unknown contexts remain copy-only; no fallback operation alters the remote host or claims access to remote shell history; opt-in integration is separately scoped and reviewed.

## Current preferred SSH presentation

The user now prefers normal inline LOCAL command composition: typing/editing/completion and inline multiline preview stay local; Enter submits to the remote shell. As composition expands, shift visible content upward locally without popup, fake remote scrolling or suggestion-only remote PTY row changes. This differs from ordinary SSH bytewise input. The former pill/centered expansion discussion is superseded and retired from the active roadmap. Preserve native local/remote shell behavior as a supported choice; local Windows custom UI remains undecided.

See [SSH local editor research](SSH-LOCAL-EDITOR-RESEARCH.md) for source seams, rendering lifecycle, ownership transitions, confidence and go/no-go tests. Remote interactive input requires reliable pass-through. Do not silently substitute the pill if feasibility constraints arise; disclose them.

## Required: completion edits use the current buffer and cursor

Local suggestions must continuously track the actual editable text and cursor as typing/editing continues. Acceptance applies only the missing suffix or appropriate edit for the CURRENT state, whether completing a partial word or a full command with arguments. In preferred client-owned SSH composition, acceptance changes the local buffer only; Enter later submits the full reviewed command once. In a remote-owned insertion mode, transmit only the needed remaining characters/edit, not the already present prefix.

Example: input `gi`, candidate `git` at the end of the buffer: acceptance adds only `t` to the editable buffer, producing `git`, never `gigit`. Client-owned composition sends no command-input bytes until Enter. When text exists after the cursor, do not duplicate an existing suffix; compute the appropriate insertion/replacement for the actual surrounding text and shell editing semantics.

Binding candidates to buffer revision and cursor state is a proposed implementation approach. Required behavior: revalidate at acceptance; recompute/invalidate stale candidates after typing, backspace, paste, history navigation, cursor changes or shell/target/input-context switches. An asynchronous candidate for an earlier buffer must never be applied blindly. No acceptance sends an execution newline or automatically runs the command.

These requirements depend on accurate editable-buffer integration and input ownership already described above. Raw keystrokes alone do not guarantee buffer state, especially with shell editing, history, remote input or full-screen applications. When state cannot be verified, fall back to the manual browser/copy route.

Acceptance fixtures:
- `gi` plus `git` inserts only `t`, in local-owned composition with no pre-Enter command transmission; any separately supported remote-owned insertion path also avoids prefix duplication.
- Cursor is between `gi` and existing `t`; accepting `git` leaves one `git`, without duplicating the suffix.
- Complete a full command/argument candidate against the current partially entered arguments, preserving existing intended text and applying only the required edit.
- A candidate requested for `gi` arrives after the user has typed `git s`; acceptance revalidates/recomputes or rejects it rather than appending the stale completion.
- Backspace, paste, history recall, cursor movement and target/context changes invalidate or refresh candidates appropriately.
- Selection/acceptance does not execute or add a newline, and unverified input contexts remain copy-only.

## Preferred architecture direction: client-owned inline editor over SSH2

The user's goal is the same familiar inline/expanded completion experience without separately installing suggestion plugins on every host. SSH is transport: a client can own the editable command buffer, cursor, history and suggestions, render inline ghost text/multiline content locally, then intentionally submit the command to the remote shell. This is the preferred direction to investigate; exact architecture is not selected and nothing is implemented. It does not inherently require remote autocomplete installation. The separate pill is no longer active product direction.

Preserve a persistent remote shell execution context, including cwd/environment and shell state; do not imply one-shot independent command execution reproduces it. Coordinate screen/input ownership and handoff when launching interactive programs/TUIs, then recover editing ownership appropriately. Native local/remote shell completion remains a supported choice alongside the preferred local editor direction.

Standard terminal text/control streams provide screen cursor positioning, not a reliable semantic command-buffer cursor index. Client ownership can establish its own known buffer/cursor, but correct prompt/command boundaries and remote state still need proof. It must not duplicate echo or fight remote line editing.

Open proof-of-concept questions: prompt/command boundaries; remote echo and double rendering; resize and asynchronous output; input/screen ownership and transitions to/from remote TUIs; remote completion data; and confirmed success capture without remote integration. Naive local WSL/bash completion does not know remote files, commands, cwd/environment or shell state. Define remote-data sourcing/cache/context validation rather than claiming identical local/remote completions automatically.

Acceptance direction: edit/complete partial and multiline commands locally without prefix/suffix duplication or stale candidate application; submit once without duplicate echo; preserve remote cwd/environment across commands; safely hand interactive input to remote programs and recover afterwards; handle asynchronous output/resize; never infer confirmed success from merely sending text. Successful-only eligibility still needs reliable per-execution success evidence. No editor prototype, host installation or shell change is authorized here.

## Protocol and integration research (2026-10-13)

[SSH2 RFC 4254 section 6](https://www.rfc-editor.org/rfc/rfc4254.html#section-6) distinguishes shell/exec sessions and channel data. [Section 6.10](https://www.rfc-editor.org/rfc/rfc4254.html#section-6.10) reports termination of the command attached to a channel: a persistent shell channel's exit status describes shell termination, not every command typed into it. Reusing the transport for separate exec requests does not share interactive cwd/environment/jobs; [OpenSSH login process](https://man.openbsd.org/sshd#LOGIN_PROCESS) documents separate shell/command execution.

[Windows Terminal shell integration](https://learn.microsoft.com/en-us/windows/terminal/tutorials/shell-integration) documents OSC 133 prompt/command boundaries and optional exit status. Verified in pinned baseline: [OutputStateMachineEngine.cpp](../../src/terminal/parser/OutputStateMachineEngine.cpp) dispatches FinalTerm actions; [adaptDispatch.cpp](../../src/terminal/adapter/adaptDispatch.cpp) implements A/B/C/D and optional status for D, guarded by Feature_ScrollbarMarks. Missing status remains optional, malformed status is treated as an error value. Code availability is not proof of enabled runtime behavior or trustworthy command/session correlation; validate those before using it for history eligibility.

Local completion operates in the local shell context; see [GNU Bash programmable completion](https://www.gnu.org/software/bash/manual/html_node/Programmable-Completion.html) (the specific page could not be retrieved during this documentation pass). Do not assume local completion knows remote state. Optional SFTP listings/exec queries could use existing remote tools without installing a custom completion plugin, but do not automatically reveal the interactive shell's current cwd, aliases or unexported variables. Query permissions, target correlation and caching need design; no host queries are authorized here.

[Kitty SSH kitten](https://sw.kovidgoyal.net/kitty/kittens/ssh/) is a configure-once/automatic-bootstrap precedent. It transmits and executes setup code and places files remotely; its documented default remote directory is .local/share/kitty-ssh-kitten. It is not zero-remote-code or guaranteed transient. Distinguish no manual per-host setup from no remote modification. Any bootstrap/integration remains an explicitly opt-in future choice; none was installed or executed.

## Audit-strengthened delivery, trust and preservation requirements

Read the [independent audit](SSH-LOCAL-EDITOR-AUDIT.md) and [research reconciliation](SSH-LOCAL-EDITOR-RESEARCH.md) before implementing. Input ownership, exact multiline submission, echo reconciliation and trustworthy result correlation require capability proofs.

On Enter, freeze the current buffer/context generation and submit once. Never automatically replay after timeout/disconnect or treat a successful pipe write as remote execution/success; ambiguous delivery/outcome stays Unknown. Persist exact semantic submitted text, not trimmed screen reconstruction. Define foreground/pipeline/compound/background/interrupt status semantics and keep per-execution identities.

Define metadata trust and a supported cooperative-shell contract. OSC133 D can be absent or forged and carries no authenticated execution identity. Duplicate/late/out-of-order/forged marks cannot promote Unknown into confirmed success; an observable stream nonce is not a security boundary against a hostile peer. Successful-only mode requires accepted reliable correlated evidence.

Preserve the remote grid and do not hide arbitrary matching output during echo reconciliation. Ownership/view shifts must preserve TSF/IME, paste, selection/copy, hyperlinks/hit-tests and accessibility. Do not hold editor/grid locks across blocking connection writes; keep terminal device replies out of local composition interception. These constraints preserve preferred local inline composition/upward view shifting without remote layout mutation.

## Samples do not establish the editor contract

The [sample map](BASELINE.md) distinguishes ConPTY pipe/input demos from production transport/control/editor seams. Existing completion menus use DEL+text outbound input, not the preferred local-owned buffer; scrollback history is not persistent execution storage. No sample changes the required ownership, exact multiline submission, echo, result-trust, Unicode/IME or local view-shift gates in the [research](SSH-LOCAL-EDITOR-RESEARCH.md) and [audit](SSH-LOCAL-EDITOR-AUDIT.md).

## App commands and resumable sessions - future scope

See [internal app-command design](APP-COMMANDS-DESIGN.md) for dedicated local ownership before any optional inline slash interception, and [resumable remote sessions](RESUMABLE-SESSIONS-DESIGN.md) for separate remote ownership, output/result retention and lifecycle policy. Saved layouts do not imply remote process persistence. These follow-ups are documentation only; ordinary shell history/editing and input pass-through remain intact.
