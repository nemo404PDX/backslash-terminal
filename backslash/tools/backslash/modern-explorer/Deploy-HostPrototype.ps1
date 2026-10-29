#requires -Version 5.1
<# Run from ordinary, non-elevated Windows PowerShell on the host.
   Default: read-only verification/plan. -Register: one attempt, with bounded rollback.
   This is pinned to the reviewed developer artifacts, not a general installer. #>
[CmdletBinding()]
param([switch]$Register)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'HostDeploymentGuards.ps1')
if($PSVersionTable.PSEdition -ne 'Desktop' -or ![Environment]::Is64BitProcess){throw 'Use 64-bit Windows PowerShell 5.1 (powershell.exe), not pwsh.'}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run without elevation; this workflow is current-user only.'}
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..'))
$run=Join-Path $repo 'artifacts\modern-explorer\20261029-104430-748-26cadc60'
$support='C:\Users\nemo\AppData\Local\Backslash\ExplorerIntegration\Dev'
$target=Join-Path $repo 'artifacts\portable\Backslash-Dev-20261029-091419-230-Release-x64-e979d558\Backslash.exe'
$name='Backslash.ExplorerIntegration.Dev';$publisher='CN=Backslash Development'
$family='Backslash.ExplorerIntegration.Dev_whmwxsrdn33ha';$full='Backslash.ExplorerIntegration.Dev_0.0.1.0_x64__whmwxsrdn33ha'
$configPath='Software\Backslash\ExplorerIntegration'
$expectedConfig=@{Owner='Backslash.ExplorerIntegration.Modern.v1';SchemaVersion='1';TargetExecutable=$target;SupportRoot=$support}
if($env:LOCALAPPDATA -ine 'C:\Users\nemo\AppData\Local' -or [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData) -ine $env:LOCALAPPDATA){throw 'Unexpected host LocalAppData context; stop and review, do not work around isolation.'}
function Assert-NoReparse([string]$path){
    $cursor=[IO.Path]::GetFullPath($path)
    while($cursor){
        if(Test-Path -LiteralPath $cursor){if((Get-Item -Force -LiteralPath $cursor).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse path refused: $cursor"}}
        $parent=Split-Path -Parent $cursor;if($parent -eq $cursor){break};$cursor=$parent
    }
}
function Assert-Hash([string]$path,[string]$hash){Assert-NoReparse $path;if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne $hash){throw "Reviewed file changed: $path"}}
function Get-ExactPackage {
    param([switch]$RequireOwnedLocation)
    $packages=@(Get-AppxPackage -Name $name)
    foreach($package in $packages){
        if($package.Name -cne $name -or $package.Publisher -cne $publisher -or $package.PackageFamilyName -cne $family -or $package.PackageFullName -cne $full){throw 'Unexpected package identity; preserved.'}
        if($RequireOwnedLocation){Assert-HostPackageLocation $package $support;Assert-NoReparse $package.InstallLocation;Assert-Hash (Join-Path $package.InstallLocation 'AppxManifest.xml') '6D13A92325CEDB0A7271BA63A97FF64BE8C769EB657695C0A4A33C70857165D4'}
    }
    return $packages
}
function Assert-ConfigAbsent {
    $key=[Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($configPath)
    if($key){$key.Dispose();throw 'Modern target configuration already exists; no adoption or replacement.'}
}
function Assert-ConfigOwned {
    $key=[Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($configPath)
    if(!$key){return $false}
    try {
        Initialize-HostRawRegistryReader
        $values=@(foreach($valueName in $key.GetValueNames()){$raw=[BackslashHostRawRegistry]::Read($key,$valueName);[pscustomobject]@{Name=$valueName;Kind=$raw.Kind;Bytes=$raw.Bytes}})
        Assert-ExpectedRawConfig $values $key.SubKeyCount $expectedConfig
        return $true
    } finally {$key.Dispose()}
}
Assert-NoReparse $repo;Assert-NoReparse $run;Assert-NoReparse $support;Assert-NoReparse $target
Assert-Hash (Join-Path $run 'files.json') 'C15C700E1D19BFFC496B838B12A6E25C4A311DBE8C4D693CE7A9E19007109E58'
Assert-Hash (Join-Path $run 'deployment-plan.json') '6F328E317904FB7A2FE0C8C717BA1A36F695D1A7760FCD9BE7BB4BF09C222DD4'
$inventory=Get-Content -Raw -LiteralPath (Join-Path $run 'files.json')|ConvertFrom-Json
if($inventory.Count -ne 8){throw 'Expected eight reviewed support files.'}
$seen=@{}
foreach($file in $inventory){
    $source=[IO.Path]::GetFullPath($file.source);$destination=[IO.Path]::GetFullPath($file.proposedDestination)
    if(!$source.StartsWith(($run+'\layout\'),[StringComparison]::OrdinalIgnoreCase) -or !$destination.StartsWith(($support+'\'),[StringComparison]::OrdinalIgnoreCase) -or $seen.ContainsKey($destination)){throw 'Inventory escaped roots or contains duplicates.'}
    $seen[$destination]=$true;Assert-Hash $source $file.sha256;Assert-NoReparse $destination
}
Assert-Hash $target '92ED27078BC1EA802DFB858CA9B0F4CEC4FC6DE5C3C8ED4D2EE33B93059DDB3D'
Assert-Hash (Join-Path (Split-Path -Parent $target) 'TerminalApp.dll') '81D8EDC1FC81A99AA3DC385DDBD6F9E4844F56D954F5CE2CEBA423677DDD3234'
$profile=Join-Path (Split-Path -Parent $target) 'settings\settings.json'
Assert-NoReparse $profile
if(!(Test-Path -LiteralPath (Join-Path (Split-Path -Parent $target) '.portable')) -or !(Test-Path -LiteralPath $profile -PathType Leaf)){throw 'Existing portable marker/profile required; no launch or profile creation permitted.'}
$profileHash=(Get-FileHash -LiteralPath $profile).Hash
$policy=Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock'
if($policy.AllowDevelopmentWithoutDevLicense -ne 1){throw 'Developer Mode must already be enabled; this script never changes it.'}
if(@(Get-ExactPackage).Count){throw 'Package already exists; no replacement.'}
Assert-ConfigAbsent
if(Test-Path -LiteralPath $support){throw 'Support directory already exists; no adoption/overwrite.'}
$plan=[ordered]@{mode=if($Register){'ONE registration requested'}else{'VERIFY ONLY: no changes'};source=$run;support=$support;files=@($inventory|Select-Object relativePath,sha256,proposedDestination);package=$full;publisher=$publisher;family=$family;target=$target;targetRegistryKey=('HKCU\'+$configPath);effects=@('Copy eight reviewed files','Register exact loose manifest for current user','Select reviewed target with expected-current <none>');noChanges=@('certificates','policies','elevation','Explorer restart','profiles','classic verbs');manualAcceptance='Main Windows 11 folder/background menu and actual folder launch still require testing.'}
$plan|ConvertTo-Json -Depth 6|Write-Output
if(!$Register){return}
# A fresh evidence directory belongs only to this invocation. No resume/retry loop.
$evidence=Join-Path $repo ('artifacts\modern-explorer\host-deploy-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $evidence|Out-Null
$ledger=[ordered]@{phase='starting';createdDirectories=@();copiedFiles=@();registrationRequested=$false;targetAttempted=$false}
function Save-Ledger {$ledger|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $evidence 'ledger.json')}
function Undo-ThisAttempt {
    Assert-NoReparse $support
    $packages=@(Get-ExactPackage -RequireOwnedLocation)
    if($packages.Count -and !$ledger.registrationRequested){throw 'Package appeared before our request; preserved.'}
    $targetExists=Assert-ConfigOwned
    if($targetExists -and !$ledger.targetAttempted){throw 'Target appeared before our operation; preserved.'}
    $expectedFiles=@{}
    foreach($file in $ledger.copiedFiles){Assert-Hash $file.proposedDestination $file.sha256;$expectedFiles[$file.proposedDestination]=$true}
    if(Test-Path -LiteralPath $support){
        $queue=New-Object 'System.Collections.Generic.Queue[string]';$queue.Enqueue($support)
        while($queue.Count){$folder=$queue.Dequeue();Assert-NoReparse $folder;foreach($entry in Get-ChildItem -LiteralPath $folder -Force){Assert-NoReparse $entry.FullName;if($entry.PSIsContainer){if($entry.FullName -cne (Join-Path $support 'Assets')){throw 'Unknown support directory; preserved.'};$queue.Enqueue($entry.FullName)}elseif(!$expectedFiles.ContainsKey($entry.FullName)){throw 'Unknown support file; preserved.'}}}
    }
    foreach($package in $packages){
        $current=@(Get-ExactPackage -RequireOwnedLocation)
        if($current.Count -ne 1){throw 'Package changed before rollback removal; preserved.'}
        Remove-AppxPackage -Package $current[0].PackageFullName -ErrorAction Stop
    }
    if(@(Get-ExactPackage).Count){throw 'Package remains; support files preserved.'}
    if($targetExists){if(!(Assert-ConfigOwned)){throw 'Target changed during rollback'};[Microsoft.Win32.Registry]::CurrentUser.DeleteSubKey($configPath,$true)}
    foreach($file in $ledger.copiedFiles){Assert-Hash $file.proposedDestination $file.sha256;Remove-Item -LiteralPath $file.proposedDestination}
    for($i=$ledger.createdDirectories.Count-1;$i -ge 0;$i--){$directory=$ledger.createdDirectories[$i];Assert-NoReparse $directory;if(@(Get-ChildItem -LiteralPath $directory -Force).Count){throw 'Nonempty created directory; preserved.'};Remove-Item -LiteralPath $directory}
    $ledger.phase='rolled back exact attempt-owned changes';Save-Ledger
}
try {
    Save-Ledger
    foreach($directory in @('C:\Users\nemo\AppData\Local\Backslash','C:\Users\nemo\AppData\Local\Backslash\ExplorerIntegration',$support,(Join-Path $support 'Assets'))){Assert-NoReparse $directory;if(!(Test-Path -LiteralPath $directory)){New-Item -ItemType Directory -Path $directory|Out-Null;$ledger.createdDirectories+=@($directory);Save-Ledger}}
    foreach($file in $inventory){
        Assert-Hash $file.source $file.sha256;Assert-NoReparse $file.proposedDestination
        # CreateNew prevents overwriting even if another writer races the check.
        $stream=[IO.File]::Open($file.proposedDestination,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
        try {$ledger.copiedFiles+=@($file);Save-Ledger;$sourceStream=[IO.File]::OpenRead($file.source);try{$sourceStream.CopyTo($stream)}finally{$sourceStream.Dispose()}}finally{$stream.Dispose()}
        Assert-Hash $file.proposedDestination $file.sha256
    }
    Assert-ConfigAbsent;if(@(Get-ExactPackage).Count){throw 'Package appeared before registration; refusing.'}
    foreach($file in $ledger.copiedFiles){Assert-Hash $file.proposedDestination $file.sha256}
    $ledger.registrationRequested=$true;$ledger.phase='one registration requested';Save-Ledger
    Add-AppxPackage -Register -Path ([Uri]::new((Join-Path $support 'AppxManifest.xml'))).AbsoluteUri -ExternalLocation ([Uri]::new(($support+'\'))).AbsoluteUri -ErrorAction Stop
    if(@(Get-ExactPackage -RequireOwnedLocation).Count -ne 1){throw 'Expected exact registered package absent.'}
    Assert-ConfigAbsent
    $ledger.targetAttempted=$true;Save-Ledger
    & (Join-Path $support 'Backslash.ExplorerHost.exe') --use-copy $target --expected-current '<none>' --apply target-only
    if($LASTEXITCODE -ne 0){throw 'Target selection failed.'}
    if(!(Assert-ConfigOwned)){throw 'Expected owned target missing.'}
    $key=[Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($configPath);try{if($key.ValueCount -ne 4){throw 'Target selection incomplete.'}}finally{$key.Dispose()}
    if((Get-FileHash -LiteralPath $profile).Hash -cne $profileHash){throw 'Portable settings changed; preserve for inspection.'}
    $ledger.phase='registered exact package and selected reviewed target; interactive acceptance pending';Save-Ledger
    Write-Output "SUCCESS: $evidence. Verify first-level folder/background menu and actual folder launch manually."
} catch {
    $failure=$_;$_|Out-String|Set-Content -LiteralPath (Join-Path $evidence 'failure.txt')
    try {Undo-ThisAttempt} catch {$_|Out-String|Set-Content -LiteralPath (Join-Path $evidence 'rollback-error.txt');throw "Deployment failed; rollback stopped to preserve changed/foreign state. Inspect $evidence"}
    throw "Deployment failed; exact attempt-owned changes rolled back. Inspect $evidence. Original error: $($failure.Exception.Message)"
}
