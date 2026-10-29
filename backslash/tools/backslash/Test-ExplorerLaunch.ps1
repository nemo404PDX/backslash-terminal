#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableDirectory, [Parameter(Mandatory)][string]$EvidenceDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$portable = (Resolve-Path -LiteralPath $PortableDirectory).Path
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$runtime = Join-Path $evidence 'runtime'
if (Test-Path -LiteralPath $runtime) { throw 'Fresh evidence directory required.' }
New-Item -ItemType Directory -Path $runtime | Out-Null
# A separate executable path has a separate unpackaged process-routing identity.
# Never copy user settings; only this fixture's profiles and shell are launched.
Get-ChildItem -LiteralPath $portable -Force | Where-Object Name -NE 'settings' | Copy-Item -Destination $runtime -Recurse
$settings = Join-Path $runtime 'settings'
New-Item -ItemType Directory -Path $settings | Out-Null
$normal = Join-Path $evidence 'normal-profile-directory'
$selected = Join-Path $evidence '日本語 & selected;literal'
$nested = Join-Path $selected ';child'
New-Item -ItemType Directory -Path $normal,$selected,$nested | Out-Null
$observed = Join-Path $evidence 'observed.jsonl'
$probe = Join-Path $evidence 'probe.ps1'
$escaped = $observed.Replace("'", "''")
@"
`$ErrorActionPreference = 'Stop'
@{cwd=`$PWD.Path} | ConvertTo-Json -Compress | Add-Content -LiteralPath '$escaped' -Encoding utf8
"@ | Set-Content -LiteralPath $probe
$profileId = '{B4D3E41A-371F-4BB6-9181-F4C922E5622F}'
$profile = @{guid=$profileId;name='Isolated Explorer launch probe';commandline=('"{0}" -NoProfile -File "{1}"' -f (Get-Command pwsh).Source,$probe);startingDirectory=$normal;closeOnExit='always'}
$config = @{defaultProfile=$profileId;firstWindowPreference='defaultProfile';profiles=@{list=@($profile)}} | ConvertTo-Json -Depth 8
$settingsFile = Join-Path $settings 'settings.json'
[IO.File]::WriteAllText($settingsFile,$config)
$hash = (Get-FileHash -LiteralPath $settingsFile).Hash
$exe = Join-Path $runtime 'Backslash.exe'
$cases = @(@{name='normal';args=@();expected=$normal}, @{name='Explorer literal semicolon and Unicode';args=@('--backslash-explorer-folder',($nested+'\.'));expected=$nested}, @{name='Explorer drive root';args=@('--backslash-explorer-folder',([IO.Path]::GetPathRoot($evidence)+'\.'));expected=[IO.Path]::GetPathRoot($evidence)})
$count = 0
foreach ($case in $cases) {
    # ProcessStartInfo.ArgumentList preserves argv boundaries without a command shell.
    $start = [Diagnostics.ProcessStartInfo]::new($exe)
    $start.UseShellExecute = $false
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    foreach ($argument in $case.args) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 200
        $rows = @(if (Test-Path -LiteralPath $observed) { Get-Content -LiteralPath $observed })
    } while ($rows.Count -le $count -and [DateTime]::UtcNow -lt $deadline)
    if ($rows.Count -ne ($count+1)) { throw "Shell probe timed out or launched extra actions: $($case.name). Inspect only this fixture's runtime; no existing windows were touched." }
    $actual = ($rows[-1] | ConvertFrom-Json).cwd
    if ($actual.TrimEnd('\') -cne $case.expected.TrimEnd('\')) { throw "Wrong working directory: $actual, expected $($case.expected)" }
    if (-not $process.WaitForExit(10000)) { throw 'Owned probe process did not exit; left intact for inspection.' }
    $saved = Get-Content -LiteralPath $settingsFile -Raw | ConvertFrom-Json
    $savedProfile = @($saved.profiles.list | Where-Object { $_.guid -eq $profileId })
    if ($savedProfile.Count -ne 1 -or $savedProfile[0].startingDirectory -cne $normal -or $savedProfile[0].commandline -cne $profile.commandline) { throw 'Fixture profile changed during launch.' }
    # First ordinary launch performs inherited settings normalization/discovery.
    # Subsequent Explorer invocations must leave those normalized bytes untouched.
    if ($count -eq 0) { $hash = (Get-FileHash -LiteralPath $settingsFile).Hash }
    elseif ((Get-FileHash -LiteralPath $settingsFile).Hash -ne $hash) { throw 'Normalized fixture settings changed during Explorer launch.' }
    $count++
    Write-Output "PASS: $($case.name), shell cwd = $actual; profile settings unchanged"
}
