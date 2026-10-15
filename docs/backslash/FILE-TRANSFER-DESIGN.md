# Integrated file navigation and transfers — future design

[Project entry point](README.md) · [Feature scope](FEATURES.md) · [Shell/session design](SHELL-PRODUCTIVITY.md)

**Exploratory future scope, not active implementation.** The user proposes an integrated split-pane file manager in Backslash, with keyboard switching similar to terminal/tmux panes and a common interface across local files and remote/network protocols. No protocol, dependency, connection or credential access has been implemented or exercised.

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

Only this standalone document is added now; shared indexes and other plans are left untouched while bezel implementation is active. No source edits, installs, credentials, build, commit or push are authorized by this design.


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

Use Backslash's inherited split tree within a tab for native side-by-side/stacked panes, directional focus, resizing, zoom/restore and close/rebalance; tmux is not required for those local operations. [Pane](../../src/cascadia/TerminalApp/Pane.cpp) and [AppActionHandlers](../../src/cascadia/TerminalApp/AppActionHandlers.cpp) provide these facilities, with [IPaneContent](../../src/cascadia/TerminalApp/IPaneContent.idl) as a future heterogeneous-content seam. Current [saved groups](SAVED-TAB-GROUPS.md) save terminal layouts only; file-view persistence needs a versioned content schema. Remote process survival is a separate [resumable-session design](RESUMABLE-SESSIONS-DESIGN.md). No file views, transfers, protocol adapters, credential storage or remote helpers were implemented or exercised here.
