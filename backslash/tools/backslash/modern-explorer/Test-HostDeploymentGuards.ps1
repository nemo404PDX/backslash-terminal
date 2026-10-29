#requires -Version 5.1
# In-memory fixtures and mocked deployment only. No registry/filesystem/package writes.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'HostDeploymentGuards.ps1')
Initialize-HostRawRegistryReader
$expected=@{Owner='Backslash.ExplorerIntegration.Modern.v1';SchemaVersion='1';TargetExecutable='D:\reviewed\Backslash.exe';SupportRoot='C:\Users\nemo\AppData\Local\Backslash\ExplorerIntegration\Dev'}
function Make-Entry([string]$Name,[string]$Text){[pscustomobject]@{Name=$Name;Kind=[uint32]1;Bytes=[Text.Encoding]::Unicode.GetBytes($Text+[char]0)}}
$valid=@(foreach($keyName in @('Owner','SchemaVersion','TargetExecutable','SupportRoot')){Make-Entry $keyName $expected[$keyName]})
$script:checks=0;$script:removals=0
function Assert-Rejected([scriptblock]$Gate){
    $rejected=$false
    try{& $Gate;$script:removals++}catch{$rejected=$true}
    if(!$rejected -or $script:removals){throw 'Unsafe input passed the removal gate.'}
    $script:checks++
}
Assert-ExpectedRawConfig $valid 0 $expected
Assert-ExpectedRawConfig @($valid[0]) 0 $expected # Owned partial record is removable.
foreach($entryIndex in 0..3){
    $normal=$valid[$entryIndex].Bytes
    $badCases=@(
        [pscustomobject]@{Kind=2;Bytes=$normal}, # REG_EXPAND_SZ must not normalize to REG_SZ.
        [pscustomobject]@{Kind=3;Bytes=$normal},
        [pscustomobject]@{Kind=1;Bytes=[byte[]]@()},
        [pscustomobject]@{Kind=1;Bytes=[byte[]]@(0)},
        [pscustomobject]@{Kind=1;Bytes=[byte[]]$normal[0..($normal.Length-3)]}, # Missing terminator.
        [pscustomobject]@{Kind=1;Bytes=[byte[]]($normal+[byte[]]@(0,0))}, # Repeated terminator.
        [pscustomobject]@{Kind=1;Bytes=[Text.Encoding]::Unicode.GetBytes($expected[$valid[$entryIndex].Name]+[char]0+'hidden'+[char]0)},
        [pscustomobject]@{Kind=1;Bytes=[byte[]]@(0,216,0,0)}, # Unpaired surrogate.
        [pscustomobject]@{Kind=1;Bytes=[byte[]]::new(131074)}
    )
    foreach($bad in $badCases){
        $fixture=@(for($i=0;$i -lt 4;$i++){if($i -eq $entryIndex){[pscustomobject]@{Name=$valid[$i].Name;Kind=$bad.Kind;Bytes=$bad.Bytes}}else{$valid[$i]}})
        Assert-Rejected {Assert-ExpectedRawConfig $fixture 0 $expected}
    }
}
Assert-Rejected {Assert-ExpectedRawConfig @($valid[1..3]) 0 $expected}
Assert-Rejected {Assert-ExpectedRawConfig ($valid+@((Make-Entry 'Unknown' 'data'))) 0 $expected}
Assert-Rejected {Assert-ExpectedRawConfig ($valid+@($valid[0])) 0 $expected}
Assert-Rejected {Assert-ExpectedRawConfig $valid 1 $expected}

