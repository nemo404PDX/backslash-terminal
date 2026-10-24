#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][ValidateSet('x64','ARM64','Win32')][string]$Architecture,
    [Parameter(Mandatory)][string]$Configuration
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
# The Dev package manifest is the sole source of the numeric Backslash version.
[xml]$manifest = Get-Content -LiteralPath (Join-Path $repo 'src\cascadia\CascadiaPackage\Package-Dev.appxmanifest') -Raw
$version = [string]$manifest.Package.Identity.Version
if ($version -notmatch '^\d+\.\d+\.\d+\.0$') { throw 'Dev version must have four numeric components and reserved revision 0.' }
foreach ($part in $version.Split('.')) { if ([int]$part -gt 65535) { throw 'Version component exceeds 65535.' } }
$base = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'BuildIdentity.json') -Raw | ConvertFrom-Json
if ($base.upstreamVersion -notmatch '^\d+\.\d+\.\d+\.\d+$' -or $base.upstreamCommit -notmatch '^[0-9a-f]{40}$') { throw 'Invalid upstream provenance.' }
$git = Join-Path $env:ProgramFiles 'Git\cmd\git.exe'
if (-not (Test-Path -LiteralPath $git)) { $git = (Get-Command git.exe -ErrorAction Stop).Source }
$revision = & $git -C $repo rev-parse --verify HEAD
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') { throw 'Cannot determine actual Git revision; refusing a misleading build identity.' }
$status = @(& $git -C $repo status --porcelain --untracked-files=normal)
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine Git dirty state.' }
$dirty = if ($status.Count) { '-dirty' } else { '' }
$utc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ',[Globalization.CultureInfo]::InvariantCulture)
$arch = if ($Architecture -eq 'Win32') { 'x86' } else { $Architecture.ToLowerInvariant() }
if ($Configuration -notmatch '^[A-Za-z0-9]+$') { throw 'Invalid configuration.' }
$identity = "Backslash $version-dev; upstream Windows Terminal $($base.upstreamVersion); built $utc; git $revision$dirty; $arch $Configuration; channel dev"
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$header = @"
// Generated for this build. Do not edit or commit.
#define BACKSLASH_VERSION_NUMERIC $($version.Replace('.',','))
#define BACKSLASH_VERSION_STRING "$version"
#define BACKSLASH_BUILD_IDENTITY "$identity"
#define BACKSLASH_UPSTREAM_COMMIT "$($base.upstreamCommit)"
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'BackslashBuildIdentity.h'),($header + [Environment]::NewLine),[Text.UTF8Encoding]::new($false))
[ordered]@{ version = "$version-dev"; upstreamVersion = $base.upstreamVersion; upstreamCommit = $base.upstreamCommit; builtUtc = $utc; gitRevision = $revision; dirty = [bool]$status.Count; architecture = $arch; configuration = $Configuration; channel = 'dev'; identity = $identity } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'build-identity.json') -Encoding utf8NoBOM
Write-Host $identity