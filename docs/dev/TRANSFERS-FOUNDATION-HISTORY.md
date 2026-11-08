# Transfers foundation historical evidence

These verbatim snapshots predate the SFTP implementation. They are retained for contributor evidence, not current user instructions. Paths in the snapshots were relative to docs/backslash.

## Previous TRANSFERS-FOUNDATION.md

```markdown
# Transfers foundation

The optional [backend settings foundation](TRANSFERS-BACKENDS.md) adds persisted enablement intent, saved SFTP profile references and a gear to the single Settings > Transfers > Backends page. It adds no transfer engine or ready backend.

Transfers is restored beside Settings in the main terminal dropdown for post-preview.2 development. The published preview.2 keeps this entry hidden because the backend is unfinished; its tag and ZIP are unchanged. This development entry opens only the foundation described below, not a working transfer engine. It is an app-owned activity and history view, distinct from the future local/remote file browser. The native view provides search combined with an All, Active, Completed, Failed or Canceled status filter. Active includes queued, connecting, transferring, verifying and cancellation awaiting acknowledgement. A direction filter is optional future work. The empty state is real: no sample jobs are inserted, no backend is connected, and no network action is offered.

Settings > Interaction > Open Transfers in selects Tab (default), Side-by-side or Separate window through the existing native pane/tab/window creation paths. This is stored independently as `backslash.transfersLaunchMode`; it does not alter Settings placement. Side-by-side checks the actual active terminal leaf and the Transfers minimum of 360 by 300 DIPs, otherwise opening a tab. The app window is not resized to force a split. Reopening focuses the existing process-local Transfers view, retaining its search and filter. Placement changes take effect after closing and reopening that view.

Transfers uses an explicitly assigned built-in identity distinct from Settings, with an independent reservation and ownership lease. Tab positions, pane IDs, window IDs, job IDs and ownership leases remain separate. The native view uses app theme resources and the existing Backslash accent, without custom color overrides. Search and status filtering sit inside one padded, rounded card using the native Expander theme resources. At wide allocations, search takes the flexible column and status uses a measured width bounded to 160–240 DIPs. At narrow allocations they stack inside the card. The redundant in-view Close button has been removed, while the existing 40-DIP header/chrome area remains available for future controls. Accessible names, heading semantics, native focus visuals and a polite empty-state announcement are provided; actual screen-reader, high-contrast and DPI behavior still require GUI validation.

Use the tab X or the guarded Close pane action to close the focused Transfers leaf. The later [focused-pane close follow-up](FOCUSED-PANE-CLOSE.md) makes a split tab's visible X close its last-focused leaf, preserving siblings; a single-pane X closes the tab. Transfers view closure still retains jobs. Explicit Close entire tab remains distinct, and pinned/read-only policies still apply. Moving a Transfers pane or a tab containing it to another window is guarded with an explanation in this first slice; the existing live native-tree handoff is specific to Settings and is not silently treated as a generic docking system. Close and reopen with the desired placement instead. Same-window tab ordering remains available.

Job records belong to a process-owned, thread-safe model from submission, independent of whether any view is open. The view subscribes for refresh and unsubscribes on close; it never changes job status or cancels work when released or destroyed. A future command frontend can submit to the same job owner. Sharing a backend library between native and TUI frontends alone would not share active jobs: that requires a common live worker/session. Ordinary PTY downloads remain ordinary child-process work and are not automatically managed; this slice does not suppress terminal process termination. No transfer engine, persistent service, queue storage, credentials or actual background worker is introduced. Full application quit behavior remains unresolved.

Future history clearing removes records, not source/destination files, and must not cancel active jobs. No Clear, Cancel or Retry controls are offered before the corresponding backend capability exists. Retention of seven days, thirty days or forever was discussed but not selected; no automatic expiry or failed/retryable record cleanup is implemented. Future cleanup policy must be explicit.

Outer placement (tab, split, window) remains separate from the internal left/right or stacked orientation of a future file browser. SFTP is the recommended first remote protocol to investigate; FTP/FTPS remain separate decisions. No protocol is implemented by this management foundation.

A separate Background sessions view was discussed only as future design. Explicitly detached Backslash-owned terminal sessions could show running/exited state, reopen and terminate actions. Discoverable Help and the empty state should explain the actual supported detach/menu/terminal command, how to return and reopen, and the differences between closing a view, terminating a session and quitting the app. No command syntax is selected here. Arbitrary shell background commands and externally detached tools do not register automatically, and network activity is not a session detector. This is not additional implementation scope.

Validation is recorded after the candidate build. No visible GUI launch or real transfer is part of this slice's automated verification.


## Earlier foundation verification checkpoint

Release x64 build `20261026-162510-173-Release-x64-8f42b09f` completed with 0 errors and 134 retained warnings in 46.67 seconds. Seven focused Transfers tests passed. The complete-runtime Settings suite passed 311 of 313 tests with no failures; two optional local-shell transport tests were skipped. The suite used the existing local argument-capture fixture, which cannot connect to a server. The test-library linker reported 38 LNK4020 debug type-record warnings, affecting available debugger symbols.

The complete candidate is `artifacts/portable/Backslash-Dev-20261026-162510-173-Release-x64-8f42b09f/Backslash.exe`. All 34 portable completeness/isolation checks passed. Keep the whole folder and its `.portable` marker together. This is uncommitted development, not the published preview. Evidence is in `artifacts/build/20261026-162510-173-Release-x64-8f42b09f/transfers-verification`. No GUI, network transfer, credential access, installation, commit or push was performed. Native visual/interaction QA remains pending.

## Filter card and close-control follow-up

The parent inspected the supplied screenshot before assigning this change. This source follow-up groups search/status in a lighter, theme-driven card, adapts to the actual inner card allocation rather than whole-window width, and removes the redundant Transfers Close button following explicit user approval. Native search/filter handlers, accessible names, status choices, empty/backend messages and lifecycle APIs are retained. Native Expander background/border and ControlCornerRadius resources follow the app theme and high-contrast mappings rather than hardcoded colors. Real appearance remains subject to native visual QA.

Twenty-two isolated C++ layout-policy checks passed, covering narrow/wide allocations, bounded localized-label widths, breakpoint search space, DIP-scale arithmetic and invalid/unmeasured sizes. XML/source checks confirmed the shared card, native resource references, retained header and absent Transfers close-button wiring. These checks do not establish XAML compilation, actual rendered colors, text scaling, keyboard traversal or per-monitor DPI behavior. The earlier candidate above predates this visual follow-up. A coordinated composite build/package is pending; no GUI or user settings were touched.

Settings close-button removal belongs to the separately coordinated Settings author. This Transfers task has not edited Settings MainPage or its button-specific tests. Underlying Settings preflight and pane/job lifetime protections remain required.

## Composite verification after polish

The coordinated profile task subsequently removed the redundant Settings close UI while retaining its 48-DIP header, hamburger and shared dirty/busy close preflight. Composite candidate `20261026-180431-416-Release-x64-85e666ed` includes both changes and the Transfers filter card. Its Release x64 build completed with 0 errors and 134 retained warnings; the isolated Settings suite passed 331 of 333 tests with zero failures and two optional real-shell cases skipped. All 22 Transfers layout checks, 10 close-UI/scope source checks and 34 portable checks passed. Evidence is in that build's `profile-verification` directory. This supersedes the pending composite-build note above; it does not establish native visual or interaction correctness. No networking, GUI, user settings mutation, installation, commit or push was performed.

```

