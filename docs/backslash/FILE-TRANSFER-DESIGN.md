# Integrated file navigation and transfers — future design

## Current recommendation (planning only, 2026-10-26)

A read-only comparison recommends evaluating one native `IPaneContent` Files workspace containing local and remote browsers plus a shared queue, backed by a UI-independent service. Local filesystem + SFTP is the first proposed slice. This is a recommendation, not an architecture decision or implemented feature. A Windows TUI remains credible; WebView2 hosting in this exact unpackaged Windows.UI.Xaml Islands host needs its own prototype before promising HTML tools. Settings is native XAML/C++, not web UI merely because settings are JSON.

Outer placement (tab, split beside a terminal, separate window) is independent of the Files workspace's internal left/right or stacked browser arrangement. Existing `IPaneContent` and Settings hosting offer seams, not a ready transfer engine or generic plugin framework. Process-owned transfer jobs, authentication, trust, cancellation and view-close behavior need their own design and fixtures. A saved SSH connection is not a reusable authenticated SSH channel.

Start a future authorized prototype with synthetic local lists, focus/resize/close/accessibility and fake cancelable jobs; select and test a backend separately before network work. Defer remote-to-remote, automatic resume/sync and cross-endpoint Move until copy/cancel/conflicts are reliable.

Portable design must work with App and Data together in an ordinary local folder; USB and split deployments are optional. Relative association and media-independent workspace IDs are future work. Any optional volume-root discovery locator must be disclosed and app-created, with volume-relative paths, multiple entries, repair and root-not-writable recovery; no autorun file or root marker is required by today's implementation. A temporary two-machine LAN endpoint is a separate future mode: it needs locally staged runtime files to survive USB removal, limited sharing scope, pairing and shutdown. SFTP access does not itself implement that mode.

A newer proposed direction supports optional native and terminal frontends over a shared transfer backend. The backend would own local/remote providers, connections, listings, queue, progress, cancellation and errors; frontends would own presentation and input. Sharing library code alone does not preserve live connections/jobs when changing or closing a frontend: that requires a common live worker/session with an explicit lifetime policy. Existing third-party TUIs do not automatically speak that backend and may need an adapter or a custom frontend. This remains a proposal, not a selected architecture.

Proposed job lifetime: an app command could enqueue a backend-owned transfer and return a job ID. A native or TUI view could then hide/close and later reopen progress, cancellation and results for that job. Closing a view is distinct from quitting Backslash. Application exit still needs an explicit warn / keep a worker / cancel policy; no always-on service is implied. This scope covers transfer-manager jobs, not automatic supervision of arbitrary shell commands.

No runtime, UI, protocol library or add-on distribution choice has been selected or installed. Current implementation/verification boundaries are in [maintainer handoff](../dev/HANDOFF.md).

[Project entry point](README.md) · [Feature scope](FEATURES.md) · [Shell/session design](SHELL-PRODUCTIVITY.md)

**Protocol and file-browser scope remains exploratory.** The [Transfers foundation](TRANSFERS-FOUNDATION.md) now adds a separate native management view and process-owned record model, with no protocol engine or real transfer actions. The browser and protocol design below remains future work.

**Original design context:** The user proposes an integrated split-pane file manager in Backslash, with keyboard switching similar to terminal/tmux panes and a common interface across local files and remote/network protocols. No protocol, dependency, connection or credential access has been implemented or exercised.

## Pane and interaction concept

Two file-browser panes can represent local → remote or remote → remote endpoints; additional terminal panes remain usable alongside them. Each browser shows endpoint/protocol, account context where verified, current directory and selection. Switching focus must make the active source/destination obvious. Directory navigation, multiselect, search/filter and copy/move actions should be keyboard accessible; exact shortcuts remain open and must coexist with terminal bindings.

Embedded TUI versus graphical pane is undecided. A TUI may fit familiar keyboard workflows; a graphical pane may simplify visible endpoints, queue/conflict dialogs and accessibility. Evaluate focus/input ownership, selection/copy, resize and screen-reader behavior before selecting either. Do not assume a terminal application screen can serve as a structured browser API.

Reuse requested SSH bookmark metadata where applicable (name/host/user/port/key reference/group), without inventing access to passwords, keys or an existing shell's authentication session. Authentication, host-key/certificate trust and credential-storage choices need explicit design and user-visible scope. A bookmark is a launch/configuration reference, not proof that a transfer can authenticate.

## Protocol adapters and first candidate

Use a common navigation/transfer model with capability-aware adapters rather than promise identical operations on every endpoint.

