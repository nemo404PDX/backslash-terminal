# Pin capture correction — 2026-10-15

**Superseded cwd behavior:** [Live cwd repair and session metadata design](LIVE-CWD-AND-SESSION-METADATA.md) replaces automatic manual fallback with per-session PowerShell reporting. **Pin current folder** requires a usable report; **Add a folder...** is separate. Historical test evidence below belongs to the earlier build.

The reported home/AppData failure is a capture-provenance error, not a restriction on those folders. In 017a79b5, `_PinFocusedDestination` passed `control.ReportedWorkingDirectory()` to `Capture(..., Reported)` even when the report was empty. The path validator then displayed the internal text “A pin needs a bounded absolute path in its own host context, without environment expansion.” This made a missing report look like an invalid user folder.

`ControlCore` deliberately seeds its working-directory value from `StartingDirectory`, possibly a literal `%USERPROFILE%`, but marks that seed unreported. Only a recognized shell cwd report sets the reported flag. A prompt reading `PS C:\Users\nemo\AppData>` and a `cd AppData` command do not themselves provide that report. The built-in prompt definitions inspected in local Windows PowerShell and PowerShell 7 contain the ordinary `PS <location>` prompt, without a cwd report. Additional shell integrations may supply reports; none is required by this fix.

Screenshot provenance: the parent supplied the exact error text and visible AppData prompt. Library read resolved the image and OCR confirmed that prompt. Local pixel inspection could not finish because the Library transfer helper uses `os.setxattr`, unavailable on Windows; no claim of direct visual screenshot inspection is made.

## Corrected flow

- A usable shell report keeps the existing quick capture path.
- Missing, empty, malformed or unresolved-token reports open **Pin a folder**. A local starting folder/home is an editable suggestion and is never presented as observed cwd.
- The user can confirm home or enter/paste AppData or another full folder path. The dialog shows **Folder that will be pinned** before the user selects **Pin folder**.
- For an explicitly confirmed local path, normal Windows variables such as `%USERPROFILE%` and `%APPDATA%` are expanded before capture. The persisted destination is the absolute literal path. `%PROFILE%` is not assumed to exist; unresolved variables receive a plain path-entry error.
- Cancel/Escape and window unload create no descriptor. Pin/Cancel return focus to the current tab. Local directory existence/access is checked on a background thread before the atomic catalog add.
- Remote/WSL requests never borrow the client's starting folder or expand local variables. They use a usable report or an explicitly confirmed absolute path in their own context. Existing generated-profile/host/elevation/launcher restrictions remain in force.
- Profile GUID/source/host and the confirmed path remain fixed; no profile setting, shell profile, SSH configuration or remote DefaultShell is changed.

## Verification

App Release/x64 build: **0 errors / 134 warnings**, **41.36 seconds**. Evidence: `artifacts/build/20261015-075007-650-Release-x64-845eb99d`.

Settings: **222/222 passed**. Eight new capture-policy tests cover missing reports, report-vs-seed precedence, unresolved reports, home/AppData, explicit variable expansion, remote isolation, cancel/no-write and clear invalid input. The existing strict seed/unknown-provenance rejection still passes. `capture-validation/full-settings-tests.log` contains the full run.

Control: **34/34 passed**, including the strengthened actual-core provenance test using `%USERPROFILE%`, home and AppData seeds. Rendering an ordinary AppData PowerShell prompt leaves ReportedWorkingDirectory empty; a recognized OSC report supplies the actual path. Full Control includes the existing backend WARP/CRT regressions.

Read-only local checks confirmed that C:\Users\nemo and C:\Users\nemo\AppData exist and their ordinary expanded paths pass validation. No contents of those folders were inspected. Both built-in prompt definitions were read using `-NoProfile`; no user shell profile was loaded or edited.

Fresh portable assembly and isolation passed. The owned local executable fixture again passed **9/9** lazy-launch/cwd/claim/natural-exit/reopen checks, with **0 forced process stops**. Those nine cases exercise runtime launch/ownership, not the new dialog's native mouse/keyboard acceptance.

Native dialog interaction remains unrun: this session exposes no supported native CUA/node_repl tool. Check the new dialog's focus, typing/paste, Enter, Escape/Cancel, resolved preview and source-session preservation manually. Earlier Move/Gather/Unpin/X/confirmation/restoration/accessibility/DPI acceptance items also remain open.

## Run and check

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-075007-650-Release-x64-845eb99d/WindowsTerminal.exe`.

Clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-PinsCaptureFix-x64-845eb99d.zip`. SHA256: **`401262299ED907F29D56A19A05540C2EF807CDA36BA52219E3E0B6108AFE1735`**. Smoke settings were moved into owned evidence; no settings directory is shipped.

1. Open ordinary Windows PowerShell without a prompt integration. In home, choose Pins → Pin current pane directory. Confirm the suggested home folder, then select the compact entry; it should open home without moving or replacing the source tab.
2. In the source shell, `cd AppData`, then pin again. Without a cwd report the dialog must explicitly ask for the folder; enter `C:\Users\nemo\AppData` or `%USERPROFILE%\AppData`, check the resolved preview, and pin it. It must not silently claim the home suggestion is current AppData.
3. Cancel/Escape one attempt and verify no pin was added. Try an undefined `%PROFILE%` and a relative `AppData`; both should give useful path-entry guidance. `%USERPROFILE%` and `%APPDATA%` should resolve before saving.
4. Navigate after creating the pin and reopen it; it must keep the originally confirmed destination. With a usable cwd report, the existing quick capture should still work.

All changes remain uncommitted atop dc027cae. Earlier source/portable artifacts, FILE-TRANSFER-DESIGN.md and user working copies were preserved. No real SSH/WSL execution, commit/push/tag/release, installation, registration or security change.

## Fresh-profile identity follow-up

The 845eb99d display-prefix comparison bug is corrected by immutable typed launch identity. See [verified cause, real adapter/Core and PowerShell 7.6.6 evidence, current portable and limits](PIN-IDENTITY-FIX.md). Latest results: 228/228 Settings, 34/34 Control, 6/6 actual capture integration and 9/9 portable runtime cases.
