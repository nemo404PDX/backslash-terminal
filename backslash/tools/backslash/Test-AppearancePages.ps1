#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableDirectory,
      [ValidateSet('global','profile','sidebar')][string[]]$Pages = @('global','profile','sidebar'))
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$portable = (Resolve-Path -LiteralPath $PortableDirectory).Path
$editor = 'Microsoft.Terminal.Settings.Editor.dll'
$builtEditor = Join-Path $repo "bin\x64\Release\Microsoft.Terminal.Settings.Editor\$editor"
if ((Get-FileHash -LiteralPath (Join-Path $portable $editor)).Hash -ne (Get-FileHash -LiteralPath $builtEditor).Hash) {
    throw 'Use a portable matching this checkout build: generated projection and DLL versions must agree.'
}
$work = Join-Path (Split-Path $repo -Parent) ('artifacts\appearance-checks\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
$fixture = Join-Path $work 'portable'
New-Item -ItemType Directory -Path $fixture | Out-Null
# Never copy user settings, Pins, leases, state or locks. The native test creates
# only synthetic built-in settings and a preview connection, never a shell.
Get-ChildItem -LiteralPath $portable -Force | Where-Object Name -NE 'settings' | Copy-Item -Destination $fixture -Recurse
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$exe = Join-Path $fixture 'AppearancePageTests.exe'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT /DAPPEARANCE_EDGE_VM_TESTS "/I$repo\src\cascadia\TerminalApp\Generated Files" (Join-Path $PSScriptRoot 'AppearancePageTests.cpp') "/Fe:$exe" "/Fo:$work\AppearancePageTests.obj" /link windowsapp.lib user32.lib windowscodecs.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> (Join-Path $work 'build.log')
if ($LASTEXITCODE) { Get-Content (Join-Path $work 'build.log') -Tail 20; throw 'Appearance page test build failed.' }
Push-Location -LiteralPath $fixture
try {
    foreach ($page in $Pages) {
        & $exe $page $work *> (Join-Path $work "$page.log")
        $result = $LASTEXITCODE
        Get-Content (Join-Path $work "$page.log")
        if ($result) { throw "Native $page Appearance check failed ($result)." }
    }
}
finally { Pop-Location }
Write-Output "Appearance page evidence: $work"
