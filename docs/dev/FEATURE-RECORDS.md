# Feature implementation and validation records

Dated contributor references moved out of primary user guides. Artifact identities, tests and acceptance reports apply only to their identified checkpoints. Superseded behavior and pending reviews below are historical, not current instructions.

## SSH implementation history and verification

Source: `docs/backslash/SSH-PORTABLE-IMPORT.md`. These are dated development records, not current user instructions.

The native regression invokes the actual production Scan and Import buttons in an owned offscreen XAML host, using redirected synthetic config only. It verifies rendered names, repeat/empty/error/recovery scans, one-selected and two-selected imports, stale selections cleared on rescan, exactly two owners/records for two selections, and post-import duplicate suppression. One probe-only out-of-range event came from an empty-inbox fixture omitting all color schemes (`AppearanceViewModel::CurrentColorScheme`, GetAt(0)); loading compiled-only defaults restored the normal profile-page precondition. Production appearance code was not changed. The exact same probe and complete fixture failed with the delivered Editor DLL and passed with the fixed DLL. Final native runs have zero unhandled XAML events. The aggregate suite passed 344 of 346 cases, with zero failures and two optional local PowerShell cases skipped; all 24 SSH cases passed. Release/x64 application build: zero errors / 134 warnings. Independent source review accepted the fix with qualifications; the requested two-selection extension passed without product changes. The accepted corrected portable is staged at `artifacts/portable/Backslash-Dev-20261027-ssh-scan-ui-fixed-f70aa588/Backslash.exe`. All 34 isolation checks passed; 261 manifest files and 244 package payload files verified; 585 imports across 25 images had no unresolved dependency. The exact packaged components also passed the native single/two-selection regression. No user settings or SSH files were copied and the delivered EXE was not launched. Editor DLL SHA256: `977BB9EF205849CE2C31C01717B3606707B445F3255C9A17EF90424117E2ECB0`; EXE SHA256: `40B9A2950CF05615FADC8549A1C8CA9B06C286EDC9C60999F6AE7FAF0447FCFA`. The fresh guarded package, before/after proof and validation are in `artifacts/build/20261027-ssh-scan-ui-fixed-f70aa588`. Older P2 package/portable directories contain the broken Scan UI and must not be reused for this fix.

Earlier independent-review corrections (2026-10-27): an edited legacy transient whose alias already has a saved resolved/reference owner now rejects Save/export with a clear conflict, retaining both the existing endpoint and all draft edits. Reviewed endpoint import explicitly transfers the legacy row's edits before marking them handled. DraftComparisonJson serializes raw draft state without ownership materialization, so adoption conflicts or the 512-record capacity limit cannot prevent dirty checks, Keep editing, Discard or Reset after a failed Save. Synthetic regressions cover both owner modes and Hide/appearance/advanced changes, full-capacity failure followed by close/reset comparisons and discard, and explicit endpoint import retaining edits. Recheck gates: all 24 SSH tests passed; full Settings suite 344 passed, 0 failed, 2 optional local PowerShell cases skipped (346 total). Separate-process alias persistence and discovery-default checks passed again, as did repository layout, appearance, startup-picker, source invariants and scoped whitespace. Release/x64 application build: 0 errors / 134 warnings; Settings test build: 0 errors / 47 warnings. No final portable staging has occurred; parent recheck remains required. Evidence: `artifacts/build/20261027-ssh-selection-p2-review-4c7dc0f3` (p2-workspace-results.log, p2-settings-full-results.log, build.log, source-hashes.json).

Earlier verification checkpoint (2026-10-27): Release/x64 application build passed with 0 errors / 134 warnings; Settings test build passed with 0 errors / 46 warnings. The complete Settings suite passed 341 of 343 cases, with 0 failures and 2 optional local PowerShell cases skipped. All 21 SSH cases passed. A separate producer/consumer process verified both visible and hidden saved alias fixtures after restart with discovery off. Two fresh-process default checks, startup/no-toggle/no-import source invariants, repository layout, appearance-content, startup-picker and scoped whitespace checks passed. The initial broader run exposed a serializer omission regression and an outdated policy assertion, both corrected; fixture permission/capture failures were resolved by the approved isolated runtime and verified local argument-capture executable. No user SSH configuration, keys, real SSH connections, GUI interaction, installation, commit or push was used. Final portable staging/isolation is pending independent parent review. Evidence: `artifacts/build/20261027-ssh-selection-review-d220d5d2` (build.log, settings-full-final-results.log and source-hashes.json).

The dated implementation history below is historical and superseded by this workflow, including the old toggle, preference-only persistence and import-first advanced-edit restriction.

# Historical implementation notes

Historical runtime verification and operational context are recorded in [maintainer notes](HANDOFF.md). The dated adapter evidence below remains specific to its older build.

Portable mode starts with **Show this computer's SSH connections** off. In Settings > Add a new profile, enable it for this Backslash process/session, or refresh it there. Fully exiting the process resets portable discovery to off on the next launch. Closing Settings or the last window does not necessarily exit the process: Compatibility > Allow Windows Terminal to run in the background can keep it alive. The note beside discovery links to that setting without enabling it. Imported workspace records remain durable; see [resident startup and background lifetime](../backslash/STARTUP-GROUP-REOPEN.md). The choice is not written to settings or a host registry key. Nonportable startup retains host discovery by default. Existing terminal connections are not terminated when the view is disabled.

Host profiles are labeled **This computer** and are temporary. Open one in Settings and select **Import into workspace**. The review shows the concrete host, user and port, or explains why import is blocked. Confirming adds a **Workspace SSH** profile to the Settings draft; **Save** commits it using the existing stale-draft/atomic-save checks. Cancel/Reset discards an unsaved import. Save or reset unrelated Settings edits before toggling/refreshing discovery; they are never discarded to refresh a view.

The first adapter imports only simple explicit Host aliases and effective HostName, User and Port (user config before system config, first applicable value wins). If User is absent, the review shows the current username captured as an explicit connection user. Host * defaults are supported; other patterns, Include, Match and every other applicable directive block import. Unsupported directives are identified by name without evaluating commands or reading referenced files. This deliberately blocks key, certificate, agent, proxy, trust-file and other dependencies rather than silently omitting behavior. It is not a full OpenSSH evaluator or a two-way sync implementation.

The initial adapter accepts nonempty aliases and usernames of at most 253 ASCII characters, using letters, digits, underscore, dot and hyphen, with no leading hyphen. HostName accepts the same characters plus colon for unbracketed IPv6. This is an adapter restriction, **not** a claim that other SSH/Windows usernames are invalid: for example `alice#ops` is rejected explicitly, never shortened to `alice`. Quoted/escaped values, Unicode names, domain-backslash usernames, substitutions/tokens and bracketed host forms require manual configuration. Ports must be 1-65535 with at most five decimal digits. Directive separators accept whitespace or one optional `=`; repeated separators such as `Port==22` and `Port = =22` are rejected. A collection is limited to 512 unique aliases, checked before mutation.

Known shell-selector choices (default, server default, Bash and PowerShell 7) are retained. Custom or advanced profile command lines block import rather than silently lose extra SSH flags, commands or authentication dependencies.

Setup export/import carries the workspace connection records as data, merges unrelated aliases and blocks conflicting endpoint definitions; it does not import SSH keys or host trust.

Duplicate aliases are compared without case sensitivity. An existing matching endpoint is a no-op; different endpoint details require resolving the conflict first. Imports use separate workspace profile identities, so turning host discovery off or changing a host alias does not remove or retarget them. The portable collection is stored as `backslash.sshConnections` in settings.json (alias/host/user/port objects); ordinary profile overrides retain appearance and remote-shell choices. Invalid collections are preserved as settings data and cause a load error rather than being silently rewritten.

