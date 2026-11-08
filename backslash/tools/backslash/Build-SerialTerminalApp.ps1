#Requires -Version 7.0
# Explicit local C1902 workaround; normal Build.ps1 remains unchanged.
[CmdletBinding()]
param([ValidateRange(1,64)][int]$MaxCpuCount=2,[string]$VsInstallPath,[switch]$Preflight,[switch]$Rebuild)
$ErrorActionPreference='Stop'
$targets=Join-Path $PSScriptRoot 'build/SerialTerminalApp.targets'
if((Get-FileHash -LiteralPath $targets).Hash -cne '973F4A5A207F61144F9248D8914C3E384799DF89818CC4FA062CF95FE92C7218'){throw 'Reviewed serial override changed'}
$existing=[Environment]::GetEnvironmentVariable('ForceImportAfterCppTargets','Process')
if($existing -and ![string]::Equals($existing,$targets,[StringComparison]::OrdinalIgnoreCase)){throw 'Existing ForceImportAfterCppTargets preserved; refusing to replace or combine a different import'}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=Join-Path $PSHOME 'pwsh.exe'
$start.UseShellExecute=$false
$start.WorkingDirectory=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
foreach($arg in @('-NoProfile','-File',(Join-Path $PSScriptRoot 'Build.ps1'),'-Configuration','Release','-Platform','x64','-MaxCpuCount',$MaxCpuCount.ToString())){[void]$start.ArgumentList.Add($arg)}
if($VsInstallPath){[void]$start.ArgumentList.Add('-VsInstallPath');[void]$start.ArgumentList.Add($VsInstallPath)}
if($Preflight){[void]$start.ArgumentList.Add('-Preflight')}
if($Rebuild){[void]$start.ArgumentList.Add('-Rebuild')}
# Child-only environment; parent value requires no restoration because it never changes.
$start.Environment['ForceImportAfterCppTargets']=$targets
$child=[Diagnostics.Process]::Start($start)
try{$child.WaitForExit();if($child.ExitCode){throw ('Serial build child failed with exit code '+$child.ExitCode)}}finally{$child.Dispose()}
