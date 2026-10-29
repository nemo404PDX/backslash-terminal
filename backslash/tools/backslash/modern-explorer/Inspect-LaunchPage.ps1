#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableDirectory)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..'))
$portable=(Resolve-Path -LiteralPath $PortableDirectory).Path
$work=Join-Path $repo ('artifacts\modern-explorer\layout-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
$fixture=Join-Path $work 'portable';New-Item -ItemType Directory -Path $fixture|Out-Null
# Only an owned runtime clone. Never copy settings or invoke a real terminal.
Get-ChildItem -LiteralPath $portable -Force|Where-Object Name -NE 'settings'|Copy-Item -Destination $fixture -Recurse
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$exe=Join-Path $fixture 'Backslash.exe' # Owned probe replaces only the cloned executable, never the source portable.
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT "/I$repo\backslash\src\cascadia\TerminalApp\Generated Files" (Join-Path $PSScriptRoot 'LaunchPageProbe.cpp') "/Fe:$exe" "/Fo:$work\LaunchPageProbe.obj" /link windowsapp.lib user32.lib windowscodecs.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\backslash\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log');throw 'Layout probe compile failed'}
& $exe $work *> (Join-Path $work 'probe.log')
if($LASTEXITCODE -or !(Test-Path -LiteralPath (Join-Path $work 'launch-page.png'))){Get-Content (Join-Path $work 'probe.log');throw 'Layout probe failed'}
Get-Content (Join-Path $work 'probe.log');Write-Output $work
