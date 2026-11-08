# Settings aggregate diagnosis (2026-10-31)

Evidence root: artifacts/preview3-independent-20261031-73e5b6c2. Original settings-results.log preserved:350 total,324passed,24failed,2optional real-shell skipped. Do not turn this into a passing aggregate by subtracting failures.

## Failure categories

| Count | Category | Evidence/disposition |
|---|---|---|
|10|SetupTransferStorage exceptions|80070005 at SetupTransferStorage.h98, MoveFileExW replacing the fixture file. Root() performs this setup before test-specific behavior. Does not establish ten independent transaction regressions.|
|10|PinnedStore exceptions|80070005 at til/io.h262, filesystem rename after temporary write. Failures precede completion of intended assertions.|
|1|PinnedStore concurrent assertion|ConcurrentWindowAddsAreAdditive reports nonempty collected exception_ptr vector. Test deliberately catches worker exceptions without logging individual causes. Access-denied relation plausible but not proven for this assertion.|
|2|Serialization reload assertions|BackslashGroupSaveNoticeState and BackslashProfilesSidebarState reload default values after Flush. ApplicationState::_write catches/logs write errors; same atomic-write path is used. Original low-verbosity log does not prove these two failed for access denial. No blanket environment classification.|
|1|Saved SSH launch fixture|Runner mistakenly named local capture executable ssh-capture.exe. SshCommandline requires basename ssh.exe or its explicit placeholder. Historical passing runner uses ssh.exe. Corrected owned copy, same bytes, focused production test passed1/1, exit0; profile-focused.log. No product relaxation.|

Twenty-three original failures remain unresolved as an aggregate gate. No retry of denied rename operations, ACL changes, security weakening or product workarounds were performed. User artwork untouched.

## Cached library / build qualification

Test DLL SHA1646001A3461C47C50EBB5117B78F80482695CEFA4D987066E4517420ADEB438; current external model SHA7323CF38A607FD64EBCADE5E2E066EE1B39481771751F03C2EB3887DB55D7AC3. Test/model source and til/io.h have no diff from HEAD. Historical 20261028 transfers-verification records348pass/2skip, but is not current acceptance. UnitTests project statically links model implementation as well as using projections/external DLLs; substituting current runtime DLLs does NOT rebuild embedded test/model code. Cached source compatibility and successful execution are qualified evidence, not clean current binary provenance.

First fresh compile attempt encountered MSYS Git NtCreateDirectoryObject OS failure. Native-Git retry stalled before compiler output and was interrupted only through its owned command session. Log contains only branch master. Partial replay reached _CheckForInvalidConfigurationAndPlatform under OCCallFeatureFlagGenerator but could not finish because installed desktop MSBuild assembly requires .NET Framework RemotingServices, unavailable in PS7 runtime. A read-only Windows PowerShell binlog-reader script was refused with AuthorizationManager check failed / UnauthorizedAccess; not retried with execution-policy bypass or another execution context. Exact build-stall cause remains unproven; no C1902 was recorded in this attempt. Do not call fresh test compilation successful.

## Next bounded gate

Obtain a fresh test build using the normal supported build environment and rerun the failing classes with normal-user host evidence, or independently diagnose the test filesystem boundary using supported access. Manual test execution must preserve synthetic-only capture and disable optional real-shell tests. Capture verbose write failures for the serialization cases and worker exception causes before changing production code. No live SSH, user settings, Explorer mutation or broader permissions are needed by intended tests. No automatic retry authorized by this note.

## Reviewed normal-host handoff (prepared, not executed)