## Previous TRANSFERS-BACKENDS.md

```markdown
# Optional Transfers backends

Development settings foundation, 2026-10-28. Not a release gate or a working transfer engine.

## Implemented contract

Settings > Transfers > Backends owns the configuration. The Transfers gear requests that same page through the existing process-wide Settings identity, dispatcher, placement preference and live draft. Repeated opens coalesce and reuse the editor. Save/Discard and stale/busy close policies remain the existing editor policies. There is no second dialog or in-view Close control.

`backslash.transferBackends` is a version 1 object with `sftp`, `sabnzbd`, `aria2` and `rclone` records containing Boolean `enabled`; SFTP additionally stores `profile`, a saved SSH profile GUID or an empty string. Defaults disable every backend. Settings copies and JSON persistence retain this data. The setter validates schema, types and identity before mutation. Unsupported or malformed loaded objects are preserved on unrelated saves, and this page displays an error instead of silently replacing data. Unknown fields require a future version migration rather than being discarded.

SFTP offers existing owned SSH profiles (`Backslash.Terminal.SSH` and saved legacy `Windows.Terminal.SSH`) whose model is neither deleted nor session-only. Hidden owned profiles remain eligible. No scan or process is invoked. Only the profile identity is stored; no host keys, passwords, private keys or copied command lines are introduced. Missing/deleted/transient profile references remain visible as missing and can be cleared/replaced. A future adapter must resolve and validate the owned profile again immediately before submission and obtain authentication through the SSH ownership boundary; it must not blindly execute its terminal command line. Selecting a profile proves configuration intent, not connectivity or SFTP capability.

The common contract separates Disabled (reject new submissions), Unconfigured (missing usable configuration), Unavailable (configuration exists but no verified runtime), and Ready (implemented runtime with fresh verification). Planned capabilities are separate from runtime capabilities. Submission requires Ready plus the requested actual capability. No adapter supplies runtime evidence in this slice, so none reports Ready or offers transfer actions. SABnzbd, aria2 and rclone toggles persist future enablement intent; all remain explicitly unconfigured. No tool choice, server configuration or executable selection is implied by these toggles.

Closing a view unsubscribes it, retaining process-owned jobs. Disabling a backend blocks future submissions without canceling active work. Future jobs must retain an immutable configuration snapshot and worker ownership until completion or explicit acknowledged cancellation. `JobOwnership` carries backend instance, owning machine and destination; a remote destination belongs to that machine unless explicitly identified otherwise. No files are represented as downloaded to this computer merely because a remote job completes. This is an architecture contract, not an implemented worker, durable queue or restart guarantee. Full-process quit policy remains open.

## Planned adapter setup

Install locally, Use existing installation, and Connect to a server appear only where a tested adapter supports them. There are no unusable setup buttons in this slice. Installation, enablement, updates and uninstall are distinct operations. Installation requires explicit source/version/download size/license disclosure, pinned official artifacts, integrity verification, managed tool storage, platform/dependency handling and an explicit installation action. No arbitrary GitHub latest downloader is allowed. Remote credentials need a separately designed credential boundary; they must not be embedded in these preferences.

Likely capability candidates are SFTP upload/download, SABnzbd NZB, aria2 download/torrent and rclone files/cloud. The native UI and future terminal UI should use the same domain contracts. Sharing code does not share running jobs: a future common live worker/session is required for that behavior.

## Release and validation

Preview.3 remains stabilization first. This settings foundation is insufficient for release visibility. A real backend must pass end-to-end upload/download, progress, cancellation/failure handling and continued jobs after view closure before the Transfers release gate is met. Published preview.2, its tag, folder and ZIP remain immutable.

Regression coverage extends the existing Transfers settings suite with persistence/copy/reload, invalid configuration preservation and rejection, enable/disable state, capability filtering and pending/repeated Settings destination requests. Native rendering, actual gear interaction, theme/DPI/accessibility, ARM64 and Win32 require explicit executed evidence; source checks and x64 compilation alone do not establish them. No installation, network connection, credential setup or user-settings mutation is part of these tests.

## Verified settings checkpoint (2026-10-28)

Final-source Release/x64 build `20261028-141920-589-Release-x64-66e9d15d`: zero errors, eight retained APPX4001/WMC1004 warnings. The Settings test-library build has zero errors and 42 retained warnings, including LNK4020 debug type-record warnings; no provider or packaging flag was changed to suppress them.

| Gate | Executed result |
| --- | --- |
| Complete Settings suite | 350 total: 348 passed, zero failed, two optional real-shell cases skipped |
| New backend model cases | Four passed: persistence/copy/reload, invalid configuration preservation/rejection, enablement/capabilities and pending/repeated/failed Settings destination flow |
| Owned offscreen production page | Passed navigation, hidden owned SSH profile selection, repeated draft retention, source isolation, Discard and invalid-version rejection |
| Existing native SSH scan/import regression | Passed, including one/two selections, repeated/empty/error/recovery scans and duplicate suppression; no unhandled XAML errors |
| Portable completeness/isolation | 34 checks passed against a separately assembled development runtime |
| Resource fallbacks/self-tests | Passed: 48 files, 15,790 entries |
| Working-tree whitespace | Passed with CRLF recognized |

Evidence: `artifacts/build/20261028-141920-589-Release-x64-66e9d15d/transfers-verification`. Native fixture: `artifacts/ssh-scan-ui/20261028-142231-269-f38f6923`. The preceding Settings run at `20261028-141427-992-Release-x64-2a4c3393` recorded one existing Unicode-search failure caused by an editing-script encoding mistake. Original Unicode and line endings were restored; that failure is retained as historical evidence, and the complete corrected rerun passed.

Interactive gear/tab/split/separate-window behavior, theme/DPI/accessibility appearance, ARM64 and Win32 are unrun. Offscreen native regression is narrower than those interactive gates. No real backend, server connection, credential setup, backend installation, user-settings mutation, commit or push occurred. This does not meet the Transfers release gate.

## Native acceptance follow-up

The independent source review accepted the bounded development slice with no blocking defect. Only the focused native fixture and documentation changed in this follow-up; all 19 `backslash/src/` feature paths and the application executable remained byte-identical to the preceding verified checkpoint. No full application rebuild was needed.

The expanded owned offscreen production fixture passed: missing saved GUID presentation and Unconfigured status; genuine Deleted and transient discovery filtering while retaining hidden owned profiles; missing-reference clearing/replacement across repeated navigation; native Save followed by actual disk reload and a new editor; configured-but-Unavailable state without an engine; loaded invalid Boolean and unsupported-version objects rendering only heading/notice/error with no editable backend controls; preservation of malformed/unknown fields during unrelated native Save and disk reload. The existing NewTabMenu and SSH scan/import regressions also passed with no unhandled XAML errors. Its compiler gate used /W4 /WX.

Evidence: `artifacts/ssh-scan-ui/20261028-145922-647-e3445b75`; summary and final inventory: `artifacts/build/20261028-141920-589-Release-x64-66e9d15d/transfers-verification/native-acceptance-followup`. Earlier failed fixture runs are retained: one used the legacy endpoint-import review instead of the selected-alias API, and another deliberately injected a detached transient candidate into the Save model, triggering existing SSH validation. The final fixture separates filter-only injected data from normal owned-profile Save. No product defect or product-code change was required.

Interactive visual QA was attempted by requesting launch of an isolated copied portable executable with synthetic settings at `C:\Users\nemo\Documents\Codex\2026-10-28\task-4\visual-qa\Backslash.exe`. Computer Use returned **app approval timed out**. A subsequent inventory contained no window for that QA copy. The launch was not retried; approval was not accepted or explicitly denied, and this is no longer a pending turn wait. No actual pixels or screenshots were captured. Actual gear interaction, Settings tab/split/separate-window routing, ordinary/minimum-width appearance, theme readability and interactive accessibility remain unrun. Offscreen native coverage above is not interactive coverage.

Verified current portable executable: `artifacts/portable/Backslash-Dev-20261028-141920-589-Release-x64-66e9d15d/Backslash.exe`, SHA256 `4262D35DA28FD090009F67F28EB1EAF2328292CF95033073F5F05C6E9871657B`. No engine, credential, network transfer, real user-settings change, commit or push was introduced.

```
