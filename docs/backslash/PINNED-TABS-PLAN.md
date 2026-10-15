# Pinned tabs follow-on plan

[Project entry point](README.md) | [Saved tab groups](SAVED-TAB-GROUPS.md) | [Settings layout proposal](SETTINGS-LAYOUT-PROPOSAL.md)

Status: **buildable preview checkpoint, 2026-10-15 UTC**. The user selected global unviewed collection, lazy launch and focused-pane capture. Runtime/menu/persistence integration and the live-cwd repair are implemented; 230/230 Settings tests, 34/34 Control tests, 759 parser tests (one preexisting skip) and 9/9 owned local runtime assertions pass. Native latest live-cwd UI interaction/visual acceptance remains pending. See [implementation and evidence](PINNED-TABS-IMPLEMENTATION.md). Historical questions below are superseded by the approved choices near the end. Baseline is `dc027cae`.

Current scoped cwd repair and separate future listing slices: [Live cwd and session metadata](LIVE-CWD-AND-SESSION-METADATA.md).

## Desired core model: remembered destination

The user's desired analogy is file-browser Quick Access for a fixed folder location: a pin remembers a **destination/launch descriptor**, while an open connection is its **runtime instance**. This is the desired product model, not a claim about every file browser implementation. Compact tab presentation is one view of that remembered destination; the path belongs in the pin design rather than being merely cosmetic tab metadata.

A destination may describe a local profile/shell plus path, a saved SSH endpoint plus remote path, or, once such a pane exists, an SFTP endpoint plus directory. These are distinct destination types; no SFTP pane, remote-directory launch mechanism or new endpoint schema is implemented or approved here. Reuse stable existing profile/endpoint references where possible and validate destination paths for their own local/remote context.

Stable pin identity identifies the remembered descriptor; runtime tab/pane/connection/content IDs identify an open instance. Moving/focusing an instance does not inherently change the destination; reopening launches freshly from the descriptor under the chosen policy. Live cwd/navigation and saved destination must not be conflated.

**Agreed fixed destination:** a pin locks its specific destination/path at pin time. Subsequent navigation/cwd changes MUST NOT live-update it. Reopen uses the original pinned path; a different destination means creating another pin, preserving the original unchanged. Remove an undesired pin; no pin edit, repoint or rename workflow is in scope. Capture validation, unavailable-path fallback and persistence mechanics still require design. The settled Unpin/individual close/window-close semantics remain unchanged.

Customization belongs to **groups/workspaces**: choosing profiles and locations, composing layouts, and potentially remembering last location as a separately scoped future behavior. Pins are **simple fixed-location quick shortcuts**, with no group-like profile/location editor, folder-mode selector, split-layout customization, saved-focus controls or last-location settings. This distinction adds no group implementation or change to existing group scope.

## Agreed creation UX: separate background destination

Creating a pin from the current known destination adds a **separate compact pinned destination tab/background entry**. It does not convert the current working tab into the pin, switch tabs, steal focus, change cwd or replace/restart the current session. The working terminal stays active so the user can continue navigating. The new pin retains the captured fixed path independently of that terminal's later location.

Example: work in folder A, pin A in the background, stay in the same active terminal and navigate to folder B. Selecting the separate pinned entry accesses a separate session at A, avoiding cd back/forth in the working terminal at B. Creation must not make the current tab's live attachment the pin's runtime instance merely because it supplied the destination.

**Candidate, not user-selected implementation requirement:** defer creating the pin's connection until first selection. Lazy launch could avoid unnecessary processes/remote connections at creation; eager background launch versus lazy launch remains a decision. Whichever is chosen, activation first opens a separate session at the captured destination or accesses that pin's already-open separate session. Subsequent selection of an open instance preserves its live cwd rather than silently restarting it at A; fresh reopen uses fixed A.

The future /pin convenience, if implemented later, creates this new background destination entry under the same rules. It does not repoint an existing pin or convert the working tab. Keep pin creation simple; durable configuration/composition belongs to groups/workspaces, which remain the current priority.

## Agreed compact presentation

A pinned tab has a fixed compact width containing the existing profile/connection icon and a close **×**. The full saved name and host are available in the hover tooltip, Ctrl+Tab switcher and accessible tab name. Hover must not expand the tab or shift neighboring tabs. Exact width, hit-target sizing and rendering are implementation measurements, with keyboard, high-DPI and touch usability required.