Use normal non-admin PowerShell7 after source/helper review. Requires the existing current full-app Release/x64 dependencies; this rebuilds the test project, not every static dependency. It records the linked model static library hash and current test/model source hashes, plus git HEAD/diff and runtime DLL hashes. Do not describe it as a clean build of the whole application. Reconcile any later model/source edits with a current full-app build first.

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\Test-SettingsHost.ps1' -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -RuntimeDependenciesDirectory 'D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261029-091419-230-Release-x64-e979d558'
```

Default is preflight only. After reviewing its plan, use the same command with `-Run`. No execution-policy bypass, administrator shell or permission adjustment. If normal host refuses the script or build stalls, stop and provide the exact error/evidence path; do not weaken guards. Run from a fresh shell: VS environment is scoped to this helper process. A custom ForceImportAfterCppTargets is refused.

`-Run` rebuilds only SettingsModel.UnitTests (Rebuild, BuildProjectReferences=false, Release/x64, MSBuild /m:1) in normal existing ignored build outputs. It uses native Windows Git, logs normal-verbosity build/binlog, then copies fresh test outputs into a uniquely owned artifacts/settings-host-<guid>/runtime. The specified old portable directory supplies ONLY framework/root DLL dependencies; current built Model/Editor/App/Control/Connection DLLs replace product copies, and portable settings/state/keys/executable launchers are never copied. It compiles the known synthetic capture client as ssh.exe and disables optional real-shell tests. Four TAEF selections run in separate processes: SetupTransferStorageTests, PinnedStoreTests, SerializationTests and BackslashSshSavedLaunchArgv. Their temporary file writes are synthetic fixtures; no live SSH or personal settings are intended. No listener, keys, ACL/security, registry or Explorer changes. Preserve any OS denial and do not substitute an access workaround.

Results: printed evidence directory contains result.json with per-selection exit codes, settings-build.log/binlog, four named high-verbosity test logs, source.patch/source-hashes.json/model-static-library.json/runtime-hashes.json and capture-build.log. A failing build prevents tests; failures preserve all evidence. There is no automatic cleanup or process-tree kill. Ctrl+C can stop the owned command if progress stalls; report uncertain child state before rerunning.

Test-only change: ConcurrentWindowAddsAreAdditive now logs each already-caught worker exception after joining all workers, before its unchanged failures.empty assertion. No production handling is changed. Source parser/ordering checks passed; fresh compile and normal-host execution of this new logging remain pending. Serialization tests already log owned temp paths; high-verbosity logs plus retained state/temp files should distinguish persist/reload failure. Do not infer the caught write error without evidence.

## Host build error corrected (2026-11-03)

User-run settings-host-bd36e5a4d6a74376aa08d3556e3999a0 reached actual current compilation (30.76s), proving this attempt did not reproduce the earlier executor build stall. It stopped with0warnings/6errors: missing Log namespace in three newly added worker diagnostic calls. Corrected all three to WEX::Logging::Log::Comment. Real compiler /W4 /WX syntax check of full corrected PinnedStoreTests.cpp passed using recorded host include/define flags, /Y- and /Zs to avoid shared PCH/PDB/output mutations. Evidence artifacts/settings-logging-fix-f12d6007640748bbba2e35ef6bb28d27/syntax.log and syntax.rsp. This is syntax acceptance, not a successful linked fresh test library or test result. Original failures still unresolved; rerun the same Test-SettingsHost.ps1 -Run normal-host command once after review, obtaining a fresh unique evidence directory. Successful SFTP Lifetime run is separate and must not be repeated.

## Fresh normal-host gate passed

Evidence artifacts/settings-host-63d7f0a063244e19b3f88f8fc1e0c77b: FreshTestBuild true; build0warnings/0errors; all four selections exit0, Error null. SetupTransferStorage11/11, PinnedStore12/12, Serialization79/79 and BackslashSshSavedLaunchArgv1/1:103passed/0failed/0skipped. Inspected actual logs, not merely wrapper return. All previously failing classes now pass on the normal host with fresh test compilation and the qualified prebuilt current model static library recorded by hash. This closes their blocking host reproduction gate. It is not a fresh350-test aggregate or proof of the precise executor-denial mechanism. Original failed logs remain preserved, no permission/security/product relaxation was used. Test-only namespace correction is included. Broader interactive product acceptance is separate.
