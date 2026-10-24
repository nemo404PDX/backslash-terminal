#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildRun,
    [string]$VsInstallPath = 'C:\Program Files\Microsoft Visual Studio\18\Community'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$run = (Resolve-Path -LiteralPath $BuildRun).Path
if (-not $run.StartsWith((Join-Path $repo 'artifacts\build\'), [StringComparison]::OrdinalIgnoreCase)) { throw 'BuildRun must be a build evidence directory in this checkout.' }
$log = Get-Content (Join-Path $run 'build.log') -Raw
if ($log -notmatch '0 Error\(s\)') { throw 'BuildRun does not contain a successful build.' }
$work = Join-Path $run ('portable-resources-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $work | Out-Null
$kit = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
$packages = @(Get-ChildItem (Join-Path $run 'packages') -Recurse -File -Filter '*_x64.msix')
if ($packages.Count -ne 1) { throw 'Expected exactly one x64 MSIX build payload to unpack without installing.' }
$layout = Join-Path $work 'terminal'
& (Join-Path $kit 'makeappx.exe') unpack /p $packages[0].FullName /d $layout /o | Out-File (Join-Path $work 'terminal-unpack.log')
if ($LASTEXITCODE) { throw 'Terminal payload unpack failed.' }
[xml]$manifest = Get-Content (Join-Path $layout 'AppxManifest.xml')
if ($manifest.Package.Identity.ProcessorArchitecture -ne 'x64') { throw 'This first distribution script supports Release x64 only.' }
$dest = Join-Path $repo ('artifacts\portable\Backslash-Dev-' + (Split-Path $run -Leaf))
if (Test-Path -LiteralPath $dest) { throw 'Refusing to overwrite an existing portable distribution.' }
New-Item -ItemType Directory -Path $dest | Out-Null
# Adapted from New-UnpackagedTerminalDistribution.ps1: retain runtime resources, omit package registration material.
Get-ChildItem -LiteralPath $layout | Where-Object { $_.Name -notlike 'Appx*' -and $_.Name -notlike '*.xml' -and $_.Name -notlike '*.winmd' -and $_.Name -notlike '*.pdb' -and $_.Name -notlike '*.appxrecipe' } | Copy-Item -Destination $dest -Recurse
$xaml = Join-Path $work 'xaml'
$kit = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
$xamlPackage = Join-Path $repo 'packages\Microsoft.UI.Xaml.2.8.4\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.8.appx'
& (Join-Path $kit 'makeappx.exe') unpack /p $xamlPackage /d $xaml /o | Out-File (Join-Path $work 'unpack.log')
if ($LASTEXITCODE) { throw 'XAML unpack failed.' }
Copy-Item (Join-Path $xaml 'Microsoft.UI.Xaml.dll'),(Join-Path $xaml 'Microsoft.UI.Xaml') -Destination $dest -Recurse
$dump = Join-Path $work 'terminal.pri.xml'
& (Join-Path $kit 'makepri.exe') dump /if (Join-Path $dest 'resources.pri') /of $dump /dt detailed | Out-File (Join-Path $work 'dump.log')
if ($LASTEXITCODE) { throw 'Terminal PRI dump failed.' }
[xml]$document = Get-Content $dump
$map = $document.PriInfo.ResourceMap
$files = $map.ResourceMapSubtree | Where-Object Name -eq 'Files'
$collision = $files.ResourceMapSubtree | Where-Object Name -eq 'Microsoft.UI.Xaml'
if ($null -ne $collision) { $null = $files.RemoveChild($collision); $document.Save($dump) }
# Use upstream PRI merge support, preserving its temporary evidence instead of recursive cleanup.
$merge = Join-Path $work 'Merge-PriFiles.ps1'
$mergeSource = [IO.File]::ReadAllText((Join-Path $repo 'build\scripts\Merge-PriFiles.ps1'))
$mergeSource = $mergeSource.Replace('Remove-Item -Recurse -Force $tempDir', 'Write-Verbose "Retained PRI merge evidence: $tempDir"')
[IO.File]::WriteAllText($merge, $mergeSource, [Text.UTF8Encoding]::new($false))
$merged = Join-Path $work 'resources.pri'
& $merge -Path $dump,(Join-Path $xaml 'resources.pri') -IndexName $map.name -OutputPath $merged -MakePriPath (Join-Path $kit 'makepri.exe') | Out-File (Join-Path $work 'merge.log')
if ($LASTEXITCODE -or -not (Test-Path -LiteralPath $merged)) { throw 'PRI merge failed.' }
Copy-Item -LiteralPath $merged -Destination (Join-Path $dest 'resources.pri')
$crt = Get-ChildItem (Join-Path $VsInstallPath 'VC\Redist\MSVC') -Directory | Where-Object Name -match '^\d+\.' | Sort-Object Name -Descending | Select-Object -First 1
Copy-Item (Join-Path $crt.FullName 'x64\Microsoft.VC145.CRT\*.dll') -Destination $dest
Copy-Item (Join-Path $repo 'LICENSE'),(Join-Path $repo 'NOTICE.md'),(Join-Path $repo 'src\cascadia\CascadiaPackage\NOTICE.html') -Destination $dest
Copy-Item (Join-Path $repo 'packages\Microsoft.UI.Xaml.2.8.4\license.txt') -Destination (Join-Path $dest 'Microsoft.UI.Xaml.LICENSE.txt')
[IO.File]::WriteAllText((Join-Path $dest '.portable'), '')
@"
Backslash Dev portable development build
Version: $($manifest.Package.Identity.Version), x64 Release
Run Backslash.exe by full path in this directory. Helpers and package executable paths use that built target name.
Settings and state: adjacent settings directory. Missing marker fallback: LocalAppData\Backslash\Terminal.
External fragments and automatic stable-settings migration are disabled.
The Dev application uses supplied Backslash icon artwork. Helper icons outside images-Dev remain unchanged. Supplied Backslash SVG badge artwork is included.
This is a local development artifact. Do not register/install the upstream-identity MSIX from build output.
No aliases, package registrations, certificates, default-terminal or Explorer integration are installed.
CRT source: $($crt.FullName)\x64\Microsoft.VC145.CRT (application-local DLLs; no installer run).
Upstream/component notices are retained. Windows inbox system libraries remain OS prerequisites.
"@ | Set-Content (Join-Path $dest 'BACKSLASH-PORTABLE.txt')
if ($manifest.Package.Applications.Application.Executable -ne 'Backslash.exe') { throw 'Dev package manifest must launch the built Backslash.exe target.' }
if (Test-Path -LiteralPath (Join-Path $dest 'WindowsTerminal.exe')) { throw 'Stale upstream-named host executable must not enter a Backslash portable.' }
$required = 'Backslash.exe','OpenConsole.exe','TerminalApp.dll','Microsoft.Terminal.Settings.Model.dll','Microsoft.Terminal.Control.dll','Microsoft.UI.Xaml.dll','resources.pri','vcruntime140.dll','msvcp140.dll','.portable','LICENSE','NOTICE.md','NOTICE.html'
foreach ($file in $required) { if (-not (Test-Path -LiteralPath (Join-Path $dest $file))) { throw "Distribution missing $file" } }
Get-ChildItem -LiteralPath $dest -Recurse -File | ForEach-Object { [pscustomobject]@{ Path = [IO.Path]::GetRelativePath($dest,$_.FullName); Length = $_.Length; SHA256 = (Get-FileHash -LiteralPath $_.FullName).Hash } } | ConvertTo-Json | Set-Content (Join-Path $run 'portable-files.json')
Write-Output $dest