Workspace launches select an available Windows OpenSSH client locally and use an app-owned process-local executable reference. They pass explicit address/user/port with `-F none`; they do not resolve the alias through the host's SSH config. Host trust is kept in adjacent `settings/ssh_known_hosts`, with fingerprint prompting and no system trust-file fallback. Private identity files and agent use are disabled for this narrow imported connection adapter; authenticate interactively for each new connection. **No key, credential, certificate, host trust, host file or agent state is imported.** A saved connection is not a logged-in session. Key-backed workspace authentication and bundled clients are future separate work.

`BACKSLASH_SETTINGS_DIR` and `BACKSLASH_SSH_EXE` are overwritten only inside Backslash's process environment for launch resolution. No global environment/registry setting is created. Workspace paths are resolved from Backslash's current settings directory, including spaces and drive-letter changes. An absent Windows SSH client yields a launch failure; Backslash installs nothing. The adapter's shell selector and pin launcher share the structured base parser so connection options survive Bash/PowerShell/custom shell selection and fixed-folder reopening. Remote CWD capture still requires cooperating remote shell reporting; this feature does not add remote shell integration or reconnect an existing session.

Old explicitly saved host-generated profiles are preserved; without discovery they can remain orphaned, as before. New transient discovery rows are omitted from settings serialization and generated-profile bookkeeping. Existing disabledProfileSources and machine policy filtering still apply to host discovery and workspace generation. The view refreshes when requested or settings reloads; there is no host-config watcher or background synchronization.

Validation uses synthetic SSH config strings and model fixtures only. No user's SSH config/key import, SSH connection, listener, host agent/service/firewall change, visible app launch or installation is part of validation. Final build and test results are recorded after verification below.

## Historical SSH adapter verification (2026-10-25)

Release/x64 build: **0 errors, 134 warnings**, 28.68 seconds incremental. Evidence: `artifacts/build/20261025-165031-601-Release-x64-e794cf6d`. Fresh portable: `artifacts/portable/Backslash-Dev-20261025-165031-601-Release-x64-e794cf6d`. **34 portable isolation/completeness checks passed.**

Complete-runtime Settings suite: **290 total, 288 passed, 0 failed/blocked/not-run, 2 optional local-shell tests skipped in that suite**. Both optional tests passed separately against the complete runtime: ActualLocalPowerShellTransport **1/1** and PowerShellPromptAndStatusPreserved **1/1**. All 290 distinct cases were successfully exercised across these runs, including the 12 SSH regressions. Two fresh portable-process checks passed, including off-by-default after a previous process enabled discovery. Synthetic OpenSSH `-G` validation passed, including explicit endpoint settings and a trust path containing spaces; no connection was made.

An earlier optional-shell run used an incomplete test runtime missing OpenConsole.exe and ended with an access violation. Restaging the complete runtime resolved the fixture failure without a product workaround. Test linking emitted LNK4020 PDB type-record warnings; logs retain them. Native Settings interaction and real SSH/SFTP authentication/transfer were not run. No private keys, credentials or real host SSH config were imported; no agent/service/firewall or persistent authentication setup was changed.

Import review blocks existing profile name/identity collisions. Legacy saved host profiles are preserved as unavailable/orphaned Settings entries when discovery is off and excluded from active launch profiles. Imported workspace entries remain; transient discoveries are not implicitly persisted.

Portable Backslash.exe SHA256: `2AC684EE79B8AB6C2F3EA80852D28A148DC8739FD1F5E4524090BBE82EDBE535`.

## Host preference persistence candidate (2026-10-27, not yet runtime verified)

Transient host discoveries now have an explicit preference map keyed by generated GUID. The map retains hide/unhide, selected appearance properties, and a remote shell mode/custom remote command. It is applied only after a Windows.Terminal.SSH discovery matches; it cannot generate a connection when discovery is disabled. Shell selection is rebuilt against the freshly generated connection. Generated command lines, host endpoints, environment, configuration and identity data are excluded from this map. Existing boolean visibility records are read compatibly.

Refresh updates discovery; it does not import a connection. Import into workspace is on each discovered profile page above Command line while discovery is enabled. Imported connections retain their existing durable identity and remain available with discovery disabled.

This candidate is not ready for release. Advanced command-line/environment edits still lack a user-approved persistence policy; the editor currently permits edits that transient serialization does not retain. The appearance allowlist and synthetic save/reload/discovery/import regression matrix still require completion and verification. Do not present this candidate as complete or distribute it over the last verified portable.

Supported-field verification (2026-10-27): latest Release/x64 Settings test DLL rebuilt successfully. Isolated copied runtime: WorkspaceSshTests 15/15, HostSshPreferenceBoundary 1/1, SetupTransferTests 17/17, SetupTransferValidationTests 7/7; 40 passed, zero failed/blocked/skipped. Logs: artifacts/build/host-preferences-*-results.log and host-preferences-tests-build.log. Shell/appearance test exercises modes 0 through 4 across discovery-off serialization and discovery-on reconstruction; existing tests cover hide/unhide disk-save callbacks, repeated imports and stale import rejection. Stale import test compares against the current draft after the user's shell edit, ensuring rejection itself causes no mutation. Recursive font/unfocused preference validation rejects unknown nested fields and malformed values. Invalid custom remote commands reject rather than silently selecting default shell.

Release remains pending the explicit advanced command/environment policy decision. Options are retaining explicit user-authored advanced overrides separately, or requiring durable workspace import before those edits can be saved. Current transient editor behavior accepts then omits these fields, so this review candidate must not be advertised as a completed persistence fix.
Review checkpoint: Release/x64 build 20261027-103606-195-Release-x64-df3f2230 succeeded with 0 errors and 134 warnings. Review-only portable staged at artifacts/portable/Backslash-Dev-20261027-103606-195-Release-x64-df3f2230. Full SerializationTests 79/79 plus WorkspaceSshTests 15/15, SetupTransferTests 17/17 and SetupTransferValidationTests 7/7 = 118 distinct passing tests (boundary test included in SerializationTests). Advanced-edit policy remains pending; do not deliver this candidate as complete.


## Approved advanced-edit policy (2026-10-27)

Discovered transient host profiles retain remote-shell selection, visibility and appearance directly. Advanced command-line and environment changes require Import into workspace, above Command line on the profile page. Refresh only refreshes discovery. The command editor and browse action are unavailable for transient host profiles; the reset action remains available for existing advanced drafts. Imported workspace profiles retain normal advanced editing.

The model validates all transient host profiles before writing settings or updating generated-profile state. An unsupported advanced command or explicit environment override rejects the entire save with an import-required explanation. The current draft remains intact, including those existing advanced edits; there is no automatic destructive migration. Restore the generated command using its reset action before importing, then make advanced edits on the imported profile. JSON/API environment edits receive the same save-boundary validation; this editor has no standalone environment-variable editor.

Earlier pending-policy notes above describe historical checkpoints and are superseded by this approved policy.
Final policy verification: regenerated model API and rebuilt Settings tests; WorkspaceSshTests 16/16, SerializationTests 79/79, SetupTransferTests 17/17, SetupTransferValidationTests 7/7 = 119 distinct passes with zero failures/blocked/skipped. Evidence: artifacts/build/host-preferences-policy-*-results.log. AdvancedTransientSaveRejectsWithoutMutation verifies writer is not called, generated-profile state stays unchanged, and command/environment draft remains intact with an import-required error. First package retries exposed an editor private-header dependency (fixed with projected IsTransientHostSsh) and then compiler memory exhaustion; final retry reduces build/compiler concurrency.
Final approved-policy candidate: Release/x64 build 20261027-105526-705-Release-x64-0146767e succeeded, 0 errors / 135 warnings, 1m55s with reduced concurrency. Portable: artifacts/portable/Backslash-Dev-20261027-105526-705-Release-x64-0146767e. Portable isolation evidence: artifacts/build/host-preferences-policy-portable-isolation.log. All 119 relevant tests passed against regenerated binaries. No private SSH data, real connections, GUI interaction, settings mutation, installs, commit or push were used.


