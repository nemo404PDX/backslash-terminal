#Requires -Version 7.0
[CmdletBinding()]
param(
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [ValidateSet('x64','ARM64','Win32')][string]$Platform = 'x64',
    [ValidateRange(1,64)][int]$MaxCpuCount = 2,
    [string]$VsInstallPath,
    [switch]$Preflight,
    [switch]$Rebuild
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$buildTarget = if ($Rebuild) { 'Terminal\CascadiaPackage:Rebuild' } else { 'Terminal\CascadiaPackage' }
$exitCode = 1
$transcribing = $false
$pushed = $false
function Invoke-LoggedNative {
    param([string]$Executable, [string[]]$Arguments, [string]$Log)
    & $Executable @Arguments 2>&1 | Tee-Object -FilePath $Log | Out-Host
    $code = $LASTEXITCODE
    if ($code -ne 0) {
        $script:exitCode = $code
        throw "$Executable failed with exit code $code. See $Log"
    }
}
try {
    foreach ($relative in @('OpenConsole.slnx','dep\nuget\nuget.exe','dep\nuget\packages.config','src\cascadia\CascadiaPackage\CascadiaPackage.wapproj')) {
        if (-not (Test-Path -LiteralPath (Join-Path $repoRoot $relative) -PathType Leaf)) {
            throw "Expected source file missing: $relative (repository: $repoRoot)"
        }
    }
    if (-not $VsInstallPath) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) { throw 'vswhere.exe missing; supply -VsInstallPath for an existing installation.' }
        $json = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
        if ($LASTEXITCODE -ne 0) { $exitCode = $LASTEXITCODE; throw 'Visual Studio discovery failed.' }
        $instances = @($json | ConvertFrom-Json)
        if ($instances.Count -eq 0) { throw 'No installed Visual Studio with MSBuild and C++ tools found.' }
        $VsInstallPath = $instances[0].installationPath
    }
    $VsInstallPath = (Resolve-Path -LiteralPath $VsInstallPath).Path
    $devShell = Join-Path $VsInstallPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    $msbuild = Join-Path $VsInstallPath 'MSBuild\Current\Bin\amd64\MSBuild.exe'
    foreach ($tool in @($devShell,$msbuild)) {
        if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Required installed tool missing: $tool" }
    }
    Write-Host "Repository: $repoRoot"
    Write-Host "Visual Studio: $VsInstallPath"
    Write-Host "MSBuild: $msbuild"
    Write-Host "Target: $buildTarget ($Configuration|$Platform)"
    Write-Warning 'Default upstream WindowsTerminalDev package identity is retained. This is not an independently identified Backslash release.'
    if ($Preflight) {
        Write-Host 'Preflight passed: source and tool paths exist. No restore, build, writes or developer-shell changes performed.'
        exit 0
    }
    $runName = '{0}-{1}-{2}-{3}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'),$Configuration,$Platform,([guid]::NewGuid().ToString('N').Substring(0,8))
    $runDir = Join-Path (Split-Path $repoRoot -Parent) "artifacts\build\$runName"
    New-Item -ItemType Directory -Path $runDir | Out-Null
    Start-Transcript -Path (Join-Path $runDir 'session.log') | Out-Null
    $transcribing = $true
    Write-Host "Build evidence: $runDir"
    Push-Location -LiteralPath $repoRoot
    $pushed = $true
    $gitCmd = Join-Path $env:ProgramFiles 'Git\cmd'
    if (Test-Path -LiteralPath (Join-Path $gitCmd 'git.exe') -PathType Leaf) { $env:PATH = "$gitCmd;$env:PATH" }
    Import-Module $devShell
    $architecture = if ($Platform -eq 'Win32') { 'x86' } else { $Platform.ToLowerInvariant() }
    Enter-VsDevShell -VsInstallPath $VsInstallPath -SkipAutomaticLocation -DevCmdArguments "-arch=$architecture -host_arch=x64"
    $nuget = Join-Path $repoRoot 'dep\nuget\nuget.exe'
    Invoke-LoggedNative $nuget @('restore','dep\nuget\packages.config','-NonInteractive') (Join-Path $runDir 'restore-dependencies.log')
    Invoke-LoggedNative $nuget @('restore','OpenConsole.slnx','-NonInteractive') (Join-Path $runDir 'restore-solution.log')
    $packageDir = Join-Path $runDir 'packages'
    $arguments = @('OpenConsole.slnx',"/t:$buildTarget","/p:Configuration=$Configuration","/p:Platform=$Platform",'/p:AppxSymbolPackageEnabled=false','/p:AppxPackageSigningEnabled=false',"/p:AppxPackageDir=$packageDir\","/m:$MaxCpuCount","/bl:$(Join-Path $runDir 'build.binlog')")
    Invoke-LoggedNative $msbuild $arguments (Join-Path $runDir 'build.log')
    Write-Host "Build completed with exit code 0. Logs and package output: $runDir"
    $exitCode = 0
}
catch { Write-Error -ErrorAction Continue $_ }
finally {
    if ($pushed) { Pop-Location }
    if ($transcribing) { Stop-Transcript | Out-Null }
}
exit $exitCode