Reuse the SSH menu/profile icon resolution rather than introduce a separate SSH icon vocabulary. A future FTP/SFTP pane should reuse its connection icon once that pane type exists; no FTP/SFTP implementation is implied here. Optional tab colors can help distinguish identical icons, but cannot be the sole identity or state signal. Full textual identity must remain available without hover and be distinguishable to screen readers.

## Agreed close, Unpin and window-close behavior

The app-level remembered-pin storage design remains proposed, but the user has settled these three separate actions:

- **Unpin:** remove the remembered pin preference, keep the current session open, and move its tab right into the normal, unpinned tab area. Preserve the live connection, split panes and runtime identity; do not close or restart it.
- **Close with ×:** close the pinned tab/current session and remove its remembered pin in one action. Never require Unpin first before Close, and never treat × as merely unpinning. Do not leave a remembered or dormant pin behind after this explicit close.
- **Close the whole window:** preserve remembered pins; their subsequent access/restoration follows the unresolved cross-window policy, independent of window close order. Window teardown may dispose live tabs/connections but must not invoke per-tab explicit Close/unpin persistence semantics or erase the catalog.

Do not reconstruct the catalog from the final surviving window, previous-session autosave or automatic group capture. Separate the cause of tab disposal from visual tab removal so window shutdown cannot accidentally perform explicit pin removal.

Label Close versus Unpin clearly and apply existing close confirmations where appropriate. A canceled close must leave both the live session and remembered pin unchanged. Explicit close must not immediately reconnect or execute anything to replace the tab. The mapping of close shortcuts, pane-only close and separate bulk tab-close actions to pin removal still needs a defined contract; this does not reopen the settled ×, Unpin or whole-window-close behavior.

Reopening a remembered pin creates a fresh terminal/connection using a validated fixed destination descriptor and the current profile launch configuration. It is not remote-process persistence, attachment to an old PTY, restoration of credentials, or recovery of output. A profile's configured command/RemoteCommand may execute on launch; disclose that normal behavior and do not add captured shell commands, environment snapshots, secrets or scrollback to the pin descriptor.

Exact storage location, versioned schema, stable ID format, Save/Reset versus immediate pin persistence, maximum sizes and invalid-data preservation are unresolved. Prefer explicit pin/Unpin/Close changes and safe write-failure handling over automatic descriptor updates from cwd, live process titles or shutdown.

## Cross-window restoration policy: UNRESOLVED

**Firm invariant: every eligible window can create pins from its own tabs without converting those working tabs. There is no exclusive "pin-capable window".** A surviving window must not require closing/reopening to regain access to remembered pins whose live attachments ended in another window. The mechanism and persistence topology are not selected.

Keep the stable remembered preference set conceptually distinct from live attachments and their window locations. The app-level set plus exclusive live ownership model is one proposal, not finalized architecture. The earlier blanket rule that additional windows never reopen pins and the subsequent automatic unowned-pin claim rule are both insufficient as settled policy.

Preserve the user's earlier example as a design scenario: A has pins 1/2, B gets pin 3; close B and open C, and only pin 3 returns while A keeps 1/2. Close all windows and launch again, and 1/2/3 return together. The latest objection adds the missing case: after B closes while A remains open, A must be able to regain access to remembered pin 3 without closing/reopening, and A must remain able to pin its own tabs. Do not treat the example as approval of an exclusive owner or automatic reopening mechanism.

Candidate solutions, **neither chosen by the user**:

- App-level remembered set plus live attachment tracking, with an explicit **Open remembered pins** command/menu available in eligible existing windows for currently unowned pins. This could provide deliberate access without automatically creating remote connections/tabs when another window closes. Decide discovery, selection, ownership/dedup and new-window behavior; the command is a proposal, not an approved control.
- Simpler window-local pins with last-window restore. Evaluate window identity, remembered-set aggregation, close-order dependence and how a surviving window accesses the closed window's remembered pins. This alternative has not been selected and must be reconciled with the earlier all-pins restoration example and the latest survivor-access invariant.

Concrete three-window decision scenario: A, B and C each hold their own pins. Close B and C while A stays open, then open D. Should D combine pins from both closed windows? A global unowned-pin pool would do so, but this merges previously separate workspaces. That is a consequence of the proposal, **not accepted behavior**. Decide whether the user selects which closed window/workspace to reopen, deliberately combines sets, or uses another policy; A still needs access without reopening and remains able to pin.

