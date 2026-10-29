#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$TargetExecutable)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$target=(Resolve-Path -LiteralPath $TargetExecutable).Path
$work=Join-Path (Split-Path $repo -Parent) ('artifacts\modern-explorer\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
$layout=Join-Path $work 'layout'
New-Item -ItemType Directory -Path $layout,(Join-Path $layout 'Assets'),(Join-Path $work 'identity') | Out-Null
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$libraries=@('advapi32.lib','shell32.lib','shlwapi.lib','ole32.lib','user32.lib','uuid.lib')
$common=@('/nologo','/EHsc','/std:c++20','/utf-8','/W4','/WX','/MT')
& cl.exe @common (Join-Path $PSScriptRoot 'ExplorerHost.cpp') "/Fe:$layout\Backslash.ExplorerHost.exe" "/Fo:$work\ExplorerHost.obj" /link @libraries /MANIFEST:EMBED "/MANIFESTINPUT:$PSScriptRoot\Host.manifest" *> (Join-Path $work 'host-build.log')
if($LASTEXITCODE){throw "Host compile failed: $work"}
& cl.exe @common /LD (Join-Path $PSScriptRoot 'ExplorerCommand.cpp') "/Fe:$layout\Backslash.ExplorerCommand.dll" "/Fo:$work\ExplorerCommand.obj" /link @libraries "/DEF:$PSScriptRoot\ExplorerCommand.def" "/IMPLIB:$work\ExplorerCommand.lib" *> (Join-Path $work 'command-build.log')
if($LASTEXITCODE){throw "Command DLL compile failed: $work"}
& cl.exe @common (Join-Path $PSScriptRoot 'Tests.cpp') "/Fe:$work\ModernExplorerTests.exe" "/Fo:$work\ModernExplorerTests.obj" /link @libraries *> (Join-Path $work 'tests-build.log')
if($LASTEXITCODE){throw "Fixture compile failed: $work"}
& cl.exe @common (Join-Path $PSScriptRoot 'LaunchSink.cpp') "/Fe:$work\LaunchSink.exe" "/Fo:$work\LaunchSink.obj" /link @libraries *> (Join-Path $work 'sink-build.log')
if($LASTEXITCODE){throw "Launch sink compile failed: $work"}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'AppxManifest.xml') -Destination $layout
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'AppxManifest.xml') -Destination (Join-Path $work 'identity')
foreach($pair in @(@('StoreLogo.scale-100.png','StoreLogo.png'),@('Square150x150Logo.scale-100.png','Logo150.png'),@('Square44x44Logo.scale-100.png','Logo44.png'))){Copy-Item -LiteralPath (Join-Path $repo ('res\terminal\images-Dev\'+$pair[0])) -Destination (Join-Path $layout ('Assets\'+$pair[1]))}
Copy-Item -LiteralPath (Join-Path $repo '..\LICENSE'),(Join-Path $repo '..\NOTICE.md') -Destination $layout
& (Join-Path $layout 'Backslash.ExplorerHost.exe') --manifest-identity (Join-Path $layout 'AppxManifest.xml') *> (Join-Path $work 'manifest-identity.json')
if($LASTEXITCODE){throw "Windows manifest reader failed: $work"}
$kit='C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
& (Join-Path $kit 'makeappx.exe') pack /d (Join-Path $work 'identity') /nv /p (Join-Path $work 'Backslash.ExplorerIntegration.Dev.identity.msix') *> (Join-Path $work 'package-build.log')
if($LASTEXITCODE){throw "Unsigned identity package assembly failed: $work"}
& (Join-Path $layout 'Backslash.ExplorerHost.exe') --status *> (Join-Path $work 'current-target-readonly.json')
if($LASTEXITCODE){throw "Read-only target status failed: $work"}
$identity=Get-Content -Raw (Join-Path $work 'manifest-identity.json')|ConvertFrom-Json
$current=Get-Content -Raw (Join-Path $work 'current-target-readonly.json')|ConvertFrom-Json
$support=Join-Path $env:LOCALAPPDATA 'Backslash\ExplorerIntegration\Dev'
$inventory=@(Get-ChildItem -LiteralPath $layout -Recurse -File|ForEach-Object{[ordered]@{relativePath=[IO.Path]::GetRelativePath($layout,$_.FullName);source=$_.FullName;proposedDestination=Join-Path $support ([IO.Path]::GetRelativePath($layout,$_.FullName));sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}})
$inventory|ConvertTo-Json -Depth 5|Set-Content (Join-Path $work 'files.json')
$expected=if($current.target){$current.target}else{'<none>'}
$plan=[ordered]@{
 status='NOT APPROVED / NOT EXECUTED';architecture='x64 only';identity=$identity;clsid='35af07f8-0746-4fba-9e2f-f1a1b1231dfa';supportRoot=$support
 unsignedPackage=Join-Path $work 'Backslash.ExplorerIntegration.Dev.identity.msix';unsignedPackageSHA256=(Get-FileHash (Join-Path $work 'Backslash.ExplorerIntegration.Dev.identity.msix')).Hash
 developerRegistration=@{api='RegisterPackageByUriAsync (or Add-AppxPackage -Register)';manifest=Join-Path $support 'AppxManifest.xml';externalLocation=$support;scope='current user';usesExistingDeveloperMode=$true;certificateChanges=@();forceShutdown=$false;requiresSeparateApproval=$true}
 targetSelection=@{executable=Join-Path $support 'Backslash.ExplorerHost.exe';arguments=@('--use-copy',$target,'--expected-current',$expected,'--apply','target-only');key='HKCU\Software\Backslash\ExplorerIntegration';owner='Backslash.ExplorerIntegration.Modern.v1';schemaVersion='1';SupportRoot=$support;TargetExecutable=$target;requiresSeparateApproval=$true}
 removal=@{package=@{scope='current user only';verifyName=$identity.name;verifyPublisher=$identity.publisher;verifyFamily=$identity.family;api='RemovePackageAsync for the exact verified installed full name';forceShutdown=$false};target=@{verifyOwner='Backslash.ExplorerIntegration.Modern.v1';key='HKCU\Software\Backslash\ExplorerIntegration';requireExplicitCurrentTarget=$true};files='Only exact hash/ownership-inventory support files after package removal; refuse foreign content; no recursive wildcard cleanup';classicKeys='Leave both existing Backslash.Open keys untouched'}
 boundaries=@('No deployment/signing/trust/policy/Explorer changes executed','No main Backslash identity/profile changes','Unsigned MSIX is review/build input, not an approved end-user installation','Target must support --backslash-explorer-folder; executable shape is validated, protocol support is not auto-probed','Target writes are bounded and expected-copy checked, not transactional across external writers')
}
$plan|ConvertTo-Json -Depth 9|Set-Content (Join-Path $work 'deployment-plan.json')
Write-Output "Prototype build complete (no tests/deployment performed): $work"
