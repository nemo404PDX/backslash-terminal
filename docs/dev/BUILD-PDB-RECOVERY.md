# Local C1902 build recovery — 2026-10-31

The standard Release/x64 build failed at TerminalAppLib pch.h inclusion with C1902 Program database manager mismatch. This is tool/debug-database evidence, not a source syntax failure. Microsoft reference: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1902?view=msvc-170 .

## Evidence and limits

Actual build command uses installed MSVC14.51.36231/HostX64/x64/CL.exe. cl file version19.51.36257.0; mspdb140/core/srv and other checked PDB components all14.51.36257.0. No missing/mismatched installed file version was found. This does not prove which DLL a failing worker loaded; no privileged loader/process trace was used. Fresh isolated serial /Zi PCH creation/reuse passed; /MP2 probe stalled and its owned tool session was interrupted, without killing shared processes. The subsequent /FS probe step was not reached.

Preserved exact ignored generated TerminalAppLib pch.pch and TerminalAppLib.pdb into artifacts/build/pdb-diagnosis-20261031-b46a2c1f/quarantine, with absolute containment/reparse/ignore checks and hash inventory; no recursive move/delete, user or release files. Normal rebuild regenerated the pair but still failed C1902 (48errors). Therefore stale cached files alone were NOT the cause. No system installs/repairs, PATH/global registry/environment settings, warnings suppression or security changes.

Build-local supported ForceImportAfterCppTargets hook (Microsoft.Cpp.Current.targets148) imports SerialTerminalApp.targets, SHA973F4A5A207F61144F9248D8914C3E384799DF89818CC4FA062CF95FE92C7218. It changes ONLY TerminalAppLib ClCompile.MultiProcessorCompilation=false, including generated items, before ClCompile. /Zi, /W3, /WX, optimization, code and security flags remain. The original validated copy lives in ignored artifacts; the opt-in tracked copy/helper described below now makes the same bytes reproducible. Ordinary repository build defaults remain unchanged. Environment property was scoped to the tool process, not persisted to user/system. Standard /MP path is still unresolved and must not be called fixed globally.

First serial buildb8ed7803 (see exact table) passed0errors but had50LNK4020 debug type-record warnings. Preserved its current generated pair in post-parallel-pair inventory, then regenerated BOTH entirely under serial mode and rebuilt normally. Final build again0errors/58warnings. Thus debug PDB quality limitation persists even with fresh serial pair; do not claim corruption solved or attribute it only to old cached data. Stop the rebuild loop; any deeper installed-tool/PDB repair requires a separately justified plan, not disabling warnings/debug checks. Runtime image validation and debug-symbol quality are distinct.

| Run | Result |
|---|---|
| artifacts/build/20261030-181313-810-Release-x64-990d5e76 |6warnings/6errors, initial C1902|
| artifacts/build/20261030-181838-068-Release-x64-46808d44 |6warnings/48errors after cache regeneration; C1902 remains|
| artifacts/build/20261030-182237-004-Release-x64-b8ed7803 |0errors/58warnings, serial override|
| artifacts/build/20261030-182726-584-Release-x64-03cdeaff |0errors/58warnings, fresh serial pair, FINAL raw build|

Final warnings:50 LNK4020 (some debugger symbols/types may be inaccessible),6 APPX4001 and2 WMC1004. NuGet restore separately reported NU1900 when vulnerability-feed metadata could not be reached; auditing was not disabled. Build log verifies override message once, no /MP on TerminalAppLib command, /Zi and warning flags retained, zero C1902. Exact internal parallel/PDB failure mechanism remains unproven; no invented sandbox/upgrade/corruption root cause.

## Deliverables/validation

Final build-validation.json in final run holds current executable/DLL/source/artwork hashes. Raw build outputs at backslash/bin/x64/Release (Backslash.exe under WindowsTerminal, TerminalApp.dll under TerminalApp, Settings.Editor/Model DLLs in their component directories). No new portable ZIP/release assembly was done. Current editor includes exact one-line Startup breadcrumb width-binding removal; no margins/stretch/chrome change. Independent runtime layout evidence belongs to artifacts/startup-title-review/prepared-e2923a3d, not a new interactive test in this recovery.

