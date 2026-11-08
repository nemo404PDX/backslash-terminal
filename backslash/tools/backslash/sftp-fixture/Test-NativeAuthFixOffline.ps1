# Offline source-order and in-memory postcondition tests; no ACL writes/keys/network.
$ErrorActionPreference='Stop'
$scriptPath=Join-Path $PSScriptRoot 'Invoke-NativeAuthFix.ps1'
$t=$null;$e=$null;$ast=[Management.Automation.Language.Parser]::ParseFile($scriptPath,[ref]$t,[ref]$e)
if($e){throw 'Parser error'}
$text=[IO.File]::ReadAllText($scriptPath)
$create=$text.IndexOf("OwnWrite (Join-Path `$root 'keys\authorized_keys')")
$protect=$text.IndexOf('[FixtureKeyAcl]::Protect($authorization,$root,$owned[$authorization])')
$mark=$text.IndexOf("`$result['AuthorizationFileProtected']=`$true")
$syntax=$text.IndexOf("Run 'sshd-syntax'")
$batch=$text.IndexOf("& `$runner `$prefix 120000")
if($create -lt 0 -or $protect -le $create -or $mark -le $protect -or $syntax -le $mark -or $batch -le $syntax){throw 'Authorization protection must succeed before sshd/batch'}
if(([regex]::Matches($text,[regex]::Escape('[FixtureKeyAcl]::Protect($authorization,$root,$owned[$authorization])'))).Count -ne 1){throw 'Unexpected protection call count'}
if($text -match 'Set-Acl|SetOwner|StrictModes\s+no|ExecutionPolicy|require_escalated'){throw 'Unexpected security scope'}
if(!(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'native-authfix-sshd_config') | Where-Object {$_ -ceq 'StrictModes yes'})){throw 'StrictModes weakened'}
$helper=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'FixtureKeyAcl.cs'))
if(([regex]::Matches($helper,'Guard\(path,root\)')).Count -ne 2 -or !$helper.Contains('Exact(after,sid)') -or !$helper.Contains('Owner mismatch') -or !$helper.Contains('Content changed before ACL') -or !$helper.Contains('before.IndexLow!=end.IndexLow')){throw 'Guarded helper postconditions missing'}
& (Join-Path $PSScriptRoot 'Test-KeyAclMemory.ps1')
# Predicate tests operate on in-memory descriptors, including invalid inherited/extra ACEs.
$root='D:\codex-workspace\backslash-terminal\artifacts\sftp\native-authfix-20261031-8a63e024'
[FixtureKeyAcl]::Guard(($root+'\keys\authorized_keys'),$root)
$refused=$false;try{[FixtureKeyAcl]::Guard('C:\Users\nemo\.ssh\authorized_keys',$root)}catch{$refused=$true};if(!$refused){throw 'Personal path not refused'}
'PASS: protected authorization ordering, unchanged StrictModes, guarded content/identity/ACL postconditions and memory negative predicates. No security writes.'
