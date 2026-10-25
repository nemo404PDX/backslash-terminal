#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory,[switch]$LayoutOnly,[string]$VsInstallPath='C:\Program Files\Microsoft Visual Studio\18\Community')
$ErrorActionPreference='Stop';$PSNativeCommandUseErrorActionPreference=$false
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
$work=Join-Path $evidence ('badge-composed-'+[guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $work,"$work\Assets\branding" | Out-Null
foreach($name in 'backslash-badge.svg','backslash-badge-outline.svg','backslash-badge-currentcolor.svg'){Copy-Item -LiteralPath "$repo\..\docs\images\$name" -Destination "$work\Assets\branding"}
Import-Module (Join-Path $VsInstallPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsInstallPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT "/I$repo\packages\Microsoft.Windows.ImplementationLibrary.1.0.250325.1\include" (Join-Path $PSScriptRoot 'BadgeComposedTests.cpp') "/Fe:$work\BadgeComposedTests.exe" "/Fo:$work\BadgeComposedTests.obj" /link windowsapp.lib user32.lib windowscodecs.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> "$work\build.log"
if($LASTEXITCODE){Get-Content "$work\build.log" -Tail 25;throw 'Composed badge test build failed.'}
if($LayoutOnly){ & "$work\BadgeComposedTests.exe" $work --layout-only *> "$work\result.log" }
else { & "$work\BadgeComposedTests.exe" $work *> "$work\result.log" }
$code=$LASTEXITCODE;Get-Content "$work\result.log"
if($code){throw "Composed badge regression failed. Evidence: $work"}
Write-Output "Composed badge evidence: $work"