Existing current-code offline lifetime sequence and marker-gate tests passed; normal process/native fake-child regressions passed at artifacts/sftp/process-0d311aeb32354516a3a7223da0ccbc3b and native-001eee527cbd4504bbe646edbeb0e8c7. No live SSH/server/keys/ACL or app/profile/Explorer registration operations ran. Full Settings/interactive whole-app acceptance and lifetime user live result remain separate gates. No ARM/Win32 claim. Artwork hashes unchanged from recovery start; all user changes preserved. No commit/tag/push or Release publication.

## Fresh-checkout reproduction (explicit opt-in)

The reviewed override is now repository-owned at backslash/tools/backslash/build/SerialTerminalApp.targets; no ignored artifact is required. Exact SHA256:973F4A5A207F61144F9248D8914C3E384799DF89818CC4FA062CF95FE92C7218. Its adjacent .gitattributes marks ONLY this file -text so Git preserves the reviewed bytes instead of rewriting line endings. Source content:

```xml
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup Condition="'$(MSBuildProjectName)'=='TerminalAppLib'">
    <ClCompile><MultiProcessorCompilation>false</MultiProcessorCompilation></ClCompile>
  </ItemDefinitionGroup>
  <Target Name="BackslashDiagnosticSerialTerminalApp" BeforeTargets="ClCompile" Condition="'$(MSBuildProjectName)'=='TerminalAppLib'">
    <ItemGroup><ClCompile Update="@(ClCompile)"><MultiProcessorCompilation>false</MultiProcessorCompilation></ClCompile></ItemGroup>
    <Message Importance="High" Text="Diagnostic build-local override: TerminalAppLib compiler /MP disabled; debug/warnings unchanged." />
  </Target>
</Project>
```

After a fresh checkout with normal repository submodules/dependencies and installed VS prerequisites:

```powershell
Get-FileHash -LiteralPath '.\backslash\tools\backslash\build\SerialTerminalApp.targets'
& '.\backslash\tools\backslash\Build-SerialTerminalApp.ps1' -Preflight
& '.\backslash\tools\backslash\Build-SerialTerminalApp.ps1'
```

This wrapper supports the validated Release/x64 mode only. MaxCpuCount and VsInstallPath are explicit optional parameters. Ordinary Build.ps1 remains unchanged. The wrapper starts a separate PowerShell7 process using structured ArgumentList, sets ForceImportAfterCppTargets ONLY in that child environment and waits/propagates failures. Parent environment is never changed, so no restoration is needed. A different existing import is refused and preserved rather than overwritten or combined silently. Preflight does not restore/build/register anything. Wrapper preflight, unchanged-parent check, conflict-refusal and parser/XML checks passed. No complete additional build was run merely to retest the wrapper: the exact same target hash already produced the recorded builds.

If invoking the underlying build manually instead, preserve and restore the process variable explicitly:

```powershell
$priorImport = [Environment]::GetEnvironmentVariable('ForceImportAfterCppTargets', 'Process')
try {
    if ($priorImport) { throw 'Existing import preserved; use the reviewed wrapper or reconcile it first.' }
    $env:ForceImportAfterCppTargets = (Resolve-Path '.\backslash\tools\backslash\build\SerialTerminalApp.targets').Path
    & '.\backslash\tools\backslash\Build.ps1' -Configuration Release -Platform x64 -MaxCpuCount 2
} finally {
    if ($null -eq $priorImport) { Remove-Item Env:ForceImportAfterCppTargets -ErrorAction SilentlyContinue }
    else { $env:ForceImportAfterCppTargets = $priorImport }
}
```

Do not recreate the target via an editor and assume a matching byte hash; copy the tracked file or verify/review changed bytes. Do not set machine/user environment variables or install tools. The standard parallel build remains unresolved. No byte-identical application-build promise is made. The local override recovers compilation but does NOT resolve the50 LNK4020 unreliable debug type/symbol records. These warnings neither prove runtime corruption nor establish harmlessness; release review must accept/disclose or resolve the limitation explicitly. No tool/PDB error was suppressed. Quarantined generated pairs remain recoverable and ignored; no cleanup is implied.

## Additional Settings/resource gate (2026-10-31)

Resource fallback self-test/audit passed48files/15790entries. The owned Settings aggregate completed350tests:324passed,24failed,2optional real-shell skipped. Hash-verified cached test DLL was used only after confirming test/model source unchanged; current production Model/Editor/App DLLs replaced runtime copies. Fresh test-library build did not complete (MSYS Git runtime failure first; native Git retry stalled and owned build was interrupted). This is not a clean rebuild or passing aggregate. Recorded failures include80070005 AccessDenied during file replacement and separate saved-profile/serialization assertions; root cause of all failures is unresolved. No permission changes/retry workaround were applied. Evidence: artifacts/preview3-independent-20261031-73e5b6c2/settings-results.log and runner/build logs. No TE/cl/MSBuild process was visible in the bounded post-run process query; no shared process was killed.

