#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$exe = Join-Path $evidence 'ExplorerIntegrationTests.exe'
if (Test-Path -LiteralPath $exe) { throw 'Fresh evidence directory required.' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /utf-8 /W4 /WX /MT (Join-Path $PSScriptRoot 'ExplorerIntegrationTests.cpp') "/Fe:$exe" "/Fo:$evidence\ExplorerIntegrationTests.obj" /link advapi32.lib shell32.lib *> (Join-Path $evidence 'build.log')
if ($LASTEXITCODE) { throw 'Explorer fixture compile failed.' }
& $exe $evidence *> (Join-Path $evidence 'tests.log')
if ($LASTEXITCODE) { throw 'Explorer fixture tests failed.' }
Get-Content (Join-Path $evidence 'tests.log')