A further candidate is **window/workspace-associated pins with explicit reopen**, possibly using saved groups as stable workspace identity. Neither workspace identity nor global pooling is chosen. Research association lifetime, unnamed windows, multiple windows of one group, group rename/delete/recreate and independent pin ordering before any schema extension. Using a group as identity must not imply automatic group autosave, changing the approved initial-launch-only startup rule, or collapsing distinct terminals.

Do not silently choose any candidate or automatic claim/reopen behavior. Reopening still creates fresh sessions and may execute the profile's normal command/RemoteCommand, so access to a remembered preference must be distinguished from executing its recipe.

The selected named startup group remains **initial ordinary launch only** regardless of pin policy. Pin access/restoration must not rerun that group in a surviving or additional window. Ordinary shortcut/new-window behavior, existing-window command routing, explicit commands/actions/profile requests, startup connections, transfers and restore-created windows require a documented pin eligibility policy. Preserve requested content until those choices are settled.

Previous-session restoration precedence, pin/group composition, Default profile behavior when pins exist, empty/failed fallback, ordering and focus remain pending. Do not append duplicate recipes to restored windows. Preserve group order/saved focus and avoid an accidental extra default tab. No pin persistence topology or automatic execution policy is approved by this section.

## Global Pins menu candidate (pending user choice)

The user is exploring a global remembered set with a new window collecting currently unviewed pins, and asks how to combine pins into an already-open window without closing everything. A global **Pins** dropdown could provide that access. The collection policy remains under discussion; do not discard this preference because another candidate uses window-local/workspace-associated pins. No menu action below is approved or implemented.

Candidate menu rows show the full pin identity and current owning window or **Closed**, keyed by stable pin ID rather than name/profile/host. Proposed actions:

- **Open here** for a closed pin: validate its recipe and launch a fresh session in the current window.
- **Move here** for an open pin: transfer its live tab into the current window, preserving the same session, panes and current cwd.
- **Gather all pins here:** deliberately combine live pinned tabs from other windows and open closed remembered pins here. Transfer open sessions without restarting them; validate fresh launches separately. Define scope, ordering, focus, partial failure and confirmation before implementation.

Keep these distinct from **Focus existing**, which activates the owning window/tab without transferring or launching; **Duplicate as new session**, which explicitly creates another fresh terminal with a separate runtime identity; and **Reopen**, which launches from a remembered recipe after the old attachment has ended. Duplicate behavior, remembered-template association and automatic new-window collection need decisions; no action silently duplicates a session or steals input focus.

If this candidate is chosen, recheck row ownership at action time. Concurrent Move/Open/Gather/new-window requests, a closing owner, transfer cancellation and settings changes must resolve by stable pin ID with atomic claim/handoff and clear conflict feedback. A stale menu must not launch a second live attachment accidentally. Gathering is an explicit user operation, not automatic execution when a window closes; every eligible window remains able to create separate background pins from its own tabs.

## Agreed fixed pin path; capture/fallback details pending

Profiles select the shell/connection. Groups compose a set and may use profile-default folders rather than fixed folders. Pins provide quick access to a specific host/path destination captured at pin time. Existing group profile-default/captured-folder modes remain unchanged; pin policy must not alter them.

The pin destination is fixed: subsequent navigation, cwd reports, live titles, shutdown and settings reload MUST NOT update it. Reopening launches a fresh session at the original pinned path. For a different location, create a **new pin** with its own stable ID and fixed destination, preserving the original pin unchanged. Remove/unpin an undesired pin and create another for another destination. No editing, repointing or renaming workflow is planned. Remember-last-known-folder/live-update behavior is not a pin mode or candidate.

A future **/pin** app command could create a new pin from the current known directory/destination. This is a deferred convenience for later work, not current implementation or an automatic live-update mechanism. It would still require valid local/remote context and create another remembered descriptor rather than overwrite the original. Groups remain the current priority.

Moving the existing live tab preserves its current running session and cwd, which may have diverged from the pinned path. That move does not rewrite the pin. Closing and reopening the pin uses its fixed remembered destination, not the moved session's latest cwd. Focus existing likewise changes neither session location nor descriptor.

