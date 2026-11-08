# Backslash v0.0.1-preview.3

Experimental, unsigned Windows x64 portable preview. Extract the complete ZIP into a writable folder and run `Backslash.exe`. Keep `.portable` and all supporting files together. Back up the adjacent `settings` folder before updating; the preview does not automatically import Windows Terminal settings.

**SFTP is experimental and has not been verified for dependable real-world use in this release.** Automated and isolated fixture tests do not establish full end-to-end user workflows. Use noncritical files, keep backups, and verify transfer results. Whole-application Settings/Transfers placement and close-flow acceptance is also incomplete.

## Changes

- Experimental SFTP file browsing and single-file upload/download through saved SSH profiles. Transfers shows progress, results, cancellation and retry actions. Local replacement requires an explicit decision; uploads refuse an existing remote destination.
- Process-owned transfer jobs survive closing the Transfers view. Application shutdown cancels work before publication and waits for publication already in progress. If shutdown cannot be confirmed, Backslash offers continued waiting or explicit force quit with a warning.
- Optional Explorer integration controls for status, selecting this portable copy, repair and removal. Paths and diagnostics are grouped under collapsed Details. Restart Explorer is an explicit, confirmed action; enabling integration does not automatically restart it.
- Corrected Settings header alignment when navigation width changes and matched Transfers Backends to the shared Settings body width.
- Standalone Transfers opens on shared activity and history. A left menu separates activity, the SFTP browser and backend settings; each view uses a centered, padded content panel that follows the application theme.
- Updated project artwork and documentation.

## SFTP requirements and limits

Choose a supported saved SSH profile in Settings > Transfers > Backends. Configure SSH access separately: noninteractive authentication and an already trusted host key are required. Transfers does not prompt for passwords, accept unknown or changed hosts, change known-host files or install tools. Unsupported profile command lines are refused.

Only individual regular files and supported local drives are available. Recursive copies, remote replacement, UNC/mapped-network destinations, automatic reconnect/resume and persistent queues/history are unavailable. A canceled or interrupted upload may retain a remote temporary file; the job reports its path when known. Check reported outcomes before retrying an uncertain transfer. Server compatibility and broader interactive lifecycle coverage remain limited.

## Other known limitations

- Modern Windows 11 Explorer integration uses a development helper and requires Developer Mode. It is optional, explicit opt-in and not a signed general-user installer. Moving or deleting the selected portable copy can require repair or removal. See the [Explorer guide](EXPLORER-INTEGRATION.md).
- An intermittent blank/transparent Settings pane remains a known preview issue. Theme, DPI, accessibility and clean-machine compatibility testing is incomplete.
- Windows x64 is the release target; ARM64, Win32, Linux and macOS are not validated release platforms.
- Debug-symbol quality remains unverified: x64 builds report LNK4020 warnings. PDB files are not included in the portable payload. Source builders should read the [build recovery and limitations](../dev/BUILD-PDB-RECOVERY.md).
- SABnzbd, aria2 and rclone backend settings do not provide working transfer engines. Saved terminal groups reopen fresh sessions and do not preserve processes.

Read the [Transfers guide](TRANSFERS-FOUNDATION.md), [SSH requirements](TRANSFERS-BACKENDS.md) and [portable storage guide](PORTABLE-IMPLEMENTATION.md) before relying on these features. Original Microsoft Windows Terminal attribution, MIT license and third-party notices are retained.