| Endpoint/protocol | Distinction | Design implication |
| --- | --- | --- |
| Local filesystem | Windows paths, drives and permissions | Preserve path/Unicode semantics; handle locked files and reparse points explicitly |
| SFTP | File-transfer subsystem over encrypted SSH transport; distinct from FTP | Recommended first remote candidate; validate server capabilities and SSH identity/authentication |
| FTP | Traditional FTP can expose credentials/data in plaintext | Label plainly; do not imply SSH or TLS protection |
| FTPS | FTP protected using TLS; distinct from SFTP | Define TLS certificate validation and control/data-channel protection |
| SMB shares | Network filesystem/share access, not FTP/SFTP | Use appropriate share/path/account semantics; do not promise every share uses encryption |

Sources: [OpenSSH SFTP](https://man.openbsd.org/sftp), [FTP security considerations](https://www.rfc-editor.org/rfc/rfc2577.html), [FTP over TLS](https://www.rfc-editor.org/rfc/rfc4217.html), [Microsoft SMB overview](https://learn.microsoft.com/en-us/windows-server/storage/file-server/file-server-smb-overview).

Libraries/process adapters and their licenses remain unselected. Other local-network protocols are future candidates, not a universal-protocol promise. Expose capabilities for rename, resume, metadata, symlinks, checksums and server-side copy; gray out or explain unsupported operations.

## Transfer sessions are separate from shell input

Do not implement transfers by injecting commands into an active interactive shell stream. Use independent transfer sessions/channels/processes or filesystem adapters. Possible transport reuse must be designed deliberately; the current external ssh.exe/ConPTY path does not automatically expose reusable SFTP channels or credentials.

A transfer browser's directory is independent of the shell's cwd. Optional synchronization requires reliable context and an explicit action; a displayed prompt path is not authoritative transfer metadata. Remote-to-remote transfer may relay through the client unless the selected endpoints support a validated direct operation. Explain routing, local staging/privacy, bandwidth and cleanup; do not assume a universal direct remote-copy command.

## Queue, conflicts and move semantics

Provide queue/progress, cancellation and explicit retry, with clear per-item source/destination and partial/failure states. Resume only when the adapter can validate compatibility of partial data; do not blindly append to an unrelated destination. Define retry idempotency, temporary naming, disconnect recovery and partial-file cleanup.

Resolve overwrite conflicts explicitly: skip, replace, rename/keep both or cancel, with accurately scoped batch choices. Do not silently replace newer data or follow destructive symlink/reparse paths. Metadata preservation and case/path collisions vary across endpoints and need a selected policy.

A cross-endpoint **Move** is an explicitly selected operation: copy → verify completion under the defined adapter policy → delete the source only after verified success. It is not an atomic rename. Use destination acknowledgment/size and stronger checksum evidence where supported; specify verification strength rather than claim every protocol supplies hashes. If completion/delivery is uncertain, retain the source. Cancellation or copy failure must not delete it. If final deletion fails, report “copied; source retained,” not a completed move. Same-endpoint atomic rename, where supported, is a different capability.

## Open choices and bounded validation

Choose UI type, pane/focus model, initial local + SFTP slice, adapter/library, authentication/secret storage, remote-to-remote routing, verification/resume policy, queue persistence, metadata/symlink behavior and conflict UX before implementation.

Future fixtures: local-to-SFTP copy with spaces/Unicode; focus switching beside terminal panes; existing destination conflict; cancel/retry/disconnect; source retained after failed/unknown copy; deletion failure after verified move-copy; separate hosts/accounts and visible origin; unsupported adapter capabilities; inaccessible local/share paths; keyboard/accessibility and resize behavior. No tests or live connections were performed for this document.

Historical authoring boundary: the original design-only task changed no product code or runtime. This document is now synchronized with later research; it still does not describe an implemented transfer feature.


## Reference evaluation: superfile (2026-10-13)

The user supplied [yorukot/superfile](https://github.com/yorukot/superfile) as an interesting reference, not a request to copy its design. Its repository is Go-based with an MIT license and configurable themes/plugins/hotkeys; the README labels Windows not fully supported. No installation, code reuse or embedding choice was made.

- **Panels/focus:** the [official tutorial](https://superfile.dev/getting-started/tutorial/) documents creating/closing multiple file panels, cycling focus, and distinct sidebar/processes/metadata/clipboard views. Useful Backslash idea: keep active endpoint and auxiliary-view focus explicit while terminal panes coexist; exact layout/keybindings remain ours to design.
- **Operations/progress:** the tutorial separates bulk-selection mode from browsing, shows copied/cut items in a clipboard panel and operation progress in a processes panel. This suggests visible pending selections plus ongoing work without blocking browsing. Our queue/cancel/retry and cross-endpoint verification semantics remain proposals, not verified superfile capabilities.
- **Keyboard interaction:** [custom hotkeys](https://superfile.dev/configure/custom-hotkeys/) are configurable and identify terminal key collisions such as Ctrl+I/Tab. Backslash needs focus-aware bindings and visible modes; do not blindly adopt shortcuts that conflict with terminal input.
- **Preview:** [image preview docs](https://superfile.dev/getting-started/image-preview/) currently document Kitty-protocol images with ANSI fallback, not Sixel support. Existing Backslash Sixel rendering alone would not establish superfile image-preview compatibility. An embedded TUI would need real capability checks; a graphical pane has a separate preview-rendering decision.
- **Extensions:** the [plugin list](https://superfile.dev/list/plugin-list/) includes metadata, MD5 display and zoxide directory navigation. These are useful separation examples; they do not certify transfer integrity, endpoint identity or remote protocols.

The inspected official pages did not establish SFTP/FTP/FTPS/SMB adapters. Do not infer SFTP from “file manager,” and do not claim a exhaustive absence audit. Backslash's proposed protocol support, remote-to-remote routing and credential model remain independent design work.

## Ad-hoc app-command workflow and native panes (2026-10-14 proposal)

The user wants opening views without a mandatory preset: proposed `/files user@hostname-or-IP /remote/path` or a saved SSH alias, with optional explicit local-directory override. Syntax is not finalized; see [app commands](APP-COMMANDS-DESIGN.md). Opening is separate from transferring selected files. CLI `sftp` put/get and source-destination transfer examples can inform semantics, but are not a selected UI/transport implementation.

From a reliably established local shell folder, keep shell access and open embedded file views: left represents that local folder, adjacent right represents the authenticated remote target directory. A file view is not another shell pretending to be a browser. If cwd reporting/ownership is unknown, stale or actually SSH-remote, ask for an explicit local folder/default; do not infer local cwd from remote state or infer remote cwd from the SSH client's local working directory. Current saved-group capture distinguishes initial profile folder from a client report, but does not establish universal trustworthy local/remote cwd. Omitted remote directory uses the authenticated server's default/home only once resolved and displayed. Define quoting/spaces, IPv6/ports/aliases, platform paths and ambiguity before implementation.

The right view owns connecting, key/agent/config selection where supported, secure password prompting if needed and host-key verification. Passwords never enter app-command text/history, and unknown/changed keys are never accepted silently. Preview/completion never authenticates or connects. Cancel/failure preserves the local view, shell and target text; retry is explicit. Focus should start in the newly opened view only after explicit submission and restore the originating shell on cancel/close. Transfer confirmation, conflict handling and integrity/resume semantics remain separate.

Use Backslash's inherited split tree within a tab for native side-by-side/stacked panes, directional focus, resizing, zoom/restore and close/rebalance; tmux is not required for those local operations. [Pane](../../backslash/src/cascadia/TerminalApp/Pane.cpp) and [AppActionHandlers](../../backslash/src/cascadia/TerminalApp/AppActionHandlers.cpp) provide these facilities, with [IPaneContent](../../backslash/src/cascadia/TerminalApp/IPaneContent.idl) as a future heterogeneous-content seam. Current [saved groups](SAVED-TAB-GROUPS.md) save terminal layouts only; file-view persistence needs a versioned content schema. Remote process survival is a separate [resumable-session design](RESUMABLE-SESSIONS-DESIGN.md). No file views, transfers, protocol adapters, credential storage or remote helpers were implemented or exercised here.

## Preview placement and TUI icons (future design, 2026-10-26)

The future file browser should offer item context menu > Preview. Preview defaults to a new window, with an independent tab, split or window preference; Transfers placement does not control it. Closing a preview preserves the browsers and managed jobs. Start with images; PDF support can follow through an appropriate renderer, potentially web. Native versus web is a renderer detail behind the same Preview action, rather than a different workflow.

Remote preview begins only after an explicit request, uses a bounded fetch and shows loading and error states. It does not download every item in a directory automatically. A TUI could use an existing icon-capable font with tested cell alignment and a readable fallback; no font has been selected or installed. None of this preview or font work is implemented in the Transfers foundation.

## Accepted direction and delivery slices (2026-10-26)

This follow-up consolidates the latest discussion after the prior documentation sync. The [maintainer handoff](../dev/HANDOFF.md) separates the published preview from uncommitted work. The [Transfers foundation](TRANSFERS-FOUNDATION.md) is implemented and built; its native interaction QA is pending. Real transfer protocols, the local/remote file browser, previews, history cleanup and Background sessions below are later slices, not current capabilities.

### Implemented management foundation

The main terminal dropdown includes Transfers beside Settings/About. The native management view combines search with All, Active, Completed, Failed and Canceled status filtering. Its placement preference is independently configurable as tab, side-by-side or separate window; reopening focuses the existing view. Close affects that view only, with mixed-pane tab-X protection while it is active. The process-owned record model exists independently of a view from submission; view destruction unsubscribes and releases UI ownership without canceling records. No sample jobs or working network actions are inserted. Native live window movement remains explicitly guarded in this slice.

### Accepted file-browser and transfer priorities

Prioritize endpoint identity, conflict handling and interrupted-transfer correctness before visual extras. Show the local/remote machine, connection/account context and full current path clearly in each browser. Keep connection identity separate from an unverified title or shell report. Provide back/forward navigation, an editable path, folder bookmarks, sorting and a hidden-files option, with per-endpoint behavior and clear access errors.

Conflict review should offer Skip, Rename and Replace, displaying both size and modification date and an explicitly scoped apply-to-batch option. Failed replacement must preserve the prior good destination. Stage to a distinct temporary destination and verify before committing; use an atomic replacement only when the adapter/filesystem supports it. Unsupported commit/backup semantics need an explicit capability/policy decision, not a promise of atomic replacement on every server. Interrupted, failed, cancel-pending and unknown-delivery outcomes must remain distinguishable and must not appear as completed. Define partial-file naming/cleanup, explicit retry and validated resume before implementing them. Preserve source data after failed or uncertain copy and never blindly append or silently delete it.

Completion actions should include Open containing folder, Copy path and Preview where the endpoint supports them. Opening a local containing folder can use the local file browser/Explorer; a remote folder needs its authenticated remote browser context. Do not imply an arbitrary remote path is a local Explorer location. Keep action targets explicit.

Outer placement of the file browser (tab, split or window) is independent of its internal local/remote orientation (left/right or stacked). A TUI can render both browsers inside one PTY, while native or web content can render both inside one app pane. A dedicated app command can open an app-owned graphical view through a validated typed action; it does not force a TUI and must never treat arbitrary terminal output as commands. The final command syntax remains unresolved.

An optional native/TUI frontend can share a backend library, but shared active jobs require the same live job owner/worker session, with an explicit local IPC contract if separate processes participate. Library reuse alone does not share running jobs. Ordinary PTY downloads and externally detached tools are not automatically managed. The actual worker, protocol engine and full-app quit policy are not implemented or selected here; no persistent service is introduced. Local + SFTP remains the first suggested remote slice, with FTP/FTPS separate protocol decisions.

### History policy still to select

Clearing history removes records only, never transferred files, and does not cancel active jobs. A direction filter is optional later work. Seven days, thirty days and forever were discussed as retention choices, but no duration is selected and no automatic expiry exists. Failed, interrupted and retryable records need an explicit retention/cleanup policy; neither silently discarding them nor blindly retrying them is accepted behavior. Decide how interrupted outcomes appear in future filters without misrepresenting a live cancellation as a finished Canceled job.

### Preview is a separate action and view

Use item context menu > Preview, defaulting to a new window, with its own configurable tab/split/window placement independent of Transfers. Closing a preview preserves browsers and jobs. Start with images; PDFs can follow via an appropriate renderer, potentially web. The same action owns the workflow regardless of native/web renderer. Include next/previous item navigation in the preview. A remote preview or next/previous action explicitly requests a bounded fetch with visible loading/error states; directory browsing must not automatically download every item. Existing icon-capable fonts may support TUI file icons after alignment/fallback testing; no font is selected or installed.

### Background sessions are separate future scope

An explicit Background sessions view could list detached Backslash-owned terminal sessions with running/exited state and reopen/terminate actions. Discoverable Help and empty-state instructions must show the actual supported detach/menu/terminal command, how to return and reopen, and how closing a view differs from terminating a session or quitting the app. No command syntax has been chosen or implemented. Do not infer sessions from network traffic or automatically register arbitrary shell background commands or externally detached tools. This discussion adds no session implementation to Transfers.

### Suggested next order and open gates

1. Validate the management view's native placement, repeated focus, close protection, keyboard access, high contrast and narrow widths.
2. Establish explicit local/remote browser identity and navigation using disposable fixtures, then select the SFTP adapter/authentication/trust design.
3. Implement copy, conflict review and safe interrupted/replacement behavior with a shared worker and explicit cancellation/progress/error semantics.
4. Add supported completion actions and image Preview, then assess PDF rendering, optional TUI integration and separate Background sessions.

Backend/library choice, credentials/known-host interoperability, queue persistence, full-app exit behavior, retry/resume verification, failed-record cleanup and retention duration remain unresolved. These are planning priorities and validation gates; no real transfer has been exercised.
