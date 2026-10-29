# Maintainer handoff notes

Internal development context for contributors and coding agents. This file collects machine-local artifact identities, operational restrictions, reported acceptance, unfinished checks and task history. It is not a product overview, download guide or release announcement. Use release notes for version-specific availability and the root README for user instructions.

Keep future agent narration, authorization/commit state and checkpoint handoffs here. Keep reproducible technical tests and implementation details in their feature-specific contributor references; keep user-visible limitations in the relevant public guide. Do not modify historical published release notes or assets to update a handoff.

## Earlier development checkpoints

Optional [Transfers backend settings](../backslash/TRANSFERS-BACKENDS.md) are under development alongside stabilization: one Settings page and gear navigation, persisted enablement and saved SFTP profile references, with explicit unconfigured/unavailable states. No engine, installer or real transfer is implemented; this does not meet the Transfers release gate.

Snapshot: 2026-10-28 (post-preview.2 development). This separates the published preview, the current uncommitted checkout and future designs. Dated build and review records remain evidence for their own checkpoints.

SSH Scan UI correction checkpoint (2026-10-27): the production scan/list binding error `0x80070057` is fixed and independently source-reviewed. The actual offscreen production UI regression passes one- and two-selection imports, stale-selection clearing, empty/error recovery and duplicate suppression with synthetic SSH config and no unhandled XAML events. Settings suite: 344 passed, zero failed, two optional skipped (346 total), including all 24 SSH tests. Release/x64 build: zero errors / 134 warnings. Corrected portable: `artifacts/portable/Backslash-Dev-20261027-ssh-scan-ui-fixed-f70aa588/Backslash.exe`. Its 34 isolation checks, complete manifest/payload hashes, dependency checks and packaged-component native UI gate passed; no user state was copied or actual app/SSH launched. Evidence: `artifacts/build/20261027-ssh-scan-ui-fixed-f70aa588`; do not reuse the earlier P2 package with the broken Editor DLL. See [SSH behavior and proof](../backslash/SSH-PORTABLE-IMPORT.md).

User reports preview.2 published at commit `13e97bc945c8f57d6be4e53ab654f6dddd5b46c3`. Current development restores Transfers and begins focused warning stabilization; it is not part of that immutable release. The earlier dated records below describe their original checkpoints.

Current warning-stabilization verification: full Release/x64 rebuild passed with 0 errors / 5 warnings (previous clean preview.2: 132); Settings 344 passed, zero failed, two optional skipped. See [warning causes, fixes and remaining gates](../backslash/WARNING-STABILIZATION.md). This does not change the published release.

## Earlier published preview.1

`v0.0.1-preview.1` is the published unsigned portable preview. Local `master`, `HEAD` and the peeled annotated tag resolve to `05e47f621c1a7d2d818b08c6eaa622dadd18da1e`. The later changes below are uncommitted and are not part of that tag or its published ZIP. A development artifact suffix is a build identity, not a Git commit.

The user reports downloading the published ZIP on a second Windows machine and launching it successfully, with the Windows unsigned-application warning. This is user-reported portability evidence, not a clean-machine compatibility matrix or a signing claim. GitHub workflows were manually disabled at the user's request; that operational state is not an automatic build/release guarantee. Signing, independent installed-package identity and broader compatibility remain outstanding.

## Current implementation and development changes

