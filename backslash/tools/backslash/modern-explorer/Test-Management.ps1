#Requires -Version 7.0
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..'))
$work=Join-Path $repo ('artifacts\modern-explorer\management-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $work|Out-Null
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /utf-8 /W4 /WX /MT (Join-Path $PSScriptRoot 'ManagementTests.cpp') "/Fe:$work\ManagementTests.exe" "/Fo:$work\ManagementTests.obj" /link advapi32.lib shell32.lib ole32.lib user32.lib shlwapi.lib windowsapp.lib *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log');throw "Management compile failed: $work"}
& (Join-Path $work 'ManagementTests.exe') (Join-Path $work 'owned-files') (Join-Path $repo 'artifacts\modern-explorer\20261029-104430-748-26cadc60\layout') *> (Join-Path $work 'tests.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'tests.log');throw "Management fixtures failed: $work"}
Get-Content (Join-Path $work 'tests.log')
Write-Output $work
