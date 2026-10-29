# Post-preview.2 warning stabilization

Status: uncommitted development, 2026-10-28. The published preview.2 tag/ZIP remain unchanged. Transfers is restored only in development; there is still no transfer engine. No provider/registration flag, dependency pin, signing/install setting or warning suppression was changed.

## Verified corrections

- Audited all 60 localized-only PRI identities against static literals, XAML UIDs and dynamic enum-key construction. Removed only 93 obsolete localized data elements in 32 files; surviving values/comments and original encoding/newline style are unchanged. No translations were invented. See [per-key audit](RESOURCE-KEY-AUDIT.md).
- Cached folder-tree rows now expose INotifyPropertyChanged and forward mutable Name/Icon changes through a weak, auto-revoked subscription. OneWay bindings remain OneWay. NewTabMenu exposes its existing callback interface, raises WindowRoot on navigation and uses its existing notifying ViewModel setter. No immutable binding shortcut was used.
- ModelLib's resource-bearing static librarian now specifies MachineX64/MachineARM64 instead of inferring X64. The existing Win32 common setting is retained. x64 is built; ARM64 metadata is source-reviewed only, not compiled.

## Before/after evidence

Both application runs use Release/x64, the installed VS 18.10.1 toolchain, SDK 10.0.26100.0 and the normal unsigned packaging target. Baseline is the isolated clean preview.2 build; after is an explicit CascadiaPackage:Rebuild in the development checkout. Dependency caches/paths differ, so totals alone are not a behavioral test.

| Warning | Clean preview.2 | Final full rebuild | Disposition |
| --- | ---: | ---: | --- |
| PRI263 | 120 (60 identities reported twice) | 0 | Obsolete resources removed after reference audit |
| WMC1506 | 6 | 0 | Mutable bindings now have notification paths |
| LNK4068 | 1 | 0 | Explicit librarian target machine |
| WMC1004 | 2 | 2 | Retained: documented manual/delayed XAML provider initialization |
| APPX1707 | 3 | 3 | Retained pending dedicated package/registration review |
| Total | 132 | 5 | No blanket suppression |

The successful earlier incremental run emitted 8 warnings (6 APPX4001 + 2 WMC1004); it did not rerun all packaging metadata targets. That is not the final comparable count and does not establish APPX1707 was fixed. Preparation preview.2 likewise differed from its clean build.

Fresh Settings test-library build: 0 errors / 42 warnings (4 APPX4001, 38 LNK4020). APPX4001 calculates bundle architecture on non-release-package reference targets. LNK4020 concerns ModelLib PDB type records and limits debugger symbols; the same diagnostic/objects exist in earlier tests-build evidence. Neither is suppressed or established fixed. The isolated preview.2 test build's 8 warnings instead comprised 4 APPX4001 + 4 PRI263; these differing test-build states are not a zero-regression claim about symbols.

WMC1004's DoNotGenerateOtherProviders is intentional in TerminalAppLib.vcxproj, and App.cpp explicitly loads Control/WinUI providers and delays Settings Editor. APPX1707 involves WinGet COM, XAML and WebView2 WinMD implementation metadata. Existing targets deliberately reference WebView2 metadata without its runtime because no WebView2 control is used. Adding arbitrary Implementation values or changing provider generation could alter activation/registration; this patch stops before that boundary. No installed-package runtime proof is claimed.

## Fresh validation

- Full application rebuild: 0 errors / 5 warnings, 6:13.24.
- Settings: 346 total; 344 passed, 0 failed, 2 optional real-shell cases skipped. A fresh local network-incapable capture fixture supplied argv checks.
- Resource fallback check: 48 files / 15,778 entries; synthetic valid/missing/duplicate/malformed fixtures pass. Original audit state failed the check with exactly 93 missing-fallback entries. Remaining translations match the published source structurally.
- Owned native production-component probe: cached folder names/icons notify, root row works, discarded rows are not retained, page notification interface exists, and Scan single/two-selection/repeat/empty/error/recovery/duplicate cases pass. The same probe with a copied published Editor DLL fails at the missing row notification interface; rebuilt DLL passes. No unhandled XAML failures in the passing run.
- Review follow-up: the production NewTabMenu now has a focused Frame navigation regression using two distinct synthetic models/host roots. Frame.Navigated installs observers before OnNavigatedTo: eight real A/B entries each raise ViewModel and WindowRoot exactly once. The actual ListView ItemsSource and rendered rows switch to B, mutations to A values/collection do not alter B, B still updates live, and three repeated cycles plus detached teardown pass. This tests ordinary navigation and observer teardown, not same-instance cached-page reuse or guaranteed immediate collection of all XAML pages. Earlier cache-assumption fixture runs failed and are retained separately; there was no product defect established by those failures.
- Reproducible structural audit: run `python backslash/tools/backslash/Test-ResourceCleanupAudit.py` from the repository root. It reads raw UTF-8 Git bytes at preview.2 and compares parsed Unicode XML structures including comments, avoiding console decoding and XML reserialization. All 48 files pass: exactly 93 removed data elements across 32 files / 60 identities; surviving values/comments and non-data XML unchanged.
- Portable: 34 isolation checks, 261 manifest files, 244 payload files, 25 images / 585 imports, no missing dependencies or user state.
- Repository layout, appearance-content and startup-picker checks pass. Native DPI/theme/accessibility and actual cross-window navigation/interaction remain pending; the page-interface probe does not prove every navigation scenario. ARM64/Win32 builds were not run.

Final build evidence: `artifacts/build/20261028-132801-001-Release-x64-ebd627a6` (build.log/binlog, packages, source inventory and final-portable-verification.json). Initial native evidence: `artifacts/ssh-scan-ui/20261028-133444-730-5656c094`. Expanded navigation/native evidence: `artifacts/ssh-scan-ui/20261028-135341-141-39f9e789`. Supporting logs and scripts are in task-local `warning-verification`; they include warning-counts.json, native-before.log, native.log and isolated-settings-results.log.

Two earlier failed application builds exposed a duplicate notification declaration and a CRLF-sensitive attempted removal; corrected before the final full rebuild. One initial native fixture compilation used the wrong public constructor overload; corrected before native execution. These failure logs are retained, not counted as passing checks. No real SSH connection, user configuration mutation, GUI automation, installation, commit or push occurred.