- **Portable SSH discovery and import:** startup never scans SSH configuration. Settings > Add a new profile offers Scan, a selectable candidate list and Import selected. Save commits selected aliases and adopts edited old discoveries, including Hide; Cancel/failed Save commit nothing. Alias references depend on this computer's SSH configuration and authentication; existing resolved endpoint records retain their prior semantics. No keys, credentials or host configuration are copied; see [SSH import](../backslash/SSH-PORTABLE-IMPORT.md).
- **Background lifetime and startup:** closing all windows can leave the process running when Compatibility > Allow Windows Terminal to run in the background is enabled. This process lifetime is distinct from SSH ownership: startup does not scan, and saved SSH connections remain available without scanning. Closing a window alone does not establish a fresh-process restart. An ordinary resident-process reopen from zero app windows applies the configured saved startup group, while additional windows and explicit requests retain their exclusions. See [resident startup](../backslash/STARTUP-GROUP-REOPEN.md).
- **Settings placement and lifecycle:** Interaction > Open Settings in selects Tab, Side-by-side or Separate window; default Tab. One process-wide Settings identity reuses the live editor and draft. A split uses the actual active leaf's available size and falls back to a tab if unsuitable. Same-process containing-tab moves retain the live editor; pane-only and unsupported transfers are guarded. See [Settings placement](../backslash/SETTINGS-PLACEMENT.md).
- **Settings chrome and close:** the dedicated 48-DIP header and hamburger remain; redundant in-view Close buttons in Settings and Transfers have been removed. Dirty/stale drafts still require a decision; busy operations guard close through ConfirmCloseSettings. A split tab's X closes its last-focused leaf; standard Close pane shares the Settings draft/busy guard, while explicit Close entire tab retains whole-tab semantics. Parent selections resolve through strict retained focus history; absent history is a no-op. The portable notice wraps in its own row above responsive Save/Discard controls. See [focused close and footer](../backslash/FOCUSED-PANE-CLOSE.md). The outer focus cycle includes the chrome. Compact and normal breadcrumb margins are both `0,4,0,0`, with viewport-constrained native overflow rather than clipping.
- **Transfers foundation:** the native activity/history view is restored in the development dropdown after publication of preview.2. The published preview.2 tag and ZIP retain the hidden entry. The view provides combined search and All/Active/Completed/Failed/Canceled filtering. Search/status share a padded, theme-driven card with wide-row/narrow-stack layout. Interaction offers an independent tab/split/window preference; repeated opens focus the existing view. Tab/pane closing preserves neighboring terminals and process-owned job records. No backend, real jobs, cancellation/retry actions, persistent worker or network protocol is implemented. Live view moves remain guarded. See [Transfers foundation](../backslash/TRANSFERS-FOUNDATION.md).
- **Terminal-pane profile actions:** the tab menu offers Split with profile below the existing direct Split, plus Relaunch with profile grouped with restart operations. Both target only the clicked tab's captured terminal leaf. Relaunch confirms interruption, optionally saves a scoped warning preference, applies the chosen profile in place and preserves siblings/layout. Group capture records its new GUID/source. Inactive-tab updates do not replace the selected window background or take keyboard focus. See [profile actions](../backslash/PROFILE-PANE-ACTIONS.md).
- **Brand accent:** application-scoped mint-green tones use the shipped Backslash cursor/bright-cyan `#14FFCA` anchor. Light mode uses darker tones; high contrast maps to system highlight colors. Warning/error resources and terminal palettes are independent. See [UI accent](../backslash/UI-ACCENT.md).
- **Phosphor, pictures and scanlines:** opt-in Preserve scheme tones in phosphor retains tonal ordering without the legacy contrast guard. Rendering handles background ownership, opacity and picture processing, including shared-window geometry/pane crops. Scanline color adds Black/White with Black remaining the default. Read [scheme-sensitive phosphor](../backslash/PHOSPHOR-SCHEME-TONES.md), [scanline color](../backslash/SCANLINE-COLOR.md) and the [rendering evidence](../backslash/CRT-RENDERING-PLAN.md); specific GPU/readback claims belong to those records, not the settings suite.

## Earlier development candidate (audit 2026-10-28)

`artifacts/portable/Backslash-Dev-20261027-ssh-scan-ui-fixed-f70aa588/Backslash.exe`

The audit freshly verified that this executable exists (Backslash 0.0.1.0) and matches SHA256 `40B9A2950CF05615FADC8549A1C8CA9B06C286EDC9C60999F6AE7FAF0447FCFA`. The packaged Editor DLL matches `977BB9EF205849CE2C31C01717B3606707B445F3255C9A17EF90424117E2ECB0`. All 23 files in the build's source-hash snapshot still matched before this documentation sync. That is a bounded snapshot, not a full-tree build provenance claim. Keep the complete portable folder and `.portable` marker together.

The build and automated results in the opening paragraph are recorded October 27 evidence, not reruns for this audit. No build, real SSH connection or user-settings change was performed during the audit. Local HEAD/master/tag remain at the published preview commit; no later commits exist. The working tree contains 107 modified tracked paths and 36 untracked entries at audit time, including source, tests, docs and artwork. Local remote-tracking refs match the tag; no network fetch was performed.

On October 28 the user confirmed scanning, selecting one desired connection, importing/saving it and finding it still present after reopening (`winblows-nuc`). This is real user acceptance of that workflow and restart persistence. It does not establish SSH authentication, hidden-profile persistence, all-machine compatibility or broader Settings/Transfers GUI QA. Automated synthetic hidden-profile restart checks remain separate evidence. Background sessions, file-browser/protocol backends, previews and durable history/cleanup remain future scope; the current Transfers activity model is process-local.

