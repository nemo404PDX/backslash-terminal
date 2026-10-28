# Saved startup group on resident reopening

Implemented in uncommitted development, not the published preview. See [current candidate and verification](DEVELOPMENT-STATUS.md); the dated focused evidence below remains historical. The user reports discovery remaining enabled after last-window close/reopen with background running enabled. This does not prove every startup/hidden-window path.

Configured On startup > Saved group now applies to an ordinary window request when the existing Backslash process has zero app windows. Adding a window while another exists does not replay the group. Initial launch behavior is retained.

Explicit command arguments, connection/content requests, restored layouts, and configured startup actions retain their existing precedence. Settings-only, hidden, and quake windows count as app windows; visibility is not used. A main-thread creation reservation counts pending creations before initialization can dispatch another request, and releases on success or failure. This prevents overlapping requests from both claiming a zero-window reopening. It does not change background lifetime or restore saved window layouts.

The transient StartupGroupReopen remoting field is separate from InitialAppLaunch. StartupGroupActivation.h supplies the shared policy and reservation; WindowEmperor creates the reservation, and TerminalWindow applies the existing saved-group exclusions.

## SSH discovery explanation

Settings > Add a new profile explains that host discovery lasts for the Backslash process, including background operation after all windows close. Fully exiting ends that choice; a fresh portable process defaults off. Background running settings navigates to Compatibility and focuses AllowHeadless without changing it. Imported workspace connections remain durable.

## Historical focused verification

Six focused startup policy/reservation tests passed, including connection/content ownership transfer before and after initialization. Initial payload presence is retained before moving it into the page, so saved-group preparation cannot mistake an emptied member for an ordinary request. The complete settings suite reported 296 total, 294 passed, zero failed, and two optional local-shell cases skipped in that run. Both optional cases passed separately using the complete runtime, exercising all 296 unique cases. Fresh independent portable processes confirmed discovery defaults off. Six source-wiring checks passed; the Release/x64 application build completed with zero errors and 134 warnings. Portable isolation checks passed.

Candidate: artifacts/portable/Backslash-Dev-20261025-183242-587-Release-x64-e4eb708e/Backslash.exe

GUI clicks, actual resident window reopening, simultaneous real windows, and hidden/quake routing were not exercised interactively. Tests cover the production policy/reservation helper and static routing, not an end-to-end UI automation. No real host SSH configuration, keys, connections, or user settings were used.
