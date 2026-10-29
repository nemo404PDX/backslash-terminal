# Settings placement

Interaction > Open Settings in selects Tab (default), Side-by-side, or Separate window. Menu and Ctrl+, requests use the preference. Explicit JSON/default-file actions keep their original route. Side-by-side requires an active terminal and checks the actual active leaf allocation: half its width must fit the incoming Settings minimum plus 4 DIPs of border allowance, and its height must fit the Settings minimum plus 4 DIPs. The terminal sibling must also pass the existing split check; otherwise Settings opens a tab. The whole tab/window width alone is not sufficient. Settings minimum pane size is 480 by 360 DIPs.

The process reserves one semantic built-in identity: Settings = 0. The initial allocation boundary is 10 and can grow without changing existing explicit built-in values. This namespace is separate from tab indexes, per-tab pane IDs, window IDs and terminal content IDs. Repeated opens focus the existing Settings pane/window and keep its current placement, draft and navigation. Changing the preference affects the next newly created editor. Close releases its ownership lease; pending creation prevents duplicates. Generation tokens reject stale callbacks.

## Moving Settings

Whole-tab moves/tear-out within this process retain the exact live native pane tree. Current AppHost/WindowEmperor routing creates and attaches windows on one UI thread; the transfer checks thread ownership rather than attempting to marshal XAML. The source visual is removed before target attachment, avoiding two parents. Source tab/editor interaction is suspended until the target is attached and its UI dispatcher acknowledges readiness. Only then is the source tab removed. Target failure restores the original tree, host callbacks and ownership. An unclaimed destination times out after 15 seconds and restores the source. Closing pending source/target tabs/windows is guarded.

This retains unsaved text, view-model state, current navigation and the existing conflict-safe save baselines rather than rebuilding a draft from settings JSON. Host-window/key/JSON/warning delegates are rebound to the destination. Settings does not write its draft to disk during transfer. Existing terminal siblings move as part of the same retained tree.

Moving only the Settings pane is currently blocked with an explanatory message; move its containing tab. Open dialogs, setup transfer and a save awaiting reload also block moving, preserving the editor. Cross-thread live attachment is rejected and restoration is queued on the owner thread. Cross-process drag routing is not supported by the existing same-process tab drag filter. Live editor drafts are not a persisted cross-process/restart format.

## Closing only Settings

The redundant in-view Close button has been removed. The dedicated 48-DIP header and Settings hamburger remain. Use the tab X or Close pane to close the focused Settings leaf while leaving terminal siblings running.

Dirty or stale drafts offer Discard and close / Keep editing; busy operations and save-awaiting-reload guard closing through ConfirmCloseSettings. The containing tab's visible X closes its last-focused leaf when split, preserving its siblings; for one pane it closes the tab. Ctrl+Shift+W and context Close pane share that safe leaf request. Explicit Close entire tab remains whole-tab closure. See [focused-pane close behavior and verification](FOCUSED-PANE-CLOSE.md), including inactive tabs, parent selections and pinned/read-only distinctions.

The outer grid owns the keyboard focus cycle, with local traversal inside NavigationView, so forward/reverse traversal can include the header controls. The normal and minimal NavigationView header margins both use `0,4,0,0`: this removes stale native hamburger compensation after moving the hamburger to its own row. Native breadcrumb overflow and ancestor navigation are retained; clipping does not hide unreachable labels.

## Known limitations

Historical build/test results are retained in [maintainer notes](../dev/HANDOFF.md).

Broader responsive layout/localization/DPI, keyboard traversal, save/discard interaction and live tab transfer/rollback testing is incomplete. An intermittent blank or transparent Settings split has been reported and remains unresolved.
