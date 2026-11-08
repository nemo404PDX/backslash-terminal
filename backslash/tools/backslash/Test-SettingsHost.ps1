#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$VsInstallPath,
    [Parameter(Mandatory)][string]$RuntimeDependenciesDirectory,
    [switch]$Run
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$workspace=Split-Path $repo -Parent
$vs=(Resolve-Path -LiteralPath $VsInstallPath).Path
$dependencies=(Resolve-Path -LiteralPath $RuntimeDependenciesDirectory).Path
$msbuild=Join-Path $vs 'MSBuild/Current/Bin/amd64/MSBuild.exe'
$devshell=Join-Path $vs 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
$git=Join-Path $env:ProgramFiles 'Git/cmd/git.exe'
foreach($file in @($msbuild,$devshell,$git,(Join-Path $dependencies 'Microsoft.UI.Xaml.dll'))){
    if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw "Required existing dependency absent: $file"}
}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run in normal non-admin PowerShell'}
if($env:ForceImportAfterCppTargets){throw 'Existing ForceImportAfterCppTargets preserved; start a normal shell without a custom import'}
Write-Host 'Plan: rebuild Release/x64 Settings test project against current built dependencies; copy DLL-only runtime dependencies; compile synthetic ssh.exe; run four focused classes. No live SSH, real-shell opt-in, registration, elevation or policy changes.'
if(!$Run){Write-Host 'Preflight only; use -Run after review.'; return}
$evidence=Join-Path $workspace ('artifacts/settings-host-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $evidence | Out-Null
Write-Host "Evidence: $evidence"
$result=[ordered]@{Phase='preparing';FreshTestBuild=$false;TestExit=$null;ClassResults=@();Error=$null}
$oldPath=$env:PATH
$oldCapture=$env:BACKSLASH_TEST_SSH_EXE
$oldShell=$env:BACKSLASH_TEST_PIN_REAL_SHELL
try {
    Import-Module $devshell
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
    $env:PATH=(Split-Path $git -Parent)+';'+$env:PATH
    & $git -C $workspace rev-parse HEAD | Set-Content (Join-Path $evidence 'head.txt')
    & $git -C $workspace diff -- backslash/src/cascadia/UnitTests_SettingsModel backslash/src/cascadia/TerminalSettingsModel backslash/src/inc/til/io.h | Set-Content (Join-Path $evidence 'source.patch')
    $sources=@(Get-ChildItem (Join-Path $repo 'src/cascadia/UnitTests_SettingsModel') -File; Get-ChildItem (Join-Path $repo 'src/cascadia/TerminalSettingsModel') -File)
    $sources | Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $evidence 'source-hashes.json')
    $modelLib=Join-Path $repo 'bin/x64/Release/Microsoft.Terminal.Settings.Model.Lib/Microsoft.Terminal.Settings.Model.Lib.lib'
    if(!(Test-Path -LiteralPath $modelLib -PathType Leaf)){throw 'Current full-app model static library missing; build the candidate first'}
    Get-FileHash -LiteralPath $modelLib | Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $evidence 'model-static-library.json')
    $result.Phase='building'
    # Rebuild only this project; current production/static dependencies must already exist.
    & $msbuild (Join-Path $repo 'src/cascadia/UnitTests_SettingsModel/SettingsModel.UnitTests.vcxproj') /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false "/p:SolutionDir=$repo\" /m:1 /nologo /v:normal "/bl:$evidence/settings-build.binlog" *> (Join-Path $evidence 'settings-build.log')
    if($LASTEXITCODE){throw "Fresh test build failed ($LASTEXITCODE); tests were not run"}
    $result.FreshTestBuild=$true
    $built=Join-Path $repo 'bin/x64/Release/UnitTests_SettingsModel'
    $runtime=Join-Path $evidence 'runtime'
    New-Item -ItemType Directory -Path $runtime | Out-Null
    Get-ChildItem -LiteralPath $built -Force | Copy-Item -Destination $runtime -Recurse
    # Copy root DLLs only: no portable settings, keys, state or executable launchers.
    Get-ChildItem -LiteralPath $dependencies -File -Filter '*.dll' | Copy-Item -Destination $runtime -Force
    foreach($component in @('Microsoft.Terminal.Settings.Model','Microsoft.Terminal.Settings.Editor','TerminalApp','Microsoft.Terminal.Control','TerminalConnection')){
        $dll=Join-Path $repo "bin/x64/Release/$component/$component.dll"
        if(!(Test-Path -LiteralPath $dll -PathType Leaf)){throw "Current component absent: $dll"}
        Copy-Item -LiteralPath $dll -Destination $runtime -Force
    }
    [IO.File]::WriteAllText((Join-Path $runtime '.portable'),'')
    $capture=Join-Path $evidence 'ssh.exe'
    & cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'SshCaptureClient.cpp') "/Fe:$capture" "/Fo:$evidence/capture.obj" *> (Join-Path $evidence 'capture-build.log')
    if($LASTEXITCODE){throw 'Synthetic capture compilation failed'}
    $env:BACKSLASH_TEST_SSH_EXE=$capture
    $env:BACKSLASH_TEST_PIN_REAL_SHELL='0'
    Get-ChildItem -LiteralPath $runtime -File -Filter '*.dll' | Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $evidence 'runtime-hashes.json')
    $result.Phase='testing'
    Push-Location -LiteralPath $runtime
    try {
        $result.TestExit=0
        foreach($selection in @('SetupTransferStorageTests','PinnedStoreTests','SerializationTests','BackslashSshSavedLaunchArgv')){
            & './TE.exe' './SettingsModel.Unit.Tests.dll' "/name:*$selection*" /inproc /logOutput:High *> (Join-Path $evidence "$selection.log")
            $code=$LASTEXITCODE
            $result.ClassResults+=@{Selection=$selection;ExitCode=$code}
            if($code){$result.TestExit=$code}
        }
    } finally {Pop-Location}
    if($result.TestExit){throw "Focused tests failed ($($result.TestExit)); preserve evidence"}
    $result.Phase='passed'
} catch {
    $result.Error=$_.Exception.Message
    $result.Phase='failed'
    throw
} finally {
    $env:PATH=$oldPath
    $env:BACKSLASH_TEST_SSH_EXE=$oldCapture
    $env:BACKSLASH_TEST_PIN_REAL_SHELL=$oldShell
    $result | ConvertTo-Json | Set-Content (Join-Path $evidence 'result.json')
    Write-Host "Evidence retained: $evidence"
}
