#Requires -Version 7.0
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$work=Join-Path $repo ('artifacts\builtin-view-activation\'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
$source=Get-Content (Join-Path $repo 'backslash\src\cascadia\TerminalApp\TerminalPage.cpp') -Raw
foreach($name in '_FocusSettingsEditor','_FocusTransfersView'){
    $start=$source.IndexOf("void TerminalPage::$name(")
    if($start -lt 0){throw "Missing production callback $name"}
    $end=$source.IndexOf('    void TerminalPage::',$start+1)
    if($end -lt 0){$end=$source.Length}
    $body=$source.Substring($start,$end-$start)
    if($body -notmatch 'ActivateBuiltinViewWindow\(\*_hostingHwnd\)' -or $body -match 'SW_RESTORE'){throw "Production focus wiring regressed: $name"}
}
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$exe=Join-Path $work 'BuiltinViewWindowActivationTests.exe'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'BuiltinViewWindowActivationTests.cpp') "/Fe:$exe" "/Fo:$work\test.obj" /link user32.lib *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log');throw 'Activation test build failed'}
& $exe *> (Join-Path $work 'test.log')
$code=$LASTEXITCODE
Get-Content (Join-Path $work 'test.log')
Write-Output "Activation evidence: $work"
if($code){throw "Activation tests failed ($code)"}
