# Saved startup group on resident reopening

Configured On startup > Saved group now applies to an ordinary window request when the existing Backslash process has zero app windows. Adding a window while another exists does not replay the group. Initial launch behavior is retained.

Explicit command arguments, connection/content requests, restored layouts, and configured startup actions retain their existing precedence. Settings-only, hidden, and quake windows count as app windows; visibility is not used. A main-thread creation reservation counts pending creations before initialization can dispatch another request, and releases on success or failure. This prevents overlapping requests from both claiming a zero-window reopening. It does not change background lifetime or restore saved window layouts.

The transient StartupGroupReopen remoting field is separate from InitialAppLaunch. StartupGroupActivation.h supplies the shared policy and reservation; WindowEmperor creates the reservation, and TerminalWindow applies the existing saved-group exclusions.

## SSH discovery explanation

Settings > Add a new profile explains that host discovery lasts for the Backslash process, including background operation after all windows close. Fully exiting ends that choice; a fresh portable process defaults off. Background running settings navigates to Compatibility and focuses AllowHeadless without changing it. Imported workspace connections remain durable.

## Historical focused verification

GUI clicks, actual resident window reopening, simultaneous real windows, and hidden/quake routing were not exercised interactively. Tests cover the production policy/reservation helper and static routing, not an end-to-end UI automation. No real host SSH configuration, keys, connections, or user settings were used.

Native hidden-window/startup-path coverage is incomplete. [Contributor verification records](../dev/FEATURE-RECORDS.md#resident-startup-verification) describe the dated checks.
