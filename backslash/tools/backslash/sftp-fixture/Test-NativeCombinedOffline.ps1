# Offline-only tests; no keys, server or filesystem security writes.
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'NativeFixturePolicy.ps1')
$root='D:\codex-workspace\backslash-terminal\artifacts\sftp\native-combined-20261030-71d4b908'
$lines=@(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'native-combined-sshd_config') | Where-Object {$_} | ForEach-Object {$parts=$_ -split ' ',2;$key=$parts[0].ToLowerInvariant();$value=$parts[1];if($key -eq 'listenaddress'){$value+=':48223'};if($key -eq 'subsystem'){$value='sftp internal-sftp'};if($key -eq 'forcecommand'){$value=$parts[1]};"$key $value"})
Assert-NativeEffectiveConfig $lines $root
function Refused([string[]]$bad){$failed=$false;try{Assert-NativeEffectiveConfig $bad $root}catch{$failed=$true};if(!$failed){throw 'Unsafe effective config accepted'}}
Refused @($lines | Where-Object {$_ -notmatch '^strictmodes '})
Refused @($lines+'strictmodes yes')
Refused @($lines -replace '^listenaddress .*','listenaddress 0.0.0.0:48223')
Refused @($lines -replace '^passwordauthentication .*','passwordauthentication yes')
Refused @($lines -replace '^authorizedkeysfile .*','authorizedkeysfile C:/Users/nemo/.ssh/authorized_keys')
Refused @($lines -replace '^forcecommand .*','forcecommand none')
& (Join-Path $PSScriptRoot 'Test-KeyAclMemory.ps1')
'PASS: offline effective config allows exact fixture, refuses missing/duplicate/unsafe settings; ACL predicates passed.'
