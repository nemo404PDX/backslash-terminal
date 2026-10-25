#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [string]$VsInstallPath = 'C:\Program Files\Microsoft Visual Studio\18\Community'
)
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$work = Join-Path $evidence ('badge-xaml-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $work,(Join-Path $work 'Assets\branding') | Out-Null
foreach ($name in 'backslash-badge.svg','backslash-badge-outline.svg','backslash-badge-currentcolor.svg') {
    Copy-Item -LiteralPath (Join-Path $repo "..\docs\images\$name") -Destination (Join-Path $work 'Assets\branding')
}
Import-Module (Join-Path $VsInstallPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsInstallPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$probe = Join-Path $work 'BadgeXamlDecodeProbe.exe'
$build = Join-Path $work 'build.log'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT "/I$repo\packages\Microsoft.Windows.ImplementationLibrary.1.0.250325.1\include" (Join-Path $PSScriptRoot 'BadgeXamlDecodeProbe.cpp') "/Fe:$probe" "/Fo:$work\BadgeXamlDecodeProbe.obj" /link windowsapp.lib user32.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> $build
if ($LASTEXITCODE) { Get-Content -LiteralPath $build -Tail 20; throw 'Native badge probe compilation failed.' }
$result = Join-Path $work 'result.log'
& $probe *> $result
$code = $LASTEXITCODE
Get-Content -LiteralPath $result
if ($code) { throw "Native XAML SVG decode failed with exit code $code. Evidence: $work" }
Write-Output "Native XAML badge evidence: $work"