Exact-tag clean build: the opt-in wrapper now also forwards explicit -Rebuild to existing Build.ps1, selecting CascadiaPackage:Rebuild. Use `Build-SerialTerminalApp.ps1 -Rebuild` from the clean tagged checkout after acceptance. No default/global flag changes; reviewed target bytes/hash unchanged. This is a regeneration of build outputs, not a byte-identical guarantee or promise of valid debug records. Retain any repeated LNK4020 in final evidence; do not hide it. If the rebuild fails, stop release assembly and diagnose rather than reuse a dirty-tree package.

Fresh-checkout fixture consistency: sftp-fixture/.gitattributes preserves exact bytes (-text) because reviewed scripts pin helper/config hashes. This prevents core.autocrlf from invalidating the accepted fixture contract. The dedicated serial targets already have their own narrow -text attribute. Pinned fixture trailing blank lines are retained intentionally; the staged whitespace audit reports them rather than rewriting reviewed hashes. No broad formatter or repository-wide line-ending change.

## Settings Editor serial compilation during UI review

A later build reproduced C1902 in Settings Editor's MainPage, Launch and generated settings index compilation. The build-local composite `artifacts/transfers-standalone-review-20261104/SerialUi.targets` also serializes that project; its SHA256 in this checkout is `4B2EAE482775573220073653EFC5CE72B985F0877C0A4256A7636647AEEAD703`. The pinned TerminalApp target remains unchanged. This explicit option is not a default or toolchain repair. The final clean exact-tag build must use the reviewed option if ordinary compilation still fails, rather than reuse a dirty-tree package.

Recreate the composite from repository root in PowerShell 7. Its absolute import changes the hash when the checkout moves. No warning/debug flags are removed:

```powershell
$repo = (Get-Location).Path
$base = Join-Path $repo 'backslash/tools/backslash/build/SerialTerminalApp.targets'
if ((Get-FileHash -LiteralPath $base).Hash -ne '973F4A5A207F61144F9248D8914C3E384799DF89818CC4FA062CF95FE92C7218') { throw 'Review the changed base override first' }
$target = Join-Path $repo 'artifacts/SerialUi.targets'
if (Test-Path -LiteralPath $target) { throw 'Preserve and inspect the existing target first' }
$xml = @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<Import Project="@IMPORT@" />
<Target Name="SerialSettingsEditor" BeforeTargets="ClCompile" Condition="'$(MSBuildProjectName)'=='Microsoft.Terminal.Settings.Editor'">
<ItemGroup><ClCompile Update="@(ClCompile)"><MultiProcessorCompilation>false</MultiProcessorCompilation></ClCompile></ItemGroup>
<Message Importance="High" Text="Build-local serial Settings Editor compiler; debug and warning settings unchanged." />
</Target></Project>
'@
$xml = $xml.Replace('@IMPORT@', [Security.SecurityElement]::Escape($base)).Replace("`r`n", "`n")
[IO.File]::WriteAllText($target, $xml, [Text.UTF8Encoding]::new($false))
$priorImport = [Environment]::GetEnvironmentVariable('ForceImportAfterCppTargets', 'Process')
try {
    if ($priorImport) { throw 'Existing import preserved; reconcile it before proceeding' }
    $env:ForceImportAfterCppTargets = $target
    & (Join-Path $PSHOME 'pwsh.exe') -NoProfile -File (Join-Path $repo 'backslash/tools/backslash/Build.ps1') -Configuration Release -Platform x64 -MaxCpuCount 1 -Rebuild
    if ($LASTEXITCODE) { throw 'Build failed; stop release assembly' }
} finally {
    if ($null -eq $priorImport) { Remove-Item Env:ForceImportAfterCppTargets -ErrorAction SilentlyContinue }
    else { $env:ForceImportAfterCppTargets = $priorImport }
}
```

The process import is scoped and restored; no user/machine environment, installation, service or security setting changes. Review build `20261103-174127-110-Release-x64-85eee580` passed with 0 errors/8 incremental warnings. Earlier 50 LNK4020 records remain unresolved; eight incremental warnings do not certify repaired symbols. Final exact-tag rebuild and explicit symbol-warning disposition remain release gates.