Draft/export protection refinement: comparison-only DraftComparisonJson includes complete explicit transient profile edits in memory, so environment-only changes and distinct unsupported commands remain dirty. Close, reset, discovery and import-review guards use that comparison. It is never persisted or exported. ExplicitSettingsJson remains filtered and now validates the same import-first boundary as Save; setup export rejects unsupported transient edits with the actionable import-required error instead of omitting them.
Final draft/export refinement tests: WorkspaceSshTests 16/16, SerializationTests 79/79, SetupTransferTests 17/17, SetupTransferValidationTests 7/7 and SettingsPlacementTests 20/20 = 139 distinct passes, zero failures/blocked/skipped. Logs: artifacts/build/host-preferences-draft-*-results.log; test build: host-preferences-draft-tests-build.log. AdvancedTransientSaveRejectsWithoutMutation now also verifies environment-only dirty comparison, different unsupported command detection, explicit export rejection, and ConfirmDiscard from the settings close policy for the dirty draft. Close/reset/discovery/import guard call sites were verified to use DraftComparisonJson; export and validation continue to use ExplicitSettingsJson.
Replacement final candidate after draft/export review: artifacts/portable/Backslash-Dev-20261027-110129-176-Release-x64-5658811b. Release/x64 build succeeded in 5m52s, with 139 selected model/policy tests passed and 34 portable isolation/completeness checks passed. Evidence: artifacts/build/20261027-110129-176-Release-x64-5658811b and host-preferences-draft-*-results.log / host-preferences-draft-portable-isolation.log. This supersedes the earlier 0146767e candidate.

Scan UI correction (2026-10-27): the reported later "parameter is incorrect" error was reproduced as HRESULT `0x80070057` at the production ListView ItemsSource boundary. The scan service found both synthetic aliases, but its typed `IVectorView<Profile>` did not expose `IIterable<IInspectable>` for XAML. The editor now wraps the same Profile objects in an object-observable collection, preserving GUIDs and keeping candidates outside the Settings draft. Failed scans clear stale rows, disable Import and show a friendly message with operation phase, HRESULT and available detail.

## Pins implementation verification

Source: `docs/backslash/PINNED-TABS-IMPLEMENTATION.md`. These are dated development records, not current user instructions.

## Validation

Final preview checkpoint: `artifacts/build/20261014-214639-341-Release-x64-017a79b5`. Release/x64 app build **0 errors / 134 warnings**, 46.46 seconds. Final Settings test build **0 errors / 8 warnings**; **214/214 Settings tests passed** (12 destination, 12 store and 6 launch/reference/group tests plus the existing 184). Final model evidence: `artifacts/build/pins-validation-20261014-213706/final-store-tests/full-settings-tests.log`.

Unchanged Control/rendering regression evidence is in `artifacts/build/20261014-214010-183-Release-x64-2b82ae13`: **34/34 Control**, CRT ABI/WARP output, render visibility and diagnostic snapshot checks passed. These were run before the final app-only restore-reference retry/elevation repair; no renderer/Control source changed afterward. Final portable assembly/isolation passed again. Its owned local fixture passed **9/9** lazy startup/cwd/claim/natural-exit/reopen assertions and all fixture sessions/windows ended naturally (**0 forced stops**). This proves those paths; it does not prove native menu, move/unpin, confirmation, restoration or visual acceptance.

The first smoke fixture incorrectly relied on a transient parent environment variable and produced no marker. A fixture using its own executable directory passed in a fresh evidence root, then passed again against the final preview. Failed build/test/smoke artifacts remain intact. No real SSH server or WSL distro was executed. This session exposes no node_repl or native computer interaction/screenshot tool.

Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261014-214639-341-Release-x64-017a79b5/WindowsTerminal.exe`.

Clean preview ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-PinsPreview-x64-017a79b5.zip`; SHA256 **`97198486A022EBB1AC0EEB5B0089697FBA9DAF1236ECC590954E6F83A06C0EC2`**. The adjacent settings directory was moved into owned test evidence after smoke; the delivery folder and ZIP contain no smoke settings.

Native acceptance still needed:

1. Pin current reported directory from a focused split pane; confirm a separate compact background entry, unchanged source selection/cwd/splits, one fresh session on first selection and the original destination after later navigation/reopen.
2. Add splits to a live pin, then Unpin; verify unchanged process/core IDs and layout, and a normal tab to the right of pins. Verify individual X removes its remembered preference.
3. Test accepted/canceled read-only and window Close All/Quit confirmations. Cancellation must leave the request's preferences/sessions intact; window exit preserves the catalog.
4. Exercise Open/Move/Gather across same-process windows, failed transfer/timeouts, failed writes, source/target shutdown during each asynchronous phase, and failed restore-entry retries. Verify no duplicate live core. Other-process/elevation owners should give the documented bounded error.
5. Check explicit commands, restored layouts and saved-group startup, existing Settings drafts/inheritance and ordinary-new-window collection. Verify dormant pin exclusion and live pin capture as ordinary group layouts.
6. Check all tab-width modes, hover (no expansion), icon/close target, status tooltip/Ctrl+Tab/UIA text, keyboard navigation, narrow layouts and fractional DPI.

## Capture usability correction

The home/AppData capture error is corrected by [explicit folder confirmation when shell metadata is unavailable](../backslash/PIN-CAPTURE-FIX.md). Latest validation is 222/222 Settings, 34/34 Control and 9/9 owned local runtime checks. Native dialog interaction remains pending.

## Fresh-profile identity follow-up

The 845eb99d display-prefix comparison bug is corrected by immutable typed launch identity. See [verified cause, real adapter/Core and PowerShell 7.6.6 evidence, current portable and limits](../backslash/PIN-IDENTITY-FIX.md). Latest results: 228/228 Settings, 34/34 Control, 6/6 actual capture integration and 9/9 portable runtime cases.

## Saved-group operational verification

Source: `docs/backslash/SAVED-TAB-GROUPS.md`. These are dated development records, not current user instructions.

## Validation and remaining checks

The fresh build and test evidence/artifact paths are recorded in the build run's `groups-delivery.json`. Model fixtures cover serialization/reload, stable rename/delete/recreate identity, deep clone/cancel isolation, unsupported version preservation, exact reference resolution, folder-mode/fallback behavior, split/focus arguments, invalid layout/name/command data and exclusion of command/content IDs. Control coverage distinguishes seeded from reported cwd. Existing settings, SSH/WSL/bezel, CRT pixel/resource, visibility and portable-isolation regressions are retained.

Native UI/manual checklist: use a fresh portable copy; create two tabs including nested 30/70 and 40/60 splits; save a named group in each folder mode; reopen while existing tabs remain and verify order, ratios, saved focus and fresh shells. Rename, Reset/cancel, Save, delete and recreate the same name; verify stable/new IDs. Remove/disable a referenced fixture profile and verify opening blocks without launching another; try missing/unreported/native/WSL/SSH folder cases and cancel the fallback. Verify quoted/Unicode names, duplicate name rejection, keyboard/accessibility, menu discoverability, tab movement/reparenting, and CRT/bezel appearance. Real SSH, native screenshot/accessibility and hardware GPU checks remain unrun. Last-location updates, heterogeneous file panes, persistent remote sessions and broad Settings reorganization are not implemented here.



