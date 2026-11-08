#Requires -Version 7.0
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'));$work=Join-Path $repo ('artifacts\sftp\host-shutdown-'+[guid]::NewGuid().ToString('N'));New-Item -ItemType Directory $work | Out-Null
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll';Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$dll=Join-Path $work 'OwnerFixture.dll';$exe=Join-Path $work 'HostTests.exe'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT /LD (Join-Path $PSScriptRoot 'SftpShutdownOwnerFixture.cpp') "/Fe:$dll" "/Fo:$work\owner.obj" /link shell32.lib ole32.lib *> (Join-Path $work 'owner-build.log');if($LASTEXITCODE){Get-Content (Join-Path $work 'owner-build.log');throw 'Owner fixture build failed'}
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'SftpHostShutdownTests.cpp') "/Fe:$exe" "/Fo:$work\host.obj" *> (Join-Path $work 'host-build.log');if($LASTEXITCODE){Get-Content (Join-Path $work 'host-build.log');throw 'Host fixture build failed'}
& $exe $work $dll *> (Join-Path $work 'tests.log');if($LASTEXITCODE){Get-Content (Join-Path $work 'tests.log');throw 'Host shutdown tests failed'}
$hostSource=Get-Content -Raw -LiteralPath (Join-Path $repo 'backslash\src\cascadia\WindowsTerminal\WindowEmperor.cpp')
if(([regex]::Matches($hostSource,'BeforeOwnerTermination\(')).Count -ne 1 -or $hostSource.LastIndexOf('TerminateProcess(GetCurrentProcess()') -lt $hostSource.IndexOf('BeforeOwnerTermination(')){throw 'Owner termination barrier placement regressed'}
if($hostSource -match '#include[^\r\n]*SftpNativeSession|SharedService\('){throw 'Executable must not instantiate a second transfer owner'}
if($hostSource -notmatch 'transferLogic\.BeginTransferShutdown\(' -or $hostSource -notmatch 'transferLogic\.TransferShutdownComplete\(' -or $hostSource -notmatch 'Shutdown::ShouldPostQuit\('){throw 'Production DLL drain/headless policy wiring missing'}
'PASS production source: one owner barrier precedes final forced exit; projected DLL ownership; shared multiwindow/headless policy.' | Add-Content (Join-Path $work 'tests.log')
Get-Content (Join-Path $work 'tests.log');$work