## Earlier profile-actions candidate and verification

`artifacts/portable/Backslash-Dev-20261026-180431-416-Release-x64-85e666ed/Backslash.exe`

This composite candidate includes profile relaunch/split menus, the independently reviewed transaction fixes, Transfers card polish and redundant Settings/Transfers Close-button removal. It remains uncommitted development, not the published preview. Keep the complete folder and `.portable` marker together.

| Check | Recorded result |
| --- | --- |
| Release/x64 composite build | 0 errors, 134 retained warnings; 00:00:42.24 |
| Complete-runtime Settings suite | 333 total: 331 passed, 0 failed, 2 optional real-shell cases deliberately skipped |
| Focused profile regressions | 10 passed, including rollback after partial UI refresh and selected-tab focus/background policy |
| Transfers layout policy | 22 isolated C++ checks passed; existing Transfers model tests also passed in the complete suite |
| Settings/Transfers close-UI and scope source checks | 10 passed |
| Portable completeness/isolation | 34 passed |
| Changed-source whitespace | Passed with CRLF recognized |

Evidence: `artifacts/build/20261026-180431-416-Release-x64-85e666ed/profile-verification`. The test-library link retained 38 LNK4020 debug type-record warnings. The final suite used disposable task-local fixture directories, process-only Windows path casing normalized for existing case-sensitive assertions, and a verified network-incapable capture executable named ssh.exe. Earlier restricted-run failures and the fixture naming correction are retained in the evidence; no product behavior was changed to bypass those test-environment failures. Real-shell opt-ins remained disabled. No GUI, network transfer, user settings mutation, commit or push occurred. Native menu width, focus, launch-failure presentation, high contrast/DPI and Transfers appearance remain visual QA gaps. The intermittent blank-tab report remains unresolved.

The profile transaction's independent source re-review at 2026-10-27 01:14 UTC closed all three prior P2 findings with no new actionable finding in scope. Candidate 85e666ed was delivered with the documented test results and native GUI limitations. This review did not independently execute the build or validate rendered interaction; native QA remains outstanding. The repository remains uncommitted and unpushed.

## Earlier focused-close candidate and verification

Repository-relative executable:

`artifacts/portable/Backslash-Dev-20261026-172834-023-Release-x64-687a34d6/Backslash.exe`

This candidate includes the Transfers foundation alongside the preceding Settings, compact-breadcrumb and brand-accent changes. It also includes the focused-leaf close guards and responsive portable footer follow-up. The earlier side-by-side reopening report remains intermittent/unreproduced; see [focused close and footer](../backslash/FOCUSED-PANE-CLOSE.md). It is a local development candidate, not an update to the published preview. Keep its complete folder and `.portable` marker together. Build and portable assembly commands are in the [root README](../../README.md) and [build guide](../../backslash/tools/backslash/README.md).

| Check | Recorded result |
| --- | --- |
| Release/x64 application build | 0 errors, 134 warnings; 00:00:49.15 |
| Complete-runtime settings suite | 323 total: 321 passed, 0 failed, 2 optional local-shell cases skipped in this run |
| Optional local-shell cases | Both passed separately against this complete runtime; all 323 distinct settings cases exercised |
| Transfers model and placement tests | 7 passed, including combined Unicode search/status filtering, concurrent job submission, view-close independence and Settings/Transfers identity/preference isolation |
| Accent resource/contrast checks | 18 passed, including opacity-adjusted light/dark fill states and high-contrast mappings; palette unchanged |
| Close/footer/chrome/breadcrumb source checks | 62 passed: 33 close/batch-routing, 10 footer, 10 chrome and 9 breadcrumb |
| Portable completeness/isolation checks | 34 passed |
| Fresh-process portable SSH default | Passed in fresh processes after a preceding process enabled discovery |
| Diff whitespace | Passed |

Build evidence: `artifacts/build/20261026-172834-023-Release-x64-687a34d6`. Warnings include PRI263 missing neutral resources and WMC1004/WMC1506 XAML warnings. The test-library build also reported LNK4020 debug type-record warnings; tests passed, but some debugger symbols may be unavailable. Historical Transfers fixture results remain in its own record. Current test and source-check logs are under this build evidence directory's `batch-close-verification` folder. These are retained warnings, not a warning-free build. Narrow independent source reviews accepted the focus, compact-breadcrumb and accent changes. Source checks and calculated contrast do not establish actual on-screen resource resolution or interaction.