## User acceptance checkpoint (2026-10-15 UTC)

The user confirmed group creation, startup and saving while Settings is open work. This records user-reported interactive acceptance of those workflows before the local checkpoint. The automated coverage and detailed accessibility/DPI/edge-case limits above remain applicable. Background opacity and phosphor tint were also confirmed working after the hue-retaining correction documented in CRT-RENDERING-PLAN.md.


Thirteen local-shell runtime checks in owned portable copies passed: initial three-pane/two-tab group without Default, ordinary launch while running without repeated group, rename by GUID, default mode, explicit CLI, configured startupActions, deleted selection, same-name recreation, missing profile, wrong source, valid captured folders, actual two-window previous-session restoration without group injection, and restore enabled without a layout. Known user settings/state hashes and existing Terminal process identities were unchanged. Exact executable, ZIP/checkpoint hashes and logs are in the build run's `startup-group-delivery.json`.

Native UI/manual checks remain **unrun** because the computer-use Windows runtime cannot start: mode/picker keyboard and screen-reader behavior, conditional search navigation and hidden-picker focus, Save/Reset/restart interaction, warning visibility, captured-folder OK/Cancel and focus, split ratios/order/focus on screen, group-name input regression and title/Ctrl+Tab presentation. Connection/content handoff and tear-out eligibility are covered by policy/source checks, not live transport automation. Real SSH and hardware-GPU checks remain unrun. In a disposable copy, test those paths plus missing/deleted groups and exact profiles. No pins implementation, commit/push, installation, registration, security change or user-window disruption accompanies this slice.

Fresh build/package/test details are in `artifacts/build/20261014-110125-908-Release-x64-946c8f36/group-input-delivery.json`. Existing model/control/SSH/WSL/bezel/CRT/visibility and isolation regressions plus a local-shell startup fixture are the automated coverage. Those checks do not establish interactive text input. Native UI testing is blocked by the unavailable Windows computer-use runtime; the Library screenshot transfer helper also failed on this host's unsupported extended-attribute API. Neither failure changed the Library file or user settings/windows.

All six interactive paths remain **unrun**, including screen-reader/IME and actual focus restoration.

The user reported a crash selecting an existing saved group under On startup, including after restarting with the group already persisted. Application events confirm crashes in portable build `56aa0b7d`: XAML exception `0xc000027b`, WER HRESULT `0x80070057` (`E_INVALIDARG`). Existing minidumps were read only for exception metadata; the stowed-error list was not included in captured memory, and no existing native debugger was found. The event/HRESULT supports a UI argument/binding failure but does not establish an exact throwing stack. The earlier model/startup checks did not exercise this native picker interaction.

Regression coverage selects initial, repeated and subsequent groups; serializes/reloads an already-created catalog; covers empty/invalid/missing choices, rename, stale same-name recreation, search and Settings draft isolation. A read-only copy of the user's existing saved catalog is injected into the model fixture; selection and roundtrip preserve its recipe without launching its referenced profiles. This is **model coverage**, not a native AutoSuggestBox reproduction or proof that clicking the row is fixed. Exact results and fresh combined artifact paths are recorded in `picker-reset-delivery.json` in build run `20261014-130351-502-Release-x64-420a5279`.

Native selection remains unrun because no working Windows UI tool is exposed. Required manual check in a disposable copy with the existing catalog: fresh process, open On startup, search/select the persisted group, repeat selection and choose another group, Save/Reset, restart and select again; then test empty, renamed and deleted/stale suggestions. Do not repeat group creation as a prerequisite. No user settings/window changes, remote execution, install/registration, security changes, commit or push accompany this fix.

The user reported that the long description compressed the Startup group field to about 95 pixels and clipped its matching-width suggestion popup. The inherited SettingContainer template puts the description and editor in competing columns. This card now overrides only its own template: heading, independently wrapping description and search editor occupy three automatic-height rows in one available-width column. The field and inner native textbox stretch across the padded card; there is no fixed field width or oversized minimum. Normal card styling, reset/help template parts and the existing named search target are retained.

[Test-StartupPickerLayout.py](../../backslash/tools/backslash/Test-StartupPickerLayout.py) supplies repeatable **source assertions**, not native UI tests: 28 checks for row separation, stretching, dimensions, wrapping, full-name access, labels and existing handlers, plus 36 hypothetical available-width/DPI/text-scale allocations derived from those constraints. These do not execute XAML Measure/Arrange, measure fonts, alter DPI/text scale or prove popup positioning. XAML compilation and focused existing picker/reset model fixtures accompany the fresh isolated build `20261014-140517-182-Release-x64-20c45117`; detailed logs and artifact hashes are in its `picker-layout-delivery.json`.

Native screenshot/keyboard/accessibility, actual narrow/wide windows, DPI/text-scale and popup placement remain **unrun** because no working Windows UI tool is exposed. The user's screenshot is not confirmation that the earlier selection crash is accepted as fixed. Manual check in a disposable copy: narrow and wide Settings, including the reported width; search/select a persisted group and repeat selection; long spaced and unbroken Unicode names; popup wrapping, full-name hover, keyboard and screen reader; larger DPI/text scale with no horizontal squeeze; Save/Reset and missing/renamed groups. No user settings, existing windows or previous artifact was changed, and no commit/push, install/registration or security change accompanies this layout fix.

Regression fixtures cover single terminal/splits, terminal + Settings, Settings-only/zero, filtered order/focus, unsupported/mixed/incomplete panes; actual CascadiaSettings profile/global draft retention and Reset; append replay, unsaved rename/deletion/name conflict; injected failed/successful writers without publishing to live/draft; notice Continue/checked/success combinations, cancel/failure and isolated state persistence while retaining unrelated warning/seen-profile data. Exact build/tests/artifact hashes are in `group-eligibility-delivery.json` in build `20261014-151605-044-Release-x64-de0a97ce`.

Native interaction remains **unrun** because no working Windows UI tool is exposed: keep an unsaved Settings profile/global/group edit open beside terminal tabs; save while Settings and then a later terminal are focused; test notice Continue/Cancel/Escape/checkbox, name-editor cancel and restart suppression; verify zero-terminal and mixed-content errors stay visible; Save/Reset the Settings draft and compare all edits/new group IDs. Test one tab with splits, multiple filtered tabs/order/focus, keyboard/screen reader and folder warnings. Prior picker selection-crash and layout acceptance remains unconfirmed. Tests do not close user windows, execute user/remote profiles, or change user settings; no commit/push, installation, registration or security changes accompany this slice.

Only the **initial ordinary app launch** uses the saved starting set. An explicit `InitialAppLaunch` flag is attached by WindowEmperor's initial dispatch; incoming requests, later windows, persisted-window dispatch and hotkeys do not inherit it. Additional windows and ordinary launches while Backslash is already running keep normal window/profile behavior. Explicit CLI/profile/actions, configured `startupActions`, connection handoff, transferred/tear-out content and an actual persisted layout take precedence. If previous-session restoration is enabled but no layout is available, the eligible initial launch uses the selected starting set. Generic `startupActions` is not rewritten to contain the group.

Fresh build `20261014-121520-040-Release-x64-56aa0b7d`: zero errors; 180 settings, 33 control and 20 focused profile/SSH/WSL/bezel tests passed, including three focused startup-policy/settings fixtures. Fixtures cover all 64 eligibility combinations, stable rename versus same-name recreation, invalid/unsupported selection, exact source/profile preflight, folder accept/cancel, copied draft isolation and serialization while retaining Default profile. Existing CRT/WARP/1200 shader cases, render visibility and portable isolation checks passed.

