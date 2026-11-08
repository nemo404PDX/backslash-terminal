#Requires -Version 7.0
[CmdletBinding()]
param([string]$VsInstallPath='C:\Program Files\Microsoft Visual Studio\18\Community')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$work=Join-Path $repo ('artifacts\sftp\service-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
Import-Module (Join-Path $VsInstallPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsInstallPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX (Join-Path $PSScriptRoot 'SftpServiceTests.cpp') "/Fe:$work\tests.exe" "/Fo:$work\tests.obj" /link shell32.lib ole32.lib *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log');throw 'SFTP fixture compilation failed'}
& (Join-Path $work 'tests.exe') (Join-Path $work 'owned-files') *> (Join-Path $work 'tests.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'tests.log');throw 'SFTP fixtures failed'}
Get-Content (Join-Path $work 'tests.log')
Write-Output $work
