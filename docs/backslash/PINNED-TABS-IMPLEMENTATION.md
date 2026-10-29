# Pinned destinations

Pins use a global per-copy catalog, lazy launch and focused-pane capture. They are separate from named tab groups. This guide describes supported behavior and implementation boundaries.

Current cwd reporting, invalidation, manual-entry separation and the reusable metadata plan are documented in [Live cwd and session metadata](LIVE-CWD-AND-SESSION-METADATA.md). The earlier fallback confirmation evidence below is historical.

Every window has a Pins menu. Pin current folder creates a separate compact background shortcut; the source tab, splits, cwd and selection stay intact. The new shortcut starts a fresh session when first selected. The destination copies the profile GUID/source, connection identity, display label and reported cwd. Navigation, terminal titles and Settings reload do not repoint it. No rename/repoint editor is provided.

Ordinary new windows collect currently unviewed shortcuts. Explicit command lines, startup connections, transferred content, saved-group startup and restored layouts retain their requested contents. Existing windows can use Open here, Move here or Gather. Closing one window does not launch replacement sessions in surviving windows. Group capture skips dormant shortcuts and captures live terminal layouts without their pin association; the existing Settings exclusion/draft rules still apply.

The compact entry has a fixed 88 DIP width, profile icon, close button and status dot. Its tooltip and accessibility text retain the full destination plus bell, read-only, closed, progress, broadcast and zoom status. Hover does not expand it.

Unpin commits preference removal and demotes the existing live tab without recreating its sessions or splits. An individual tab Close/X removes its remembered preference before disposing the session. Window close, quit, bulk tab close, pane-only close and natural session exit preserve the catalog. Canceled confirmations and failed preference writes do not perform the requested close/unpin. Reopening starts the original single destination, not its previous process or later split layout.

`settings/pins.json` is separate from settings.json/state.json and editor drafts. A bounded versioned envelope contains immutable descriptors and separate attachment leases. A per-copy file lock protects read/modify/atomic replace. Each lease carries a window GUID, native window ID/HWND, process ID/birth time and nonce. Stale window/process recovery retains descriptors. Corrupt/unknown data is preserved with an error, never silently reset. A transfer fence remains until native cached content has attached; timeouts restore the source or dispose detached cores before releasing the claim. Independent windows cannot overwrite an old catalog snapshot or attach the same pin twice.

Local pinning supports direct cmd/PowerShell launchers and uses a reported or explicitly confirmed absolute Windows folder. WSL supports an exact generated system-directory wsl.exe/distribution and a reported or explicitly confirmed POSIX folder. Fixed SSH paths currently require a generated SSH profile with PowerShell 7 selected and a reported or explicitly confirmed remote folder. Its literal Set-Location script travels as UTF-16LE EncodedCommand; remote paths never become ssh.exe's local cwd. Config/default/Bash/custom/advanced SSH launchers are preserved and rejected for pin path launch. No remote discovery or command runs during fixture tests. Profile/source/host/elevation/launcher mismatches fail without guessing a default profile or directory.

Moving a live pin uses the existing native ContentManager cache and preserves core/session/split identity. That transport supports windows in the same process/elevation context. Other-process owners are shown in the menu but require closing their window before opening here. The catalog remains global to that portable copy across processes.

Native visual and interaction checks require supported desktop tools or manual testing: compact width/close target, tooltip/Ctrl+Tab/UIA text, create-without-focus-change, first-selection launch, split capture, Unpin retaining PIDs, Move/Gather retaining PIDs, window-close and individual-X differences, read-only confirmation cancellation, Settings drafts/startup groups/restoration, narrow windows and fractional DPI.

## Limitations

Native menu/dialog, multi-window move/unpin, restoration, keyboard/accessibility and DPI coverage is incomplete. Folder capture depends on supported shell metadata; use manual folder entry when a usable report is unavailable. See [folder reporting](LIVE-CWD-AND-SESSION-METADATA.md) and [typed launch identity](PIN-IDENTITY-FIX.md) for implementation restrictions.

Historical builds, runtime results and acceptance checklists are in the [Pins implementation verification record](../dev/FEATURE-RECORDS.md#pins-implementation-verification).