The initial `5666c86f` test build used a `ContentDialog` containing a `TextBox` for group naming. Existing [window-renamer code](../../backslash/src/cascadia/TerminalApp/AppActionHandlers.cpp#L994) documents that text boxes in ContentDialog receive no keypresses in this XAML Islands host, and uses a TeachingTip with delayed input focus instead (GH#12021). The group editor now follows that established hosting pattern. This is source-backed diagnosis of the hosting regression.

Required manual keyboard checklist for the new isolated copy:

1. Open Groups > Save current tabs as group with mouse and then with keyboard. Confirm the name field has a caret and accepts letters, spaces, quotes and Unicode. Opening Enter must not submit the empty form.
2. Edit using Backspace/Delete/arrows/Home/End, Ctrl+A, copy/paste and the context menu. The typed text must appear in the field and never in the terminal behind it. Confirm invalid/empty names disable Save.
3. Tab/Shift+Tab through name, folder mode, Save and Cancel; selecting a folder option must not unexpectedly save. Enter in the name field saves a valid name; Escape and Cancel write nothing.
4. Cancel, type a harmless local command in the shell, reopen and edit a new name. Repeat several times, including close-button cancellation and Alt+Tab. Confirm focus is not repeatedly stolen while editing and the form starts fresh.
5. Save and reopen the recipe, then rename through Settings > Startup > Saved tab groups, Save/Reset and reload. Compare the existing window and tab renamers: typing/editing and cancellation must still work.
6. In a disposable fixture, close/unload the owning window while the form is open; no save or retained popup/coroutine should result. Change/reload settings or open a nonterminal pane while editing and confirm existing validation protects capture.

## CRT implementation and validation history

Source: `docs/backslash/CRT-APPEARANCE.md`. These are dated development records, not current user instructions.

# Configurable CRT Appearance

Current later development includes [scheme-preserving phosphor and picture/opacity handling](../backslash/PHOSPHOR-SCHEME-TONES.md) and [Black/White scanline color](../backslash/SCANLINE-COLOR.md). These are separate from the [application UI accent](../backslash/UI-ACCENT.md). Consult the downloaded version's release notes for availability; dated implementation records apply to their identified builds. CRT open/close animations remain future work.

Current requested fallbacks are recorded in [Curated defaults](../backslash/CURATED-DEFAULTS.md); earlier build evidence and the frozen color references keep their historical values.

Frozen older-commit backup: [phosphor colors, formats and output math](../backslash/PHOSPHOR-COLOR-REFERENCE-dc027cae.md). See also the [cross-build regression audit](../backslash/PHOSPHOR-REGRESSION-AUDIT-960a9f9.md).

[Project entry point](../backslash/README.md) · [Roadmap](../backslash/ROADMAP.md)

## Requested product scope

Provide an optional **per-profile Appearance section** with sliders/fields and live preview. The user requests richer CRT controls inspired by [cool-retro-term](https://github.com/Swordfish90/cool-retro-term), configurable intentional glitch/effects, an old-screen overlay/frame, and actual terminal-view curvature/warping. Place this beside background image and transparency settings so their interaction is understandable.

This is a substantial requested feature, not merely a theme preset or the existing simple retro toggle. The first implementation slice exposes the existing built-in glow and scanline strengths; scanline dimensions and screen-only phosphor tint are implemented in the follow-ups below. Other proposed controls remain unimplemented.

## Proposed controls to evaluate

| Control | Proposed purpose | Design/engineering question |
| --- | --- | --- |
| Curvature | Warp the terminal view like a curved screen | Inverse coordinate mapping, clipping, cursor and selection alignment |
| Scanlines | Adjustable line pattern/strength | DPI, font readability and moiré |
| Glow/bloom | Phosphor glow around bright content | Existing sampled glow parameters, contrast and performance; extra passes only if quality requires |
| Phosphor persistence | Decaying afterimage | History buffers, lifecycle and memory limits |
| Flicker | Controlled temporal variation | Reduced motion and bounded frequency |
| Noise/glitch intensity | Deliberate visual distortion | Stable hit-testing; never induce real UI instability |
| Vignette | Darken screen edges | Readability and contrast |
| Bezel/overlay | Old-screen frame or texture treatment | UI layering, content area and asset rights |

The exact controls, ranges, algorithms and preset set remain open. Requested outcomes must guide evaluation; this table is not an accepted implementation API.

## Reuse and architectural limits

The baseline already exposes `experimental.retroTerminalEffect`, `experimental.pixelShaderPath` and `experimental.pixelShaderImagePath`. [Microsoft's appearance documentation](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-appearance) describes the toggle and shader override. Reuse those hooks where suitable; verify precedence and migration for the new section.

Do not promise everything fits one stateless shader. Persistence may require history buffers and time-aware decay. Simple sampled glow already exists in one pass; higher-quality bloom may justify additional passes. Overlays may need composition/UI work. Curvature requires coordinated rendering and inverse mouse hit-testing, selection and cursor placement; merely warping the picture is insufficient. Decide how inline images, scrollbars, background images, transparency and previews participate.

cool-retro-term is visual inspiration. Its Qt/QML and GPL components require a component-level license compatibility review before copying any code/assets into this MIT-based source tree. No code has been copied and no licensing conclusion has been made.

## Required quality bar

- Neutral zero-effect default/preset; disabling effects returns an undistorted readable view.
- Text stays readable; controls have keyboard access, labels and accessible values.
- Reduced-motion/disable-effects path removes temporal effects.
- Bound GPU load, animation rate and memory; avoid continuous redraw when unnecessary.
- No resize jitter, flickering geometry or stale history buffers.
- Cursor, selection and mouse targets align with curved content, including edge cells.
- Save controls per profile; changes preview promptly without modifying other profiles.
- Fast reset and preset comparisons with clear save/cancel behavior.
- Intentional visual glitch is a controlled optional effect. Crashes, input lag, unstable sizing and accidental rendering corruption are defects, not aesthetic behavior.

Before implementation, specify performance thresholds and supported GPU/DPI/resize scenarios. Prototype curvature and hit-testing together; prototype persistence separately. A static shader preview alone cannot demonstrate the quality bar.


## Concrete first slice: parameterize existing retro effects

Reuse [Retro.hlsl](../../backslash/samples/PixelShaders/Retro.hlsl) and [production custom_shader_ps.hlsl](../../backslash/src/renderer/atlas/custom_shader_ps.hlsl): Gaussian2D/Blur already sample a 13×13 neighborhood, main adds glow at hardcoded gain 0.3, sigma is 2×scale and Scanline/SquareWave use hardcoded strength 0.5 with DPI-scaled period. Do not invent glow from scratch.

The first proposed vertical slice exposes selected existing glow/scanline parameters through profile controls, saved settings, live preview, neutral/reset comparison and measured GPU/readability/DPI/resize checks. “Persistence” in this slice means saved control values; phosphor afterimage history remains separate later rendering work. Preserve shader ABI/override precedence and verify zero strength disables each effect cleanly.

Production [BackendD3D.cpp](../../backslash/src/renderer/atlas/BackendD3D.cpp) provides _recreateCustomShader/_executeCustomShader/RequiresContinuousRedraw. Time use already participates in redraw detection; sample README's elapsed-since-enable description differs from this runtime's performance-counter modulo 1000 seconds. Verify animation wrap, reflection failures, reduced motion and idle behavior before claiming scheduling solved.

Animate_scan/Animate_breathe are band/fade animation examples, not frame-history persistence. BackgroundImage's t0/t1 alpha composition is not a full bezel, inline image protocol or curvature/input mapping. Nop/Invert/Grayscale are comparison fixtures (Grayscale uses arithmetic RGB average); Broken/Error are diagnostics, not a guaranteed automatic fallback. See [complete sample map](../backslash/BASELINE.md) for precise paths and limits. The glow/scanline implementation and focused evidence are recorded below; the other effects remain proposals.


## Implemented glow and scanline slice (2026-10-13)

Controls: **Settings → Profiles → select a profile → Appearance**, immediately below the existing **Retro terminal effects** toggle in the Text group. Glow and scanline sliders use 0–100% with a percentage label and the standard inherited-value/reset control. The toggle remains off by default. Turning it off preserves both saved intensities. Both focused and unfocused appearance support the JSON settings and reset/inheritance path.

| JSON setting | Default | Range |
| --- | --- | --- |
| `experimental.retroTerminalGlowIntensity` | `0.3` | `0..1` |
| `experimental.retroTerminalScanlineIntensity` | `0.5` | `0..1` |

Finite out-of-range values clamp in both JSON loading and projected setters. Non-numeric JSON and non-finite values are rejected; failed validation retains the previous value. Explicit zero is a local override and round-trips. Null/reset clears the override and restores inheritance. The separate intensity macro participates in copy and JSON serialization, without adding intensity keys to appearance settings telemetry.

The production shader is `backslash/src/renderer/atlas/custom_shader_ps.hlsl`; sample shaders are unchanged. Default enabled effects retain the original gains and Gaussian/DPI behavior. Zero glow skips the 169-sample blur; zero scanline skips modulation. Both zero retain the shader pass and its base texture sample, but bypass effect work. There is no new continuous redraw request. Existing custom time/reflection behavior remains. A custom shader path still takes precedence, including compilation failure: it does not fall back to the built-in retro shader.

External shader **b0 remains 32 bytes**, with time/scale/resolution/background at offsets 0/4/8/16. Built-in intensities use separate **b1, 16 bytes**, with glow/scanline/padding at offsets 0/4/8. C++ size assertions and compiled reflection tests cover these boundaries. Parameter updates change the outer renderer settings generation, invalidate the frame, and upload b1 without changing misc shader-selection generation or recreating/compiling the shader.

Default-appearance projected-property changes use the existing 100 ms debounced preview refresh. **Unfocused preview is not implemented**: the preview subscription and `CreateForPreview` use the default appearance. Unfocused values propagate through the application adapter and ControlCore update path for actual terminal focus changes; focused/unfocused propagation and reset are covered by model/adapter tests, not an interactive focus test.

### Verification and deliverable

- Final build evidence: `artifacts/build/20261013-144052-896-Release-x64-ed9f7260`. `Build.ps1`, Release/x64: **0 errors, 135 warnings**, 2m55s. Warning count matches the preserved identity baseline; existing PRI263/packaging/link warnings remain.
- Focused settings DLL build succeeded. Direct TAEF `/name:*Backslash* /InProc`: **4 passed, 0 failed/blocked/skipped/not run** (`settings-focused-tests-filtered.log`). The three new tests cover defaults, explicit-zero/custom-path round-trip, local copy, inheritance/override source, clear/null reset, off/on value preservation, telemetry exclusion, finite clamping, nonnumeric/nonfinite rejection and retaining values after rejection, plus focused/unfocused adapter propagation. The fourth is the existing disabled-external-fragments identity test.
- Compiled shader ABI checks and WARP pixel-output tests passed in `artifacts/build/20261013-143844-146-Release-x64-ed0a8dd0/retro-shader-test.log` (same shader source, before restoration of original line endings). Reflection confirms 32/16-byte buffers; pixel cases verify both-zero neutrality, scanline 0.5/1 output, zero scanline neutrality, default gains, linear glow scaling and reset using the same shader object. Reproduce with `backslash/tools/backslash/Test-RetroShader.ps1 -Runtime -EvidenceDirectory <fresh-existing-directory>` using the installed SDK/C++ toolchain.
- Fresh portable assembly and `Test-PortableIsolation.ps1` passed, preserving the existing identity checks and required files/notices. Runnable folder: `artifacts/portable/Backslash-Dev-20261013-144052-896-Release-x64-ed9f7260`; run its `WindowsTerminal.exe` by full path.
- Hidden smoke launch loaded the preseeded isolated CRT profile and executed its PowerShell test shell. **Two existing Windows Terminal settings/state files retained identical SHA256 hashes**; only the process whose executable path matched the fresh artifact was stopped. Settings/shell smoke data was then moved to the build evidence directory, leaving the delivery folder clean. No adjacent state file was observed during this short smoke. See `crt-portable-smoke.json`; no UI was inspected.
- Clean ZIP: `artifacts/portable/Backslash-Dev-0.0.1.0-CRT-x64-ed9f7260.zip`, **258 entries**, `.portable` included, smoke settings excluded. SHA256: `186524A89ADD98BCCAD492B6AD558CEC51242C8C4E99955B3B01DA443C996F78`.
- The preserved identity baseline ZIP still hashes to `E8D9B2F3456B5EA4BF268B53EAFD5575FA83AE47AB112E959C4765CF12FB899F`. The CRT-only diff passed whitespace checking with `core.whitespace=cr-at-eol`, preserving original source encodings/line endings.
- Retained iteration evidence: an initial adapter test compile used dot access on a `com_ptr` and was corrected. A composite TAEF filter was misparsed and started an unfiltered run, which aborted at `ProfileTests::DuplicateProfileTest` with exit `0xC0000409`; its host-layout cause was diagnosed in the follow-up below; full-suite coverage is not claimed. The corrected name-filtered run passed all four requested tests. Earlier build attempts and evidence remain preserved.

### Source scope

- Model: `MTSMSettings.h`, `IAppearanceConfig.idl`, `AppearanceConfig.h/.cpp`, `CascadiaSettings.cpp` profile duplication; schema: `backslash/doc/cascadia/profiles.schema.json` (profile and unfocused).
- Adapter/control: `IControlAppearance.idl`, `ControlProperties.h`, `TerminalSettingsAppAdapterLib/TerminalSettings.cpp`, `ControlCore.cpp`.
- Editor: `Appearances.idl/.h/.cpp/.xaml`, English `Resources.resw` labels and accessible slider names. Existing resource fallback supplies English; new translations are not provided.
- Renderer: `common.h`, `AtlasEngine.h/.api.cpp`, `BackendD3D.h/.cpp`, `custom_shader_ps.hlsl`.
- Tests: `SerializationTests.cpp`, `TerminalSettingsTests.cpp`, extended `ProfileTests.cpp`, new `backslash/tools/backslash/RetroShaderTests.cpp` and `Test-RetroShader.ps1`.

No new curvature, bezel, history/persistence, flicker, glitch, vignette, preset or reduced-motion controls are implemented. Hardware-GPU performance, DPI/resize/readability, visual UI, keyboard/reset interaction, live slider preview, interactive focus switching, idle redraw measurement and custom compilation failure runtime checks remain unrun. WARP pixel tests establish shader output behavior, not performance or interactive UI readiness. No installation, registration, certificate trust, system integration, commit or push was performed. Existing identity changes, baseline portable artifacts and `BEZEL-DESIGN.md` were preserved.


## Validation follow-up: DuplicateProfileTest abort

The same pre-fix test DLL reproduced `0xC0000409` when hosted by `packages/Microsoft.Taef.10.100.251104001/build/Binaries/x64/TE.exe /InProc`, and passed `DuplicateProfileTest` individually when hosted by `bin/x64/Release/UnitTests_SettingsModel/TE.exe /InProc`. Both host executables have identical SHA256 (`B729C665A51A1E36DC24152ECDE4E4A7EFB618B75DC7387D8C6EDB103EF952C1`). The package-host folder has no `resources.pri`; the local-host folder does. This isolates the abort to test hosting/resource setup without changing identity or CRT code, so a separate baseline checkout was unnecessary.

`backslash/build/scripts/Run-Tests.ps1:31` explicitly requires the host executable beside each test DLL's `resources.pri`. `DuplicateProfile` first obtains the localized `CopySuffix` through `RS_`; `ScopedResourceLoader` uses `ResourceManager::Current().MainResourceMap()`, and `LibraryResources.cpp` fail-fasts on resource initialization/string lookup exceptions. The observed host-layout failure matches this path. No debugger was installed or used to recover the underlying resource HRESULT.

Diagnostic evidence is retained under `artifacts/build/20261013-144052-896-Release-x64-ed9f7260`: `duplicate-profile-individual.log`, `duplicate-profile-local-host.log`, and `duplicate-profile-host-comparison.json`. Use the local host for resource-dependent tests; changing only the working directory is not the repository's required host layout.

Inspection also found a separate CRT regression: the separate intensity macro had not been included in `CascadiaSettings::DuplicateProfile`, so duplicating a profile omitted its local intensity overrides. This is fixed by applying `MTSM_RETRO_INTENSITY_SETTINGS` to the existing duplication macro, retaining its default-origin rules. `DuplicateProfileTest` now sets glow to explicit zero and scanline to 0.75 and verifies both copied values and complete profile JSON equality.

Latest corrected-build evidence: **`artifacts/build/20261013-145014-247-Release-x64-8a1fbd77`**.

- Release/x64 build: **0 errors, 135 warnings**, 36.72 seconds.
- Test DLL build: succeeded. Local TAEF host: **all 10 ProfileTests passed**, plus **all 4 Backslash CRT/identity tests passed**, with no failures/blocked/skipped/not-run tests in those selections. See `profile-tests-local-host.log` and `settings-focused-tests-local-host.log`. The entire model suite was not run.
- Fresh assembly, static portable identity checks, and hidden isolated smoke passed. The profile shell ran; two existing Terminal settings/state hashes stayed unchanged; the owned app process was stopped. No adjacent state file was observed during the short smoke. Smoke settings and marker were retained under build evidence and removed from the delivery folder by a checked move.
- Latest clean ZIP: `artifacts/portable/Backslash-Dev-0.0.1.0-CRT-x64-8a1fbd77.zip`; SHA256 **`AA3F2CE07BBC5B416531517E0E0602B0300BEA118A5E23AF6780A008E29A324A`**. Earlier deliveries remain preserved; this one supersedes the initial CRT ZIP.
- Extracted runnable executable: **`D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-145014-247-Release-x64-8a1fbd77\WindowsTerminal.exe`**. The delivery folder is clean and contains `.portable`; its next launch will create adjacent settings.

### Manual visual checklist — all unrun

Use the latest clean portable folder or extract its ZIP into a new directory. Launch its executable by full path. Keep `.portable` beside it and edit only that copy's adjacent settings.

1. Open Settings → Profiles → a test profile → Appearance. In the Text group, find the retro toggle and glow/scanline sliders directly below it. Verify readable labels and 30%/50% initial values; toggle is off by default.
2. Enable retro effects with no custom shader path. Verify glow and alternating scanlines in both the preview and the saved profile's actual terminal. Move each slider and check that the default-appearance preview updates after its brief debounce.
3. Set glow to 0% and scanlines to 50%: scanlines remain, blur/glow disappears. Set glow to 30% and scanlines to 0%: glow remains, line modulation disappears. Set both to 0%: compare readable neutral output with the retro toggle off.
4. Save explicit zero values, close/reopen Settings and relaunch the portable copy. Verify zero remains saved. Turn the retro toggle off/on and verify saved intensities are preserved. Cancel a separate unsaved change and verify it is not retained.
5. In profile defaults set glow 55% and scanlines 25%. On the test profile clear both overrides: verify inheritance indicators and 55%/25% values. Override glow with 0%, then use its reset control and verify it returns to inherited 55%.
6. Tab to both sliders and use arrow/Home/End keys. Verify percentage labels and accessible names, plus keyboard access to each reset control.
7. Duplicate the test profile after explicitly setting glow 0% and scanlines 75%. Verify the duplicate retains both values after save/reload and can reset them independently.
8. Add an unfocused appearance with glow/scanlines 0% while the focused appearance has visible effects. Save, focus the actual terminal, then another window: verify actual terminal focus transitions apply the values. **Do not use the preview to judge unfocused appearance; that preview is not implemented.**
9. In this copy's `settings/settings.json`, set `experimental.pixelShaderPath` to `D:\codex-workspace\backslash-terminal\backslash\samples\PixelShaders\Nop.hlsl` for the test profile. With retro enabled, changing intensities must not add built-in effects. Then use a nonexistent shader path to trigger compile/load failure: verify it does not activate built-in retro effects. Restore an empty shader path afterward.
10. Check text, cursor, selection, background image/transparency and scrolling at 0/default/full intensity. Resize the window and test available DPI scales (e.g. 100%, 125%, 150%, 200%): look for stale bands, redraw artifacts or unreadable text. This is visual coverage only; hardware GPU timing and idle redraw measurements remain separate unrun checks.

No remaining implementation/build blocker was identified. Native UI automation is unavailable in this session, so every checklist item above remains for manual validation. The earlier WARP evidence remains applicable: this follow-up changes model duplication and its test, not shader/runtime rendering code.

## Implemented independent scanline dimensions (2026-10-14)

Appearance adds **Dark scanline thickness (DIP)**, 0.5–8, and **Clear gap between scanlines (DIP)**, 0–16; both default to 1. Each has a quarter-DIP slider, numeric value and inherited reset. Gap zero uniformly dims the screen at nonzero intensity. Zero intensity and zero glow still bypass the postprocess regardless of dimensions. A custom shader retains precedence.

Values persist per focused/unfocused appearance through JSON (`experimental.retroTerminalScanlineThickness` / `experimental.retroTerminalScanlineGap`), inheritance, null/reset, copy/duplicate and the normal live preview. Fractional pixel coverage filters thin/noninteger bands; default 100%/200% rows retain legacy output, while fractional-DPI intermediate rows intentionally differ from the former hard-floor mask. Alpha modulation and the external custom shader ABI remain unchanged. No static animation/Time dependency is added.

See [the completed rendering report](../backslash/CRT-RENDERING-PLAN.md#implemented-follow-up-scanline-thickness-and-clear-gap-2026-10-14) for tests, resource reuse/neutral measurements, pristine artifact and unrun native visual checks. Test this build in a fresh copy; existing working settings were untouched. No tint implementation is included.

## Implemented screen-only phosphor tint (2026-10-14)

Appearance adds **Screen phosphor color** (default `#00FF41`) and **Screen tint intensity** (0-100%, default zero) with inheritance, reset, duplication, serialization and default live preview. Enable Retro terminal effects. JSON uses `experimental.retroTerminalTintColor` and `experimental.retroTerminalTintIntensity` (0-1). Zero preserves post-CRT RGBA exactly and all-zero strengths bypass postprocessing; custom shaders retain precedence.

Full tint preserves linear-light luminance along a black -> chosen phosphor -> white tonal ramp; highlights can whiten, black/white phosphor gives grayscale, and ANSI hue distinctions disappear. Lower strengths blend smoothly with original colors. Tint preserves incoming post-glow/scanline alpha, including their unchanged legacy alpha modulation. It affects terminal GPU pixels (including cell backgrounds, text, cursor/selection and inline images); separate profile/window background images, wallpaper/acrylic/solid backdrops, XAML overlays, bezel/casing and tabs/chrome remain outside the pass. Static tint uses the existing Direct3D11 CRT path and reuses resources on color/strength changes.

See [the completed tint report](../backslash/CRT-RENDERING-PLAN.md#implemented-follow-up-screen-only-phosphor-tint-2026-10-14) for exact coverage, alpha/gamut behavior, tests/artifact and manual checklist. Use a fresh portable copy to inspect dark/light and transparent themes, ANSI readability, preview/reset/save/cancel and custom precedence. Native visuals/accessibility and hardware GPU/VRAM remain unrun.

## Portable identity implementation and verification

Source: `docs/backslash/PORTABLE-IMPLEMENTATION.md`. These are dated development records, not current user instructions.

## Historical implementation reference (2026-10-13)

The following dated build and source details are contributor references, not download instructions. Current builds use `Backslash.exe` and Backslash artwork.

### Identity changes

- `WindowEmperor.cpp`: Backslash/Preview/Canary/Dev routing prefixes; elevation, executable-path and user-SID hashes remain intact.
- `FileUtils.cpp`: unpackaged fallback is `%LOCALAPPDATA%\Backslash\Terminal`; `.portable` still selects the adjacent `settings` directory. `ApplicationState` already uses that same base and was not changed.
- `CascadiaSettingsSerialization.cpp`: stable-settings automatic migration is removed for every branding mode. External fragment discovery is an empty function for both runtime settings and extension-list discovery; upstream file roots, extension catalog identity and discovery-only helpers are removed. Explicit fragment parser tests remain supported. Built-in dynamic profile generators remain available.
- Package resource strings and TerminalApp context resources: existing Dev product-name/short-name/store-name/description keys identify Backslash Dev, including localized keys that existed. Microsoft legal attribution and internal DLL/namespace names remain.
- `WindowsTerminal.rc`, `.vcxproj`, and `CascadiaSettings.cpp`: executable product/description metadata and portable About/CLI version label identify `0.0.1.0 (Backslash Dev; upstream 1.25)`. This is a local development version, not an official upstream release version. Existing executable name is retained for launch/elevation helper compatibility.
- Temporary upstream Dev icons remain. No production Backslash art or new assets were downloaded.

## Reproduce and run

Run `backslash/tools/backslash/Build.ps1` (default Release/x64), then `backslash/tools/backslash/New-PortableDistribution.ps1 -BuildRun <successful-run-directory>`. The latter unpacks the exact unsigned build payload **without installation**, adapts upstream resource-merging support, merges WinUI XAML resources, copies app-local x64 VC145 CRT DLLs from the installed toolchain, retains notices/licenses, writes `.portable`, and emits a file/hash inventory. Fresh output is required; previous directories are never overwritten. Resource-staging evidence is retained, with no recursive deletion.

Run `backslash/tools/backslash/Test-PortableIsolation.ps1 -PortableDirectory <output-directory>` for focused source and distribution checks. Launch `Backslash.exe` by full path inside a current Dev portable directory; the historical artifacts below retain `WindowsTerminal.exe`. No new CLI alias is registered.

Delivery directory:
`artifacts/portable/Backslash-Dev-20261013-142327-974-Release-x64-d6d855bb`

Clean ZIP:
`artifacts/portable/Backslash-Dev-0.0.1.0-x64-d6d855bb.zip`

ZIP has 259 entries and includes `.portable`; smoke-generated settings/state and shell marker files are excluded. SHA256:
`E8D9B2F3456B5EA4BF268B53EAFD5575FA83AE47AB112E959C4765CF12FB899F`

## Validation actually performed

Evidence root: `artifacts/build/20261013-142327-974-Release-x64-d6d855bb`.

- Repository build script, amd64 MSBuild: **0 errors, 135 warnings**, elapsed 37.79 seconds. Existing baseline also reported 135 warnings. Warning categories include PRI263 missing-neutral-resource warnings, WMC1004/WMC1506, APPX4001 and LNK4068. These are retained limitations, not evidence of a warning-free build.
- Settings test DLL compiled using `SettingsModel.UnitTests.vcxproj`, Release/x64, with explicit `/p:SolutionDir=D:\codex-workspace\backslash-terminal\`. Direct TAEF run of `SettingsModelUnitTests::DeserializationTests::BackslashDisablesExternalFragments`: **1 passed, 0 failed/blocked/skipped**. Both discovery modes leave empty profiles and extension map unchanged. Full test suite was not run or packaged/registered.
- Static isolation/completeness checks passed, including adjacent settings/state base, fallback, all-branding migration removal, disabled fragment discovery, routing prefix/path/elevation separation, runtime/resources/notices, absent package manifest, executable description and version.
- A hidden smoke launch of the exact delivery directory executed its PowerShell test shell and created adjacent `settings/settings.json` and `settings/state.json`. Second `-w 0` invocation exited through same-distribution handoff and executed its shell. The resident process was verified by exact executable path and stopped after testing; no smoke app is intentionally left running.
- Two discovered existing Windows Terminal settings/state files retained identical SHA256 hashes before launch, after initial smoke, and after handoff/cleanup. Hash-only evidence is retained; no contents were logged or edited.
- Initial assembly caught a missing HTML notice, corrected before delivery. Initial standalone test-build command lacked SolutionDir, corrected and subsequently passed. Initial static metadata check found blank upstream local-build version resources, corrected with explicit development metadata and a fresh final build. Earlier builds and incomplete distribution attempts were preserved.

## Limits and scope

Visible UI/icon review, elevated/unelevated coexistence, concurrent installed-Terminal or second-copy routing, actual absent-marker fallback runtime, Preview/Canary runtime, and shared-root sentinel tests were not performed. Migration/fallback/routing coverage beyond the Dev smoke is static. Final production icons, packaged identity/signing/update policy and portability to a clean Windows machine remain future work.

The unsigned MSIX in build output retains upstream package identity and must not be installed as the Backslash portable deliverable. Use the assembled portable directory instead. Signing and independent installed-package identity are not provided by this build workflow.

## Settings placement acceptance limits

Native visual placement, responsive resizing/localization/DPI, actual keyboard traversal, save/discard interactions and live tab transfer/rollback still need interactive QA. Source review accepted the focused fixes; this is not end-to-end GUI verification. The reported intermittent blank split remains unresolved and was not reproduced in subsequent user attempts. Earlier startup-only debugger smoke tests do not clear these interaction gates.


## Resident startup verification

Implemented in uncommitted development, not the published preview. See [current candidate and verification](HANDOFF.md); the dated focused evidence below remains historical. The user reports discovery remaining enabled after last-window close/reopen with background running enabled. This does not prove every startup/hidden-window path.

Six focused startup policy/reservation tests passed, including connection/content ownership transfer before and after initialization. Initial payload presence is retained before moving it into the page, so saved-group preparation cannot mistake an emptied member for an ordinary request. The complete settings suite reported 296 total, 294 passed, zero failed, and two optional local-shell cases skipped in that run. Both optional cases passed separately using the complete runtime, exercising all 296 unique cases. Fresh independent portable processes confirmed discovery defaults off. Six source-wiring checks passed; the Release/x64 application build completed with zero errors and 134 warnings. Portable isolation checks passed.

Candidate: artifacts/portable/Backslash-Dev-20261025-183242-587-Release-x64-e4eb708e/Backslash.exe
