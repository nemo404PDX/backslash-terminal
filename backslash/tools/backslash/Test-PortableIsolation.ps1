#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
function Require([bool]$Condition,[string]$Message) { if (-not $Condition) { throw $Message }; Write-Output "PASS: $Message" }
$fileUtils = Get-Content (Join-Path $repo 'src\cascadia\TerminalSettingsModel\FileUtils.cpp') -Raw
Require ($fileUtils.Contains('L"Backslash\\Terminal\\"')) 'Independent nonportable fallback'
Require ($fileUtils.Contains('if (!IsPackaged() && IsPortableMode())') -and $fileUtils.Contains('modulePath.replace_filename(PortableModeSettingsFolder)')) 'Portable base is adjacent to module'
$state = Get-Content (Join-Path $repo 'src\cascadia\TerminalSettingsModel\ApplicationState.cpp') -Raw
Require ($state.Contains('winrt::make_self<ApplicationState>(GetBaseSettingsPath())')) 'Application state uses same settings base'
$serialization = Get-Content (Join-Path $repo 'src\cascadia\TerminalSettingsModel\CascadiaSettingsSerialization.cpp') -Raw
$loadAll = $serialization.Substring($serialization.IndexOf('Model::CascadiaSettings CascadiaSettings::LoadAll()'))
$loadAll = $loadAll.Substring(0,$loadAll.IndexOf('// Method Description:',100))
Require (-not $loadAll.Contains('_releaseSettingsPath()') -and $loadAll.Contains('const bool releaseSettingExists = false;')) 'No automatic stable migration in any branding'
$discovery = [regex]::Match($serialization,'void SettingsLoader::FindFragmentsAndMergeIntoUserSettings\([^)]*\)\s*\{([^}]*)\}').Groups[1].Value
Require ($discovery.Trim().StartsWith('// Backslash') -and -not ($discovery -match 'directory_iterator|AppExtensionCatalog|_parseFragment')) 'No external file or extension fragment discovery'
Require (-not $serialization.Contains('com.microsoft.windows.terminal.settings') -and -not $serialization.Contains('L"\Microsoft\Windows Terminal\Fragments"')) 'Upstream fragment identities absent'
$routing = Get-Content (Join-Path $repo 'src\cascadia\WindowsTerminal\WindowEmperor.cpp') -Raw
Require (-not $routing.Contains('windowClassName.append(L"Windows Terminal') -and $routing.Contains('windowClassName.append(L"Backslash Dev")')) 'Independent window-routing prefix for all branding modes'
Require ($routing.Contains('if (!IsPackaged())') -and $routing.Contains('windowClassName.append(L" Admin")')) 'Executable-path and elevation routing separation retained'
$portable = (Resolve-Path -LiteralPath $PortableDirectory).Path
foreach ($name in '.portable','Backslash.exe','OpenConsole.exe','TerminalApp.dll','Microsoft.Terminal.Settings.Model.dll','Microsoft.Terminal.Control.dll','Microsoft.UI.Xaml.dll','Microsoft.UI.Xaml','resources.pri','vcruntime140.dll','msvcp140.dll','LICENSE','NOTICE.md','NOTICE.html','Microsoft.UI.Xaml.LICENSE.txt') {
    Require (Test-Path -LiteralPath (Join-Path $portable $name)) "Portable contains $name"
}
foreach ($badge in 'backslash-badge.svg','backslash-badge-outline.svg','backslash-badge-currentcolor.svg') {
    $asset = Join-Path $portable "Assets\branding\$badge"
    Require (Test-Path -LiteralPath $asset -PathType Leaf) "Portable contains badge $badge"
    Require ((Get-FileHash -LiteralPath $asset).Hash -eq (Get-FileHash -LiteralPath (Join-Path $repo "..\docs\images\$badge")).Hash) "Packaged badge matches supplied working asset $badge"
}
Require (@(Get-ChildItem -LiteralPath (Join-Path $portable 'Assets\branding') -File).Count -eq 3) 'Only the three working SVGs are runtime branding assets'
Require (@(Get-ChildItem -LiteralPath $portable -Filter 'Appx*').Count -eq 0) 'No package manifest/registration material in portable root'
$version = [Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $portable 'Backslash.exe'))
Require ($version.FileDescription -eq 'Backslash') 'Executable description identifies Backslash development build'
Require ($version.ProductName -eq 'Backslash' -and $version.OriginalFilename -eq 'Backslash.exe') 'Executable product/original filename identify Backslash'
Require (-not (Test-Path -LiteralPath (Join-Path $portable 'WindowsTerminal.exe'))) 'No stale upstream-named host executable in portable'
Write-Output "Version: $($version.FileVersion)"