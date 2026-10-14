#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory)
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
if (-not $evidence.StartsWith("$repo\artifacts\build\", [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture evidence must be in this checkout.' }
$exe = Join-Path $evidence 'ssh.exe'
if (Test-Path -LiteralPath $exe) { throw 'Fresh fixture executable required.' }
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'SshCaptureClient.cpp') "/Fe:$exe" "/Fo:$evidence\SshCaptureClient.obj" *> (Join-Path $evidence 'ssh-capture-build.log')
if ($LASTEXITCODE) { throw 'Local SSH capture fixture build failed.' }
$identity = & $exe --fixture-identity
if ($identity -ne 'BACKSLASH LOCAL SSH ARGUMENT CAPTURE FIXTURE') { throw 'Fixture identity mismatch.' }
Write-Output $exe
