#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory, [string]$VsInstallPath)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
function Read-Block([string]$Text, [int]$Open) {
    if ($Open -lt 0 -or $Text[$Open] -ne '{') { throw 'Production cleanup block not found.' }
    $depth = 1; $end = $Open + 1
    while ($depth -gt 0 -and $end -lt $Text.Length) {
        if ($Text[$end] -eq '{') { $depth++ }
        if ($Text[$end] -eq '}') { $depth-- }
        $end++
    }
    if ($depth) { throw 'Unbalanced production cleanup block.' }
    return $Text.Substring($Open + 1, $end - $Open - 2)
}
$control = Get-Content (Join-Path $repo 'src\cascadia\TerminalControl\TermControl.cpp') -Raw
$method = $control.IndexOf('void TermControl::_updateSchemePadding()', [StringComparison]::Ordinal)
$condition = $control.IndexOf('if (!get_self<ControlCore>(_core)->RendererOwnsSchemeBackground())', $method, [StringComparison]::Ordinal)
if ($method -lt 0 -or $condition -lt 0) { throw 'Disabled-padding cleanup source not found.' }
$padding = Read-Block $control ($control.IndexOf('{', $condition))
$page = Get-Content (Join-Path $repo 'src\cascadia\TerminalApp\TerminalPage.cpp') -Raw
$method = $page.IndexOf('void TerminalPage::_UpdateWindowImagePresentations()', [StringComparison]::Ordinal)
$inactive = $page.IndexOf('if (!mapped)', $method, [StringComparison]::Ordinal)
if ($method -lt 0 -or $inactive -lt 0) { throw 'Inactive window-image cleanup source not found.' }
$inactiveBody = Read-Block $page ($page.IndexOf('{', $inactive))
$backdrop = $null
foreach ($match in [regex]::Matches($inactiveBody, 'if \(_windowImageBackdrop[^\r\n]*')) {
    $open = $inactiveBody.IndexOf('{', $match.Index)
    $body = Read-Block $inactiveBody $open
    if ($body.Contains('_windowImageBackdrop.Children().Clear()')) {
        $backdrop = $inactiveBody.Substring($match.Index, $open - $match.Index) + '{' + $body + '}'
        break
    }
}
if (-not $backdrop) { throw 'Window-image backdrop cleanup source not found.' }
$cpp = Join-Path $evidence 'XamlCleanupTests.cpp'
$exe = Join-Path $evidence 'XamlCleanupTests.exe'
$obj = Join-Path $evidence 'XamlCleanupTests.obj'
foreach ($path in @($cpp,$exe,$obj)) { if (Test-Path -LiteralPath $path) { throw 'Fresh XAML cleanup evidence paths required.' } }
$template = Get-Content (Join-Path $PSScriptRoot 'XamlCleanupTests.cpp.in') -Raw
[IO.File]::WriteAllText($cpp, $template.Replace('/* EXTRACT_PADDING */', $padding).Replace('/* EXTRACT_BACKDROP */', $backdrop), [Text.UTF8Encoding]::new($false))
if (-not $VsInstallPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $VsInstallPath = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
}
if (-not $VsInstallPath) { throw 'An existing C++ toolchain is required.' }
Import-Module (Join-Path $VsInstallPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsInstallPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT $cpp "/Fe:$exe" "/Fo:$obj" /link windowsapp.lib user32.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\src\cascadia\WindowsTerminal\WindowsTerminal.manifest"
if ($LASTEXITCODE) { throw 'XAML cleanup test compilation failed.' }
& $exe
if ($LASTEXITCODE) { throw "XAML cleanup runtime regression failed: $LASTEXITCODE" }
