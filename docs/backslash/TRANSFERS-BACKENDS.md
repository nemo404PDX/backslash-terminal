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