Capture at pin time needs a supported report or explicit user entry and must distinguish seeded profile directories from actual reported locations. Reuse conservative path validation where appropriate; reports are advisory, not authenticated evidence. For remote paths, require known/trustworthy remote context or explicit user entry. The local SSH client's cwd must never be inferred as the remote shell's path, and local drive paths must not be injected into remote launch arguments. If the destination cannot be established safely, request entry or explain why pinning that path is unavailable; do not fabricate it.

Unavailable/unsupported pinned paths on reopen require a clear fallback choice/error policy still to be designed. Any one-time fallback leaves the saved destination unchanged; pinning that other destination creates another pin. Remote path capture/launch support remains research, not an implemented recovery promise. This plan adds neither remote-process persistence nor a generic session manager, file-transfer feature or automatic command replay.

## Conditional ownership research (not implemented)

If the app-level live-ownership candidate is chosen, research atomic claim/release per stable pin ID across windows/processes, reservation/attachment commit, failed-claim rollback, concurrent existing-window Open remembered pins requests and simultaneous new windows. Keep preferences separate from transient ownership; exclusive attachment claims must never imply exclusive permission to pin tabs.

Under that candidate, window teardown releases ownership only when the old attachment can no longer race a claim; canceled confirmation retains it. Quit-all gates claims/reopens during teardown. Transfer hands off the same attachment atomically without a release/reopen gap or restart. Recover only proven stale owners after crash, preserving preferences and avoiding duplicate live sessions. Natural connection termination and dormant-slot policy need decisions.

If window-local persistence is chosen instead, research stable window/template identity, aggregation/restoration rules, survivor access, close-order behavior, transfer ownership and crash/write-failure boundaries. Compare all candidates against the same user scenarios before implementation. No coordinator or last-window overwrite strategy is selected.

## Identity, deduplication and movement

Use stable **pin template IDs** separate from runtime tab/pane/connection IDs and separate from saved group IDs. A same-profile terminal is not necessarily the same pin: two deliberate terminals using the same profile/host must remain distinct. Do not deduplicate by display name, icon, profile GUID or host alone. Group rename must not break association; delete/recreate with the same name must not silently rebind it.

If a group recipe deliberately refers to a remembered pin, deduplicate that explicit stable association within one initial composition. The representation of that association is unresolved; existing groups do not imply a pin relationship. Without an explicit association, preserve both intentional instances. Define behavior for repeated references to one pin, removed pins and group copies before adding a schema extension. Deduplication must preserve layout and focus rather than silently drop a split pane.

For existing split tabs, research only how a simple destination pin associates with its live tab/pane and what whole-tab Close/Unpin/Move does to that running instance. Preserve live panes during movement or Unpin. Do not turn the pin into a customizable multi-pane saved recipe or capture its layout/focus settings; group/workspace composition owns that behavior. Reopening a pin launches its fixed destination rather than recreating a user-customized split layout.

Tear-out and transfer preserve the live pin association, connection and runtime content identity without restarting it or creating an accidental duplicate remembered template. Attachment handoff, destination representation and source placeholder rules depend on the unresolved persistence policy. Under the app-level ownership candidate, require an atomic handoff without release/reopen duplication. Closing either window retains remembered preferences rather than implying explicit Unpin.

For missing/disabled/replaced profiles, incompatible elevation, invalid recipes or folders, validate before launch. Proposed behavior is a clear skipped-pin/fallback notice rather than silently bind another profile; decide whether a missing pin has a recoverable dormant slot and when one default-profile tab is needed if no usable startup content remains. Retain folder trust limits and explicit fallback choices from groups. No background retries, reconnection loops or new command execution as a side effect of focus, hover, settings navigation or icon refresh.

## Exit confirmation and close intent (agreed behavior)

When the window-exit **Close All** confirmation is accepted, terminate the running sessions in that window and retain remembered pins. Existing-window access and subsequent restoration follow the unresolved cross-window policy. Accepted quit-all similarly terminates the targeted windows/sessions while retaining the catalog. **Cancel makes no session or pin mutations.** An individual pinned-tab close button closes that tab and removes its remembered pin; Unpin keeps the session open and moves the tab right into the ordinary tab area. These are separate user intents even if their eventual UI removal shares code.

