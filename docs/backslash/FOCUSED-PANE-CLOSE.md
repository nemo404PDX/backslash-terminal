# Focused-pane close and Settings footer

Implemented in uncommitted development, not the published preview. The candidate is `artifacts/portable/Backslash-Dev-20261026-172834-023-Release-x64-687a34d6/Backslash.exe`. See [maintainer handoff](../dev/HANDOFF.md).

## Visible X versus explicit whole-tab closure

For a split tab, the visible tab X closes one last-focused leaf in that clicked tab. Terminal, Settings and Transfers content use the same pane-tree close/collapse mechanism. Remaining leaves keep their content and identities; only the affected branch collapses. Settings fills the tab only if it is the sole survivor. A single-pane tab's X remains whole-tab closure.

The context menu's **Close entire tab** action deliberately closes the whole tab. Close other tabs and close tabs to the right retain their tab-level meaning. Pinned/read-only/button-visibility policy still controls whether X appears. No background detach or replacement window is introduced.

Ctrl+Shift+W (the shipped Close pane binding), the ClosePane action and context-menu Close pane share the guarded leaf request. A selected parent subtree now resolves through its retained child-focus history to one leaf, rather than silently closing that subtree. Missing, expired or detached focus history is a no-op; it never falls back to closing the whole group or an arbitrary first child. Inactive-tab X/context actions capture that tab, not the globally selected other tab.

## Content guards and races

Settings close requests share one preflight: busy/save-awaiting-reload waits; dirty or stale drafts offer Discard and close / Keep editing. A Settings tab is activated before its prompt so the editor is hosted; an absent XAML root cannot silently discard. Explicit whole-tab requests also check affected Settings content. Transfers view teardown retains process-owned job records.

The host captures pane, content and root identity before any await and checks them again after prompts. Focus changes cannot retarget an accepted request to a different pane; removed or replaced content aborts it. A per-tab generation gate covers prompts and split collapse, blocks repeated requests, ignores stale completion callbacks and reconciles an ancestor collapse that bypasses the original leaf's completion event. Standard Close pane retains read-only prompts and its existing pinned single-pane behavior separately from X/whole-tab preference removal.

## Close other panes batch action

The sole external `_ClosePanes` caller is Close other panes; its former recursive continuation is removed. The batch captures IDs, content identities and the complete leaf set, then preflights all affected Settings content before closing anything. A dirty-draft cancellation or busy editor leaves every target open. Read-only leaves remain protected. A selected parent group retains its entire subtree, and a batch cannot silently become whole-tab closure.

Settings closes immediately after approval, before animation yields could admit new edits. Remaining captured targets resolve again by retained ID and content after each own collapse; legitimate promotion into a parent is accepted. External exits, replacements, additions, ownership changes or new read-only protection stop the remainder rather than retargeting it. One request gate spans the entire batch. Collapse waiting returns to the UI dispatcher and is bounded; no background detach is introduced. This prevents preflight cancellation from partially closing the batch; it does not promise rollback of already closed panes after a later external change.

Three additional model regressions cover exact leaf identity multisets, own promotion versus external changes and selected-parent preservation. Eleven batch source checks supplement the existing 22 close-routing checks. Actual batch dialogs and animation still require GUI QA.

## Portable footer

The identified portable-mode notice previously shared a fixed-height row with Save/Discard and could draw underneath them. It now occupies a separate auto-sized wrapping row above the action controls. The action row stacks its measured localized labels when required by its own available width; button text can wrap and is constrained to that row. The notice's English wording is **Backslash is running in portable mode.** Its existing Learn more link is preserved; info text follows theme resources, including high contrast.

The 48-DIP top chrome, Settings hamburger, focus cycle, breadcrumb margin fix and brand accent are retained. Redundant in-view Close buttons in Settings and Transfers have been removed; tab/pane close routes retain their guards. This footer change does not alter other warnings or OS settings.

## Verification and limits

Release/x64: zero errors, 134 warnings, 00:00:49.15. Complete-runtime settings suite: 323 total, 321 passed, 2 optional shell cases skipped in the standard run; both shell cases passed separately. The settings suite includes 20 SettingsPlacementTests methods. There are 33 close/batch-routing, 10 footer, 10 chrome, 9 breadcrumb and 18 accent source/contrast checks; portable isolation has 34 checks. Tests cover production selection/gate/ownership policies with synthetic trees and source routing, not live XAML interaction.

The user's repeated sequence worked: close terminal beside Settings, create a new terminal tab, close the surviving Settings tab, then open Settings beside the new terminal. Production ownership-coordinator regressions cover that release/reopen sequence and closing Settings directly. The earlier side-by-side reopening failure remains intermittent/unreproduced; no stale-lease diagnosis or speculative placement fix is claimed. It is distinct from the older unresolved blank/transparent-tab report.

Actual GUI X/context/keyboard prompts, nested tree collapse/animation, inactive-tab activation, resizing/localization/DPI and footer accessibility still require interaction/visual QA. No GUI, real SSH connection, private configuration import or user-setting reset was used in this verification.
