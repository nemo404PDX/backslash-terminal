# Pinned destination implementation checkpoint

The user selected a global per-copy catalog, lazy launch and focused-pane capture on 2026-10-15. Pins are separate from named tab groups. This document records implementation boundaries; the final evidence section is populated only after validation.

Current cwd reporting, invalidation, manual-entry separation and the reusable metadata plan are documented in [Live cwd and session metadata](LIVE-CWD-AND-SESSION-METADATA.md). The earlier fallback confirmation evidence below is historical.

Every window has a Pins menu. Pin current folder creates a separate compact background shortcut; the source tab, splits, cwd and selection stay intact. The new shortcut starts a fresh session when first selected. The destination copies the profile GUID/source, connection identity, display label and reported cwd. Navigation, terminal titles and Settings reload do not repoint it. No rename/repoint editor is provided.

Ordinary new windows collect currently unviewed shortcuts. Explicit command lines, startup connections, transferred content, saved-group startup and restored layouts retain their requested contents. Existing windows can use Open here, Move here or Gather. Closing one window does not launch replacement sessions in surviving windows. Group capture skips dormant shortcuts and captures live terminal layouts without their pin association; the existing Settings exclusion/draft rules still apply.

The compact entry has a fixed 88 DIP width, profile icon, close button and status dot. Its tooltip and accessibility text retain the full destination plus bell, read-only, closed, progress, broadcast and zoom status. Hover does not expand it.

Unpin commits preference removal and demotes the existing live tab without recreating its sessions or splits. An individual tab Close/X removes its remembered preference before disposing the session. Window close, quit, bulk tab close, pane-only close and natural session exit preserve the catalog. Canceled confirmations and failed preference writes do not perform the requested close/unpin. Reopening starts the original single destination, not its previous process or later split layout.

`settings/pins.json` is separate from settings.json/state.json and editor drafts. A bounded versioned envelope contains immutable descriptors and separate attachment leases. A per-copy file lock protects read/modify/atomic replace. Each lease carries a window GUID, native window ID/HWND, process ID/birth time and nonce. Stale window/process recovery retains descriptors. Corrupt/unknown data is preserved with an error, never silently reset. A transfer fence remains until native cached content has attached; timeouts restore the source or dispose detached cores before releasing the claim. Independent windows cannot overwrite an old catalog snapshot or attach the same pin twice.

Local pinning supports direct cmd/PowerShell launchers and uses a reported or explicitly confirmed absolute Windows folder. WSL supports an exact generated system-directory wsl.exe/distribution and a reported or explicitly confirmed POSIX folder. Fixed SSH paths currently require a generated SSH profile with PowerShell 7 selected and a reported or explicitly confirmed remote folder. Its literal Set-Location script travels as UTF-16LE EncodedCommand; remote paths never become ssh.exe's local cwd. Config/default/Bash/custom/advanced SSH launchers are preserved and rejected for pin path launch. No remote discovery or command runs during fixture tests. Profile/source/host/elevation/launcher mismatches fail without guessing a default profile or directory.

Moving a live pin uses the existing native ContentManager cache and preserves core/session/split identity. That transport supports windows in the same process/elevation context. Other-process owners are shown in the menu but require closing their window before opening here. The catalog remains global to that portable copy across processes.

Native visual and interaction checks require supported desktop tools or manual testing: compact width/close target, tooltip/Ctrl+Tab/UIA text, create-without-focus-change, first-selection launch, split capture, Unpin retaining PIDs, Move/Gather retaining PIDs, window-close and individual-X differences, read-only confirmation cancellation, Settings drafts/startup groups/restoration, narrow windows and fractional DPI.

No commit, push, installation, registration or security change accompanies this checkpoint. User working portable settings and installed Terminal are outside the fixture roots.

## Validation

Final preview checkpoint: `artifacts/build/20261014-214639-341-Release-x64-017a79b5`. Release/x64 app build **0 errors / 134 warnings**, 46.46 seconds. Final Settings test build **0 errors / 8 warnings**; **214/214 Settings tests passed** (12 destination, 12 store and 6 launch/reference/group tests plus the existing 184). Final model evidence: `artifacts/build/pins-validation-20261014-213706/final-store-tests/full-settings-tests.log`.

Unchanged Control/rendering regression evidence is in `artifacts/build/20261014-214010-183-Release-x64-2b82ae13`: **34/34 Control**, CRT ABI/WARP output, render visibility and diagnostic snapshot checks passed. These were run before the final app-only restore-reference retry/elevation repair; no renderer/Control source changed afterward. Final portable assembly/isolation passed again. Its owned local fixture passed **9/9** lazy startup/cwd/claim/natural-exit/reopen assertions and all fixture sessions/windows ended naturally (**0 forced stops**). This proves those paths; it does not prove native menu, move/unpin, confirmation, restoration or visual acceptance.

The first smoke fixture incorrectly relied on a transient parent environment variable and produced no marker. A fixture using its own executable directory passed in a fresh evidence root, then passed again against the final preview. Failed build/test/smoke artifacts remain intact. No real SSH server or WSL distro was executed. This session exposes no node_repl or native computer interaction/screenshot tool.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261014-214639-341-Release-x64-017a79b5/WindowsTerminal.exe`.

Clean preview ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-PinsPreview-x64-017a79b5.zip`; SHA256 **`97198486A022EBB1AC0EEB5B0089697FBA9DAF1236ECC590954E6F83A06C0EC2`**. The adjacent settings directory was moved into owned test evidence after smoke; the delivery folder and ZIP contain no smoke settings.

Current source is uncommitted atop `dc027cae58b9414c4c0bdcb5790093fb7165ba8e`. No commit, push, installation, registration or security change. FILE-TRANSFER-DESIGN.md and the previously committed SSH/WSL/bezel work were not edited.

Native acceptance still needed:

1. Pin current reported directory from a focused split pane; confirm a separate compact background entry, unchanged source selection/cwd/splits, one fresh session on first selection and the original destination after later navigation/reopen.
2. Add splits to a live pin, then Unpin; verify unchanged process/core IDs and layout, and a normal tab to the right of pins. Verify individual X removes its remembered preference.
3. Test accepted/canceled read-only and window Close All/Quit confirmations. Cancellation must leave the request's preferences/sessions intact; window exit preserves the catalog.
4. Exercise Open/Move/Gather across same-process windows, failed transfer/timeouts, failed writes, source/target shutdown during each asynchronous phase, and failed restore-entry retries. Verify no duplicate live core. Other-process/elevation owners should give the documented bounded error.
5. Check explicit commands, restored layouts and saved-group startup, existing Settings drafts/inheritance and ordinary-new-window collection. Verify dormant pin exclusion and live pin capture as ordinary group layouts.
6. Check all tab-width modes, hover (no expansion), icon/close target, status tooltip/Ctrl+Tab/UIA text, keyboard navigation, narrow layouts and fractional DPI.

## Capture usability correction

The home/AppData capture error is corrected by [explicit folder confirmation when shell metadata is unavailable](PIN-CAPTURE-FIX.md). Latest validation is 222/222 Settings, 34/34 Control and 9/9 owned local runtime checks. Native dialog interaction remains pending.

## Fresh-profile identity follow-up

The 845eb99d display-prefix comparison bug is corrected by immutable typed launch identity. See [verified cause, real adapter/Core and PowerShell 7.6.6 evidence, current portable and limits](PIN-IDENTITY-FIX.md). Latest results: 228/228 Settings, 34/34 Control, 6/6 actual capture integration and 9/9 portable runtime cases.