Model close intent separately from tab removal/shutdown: explicit individual close, explicit Unpin, window exit, quit-all, transfer/detach, natural connection termination and recovery are not interchangeable causes. Shutdown/quit-all teardown must never trigger the individual-close pin-removal side effect. Do not attach unconditional catalog deletion to _RemoveTab, tab.Shutdown or shared disposal callbacks. Confirmed pin removal must target the stable pin ID, not a profile/name or whichever window exits last.

The exit dialog's **Close All** button is an exit confirmation, not a blanket decision about a separate bulk tab-close action. Existing Close Other Tabs and Close Tabs After, plus any separately exposed Close All Tabs command, need their own pin-removal contract before implementation. Keep that bulk-action decision open; it must not weaken the settled exit, individual close and Unpin behavior.

Serialize or otherwise guard concurrent/repeated exit confirmations before destructive work. Canceling a pending confirmation leaves all sessions and preferences untouched by that request; repeated accept must not double-remove pin preferences or run per-tab explicit-close effects. Check last-window and shared/moved-pin cases, including event callbacks after the initiating window unloads.

Crash or forced process termination is not an explicit Close/Unpin instruction. Retain the last successfully persisted pin catalog; do not infer removal from missing live tabs on recovery. This does not promise recovery of running sessions, remote processes or uncommitted preference writes. Define atomic persistence/write-failure reporting for a confirmed individual close so the remembered result is not silently inconsistent with the user's action.

## Checked inherited close paths

Source checked on 2026-10-14; this is implementation research evidence, not a completed pins implementation.

