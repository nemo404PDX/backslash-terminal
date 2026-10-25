#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$exe = Join-Path $evidence 'RenderVisibilityTests.exe'
$obj = Join-Path $evidence 'RenderVisibilityTests.obj'
if ((Test-Path -LiteralPath $exe) -or (Test-Path -LiteralPath $obj)) { throw 'Fresh rendering test evidence required.' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
if (-not $vs) { throw 'Existing C++ toolchain required.' }
Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$wil = Join-Path $repo 'packages\Microsoft.Windows.ImplementationLibrary.1.0.250325.1\include'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'RenderVisibilityTests.cpp') "/I$repo\src\renderer\base" "/I$wil" "/Fe:$exe" "/Fo:$obj" *> (Join-Path $evidence 'render-visibility-fixture-build.log')
if ($LASTEXITCODE) { throw 'Render visibility fixture compile failed.' }
& $exe (Join-Path $evidence 'render-diagnostics-fixture.jsonl') *> (Join-Path $evidence 'render-visibility-fixture-tests.log')
if ($LASTEXITCODE) { throw 'Render visibility fixture tests failed.' }
$snapshot = Get-Content (Join-Path $evidence 'render-diagnostics-fixture.jsonl') -Raw | ConvertFrom-Json
if ($snapshot.reason -ne 'fixture' -or $snapshot.outputCharacters -ne 0 -or $snapshot.offscreenBytes -ne 0) { throw 'Diagnostic snapshot roundtrip failed.' }
Get-Content (Join-Path $evidence 'render-visibility-fixture-tests.log')
Write-Output 'PASS: Diagnostic JSON snapshot roundtrip'