# Test the production package gate itself, extracting only its function definition.
# Appx enumeration, reparse/hash inspection and removal are replaced with in-memory mocks.
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'Deploy-HostPrototype.ps1'),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Deployment script parse failed.'}
$definition=$ast.Find({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Get-ExactPackage'},$true)
if(!$definition){throw 'Production package gate not found.'}
. ([scriptblock]::Create($definition.Extent.Text))
$name='Backslash.ExplorerIntegration.Dev';$publisher='CN=Backslash Development'
$family='Backslash.ExplorerIntegration.Dev_whmwxsrdn33ha';$full='Backslash.ExplorerIntegration.Dev_0.0.1.0_x64__whmwxsrdn33ha'
$support=$expected.SupportRoot
function Get-AppxPackage {param($Name) return $script:packages}
function Assert-NoReparse {param($Path)}
function Assert-Hash {param($Path,$Hash) if($script:hashMismatch){throw 'Fixture manifest hash mismatch'}}
function Remove-AppxPackage {param($Package) $script:removals++}
$script:hashMismatch=$false
function Package-Fixture($Location,$Development){[pscustomobject]@{Name=$name;Publisher=$publisher;PackageFamilyName=$family;PackageFullName=$full;InstallLocation=$Location;IsDevelopmentMode=$Development}}
foreach($fixture in @(
    (Package-Fixture 'C:\other-copy\Dev' $true),
    (Package-Fixture $support $false),
    (Package-Fixture $support 'True'),
    (Package-Fixture '' $true),
    (Package-Fixture 'relative\Dev' $true)
)){
    $script:packages=@($fixture)
    Assert-Rejected {Get-ExactPackage -RequireOwnedLocation|ForEach-Object{Remove-AppxPackage -Package $_.PackageFullName}}
}
$script:packages=@(Package-Fixture $support $true)
$script:hashMismatch=$true
Assert-Rejected {Get-ExactPackage -RequireOwnedLocation|ForEach-Object{Remove-AppxPackage -Package $_.PackageFullName}}
$script:hashMismatch=$false
if(@(Get-ExactPackage -RequireOwnedLocation).Count -ne 1){throw 'Valid owned package rejected.'}
$script:packages[0].Publisher='CN=Foreign'
Assert-Rejected {Get-ExactPackage -RequireOwnedLocation|ForEach-Object{Remove-AppxPackage -Package $_.PackageFullName}}
# Exercise the production rollback entry point, with transport-only config mocks.
# Both blockers must abort before any deletion/removal branch is reached.
$undo=$ast.Find({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Undo-ThisAttempt'},$true)
if(!$undo){throw 'Production rollback not found.'}
. ([scriptblock]::Create($undo.Extent.Text))
$ledger=@{registrationRequested=$true;targetAttempted=$true;copiedFiles=@();createdDirectories=@();phase='fixture'}
$expectedConfig=$expected
function Assert-ConfigOwned {
    $script:configReached=$true
    Assert-ExpectedRawConfig $script:configFixture 0 $expectedConfig
    $script:configGatePassed=$true
    # Never let a regression in validation reach the production native deletion.
    throw 'Fixture validation unexpectedly passed; mutation branch is disabled.'
}
$script:configFixture=$valid
$script:configReached=$false;$script:configGatePassed=$false
$script:packages=@(Package-Fixture 'C:\other-copy\Dev' $true)
Assert-Rejected {Undo-ThisAttempt}
if($script:configReached){throw 'Wrong-location package reached later rollback gates.'}
$script:packages=@()
foreach($entryIndex in 0..3){
    $script:configReached=$false;$script:configGatePassed=$false
    $script:configFixture=@(for($i=0;$i -lt 4;$i++){if($i -eq $entryIndex){[pscustomobject]@{Name=$valid[$i].Name;Kind=1;Bytes=[byte[]]($valid[$i].Bytes+[byte[]]@(0,0))}}else{$valid[$i]}})
    Assert-Rejected {Undo-ThisAttempt}
    if(!$script:configReached -or $script:configGatePassed){throw 'Malformed config was not rejected at the raw validation gate.'}
}
Write-Output "PASS: $script:checks malformed/foreign preservation gates, valid raw/partial config and owned package; zero mock removals, no live registry/deployment writes."
