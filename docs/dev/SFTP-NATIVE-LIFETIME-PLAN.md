# Single-connection fixture publication lifetime correction

Offline candidate; no live keys, ACL/security or SSH/server operations. Fresh root artifacts/sftp/native-lifetime-20261031-2c6d94ea remains absent. All prior failure roots untouched; artwork and independent startup work preserved.

## Verification correction

NativeLiveLifetime.cpp replaces premature ReadFixture of published files with FixtureDownloadVerifier. It stores only expected fixture bytes (cap65536) and a received count, compares each observed downloaded chunk, rejects mismatch/overflow/incomplete or unstarted verification, and fails closed on invalid Begin. A new Begin clears previous authorization. Marker Published is called only after DownloadOne returned through production Publish; RequirePublished is enforced before upload preparation/remote writes. The callback observes bytes AFTER production local Write, never changes handles/protocol. Macro BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS adds this optional observer only in fixture compilation; ordinary application builds do not expose/execute it. No product sharing/protocol changes.

Payload verification uses another bounded verifier. After both transfers the entire NativeSession scope ends, releasing exclusive publication/source handles and ancestor locks. Only then reopen marker/download files and compare exact bytes; compare remote uploaded bytes in owned local fixture too. Outer PowerShell retains independent expected SHA256 comparisons after batch exit before adopting any file for cleanup. New explicit STAGE_MARKER_VERIFIED, STAGE_SESSION_RELEASED and STAGE_DISK_INTEGRITY_VERIFIED plus result MarkerVerified/DiskIntegrityConfirmed improve truthful reporting. On failure, no PASS and unchanged/unknown artifacts remain guarded.

Audit: marker proof precedes upload Prepare (which only stats/generates temp path until Transfer); upload source handle is read-share and released before final disk checks; second download replaces prior temporary handle safely but fixture does not rely on that early unlock; captured observer remains alive through session scope; no callbacks after scope use; source bytes fixed65536, observer data cap65536, marker fixed-length; incomplete/mismatched checks do not authorize write; cleanup only exact known expected hashes after handles end. Remote partials/unknown files remain evidence on failure. No extra connection, listener, key generation or time budget.

All prior absolute signed helper, StrictModes/auth/current-owner authorized_keys/keys, no agent/multiplexing, fixed shell alphabet/effective config, denied port guard, single loopback48223 listener,115s server/120s batch job, capped diagnostics and checked teardown remain. No user-profile/global PATH/services/firewall/account/security changes.

## Offline tests

Strict /W4 /WX batch and LifetimeTests.cpp builds passed. Tests use one actual production NativeSession against the existing owned fake SFTP child for browse -> marker download -> upload -> payload download in one connection. They verify ERROR_SHARING_VIOLATION while publication owner remains, exact bytes readable after scope release, marker mismatch forbids simulated upload, segmented positive bytes, incomplete/overflow/unstarted/oversize rejection. No real network/server. Initial and final test logs retained in artifacts/sftp/native-lifetime-build-20261031-2c6d94ea; final test-final.log passed. Test-NativeLifetimeOffline.ps1 passed prior ACL/source ordering and effective-config/shell negative cases plus marker-before-write and disk-after-release ordering. Default script mode only compiled/in-memory checks; root absent. No downloaded code executed. Actual live corrected batch remains UNRUN; no request for another user run until independent review.

## Candidate hashes and user command AFTER review/consent

- Invoke-NativeLifetime.ps1 CF20347B80B5D1BA4E00A0E51162EDDD914483BF78F665A080E5AFA3122443DD
- native-lifetime-sshd_config6BD5022C78C6D15A913C0EAF62360775642FDF6B51C1E0D03223780E65F6381D
- batch.exe FE05E7FB72E2050A929D03186C1A94D97D8F9BF99E390D41D2E2055AE2FB47F1 at artifacts/sftp/native-lifetime-build-20261031-2c6d94ea

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\sftp-fixture\Invoke-NativeLifetime.ps1' -RunFixture
```

Ordinary non-admin user only; no policy bypass/elevation. Same API/ownership guards fail closed. Result in fresh root/evidence/preflight-result.json; server/client logs bounded64KiB each. No additional ACL scope or privileges. Single test still does not cover trust rejection, cancel/retry/reconnect/interruption, real service/profile/UI/whole-app lifecycle or release gates.

## Distinct product scope: Startup breadcrumb

Parent supplied independent live-layout reproduction/evidence under artifacts/startup-title-review/prepared-e2923a3d: clearing BreadcrumbBar Width aligned Startup/Interaction headers in minimal/compact/expanded/resizing states. Applied ONLY removal of Width binding to SettingsBreadcrumbHost.ActualWidth in TerminalSettingsEditor/MainPage.xaml113. HorizontalAlignment Stretch, MinWidth, host16-DIP margins and48-DIP chrome unchanged. Product XML parse and exact one-line diff verified; this pass does not claim independent new GUI pixel acceptance. Full Release/x64 build started for final review; record final outcome separately. No startup behavior, profiles or installed integration changes.

No commit/tag/package/push/release actions. User artwork remains intentional release content and must be reviewed/included later.

Full app build completed with failure: `artifacts/build/20261030-181313-810-Release-x64-990d5e76`, C1902 program database manager mismatch in five TerminalAppLib sources plus enclosing task error (6warnings/6errors). Settings.Editor linked; no successful final app candidate. No toolchain/PDB/process cleanup attempted. Normal no-fixture process/native regressions passed at the manifest paths. This fixture's strict build/fake-child tests are independent of that app toolchain failure.

