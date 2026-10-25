# SSH local editor: grounded feasibility research

[Project entry point](README.md) · [Shell requirements](SHELL-PRODUCTIVITY.md) · [Roadmap](ROADMAP.md) · [Baseline](BASELINE.md) · [Independent audit](SSH-LOCAL-EDITOR-AUDIT.md)

Research date: 2026-10-13. Inspected stable Microsoft Windows Terminal [v1.25.2733.0](https://github.com/microsoft/terminal/releases/tag/v1.25.2733.0), commit `dc8ae365847d41f62d1ebca93815bb00959bc12f`. This document reports public-source research and local source inspection only. No prototype, build, dependency installation, SSH host query, shell configuration, feature implementation or remote bootstrap was performed.

**Current preferred direction:** familiar inline LOCAL composition for SSH: typing, editing, completion and inline multiline preview stay local; explicit Enter submits to the persistent remote shell. This deliberately differs from ordinary SSH PTY bytewise input. The pill/centered expansion proposal is retired from the active roadmap. If this direction fails a feasibility gate, report that result and revisit the decision; do not silently substitute the rejected pill.

“Source-confirmed” below means read in the pinned source, not runtime tested. “Protocol/documented” identifies official reference behavior. “Proposed” is architecture or test work awaiting authorization and validation.

## Finding and recommendation

The preferred experience is technically plausible: a local editor can own its buffer and semantic cursor without a remote autosuggestion plugin. **Confidence: high in that limited feasibility statement; medium/low in seamless integration with arbitrary unmodified interactive shells.** Owning a local buffer does not automatically reveal the remote prompt lifecycle, current shell state, completion data or command outcome.

Investigate one persistent remote PTY with explicit local-compose / remote-input ownership, keeping the native shell path supported. Start with history completion from known local records, a controlled one-shell contract and transport-independent editor fixtures. Prove handoff/echo/multiline behavior before committing to product-wide integration or an embedded SSH stack. No AI is required.

Reliable command status and remote context are separate capabilities. Without verified status, records remain Unknown and cannot enter the user's preferred Successful-only mode. Manual recall/copy remains useful without plugins, but does not demonstrate the preferred full inline editing experience.

## What the baseline actually connects

The source-confirmed generated SSH profile is an external executable, not an embedded SSH protocol client:

- [SshHostGenerator.cpp](../../backslash/src/cascadia/TerminalSettingsModel/SshHostGenerator.cpp): `_tryFindSshExePath` locates OpenSSH; `_getProfileCommandLine` constructs quoted executable plus host; `GenerateProfiles` assigns that command line.
- [TerminalPage.cpp](../../backslash/src/cascadia/TerminalApp/TerminalPage.cpp): `_CreateConnectionFromSettings` creates `ConptyConnection` for the ordinary process-backed path, passing command line, directory and environment.
- [ConptyConnection.cpp](../../backslash/src/cascadia/TerminalConnection/ConptyConnection.cpp): `_LaunchAttachedClient` uses `CreateProcessW`; `Start` creates the pseudoconsole; `WriteInput` converts UTF-16 to UTF-8 and serializes writes to the pipe; `Resize` uses `ConptyResizePseudoConsole`.
- [ITerminalConnection.idl](../../backslash/src/cascadia/TerminalConnection/ITerminalConnection.idl): `WriteInput`, `TerminalOutput`, `Resize`, `StateChanged`, `SessionId` and `State`. It exposes character streams and connection state, not SSH host-key identity, remote user, per-command status or editable-buffer revisions.

Searches of the inspected Cascadia source and dependency manifests found no `SshConnection`/libssh/libssh2 integration. This supports the ordinary external-OpenSSH path finding, not a claim about every possible extension or alternate profile command.

With external ssh.exe, Backslash observes the local pseudoconsole stream and the child process. SSH authentication/host-key verification, remote PTY modes and channel protocol belong to OpenSSH. A configured profile/host alias is useful provenance, but is not by itself verified actual destination/user after nested SSH, proxying or aliases. OpenSSH's own escape handling also matters when deliberately submitting lines beginning with tilde; do not treat the byte stream as a transparent command RPC. [OpenSSH ssh manual](https://man.openbsd.org/ssh)

An embedded connection could expose channel lifecycle and authenticated peer identity directly, but would add authentication, host-key policy, proxy/agent support, transport/channel management, licensing and packaging work. It still would not reveal each command's semantic buffer/status inside one persistent shell. The existing interface is a possible adaptation boundary, not an existing embedded-SSH API.

## Architecture comparison

| Approach | Editing/UX | State and status | Main limitation |
| --- | --- | --- | --- |
| Native shell plus integration | Shell owns line editing; preserve existing local/remote providers; terminal can use reported boundaries/completions | Persistent shell preserves cwd/environment/jobs; opt-in marks can report completion/status | Different shell capabilities; no reliable full buffer API just from screen text |
| Preferred client-owned editor into persistent remote PTY | Local buffer/cursor, familiar inline suggestions/multiline preview; explicit Enter submits once | Same remote shell retains execution state; outcome/context still require evidence | Echo, prompt boundaries and interactive handoff must be coordinated |
| Separate exec channel per command | Local composition and per-exec channel status can be clear | Independent executions; transport reuse does not share shell cwd/environment/jobs | Breaks expected interactive shell semantics unless a new stateful service is designed |
| Manual browser / remote-owned insertion | Plugin-free connection-scoped recall; copy-only when context uncertain | Manual/unknown records; no inferred success | Useful fallback, not live preferred inline editing |
| Overlay on a remote-owned line | Could visualize reported completions | Still depends on buffer/context integration | Drawing an overlay does not create editing ownership; retired pill is not active direction |

[SSH2 RFC4254 section 6](https://www.rfc-editor.org/rfc/rfc4254.html#section-6) defines session/PTY/shell/exec requests and resize messages. [Section 6.10](https://www.rfc-editor.org/rfc/rfc4254.html#section-6.10) reports the termination of the channel's program. On a persistent shell channel that is shell termination, not an exit status for every typed command. Separate exec requests do not inherit changes made inside another shell merely because the SSH transport is reused. [OpenSSH login process](https://man.openbsd.org/sshd#LOGIN_PROCESS)

## Exact pinned-source seams and limits

| Concern | Real file and symbols | Evidence and possible reuse; no implementation decision |
| --- | --- | --- |
| Keyboard/text routing | [TermControl.cpp](../../backslash/src/cascadia/TerminalControl/TermControl.cpp): `_KeyDownHandler`, `_TrySendKeyEvent`; [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `SendCharEvent`, `TrySendKeyEvent`, `SendInput`, `_sendInputToConnection` | Candidate ownership interception point before remote input. Must preserve shortcuts, IME, accessibility and terminal responses. `SendInput` writes input; it is not a semantic edit transaction. |
| Encoded input | [Terminal.cpp](../../backslash/src/cascadia/TerminalCore/Terminal.cpp): `SendCharEvent`, `SetWriteInputCallback`; [terminalInput.cpp](../../backslash/src/terminal/input/terminalInput.cpp) | Native terminal mode/key encoding needs to remain intact during remote-input ownership. Do not route program replies through a local command editor. |
| Output and screen model | [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `_connectionOutputHandler`; [Terminal.cpp](../../backslash/src/cascadia/TerminalCore/Terminal.cpp): `Write`, `GetViewportRelativeCursorPosition`; [Terminal.hpp](../../backslash/src/cascadia/TerminalCore/Terminal.hpp) | Output is parsed into terminal screen/buffer. Screen cursor is not the local editor's logical offset. |
| Rendering | [renderer.cpp](../../backslash/src/renderer/base/renderer.cpp): `Renderer::PaintFrame`, `_PaintFrameForEngine`; [IRenderData.hpp](../../backslash/src/renderer/inc/IRenderData.hpp); [terminalrenderdata.cpp](../../backslash/src/cascadia/TerminalCore/terminalrenderdata.cpp) | Rendering/accessible text participation needs design; no existing “local command editor” renderer API was identified. Do not corrupt remote screen content by pretending local preview is received output. |
| Resize/reflow | [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `_refreshSizeUnderLock`; [textBuffer.cpp](../../backslash/src/buffer/out/textBuffer.cpp): `TextBuffer::Reflow`; connection `Resize` | Local layout and remote terminal dimensions must stay coherent. Reflow is not a semantic editor history. |
| Prompt/status marks | [OutputStateMachineEngine.cpp](../../backslash/src/terminal/parser/OutputStateMachineEngine.cpp): FinalTerm dispatch; [adaptDispatch.cpp](../../backslash/src/terminal/adapter/adaptDispatch.cpp): `DoFinalTermAction`; [textBuffer.cpp](../../backslash/src/buffer/out/textBuffer.cpp): `StartPrompt`, `StartCommand`, `StartOutput`, `EndCurrentCommand` | A/B/C/D are implemented; D status is optional. Missing status stays absent; invalid status becomes error value. The text-buffer implementation sets status on a prior row's mark, not a persistent execution-ID database. |
| Existing history | [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `CommandHistory`; [ControlCore.h](../../backslash/src/cascadia/TerminalControl/ControlCore.h): `CommandHistoryContext`; [textBuffer.cpp](../../backslash/src/buffer/out/textBuffer.cpp): `Commands`, `CurrentCommand`, `_commandForRow` | Extracts marked visible buffer text; current extraction stops at screen cursor. Context exposes string history/current line/quick fixes, not durable execution provenance/status records. Do not use it unmodified as strict successful-only persistent history. |
| Existing completion protocol | [adaptDispatch.cpp](../../backslash/src/terminal/adapter/adaptDispatch.cpp): `DoVsCodeAction`; [TerminalApi.cpp](../../backslash/src/cascadia/TerminalCore/TerminalApi.cpp): `InvokeCompletions`; [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `_terminalCompletionsChanged` | Experimental OSC633 Completions parses replacementIndex, replacementLength, cursorIndex; code explicitly says index values are not currently used and forwards JSON plus replacementLength. No buffer revision is passed. |
| Existing menu/actions | [TerminalPage.cpp](../../backslash/src/cascadia/TerminalApp/TerminalPage.cpp): `_ControlCompletionsChangedHandler`, `_OpenSuggestions`; [Command.cpp](../../backslash/src/cascadia/TerminalSettingsModel/Command.cpp): `ParsePowerShellMenuComplete`; [AppActionHandlers.cpp](../../backslash/src/cascadia/TerminalApp/AppActionHandlers.cpp): `_HandleSendInput` | Menu uses screen position; completion action generates DEL/backspaces plus text. Not proof of current-state-safe completion for SSH or mid-buffer Unicode edits. Reuse model/event ideas after auditing, not blind actions. |
| Feature/settings state | [features.xml](../../backslash/src/features.xml): `Feature_ScrollbarMarks`, `Feature_ShellCompletions`; [MTSMSettings.h](../../backslash/src/cascadia/TerminalSettingsModel/MTSMSettings.h); [ApplicationState.cpp](../../backslash/src/cascadia/TerminalSettingsModel/ApplicationState.cpp): `_write`, `Flush` | XML stages inspected as AlwaysEnabled; marks have WindowsInbox exclusion. Menu also checks EnableShellCompletionMenu. Comments about Preview are not a substitute for actual feature settings. Application state JSON is a reference, not an accepted concurrency-safe command-history store. |
| Paste/Unicode | [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp): `PasteText`; [textBuffer.cpp](../../backslash/src/buffer/out/textBuffer.cpp): `GraphemeNext`, `GraphemePrev`; [CodepointWidthDetector.cpp](../../backslash/src/types/CodepointWidthDetector.cpp) | Paste filters CR/LF/control codes and adds bracketed paste markers when enabled. Reuse width/segmentation knowledge; preserve original command text and distinguish code units, graphemes and display cells. |

Source availability is not runtime certification. All named seams require focused change/design review; no new adapter/editor API is claimed to exist.

## Rendering lifecycle: readable walkthrough and ownership contract

This is the proposed preferred lifecycle, contrasted with the existing stream pipeline. It is not implemented.

1. **Connect and authenticate.** Today external ssh.exe runs under the local ConPTY connection. Its authentication, host-key questions, banners and remote terminal output arrive as text/control data. Backslash parses that data into its own screen grid, scrollback and screen cursor. Input stays native pass-through during authentication; do not anchor a command editor to a password field or capture it as history.
2. **Identify remote prompt readiness.** Remote prompt text may visually show a path, but that text is not trusted semantic cwd/host metadata. Integrated OSC133 A marks prompt start; B marks command-input start; C marks execution/output start; D marks completion with optional status. These marks describe phases; they do not provide the complete editable buffer, semantic cursor index, authenticated origin or guaranteed truth. Separate working-directory messages reach `Terminal::SetWorkingDirectory`; they are still reported data, not proof of a trusted target.
3. **Anchor local composition.** After reliable readiness/ownership evidence or an explicit supported-mode agreement, capture an anchor in client screen coordinates and let a distinct local buffer/cursor own editing. The anchor must account for prompt cells and wrapping; screen row/column alone never certifies editable-prompt ownership. No per-keystroke command input is sent remotely while composing.
4. **Render inline and multiline locally.** Lay out the local buffer/ghost suggestion from that anchor using graphemes and cell widths. Keep logical offsets separate from screen rows/columns. Do not append the preview into the parsed remote grid as if received from SSH; design separate composition/accessible text participation. Local line breaks wrap/display without submission. Async remote output and reflow may move the anchor and require a redraw or temporary compose suspension.
5. **Submit explicitly and reconcile echo.** Enter under the chosen compose contract submits the reviewed command/block once and relinquishes composition ownership. A remote shell may echo and redraw it before execution. Decide which representation owns the visible line, reconcile without suppressing unrelated output, and never assume pasted newlines equal one execution. Completion acceptance itself sends no execution newline.
6. **Execute and pass through.** Remote output continues to mutate the client grid. Interactive programs, REPLs, password reads and nested shells may own input; pass-through must preserve their key/mouse/paste semantics. Normal-screen mode does not prove a shell prompt: sudo/read/REPL cases may stay there. Ctrl-C's destination depends on current ownership. Missing completion/status evidence remains Unknown.
7. **Return to next prompt and reanchor.** A correlated completion and prompt-ready event can close the execution record, then establish a new anchor. Preserve remote cwd/environment/jobs across cycles. Ambiguous or disconnected phases do not silently create successful records or resume local editing. If reliable handoff cannot be proved, report the supported-mode limitation rather than replace the preferred UX with a pill.

Existing pipeline: `ConptyConnection::TerminalOutput` → `ControlCore::_connectionOutputHandler` → `Terminal::Write` → parser `ProcessString` / `OutputStateMachineEngine` → adapter dispatch → `TextBuffer` rows/cursor/attributes → `Renderer::PaintFrame`. For example `AdaptDispatch::CursorPosition` changes screen state, while `Terminal::GetViewportRelativeCursorPosition` reads it; neither is a semantic editing index. Native input goes through TermControl/ControlCore/TerminalInput to the connection. A local-compose ownership branch would be new work before that outbound path.

| Proposed state | Input owner | Visible content owner | Transition evidence and failure behavior |
| --- | --- | --- | --- |
| Connecting/authentication | Native SSH/remote interaction | Parsed stream grid | Verified supported prompt-ready boundary; otherwise remain pass-through with no command capture |
| Prompt-ready/local-compose | Client editor | Remote prompt/grid plus distinct local composition layout | Explicit Enter freezes the current revision and submits once; Ctrl-C cancels local edit by defined policy |
| Submitted/echo/executing | Remote process/shell | Parsed stream, with bounded echo reconciliation | Correlated lifecycle evidence; no text-matching heuristic that hides output |
| Interactive pass-through | Remote TUI/REPL/read/password flow | Parsed stream | Reliable return-to-prompt or explicit recovery; avoid premature local interception |
| Ambiguous/disconnected | No automatic composition inference | Last grid/error state | Manual recovery/native choice; Unknown outcome; no automatic success or resubmission |

This state table is a design contract to test, not an existing protocol. Resize must recalculate local buffer-to-screen mapping and notify the connection; reflow/output races must retain text, selection and IME position. A screen cursor can be moved by any program, so it cannot alone trigger state transitions.

## Requested inline expansion: shift presentation locally

As the local command/completion grows, shift visible content upward locally to reveal the inline editor, rather than open a popup or cause remote scrolling. No fake newline or scroll escape is sent to the remote shell merely for suggestion layout. Expanding local presentation must not resize remote PTY rows just to make room.

Proposed representations to validate: a local presentation offset/composition transform, or reserved local editor display rows over an intact remote terminal grid. Neither is an implemented feature. Keep parsed remote grid/cursor, scrollback origin and local editor geometry distinct. A transformed view still needs a mapping back to remote cells; local editor selection maps to its own logical buffer. Maintain active input visibility; when content exceeds the viewport, use normal deliberate navigation rather than hide input or jump to a centered panel.

Source-confirmed related seams: `ControlCore::UserScrollViewport`, `ScrollOffset`, `CursorPosition` and buffer-position mappings in [ControlCore.cpp](../../backslash/src/cascadia/TerminalControl/ControlCore.cpp); `GetViewport`/`_scrollOffset` and `GetViewportRelativeCursorPosition` in [Terminal.cpp](../../backslash/src/cascadia/TerminalCore/Terminal.cpp). These existing scrollback/viewport operations are not a ready-made local-editor reservation API. Blindly changing them could move browsing/cursor state, break selection or disagree with remote geometry.

Required engineering: transform drawing, mouse hit-testing, selection/copy, accessible text and IME anchors consistently; avoid clipping active input; recompute after resize/reflow/font/DPI/CRT curvature changes and asynchronous output. Upon submission/cancel/interactive handoff, reconcile local reservation/offset with remote prompt/echo/output without losing scrollback or jumping unexpectedly. Distinguish actual window resizing (which can notify the PTY) from suggestion-only visual growth (which must not inject remote layout input).

Add proof-of-concept gates: near-bottom multiline expansion shifts only local presentation; captured outbound stream has zero layout newlines/escapes; remote logical dimensions/grid/cursor remain unchanged by expansion alone; transformed selection/copy targets correct text; content larger than the viewport remains intentionally navigable; submit/echo and cancel restore coherent view state. This needs coordinated rendering/UI work and cannot be claimed solved by a simple scroll-offset assignment.

## Ownership and correctness risks

**Screen versus semantic cursor.** Xterm cursor reports identify row/column; they do not convey shell edit-buffer indices. Prompts, wraps, Unicode, ANSI styling and asynchronous redraws can change screen positions independently of command text. [Xterm control sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html)

**Local composition.** Proposed contract: local editor owns all composition until deliberate submit; completion acceptance edits local state only. Thus `gi` to `git` changes the local buffer by `t`, with no command-input bytes transmitted before Enter. Submit sends the complete current command once under the chosen shell contract. Earlier “send only t over SSH” applies only to remote-owned line insertion, not to this preferred local composition mode.

**Prompt/echo.** Maintain remote shell state and verify when it is ready. Reconcile its prompt and line echo with local preview so the command appears once. Blindly disabling echo or suppressing matched output risks hiding real output and password behavior. Prompt-looking text is not sufficient evidence. Define a bounded state machine and failure recovery; its states/messages are proposals, not existing protocols.

**Interactive ownership.** Remote-input mode must handle authentication, host-key confirmations, sudo/password reads, full-screen TUIs, pagers, REPLs, `read` and nested shells. Alternate-screen mode alone is insufficient: password reads/REPLs may use the normal screen. Ctrl-C while composing should cancel local editing under a defined policy; while a remote process is active it must reach that process. Authentication/private input must not become history. Resume local compose only on reliable lifecycle evidence or explicit user action; ambiguity is a disclosed unsupported condition.

**Resize and asynchronous output.** Preserve terminal output independently of local preview; avoid cursor stealing, lost redraws, flicker and duplicate text when background output arrives. Define local preview wrapping, scrollback and selection/IME/accessibility behavior. [ConPTY session design](https://learn.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session) and [ResizePseudoConsole](https://learn.microsoft.com/en-us/windows/console/resizepseudoconsole) identify the underlying communication/resize responsibilities.

**Multiline and paste.** Local newline insertion/preview differs from Enter-to-submit. Define how a multiline block becomes one deliberate submission while respecting shell grammar, continuation/heredocs and embedded newlines. Completion acceptance never submits a newline; executing a deliberately reviewed multiline block can intentionally contain newlines. Bracketed paste distinguishes pasted content but does not guarantee every remote shell handles it safely. Check negotiated mode and shell capabilities; do not automatically assume one accepted block equals one shell execution/status.

**Unicode.** Segment editing by grapheme clusters; map logical indices to rows/cells under the selected terminal measurement mode. Combining characters, emoji ZWJ, wide CJK, tabs and ligatures must not split edits or duplicate suffixes. Unicode segmentation and terminal cell width are related but distinct. [UAX29](https://www.unicode.org/reports/tr29/), [UAX11](https://www.unicode.org/reports/tr11/)

**Stale results.** Proposed revision/cursor/context token for each request; validate again on acceptance. Typing/backspace/paste/history recall/cursor movement/nested-target switch invalidates candidates. Never apply an earlier result to a newer buffer. State-sensitive edit ranges are needed for arguments and text after cursor.

## Completion data, status and no-manual-setup options

History completion needs local recorded text, not a remote plugin. Remote path/argument completion needs current remote data: optional SFTP listings or deliberate read-only exec queries can supply limited facts, but not automatically interactive aliases, unexported variables or current cwd. Such queries may occur before Enter without submitting the composed command. Bound latency/cache lifetime, distinguish their status from user's command status and request appropriate target permission in future work. No queries were made now.

Native shell integration is useful for prompt/command/status boundaries. [Microsoft integration guide](https://learn.microsoft.com/en-us/windows/terminal/tutorials/shell-integration) describes OSC133 and optional status; pinned code confirms the dispatch. Marks emitted by arbitrary application output are not authenticated host identity or guaranteed truthful outcomes. Define opt-in adapter identity, sequencing, record correlation and adversarial/spurious-mark behavior; Unknown is preferable to false success.

Configure-once bootstrap could avoid manual per-host setup while still running remote code. [Kitty SSH kitten](https://sw.kovidgoyal.net/kitty/kittens/ssh/) documents transmitted/executed bootstrap and remote files (default .local/share/kitty-ssh-kitten); it is not a zero-modification or guaranteed ephemeral approach. A Backslash transient wrapper would be a new proposal needing cleanup/lifetime proof; a persisted integration needs explicit installation/update/rollback scope. No scripts were copied or run.

GNU Bash completion/Readline references remain relevant, but their specific pages failed retrieval in this research pass: [programmable completion](https://www.gnu.org/software/bash/manual/html_node/Programmable-Completion.html), [Readline interaction](https://www.gnu.org/software/bash/manual/html_node/Readline-Interaction.html). Exact shell hooks remain unverified; do not invent a universal shell adapter.

## Persistent history and scope contract

Reuse [shell requirements](SHELL-PRODUCTIVITY.md), not a second conflicting policy. Shared-persistent mode stores per-execution IDs, outcomes, target/user/shell/session and directory/project where available, updated promptly across tabs/windows and surviving restart. Storage engine/transactions remain open. Settings control this data; they are not the history database.

Default automatic candidates stay within verified machine/connection, user and compatible shell. Directory grouping does not hide cross-directory discovery. Deliberate All connections search is origin-labeled; automatic cross-connection sharing is opt-in. Nested SSH/WSL/containers and unknown target identity require conservative separation.

History storage and suggestion source are independently configurable proposals. User preference is shared-persistent plus Successful-only: only confirmed success qualifies there; failed/unknown/incomplete/crashed records cannot silently become successful. All recorded and Off are selectable design goals, not permission to erase native shell history. Failures remain visible with per-execution markers/filters. No retention duration or 30-day policy is selected; mode changes never silently purge data. Privacy/private sessions, secret handling, archive/export and clean uninstall remain design gates.

## Small staged proof-of-concept plan — future authorization required

| Stage | Bounded experiment | Go evidence | No-go / decision needed |
| --- | --- | --- | --- |
| 1: editor-only fixtures | Pure local buffer, semantic cursor, history candidates and explicit submit sink; no transport | gi→git edits only t; mid-buffer suffix/arguments correct; stale async result rejected; Unicode/multiline text preserved; no pre-submit command bytes | Any duplicate edit, grapheme corruption, newline-on-accept or accidental execution |
| 2: stream/lifecycle replay | Recorded/synthetic output/input sequences and resize events; no user hosts | Local preview/expansion and remote output remain independent; no layout bytes or remote row changes; transformed hit-testing/copy correct; ambiguous/password/TUI context never captured/edited as commands | Prompt heuristics required to infer success/identity; output suppressed incorrectly |
| 3: controlled authorized lab | One declared shell over external OpenSSH/ConPTY; no production host | One persistent cwd/env/jobs context; submit once; echo appears once; Ctrl-C/password/REPL/TUI handoff and recovery proven | Requires silent host change, loses state or cannot reliably regain local ownership |
| 4: opt-in integration comparison | Native marks versus explicit session bootstrap with reviewed lifetime | Correlated command IDs/status/target; Unknown retained on missing status; completion queries distinguish remote context | Fake/late marks create success; channel exit mistaken for command status; cannot explain cleanup |
| 5: durability and modes | Concurrent writers, restart/crash, scope/filter/retention fixtures | No lost/duplicate executions; preferred success filter correct; broader discovery explicit; no data loss on mode switch | Cross-host suggestion leaks, status collapsed by string, implicit pruning |
| 6: limited product slice | Only after prior gates: inline local composition in one supported connection/shell mode | Native mode remains accessible; no competing providers; accessible input/rendering and performance checks pass | Do not silently substitute pill; report limitation and seek revised scope |

The [independent audit](SSH-LOCAL-EDITOR-AUDIT.md) adds priority gates and adversarial/lifecycle fixtures; the reconciliation below is part of this plan. No stages have been run. A feasibility result should include input/output traces, expected versus actual buffer/status, provenance and exact supported shell/version, not only a visual demo. No build/install authorization follows from this plan.

## Unresolved questions to decide before product implementation

Transport reuse versus embedded SSH; reliable prompt readiness and authenticated scope; local/remote echo contract; pass-through detection and explicit mode escape; IME/accessibility/selection rendering; multiline grammar/submit semantics; remote completion metadata contract; trust/correlation of success marks; pipeline/interrupt status semantics; nested host switching; concurrent history schema/retention/private modes; bootstrap lifetime/license/update/cleanup; supported shell/version matrix.

Proceed only if the preferred inline model's input ownership, remote state preservation and error recovery are demonstrated. Confidence in the transport/source findings is high; confidence in unrestricted “works on every SSH shell without integration” is low and that is not promised.


## Requested connector availability check

Tested actual callable tools on 2026-10-13, not inferred from configuration:

- Context7: resolve_library_id succeeded for Windows Terminal and returned /microsoft/terminal; query_docs succeeded for OSC133/prompt/completion semantics. Returned upstream-main code/spec excerpts are supporting research, not pinned-release evidence. In particular, do not import main-only cwd/completion behavior without checking this stable tree.
- Microsoft Learn MCP: microsoft_docs_search succeeded for terminal shell integration; microsoft_docs_fetch succeeded for the complete official shell-integration guide. It confirms optional OSC133 D status and documents marks as stable since 1.21, consistent with the inspected feature stages. A runtime build was not tested.
- Public GNU Bash manual page fetches failed in the web retrieval path as noted above. This is a specific page-retrieval limitation, not evidence that Context7 or Microsoft Learn is unavailable.

No configuration inspection, connector installation or remote-host query was needed. Tool outputs are research evidence; none authorizes setup commands from examples.

## Independent audit reconciliation

Accepted the [independent audit](SSH-LOCAL-EDITOR-AUDIT.md) verdict: proceed to narrow capability proofs, not general product implementation. No substantive disagreement with its source-grounded findings. The preferred local inline buffer/upward presentation remains valid direction; the audit does not recommend resurrecting the pill.

Four critical go/no-go capabilities are input ownership/readiness, exact multiline submission, echo reconciliation and trustworthy command-result correlation. Add the following measured requirements to the existing stages:

- **Delivery ambiguity:** freeze buffer/context generation on Enter; retain exact submitted text and trace the intended execution count. A completed pipe write is not command execution evidence. No automatic replay after timeout/disconnect, including a disconnect between delivery and status; mark uncertain delivery/outcome Unknown. Test heredocs, quoted newline, CRLF, incomplete syntax/PS2, leading tilde and bracketed paste on/off.
- **Result/provenance trust:** correlate session/execution/generation, specify accepted trust levels and a cooperative-shell assumption if used. OSC133 marks do not authenticate origin or carry an execution ID. Forged, duplicate, late/out-of-order or absent marks must not create confirmed success. A nonce visible in the same stream is not a security boundary against a hostile remote peer. Define foreground, pipeline/compound/background and cancelled outcomes before eligibility.
- **Echo ownership:** preserve the authoritative parsed remote grid. Freeze/remove local preview at a defined transition; do not discard arbitrary output because it matches submitted text. Test echo on/off and simultaneous matching job output.
- **Input/presentation completeness:** test normal-screen read/password/REPL as well as alternate-screen TUIs. Preserve pending TSF/IME, pasted text, selection, hyperlinks, hit-testing, copy and accessible text through local view shifts and ownership changes. Authentication remains pass-through with no command capture. Unknown context cannot trigger automatic local takeover.
- **Lock and response routing:** the pinned SendInput path asserts the terminal is unlocked because writes may block; output responses also use the connection path. Do not hold editor/grid locks across submission, and never swallow terminal device replies into the command editor. Source references: ControlCore.cpp SendInput/_connectionOutputHandler/_sendInputToConnection and TermControl.cpp input/TSF handling.
- **Durable exact records:** store the original submitted semantic text, including meaningful trailing spaces/newlines, rather than trimmed/reconstructed screen history. Stable execution IDs and atomic ingestion/status changes must survive concurrent windows/crash; cancellation is distinct from confirmed success. Terminal HistorySize is scrollback capacity, not the command-store retention policy.
- **Configure-once proof:** compare zero-helper limitations, pre-existing integration and explicitly scoped transient/persisted bootstrap. Prove compatibility, quoting, state preservation and cleanup; no manual setup does not mean no remote code. Restricted/unsupported shells need disclosed capabilities and native recovery, not a hidden UI substitution.

Future evidence must include outbound byte traces, grid snapshots, local buffer revisions/ownership transitions, execution records and supported shell/version. Screenshots alone cannot pass these gates. Native recovery is an intentional supported path; it must not be mislabeled delivery of the preferred inline feature. The audit report itself was read and left byte-identical.

## Sample reuse correction

See the [pinned sample map](BASELINE.md). EchoCon launches ping localhost; MiniTerm demonstrates managed IO/Ctrl-C; GUIConsole is raw VT/TextBlock and key-name input scaffolding; ReadConsoleInputStream is console event handling. None proves client-owned semantic editing, production resize/keyboard/TSF/IME or the audit's ownership/submission/echo/status gates. Use production SshHostGenerator/ConptyConnection/control/parser seams for the initial integration plan.

The existing OSC633 menu ends in DEL+text remote input, not local editor state. Marked scrollback remains reconstructed history, not durable execution storage. These reuse findings preserve the preferred local inline buffer/view shift and all capability-first audit gates.