## Remaining validation and known reports

Transfers tab/split/separate-window opening, focus reuse, own-view close, narrow-width layout, keyboard/screen-reader behavior and theme transitions still need interactive QA. No visible application was launched for this slice. Moving its view to another window is explicitly guarded pending a dedicated native handoff.

Actual GUI light/dark/high-contrast switching, compact breadcrumb overflow, header resizing/localization/DPI, keyboard traversal and Settings placement/tear-out/rollback require visual and interaction QA. Prior startup-only debugger smoke evidence is narrower. No real host SSH configuration or private keys were imported; no SSH/SFTP connection was used for these checks.

An intermittent report described a transparent/blank original Settings split after creating, splitting and closing another tab. Subsequent user attempts with terminal and Settings focus both passed. The cause is unresolved; do not call it reproduced or fixed. The compact-breadcrumb change is implemented but visually pending.

## Future work, not current features

- A folder-first portable workspace that works indefinitely with App and Data together in any selected local Portable Apps folder; USB and split App/Data deployments are optional. Relative-root relocation, workspace IDs independent of USB serial, automatic association/discovery, duplicate copies, unplug checkpoints and conflict-safe reconnection remain design work. Current `.portable` still means adjacent `settings`, not a completed App/Data migration or sync system.
- Native local/SFTP file navigation and transfer is a recommendation for a bounded prototype, not a selected architecture. A TUI is credible; optional native and TUI frontends over a shared backend are a newer proposal. Sharing code is distinct from a common live worker that preserves connections/jobs across view lifetime. WebView2 hosting in this exact XAML Islands host is unproven. Outer tab/split/window placement is independent of internal left/right or stacked browsers. See [file-transfer design](../backslash/FILE-TRANSFER-DESIGN.md).
- A temporary two-machine LAN endpoint, optional tools/updater, portable key authentication, WebView/MCP views and CRT open/close animations are not implemented by this checkout. Current Settings hosting does not supply those engines or a general plugin framework.

Transfers backend-settings checkpoint: Release/x64 `20261028-141920-589-Release-x64-66e9d15d` passed (zero errors/eight retained warnings); complete Settings suite 348 passed/zero failed/two optional skipped. The owned offscreen backend-page and existing SSH scan/import regressions passed. Interactive placement/gear/theme/DPI/accessibility, ARM64/Win32 and real transfer gates remain unrun. See [backend settings and exact evidence](../backslash/TRANSFERS-BACKENDS.md). This remains uncommitted development, outside the published preview.

Transfers native acceptance follow-up passed missing/deleted/transient profile cases, missing-reference clear/replace, actual native Save/disk reload, malformed error-only rendering and preservation during unrelated Save. Application source/binaries remain unchanged from the verified x64 build. Interactive gear/placement/layout/theme QA is blocked by a timed-out Computer Use launch approval; no QA window or screenshot was observed. See [precise follow-up scope](../backslash/TRANSFERS-BACKENDS.md#native-acceptance-follow-up).

## Documentation audience cleanup (2026-10-29)

The README and guide indexes now separate user behavior from contributor references. Removed redundant task/authorization/commit narration from product pages; substantive implementation and validation records remain in their feature documents. Original edited landing/provenance pages and the preexisting dirty-file hashes are retained locally under `artifacts/documentation-audit-20261029-100318`. They are audit evidence, not public download content. No new root-level handoff document is needed.

## Accepted development checkpoint and source publication

On 2026-10-29 the user accepted the delivered 161132 Release/x64 portable build and requested that the accumulated development source be committed and pushed. This includes post-preview.2 warning/resource cleanup, Transfers backend configuration and navigation, Explorer launch/integration/settings, and audience-focused documentation cleanup. Published preview.2 assets and tags remain unchanged. Acceptance does not establish exhaustive native removal/rollback/restart, accessibility, ARM64 or UNC coverage.

Pre-commit checks: resource fallback coverage passed across 48 files/15,790 entries; all 52 mocked host deployment ownership/preservation gates passed; CRLF-aware Git whitespace checking passed. The historical pinned resource-cleanup audit rejects the later added neutral feature strings (first: TerminalApp en-US); its original before/after evidence remains historical, while the ongoing fallback check covers the current resources. The delivered build and isolated management/UI/portable validation above remain the build evidence; no live registration, shell notification or restart was performed for publication. Generated artifacts, captures, profiles and local diagnostic outputs are excluded by Git ignore rules.