- [TerminalWindow::CloseWindow](../../src/cascadia/TerminalApp/TerminalWindow.cpp#L912) delegates to the root page. [TerminalPage::CloseWindow](../../src/cascadia/TerminalApp/TerminalPage.cpp#L2318) checks multiple tabs and ConfirmCloseAllTabs, awaits _ShowCloseWarningDialog, returns on non-primary result and raises CloseWindowRequested on acceptance. [The warning helper](../../src/cascadia/TerminalApp/TerminalPage.cpp#L908) selects CloseAllDialog. Preserve the confirmation preference and audit repeated/reentrant requests around _displayingCloseDialog.
- [TerminalPage::RequestQuit](../../src/cascadia/TerminalApp/TerminalPage.cpp#L2216) awaits _ShowQuitDialog and raises QuitRequested only for acceptance. [AppHost event wiring](../../src/cascadia/WindowsTerminal/AppHost.cpp#L260) routes window-close and quit requests separately; [AppHost::_RequestQuitAll](../../src/cascadia/WindowsTerminal/AppHost.cpp#L1052) posts a quit message. Trace host Close/_CloseRequested and orchestrator teardown through every affected window before adding catalog writes.
- [TabManagement::_HandleCloseTabRequested](../../src/cascadia/TerminalApp/TabManagement.cpp#L402) handles the individual tab read-only confirmation, records reopening actions and calls tab.Close. [TabManagement::_RemoveTab](../../src/cascadia/TerminalApp/TabManagement.cpp#L430) shuts down and removes the tab, then handles an empty window. The pinned close intent must survive this pipeline, including a pinned tab being the last tab; cancellation must not remove its preference.
- [TabManagement::_RemoveTabs](../../src/cascadia/TerminalApp/TabManagement.cpp#L821) invokes the tab-close handler for each selected tab. [Close Other Tabs / Close Tabs After](../../src/cascadia/TerminalApp/AppActionHandlers.cpp#L768) use this bulk path, which is separate from exit confirmation.
- [TerminalPage::_MoveTab](../../src/cascadia/TerminalApp/TerminalPage.cpp#L2542) and [tear-out removal](../../src/cascadia/TerminalApp/TerminalPage.cpp#L5740) also call _RemoveTab after transferring content. Removal itself cannot establish an explicit-close/unpin intent.
## Implementation research targets

Source paths below were checked on 2026-10-14; anchors are research entry points and may move as groups work continues.

- [Tab.cpp](../../src/cascadia/TerminalApp/Tab.cpp): TabViewItem construction, header, tooltip and icon updates, tab titles and switcher/automation representation. Check native TabView close affordance and minimum dimensions before selecting a width.
- [TabHeaderControl.xaml](../../src/cascadia/TerminalApp/TabHeaderControl.xaml) and [TabHeaderControl.cpp](../../src/cascadia/TerminalApp/TabHeaderControl.cpp): compact header and status indicators. Preserve bell, elevated/closed/locked/broadcast and zoom state without inaccessible clipping.
- [TerminalPage.cpp](../../src/cascadia/TerminalApp/TerminalPage.cpp): width-mode handling, profile-menu icon resolution, close requests, tab ordering, moves/tear-out, group capture/open and saved focus. Verify all inherited width modes and overflow behavior without changing ordinary tabs.
- [AppActionHandlers.cpp](../../src/cascadia/TerminalApp/AppActionHandlers.cpp): close tab/pane, Close All, move and explicit action behavior. Implement the agreed Close versus Unpin distinction, retaining inherited confirmations. Trace whole-window teardown separately so it never executes per-tab explicit pin-removal semantics; decide mappings for close shortcuts, pane-only close and separate bulk tab-close actions; exit-confirmation behavior is settled.
- [Pane.cpp](../../src/cascadia/TerminalApp/Pane.cpp), [Tab.cpp](../../src/cascadia/TerminalApp/Tab.cpp) and [SavedTabGroups.h](../../src/cascadia/TerminalSettingsModel/SavedTabGroups.h): group recipe validation as reference for exact profile/path safety, without importing split/focus customization into pins. Separate fresh launches from BuildStartupKind::Content transfers.
- [WindowEmperor.cpp](../../src/cascadia/WindowsTerminal/WindowEmperor.cpp), [TerminalWindow.cpp](../../src/cascadia/TerminalApp/TerminalWindow.cpp) and [AppLogic.cpp](../../src/cascadia/TerminalApp/AppLogic.cpp): explicit initial-launch context, previous-session restoration and additional-window dispatch. Compare existing-window remembered-pin access and both persistence candidates; conditionally research a coordinator for atomic claims/releases, transfer handoff, shutdown gating and stale-owner detection. Generic settings startupActions cannot express these ownership and launch-origin rules.
- [profiles.schema.json](../../doc/cascadia/profiles.schema.json) and [saved-groups plan](SAVED-TAB-GROUPS.md): select catalog/schema ownership, Settings clone semantics, stable references and failure reporting after the groups behavior is locked down.

## Acceptance checklist (future implementation; unrun)

1. Pinned tabs show icon plus × at a constant width. Hover/selection/tooltip does not expand or move neighbors. Verify ordinary tabs, each inherited width mode, overflow, many pins, DPI and fullscreen.
2. Full saved name/host is available through tooltip, Ctrl+Tab and screen reader. Keyboard reaches close and Unpin with clear names; focus returns predictably. Identical icons/colors remain distinguishable textually.
3. Resolve and reuse SSH profile icons, including missing icon fallback. Live mixed-profile split-tab association follows the chosen contract, without pin layout customization; future FTP/SFTP cases stay excluded until the pane exists.
4. Test the three settled actions independently: Unpin removes the preference, keeps the same live session/splits/runtime identity and moves its tab right into the normal area; × directly closes the pinned tab/session and removes the remembered pin without requiring Unpin; whole-window close preserves remembered pins; subsequent access/restoration follows the unresolved cross-window policy. Close multiple windows in either order, including a window containing the last live pin instance; assert teardown never calls per-tab explicit Close/unpin persistence semantics. Verify canceled close changes neither session nor preference, and explicit close causes no immediate reconnect. Test close shortcuts, pane close and separate bulk tab-close actions against their separately decided mappings; exit-confirmation behavior is settled.
5. Restart and settings reload preserve explicitly remembered pins and order independent of which window closed last. Failed/canceled writes preserve the previous catalog; unsupported data is not destructively rewritten.
6. Evaluate the candidate policies against the earlier A/B/C scenario: A has pins 1/2, B gets 3, close B and open C to regain 3; close all and initial launch regains 1/2/3. Also keep A open after B closes: A can regain access to remembered 3 without reopening, and every eligible existing/new window can pin its own tabs. No exclusive pin-capable window. Decide the access mechanism before testing automatic execution; selected named groups remain initial-launch-only.
7. Explicit stable pin/group associations deduplicate once, while two intentional same-profile/host terminals remain distinct. Test group rename, pin/group deletion and recreation, copies and stale associations. Pins have no rename workflow.
8. Preserve group tab order, split ratios and saved focus under the chosen pin ordering/focus policy. No lost panes or duplicate default tab; test empty, partially invalid and wholly invalid composition.
9. Missing/disabled/replaced profiles, invalid recipe/version, elevation and missing/unsupported folders produce clear notices and the chosen safe fallback before launching affected recipes. No implicit default-profile substitution inside a pin.
10. Tear-out and cross-window/process transfer keep the live connection and runtime identity, preserve the decided pin association and do not create duplicate catalog entries. Closing source/destination windows retains remembered preferences.
11. Reopen produces fresh connections, never old remote processes/output. No autosave of shell input, folder updates or live titles; no launch triggered by hover, focus or settings navigation.

12. Accept window-exit Close All: targeted sessions terminate and remembered pins remain. Cancel: zero session/pin mutations. Repeat quit-all, multiple close orders and last-window exit; verify surviving-window access under the selected policy and preserve inherited confirmation settings.
13. Repeat/reenter confirmation while a dialog is pending, cancel then retry, and exercise repeated acceptance/unload callbacks. Assert one accepted exit operation, no individual-close pin removal from teardown and no mutations from canceled requests.
14. Move/tear out a pin and preserve its live connection/runtime identity and remembered preference under each candidate policy. Test accepted/canceled exits and both close orders without implicit Unpin. For app-level ownership, verify atomic handoff and no duplicate live claim; window-local rules require equivalent explicit attachment/restoration tests.
15. Simulate crash/forced exit before and after preference persistence boundaries. Recovery retains the last successfully persisted catalog without inferring unpin from vanished tabs; no session/remote-process recovery claim. Verify explicit close/Unpin write failure is reported and does not silently corrupt the catalog.

16. After selecting a policy, race existing-window access requests, new windows, window close, pin creation, explicit Close/Unpin, settings reload and launch failure. Every eligible window remains able to pin. For app-level claims test atomicity/rollback; for window-local persistence test identity/aggregation. Neither may resurrect explicitly removed preferences.
17. During quit-all/repeated confirmation prevent unexpected replacement sessions. Test crash/recovery and last-successful-write boundaries for the chosen topology; stale live attachment state must not delete preferences or permanently prevent survivor access. No remote-session recovery claim.
18. Once decided, test ordinary shortcuts/new-window and existing-window remembered-pin access against the documented eligibility/execution policy. The proposed Open remembered pins command, if selected, requires keyboard/screen-reader discoverability and deliberate execution. Named groups stay initial-launch-only; explicit commands, startup connections, transfers and restored windows preserve intended content.

19. Decision question, not a selected result: three windows each hold pins; close two and open a fourth. Should pins from both closed windows combine, remain separate selectable workspace sets, or follow another rule? Evaluate global pooling and explicit workspace-associated reopen, including surviving-window access, independent ordering and distinct same-profile terminals. Record the chosen behavior before implementation; group-based identity, if chosen, needs rename/delete/recreate and multi-window tests without changing startup-group scope.

20. If the global menu is selected, verify full pin/owner/Closed identity and keyboard/screen-reader access. Open here launches only a closed pin freshly; Move here preserves the live session/cwd; Focus existing activates without moving; explicit Duplicate creates a fresh runtime session; Reopen never claims an old remote process. Gather deliberately transfers open tabs and opens closed recipes without restarting live sessions.
21. Race menu requests and Gather against ownership changes, close/cancel, transfer, new-window collection and settings reload. Use stable IDs, revalidate stale rows and test conflict/partial-failure feedback without duplicate attachments or lost sessions; settle ordering/focus and duplicate-template association first.
22. Pin a specific path, navigate elsewhere, move/focus its live tab and verify the same session/current cwd survives while the descriptor stays unchanged. Close/reopen and verify the original pinned path. Create another pin for a different destination and verify the original pin remains unchanged, with independent stable IDs. Test settings reload/shutdown/report arrival without live updates; group folder modes remain unchanged.

23. Verify local profile/shell plus fixed path, SSH saved endpoint plus fixed remote path, and future SFTP endpoint/directory only when that pane exists. Capture requires supported context/report or explicit entry; never infer remote path from local SSH cwd. Test unavailable/invalid paths, explicit fallback without descriptor mutation and failed/canceled new-pin persistence without changing the original.

24. In an active terminal at A, create pin A: a separate compact entry appears in the background while the original tab/session/cwd/focus remains unchanged. Navigate the original terminal to B; select pin A and verify a separate session accesses A while the original stays at B. Original-tab close/move/navigation must not mutate the fixed descriptor or imply individual pinned-tab close.
25. Once eager versus lazy launch is decided, verify creation/first selection and failures against that policy, including local and SSH destinations. Lazy launch is a candidate only. Selecting an already-open pin accesses its existing separate session; fresh reopen uses the captured path. No duplicate connection from repeated selection or focus theft during background creation.

Before implementation, choose the cross-window persistence/access and global-menu and eager-versus-lazy background-launch policies without an exclusive pin-capable window; settle close-shortcut/pane-close/separate bulk tab-close mappings, persistence ownership and create/remove workflow, startup/default-profile/restore composition, explicit dedup association, live split-tab association, ordering/focus, explicit/transfer/restore-created window claim eligibility, ownership coordinator/recovery, fixed-path capture/launch validation and invalid-pin fallback. Defer all implementation until groups are stable.

## Implementation checkpoint: independent model and action rules (2026-10-15 UTC)

`TerminalSettingsModel/PinnedDestinations.h` introduces a bounded version-1 descriptor/collection format, independent of storage topology. A descriptor contains its own stable ID, exact profile GUID/source, typed local/WSL/SSH path context and fixed host/profile-label/path. It contains no commands, credentials, environment, scrollback, runtime window/content IDs or split-layout configuration. Unknown fields/versions, duplicate IDs, invalid/oversized data and unsafe paths fail validation instead of being rewritten as an empty collection. No disk reader/writer or global claim mechanism is connected yet.

Capture requires reported directory provenance; a starting-directory seed is rejected. Capture copies the destination, so later navigation/title changes cannot mutate it. Resolution checks exact profile/source/host/context, active state, compatible elevation and launcher path support before returning the fixed descriptor. Missing local paths block without substituting a default folder/profile. Remote path descriptors never reach the local filesystem probe. These are tested model contracts, not a claim that SSH/WSL pin launch adapters or native UI already work.

The action policy separates accepted individual Close, Unpin, window exit/Quit, transfer, natural connection end and recovery. Cancel plans no mutations. Close removes the remembered entry and closes its live session; Unpin removes the remembered entry while retaining/demoting its session; whole-window/Quit preserve remembered data. The injected writer boundary completes before in-memory removal, and failed writes leave the collection untouched. Repeated removal is idempotent. Separate bulk-tab-close and pane-only-close mappings remain unresolved and are rejected by this foundation instead of silently chosen. No removal side effect is attached to `_RemoveTab`, Shutdown or teardown.

The user approved all three choices below on 2026-10-15; dependent implementation is now underway:

1. **Proposed global unviewed collection plus a Pins menu in every window.** Every window can create pins. A new ordinary window collects currently unviewed pins, so pins from closed B and C combine in D. Existing windows can explicitly open closed pins, move live pins here or gather all. Closing another window never launches replacements automatically. Explicit commands, restored layouts and selected saved-group startup preserve their existing content and use the menu for pin access. This policy is selected.
2. **Proposed lazy launch.** Creating a separate compact background entry leaves the source tab/focus intact; the fresh connection starts only on first selection. Lazy launch is selected.
3. **Proposed focused-pane capture.** A split source tab contributes only its focused terminal destination to a new single-session pin. Later splits in the pin's live tab survive Unpin/Move, while a fresh reopen uses the original single destination. Focused-pane capture is selected.

Focused `PinnedDestinationTests` exercises typed round trips, metadata provenance, immutable paths, independent IDs, strict data/bounds, Close/Unpin/exit/cancel rules, writer failure/idempotence, exact-profile/target preflight and exclusion of remote paths from local probes. Build/test logs live under `artifacts/pinned-tabs-foundation`; the initial GUID-type compile diagnostic is preserved. Compact native UI and runtime persistence/ownership integration are in progress; no native visual or completed pinned-tabs acceptance is claimed. No user settings, remote connection, commit, push, tag or release accompanies this checkpoint.

## Capture fallback correction (2026-10-15)

A usable reported cwd keeps quick capture; an unavailable/invalid report now asks for a folder explicitly. Startup seeds are suggestions, and confirmation stores an absolute literal path. Local variables are resolved before confirmation; remote paths never use local seeds/variables. See [cause, evidence and checklist](PIN-CAPTURE-FIX.md).
