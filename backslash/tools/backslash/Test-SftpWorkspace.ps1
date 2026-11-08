#Requires -Version 7.0
[CmdletBinding()]
param([string]$PortableDirectory)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
if(!$PortableDirectory){$PortableDirectory=Join-Path $repo 'artifacts\portable\Backslash-Dev-20261029-161132-494-Release-x64-b3e4c333'}
$portable=(Resolve-Path -LiteralPath $PortableDirectory).Path
$work=Join-Path $repo ('artifacts\sftp\workspace-'+[guid]::NewGuid().ToString('N'));$fixture=Join-Path $work 'portable';New-Item -ItemType Directory -Path $fixture | Out-Null
Get-ChildItem -LiteralPath $portable -Force|Where-Object Name -NE 'settings'|Copy-Item -Destination $fixture -Recurse
Copy-Item -LiteralPath (Join-Path $repo 'backslash\bin\x64\Release\TerminalApp\TerminalApp.dll') -Destination (Join-Path $fixture 'TerminalApp.dll')
$markup=Get-Content -Raw -LiteralPath (Join-Path $repo 'backslash\src\cascadia\TerminalApp\App.xaml')
$template=[regex]::Match($markup,'(?s)<DataTemplate x:Key="TransfersViewTemplate">(.*?)</DataTemplate>').Groups[1].Value
if(!$template){throw 'Current Transfers template not found'}
('<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" xmlns:muxc="using:Microsoft.UI.Xaml.Controls">'+$template+'</DataTemplate>')|Set-Content (Join-Path $work 'template.xaml')
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll';Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$exe=Join-Path $fixture 'Backslash.exe'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT "/I$repo\backslash\src\cascadia\TerminalApp\Generated Files" (Join-Path $PSScriptRoot 'SftpWorkspaceProbe.cpp') "/Fe:$exe" "/Fo:$work\probe.obj" /link windowsapp.lib shell32.lib ole32.lib user32.lib windowscodecs.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\backslash\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log');throw 'Workspace probe compilation failed'}
& $exe $work *> (Join-Path $work 'probe.log')
$probeExit=$LASTEXITCODE
Write-Output "Workspace probe exit: $probeExit; evidence: $work"
if($probeExit -or !(Test-Path -LiteralPath (Join-Path $work 'workspace-wide.png'))){Get-Content (Join-Path $work 'probe.log');throw 'Workspace probe failed'}
Get-Content (Join-Path $work 'probe.log');Write-Output $work
