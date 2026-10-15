# Fresh-profile pin identity correction — 2026-10-15

**Superseded cwd behavior:** [Live cwd repair and session metadata design](LIVE-CWD-AND-SESSION-METADATA.md) replaces automatic manual fallback with per-session PowerShell reporting. **Pin current folder** requires a usable report; **Add a folder...** is separate. Historical test evidence below belongs to the earlier build.

## Verified cause

The 845eb99d screenshot's “The live connection does not match the current profile” error came from a false comparison against a decorative title prefix. The prefix producer used `SPACE U+00B7 SPACE`; the capture guard expected `SPACE U+00C2 U+00B7 SPACE`. The capture-fix staging path had decoded a UTF-8 source fragment as Windows ANSI and re-encoded it. A newly generated `WINBLOWS · PowerShell` title therefore could not pass that boundary test, even with the correct own profile/host. This was an implementation/staging and test-coverage error, not evidence that the user's host or profile had changed.

The genuine fresh generated-PowerShell regression now creates the profile through SettingsLoader/generator layering, selects it through `GetProfileForArgs(nullptr)`, invokes the real `TerminalSettings::CreateWithNewTerminalArgs`, and constructs/initializes the real runtime ControlInteractivity/ControlCore. It reproduces the old title-prefix rejection, then passes the replacement preflight and the cwd-confirmation path. Matching data is derived by the production adapter/core rather than hand-filled strings.

## Corrected identity path

- The settings adapter records the resolved profile GUID/source and unformatted launch identity after startup command-line overrides are applied.
- ControlCore snapshots those fields and the effective launcher at construction. Settings reload, live titles and profile display-name decoration do not rewrite the initial snapshot. TermControl exposes the same frozen fields from its core; moving cached content preserves them.
- Pins checks profile GUID/source, supported launcher kind, host identity and elevation compatibility through shared production `PrepareCapture`/`PreflightCapture`. No display prefix or separator is parsed.
- Windows local computer-name comparison uses whole-string ordinal case-insensitive matching. A host-name prefix or different GUID/source is rejected. SSH configured aliases and WSL distribution identities keep exact boundaries.
- Bare/full-path local PowerShell launchers remain equivalent local kinds. SSH/WSL/remote/script startup overrides of a local profile are rejected. Unknown advanced launchers remain untouched.
- The earlier cwd fix remains: a usable report permits quick capture; otherwise the user explicitly confirms/edits a folder. Local variables are resolved before saving a fixed literal path. Starting-folder seeds and prompt text are never silently claimed as current cwd.

No new pin JSON fields or catalog migration is needed. Runtime launch metadata is not serialized into remembered descriptors. No shell profile, SSH config, keys, credentials, remote DefaultShell or user working portable settings was edited.

## Actual test evidence

App Release/x64 build: **0 errors / 135 warnings**, **6:06.34**. Final evidence: `artifacts/build/20261015-082920-327-Release-x64-e0d3b2a7`. Native interface changes account for the broad rebuild; existing packaging/resource warnings remain.

- **6/6 focused actual launch/core/capture tests passed**, no skipped/blocked tests. Evidence: `artifacts/build/20261015-082920-327-Release-x64-e0d3b2a7/identity-validation-retry3/focused-identity-merged-resource-tests.log`.
- **228/228 full Settings tests passed**, including those six cases and the existing SSH/WSL/bezel/pin regressions. Evidence: `artifacts/build/20261015-082920-327-Release-x64-e0d3b2a7/identity-validation-retry3/full-settings-tests.log`.
- **34/34 full Control tests passed**, including frozen launch metadata after settings reload and existing WARP/CRT rendering cases. Evidence: `artifacts/build/20261015-082920-327-Release-x64-e0d3b2a7/identity-validation-retry3/full-control-tests.log`.
- Fresh portable assembly/isolation and **9/9 owned local runtime cases** passed; the runtime fixture ended all its sessions naturally (**0 forced stops**). Evidence: `pin-portable-smoke.json`.

Coverage includes generated default pwsh with bare/full paths, inherited Windows PowerShell, startup arguments, Unicode/decorated/renamed titles, reload, local hostname case, GUID/source/host mismatches, unknown/remote startup overrides, generated SSH/WSL host boundaries and unavailable-cwd confirmation.

The opt-in real local transport case additionally launched an owned **PowerShell 7.6.6** through ConPTY and the actual settings adapter/core, verified its root PID, `WT_PROFILE_ID`, cwd and preflight, and completed explicit AppData confirmation. It used `-NoProfile -NonInteractive`, wrote only an owned temporary marker, and closed only its own connection. The SSH/WSL identity cases use no-process fixture transports; no remote command/distribution ran.

The standalone test harness initially needed corrections. It now activates the exact adjacent build DLL factories directly, without package registration or selecting an installed Terminal factory. Its model-only resources.pri was insufficient for TerminalConnection's exit callback; a local-PDB fault lookup resolved the crash to `GetLibraryResourceString`/LibraryResources.cpp:93. Merging model/control/connection PRI scopes beside TE.exe corrected that harness teardown. Failed compilation/activation/resource attempts are preserved in the earlier identity-validation folders and logs. These are not presented as successful tests.

## Delivery and remaining checks

Extract the **complete ZIP into a new folder** and keep `.portable` beside the executable.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-082920-327-Release-x64-e0d3b2a7/WindowsTerminal.exe`.

ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-PinsIdentityFix-x64-e0d3b2a7.zip`. SHA256 **`469B872FC4E0D88EA8BE69144DB28BB3B0AF2775280EFB9D97D9B05248DD3D64`**. No smoke settings are shipped; source/evidence checkpoint and exact changed-file hashes are beside `build.log`.

Native UI interaction remains unrun because no supported native CUA/node_repl tool is exposed. The adapter/core identity guard and confirmation policy are tested; this is not a claim of clicking the Pins menu or typing in the dialog.

1. Fresh launch the new portable's default PowerShell, choose Pins → Pin current pane directory. It should reach folder confirmation if cwd is unreported, without the false identity error.
2. Confirm home, select the compact entry, and check its new session. In the source tab, `cd AppData`; when confirmation is required, enter `%USERPROFILE%\AppData` and verify the resolved preview before pinning. Cancel/Escape must add nothing and restore focus.
3. Check display rename/title changes and settings reload; they must not create false local identity mismatches. Changed profile GUID/source or an actual different host/unsupported launcher must remain blocked.
4. Earlier native Move/Gather/Unpin/X/read-only confirmation/restoration/accessibility/DPI acceptance remains open.

All source remains uncommitted atop dc027cae. Previous 845eb99d/017a79b5 builds, ZIPs and source snapshots are retained. FILE-TRANSFER-DESIGN.md was not edited. No commit/push/tag/release, installation, registration, security change or remote execution.
