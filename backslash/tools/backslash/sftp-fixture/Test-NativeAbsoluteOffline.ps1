# Offline source-order and in-memory postcondition tests; no ACL writes/keys/network.
$ErrorActionPreference='Stop'
$scriptPath=Join-Path $PSScriptRoot 'Invoke-NativeAbsolute.ps1'
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
if(!(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'native-absolute-sshd_config') | Where-Object {$_ -ceq 'StrictModes yes'})){throw 'StrictModes weakened'}
$helper=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'FixtureKeyAcl.cs'))
if(([regex]::Matches($helper,'Guard\(path,root\)')).Count -ne 2 -or !$helper.Contains('Exact(after,sid)') -or !$helper.Contains('Owner mismatch') -or !$helper.Contains('Content changed before ACL') -or !$helper.Contains('before.IndexLow!=end.IndexLow')){throw 'Guarded helper postconditions missing'}
& (Join-Path $PSScriptRoot 'Test-KeyAclMemory.ps1')
# Predicate tests operate on in-memory descriptors, including invalid inherited/extra ACEs.
$root='D:\codex-workspace\backslash-terminal\artifacts\sftp\native-absolute-20261031-f3407b61'
[FixtureKeyAcl]::Guard(($root+'\keys\authorized_keys'),$root)
$refused=$false;try{[FixtureKeyAcl]::Guard('C:\Users\nemo\.ssh\authorized_keys',$root)}catch{$refused=$true};if(!$refused){throw 'Personal path not refused'}
'PASS: protected authorization ordering, unchanged StrictModes, guarded content/identity/ACL postconditions and memory negative predicates. No security writes.'

. (Join-Path $PSScriptRoot 'NativeAbsolutePolicy.ps1')
$cfg=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'native-absolute-sshd_config')|Where-Object {$_}
$lines=@($cfg|ForEach-Object{$parts=$_ -split ' ',2;$key=$parts[0].ToLowerInvariant();$value=$parts[1];if($key -eq 'listenaddress'){$value+=':48223'};"$key $value"})
Assert-NativeEffectiveConfig $lines $root
function Refused([string[]]$bad){$failed=$false;try{Assert-NativeEffectiveConfig $bad $root}catch{$failed=$true};if(!$failed){throw 'Unsafe effective config accepted'}}
Refused @($lines -replace '^forcecommand .*','forcecommand internal-sftp')
Refused @($lines -replace '^subsystem .*','subsystem sftp sftp-server.exe')
Refused @($lines -replace '/sftp-server.exe','/other.exe')
Refused @($lines -replace ' -u 077',' -u 000')
Refused @($lines -replace '^forcecommand .*','forcecommand none')
Refused @($lines+'subsystem sftp ignored')
Refused @($lines -replace '/remote','/../remote')
Refused @($lines -replace '^strictmodes .*','strictmodes no')
foreach($bad in @('sftp-server.exe','C:/path with space/helper.exe','C:/x&whoami','C:/x|whoami','C:/x"y','C:/%PATH%/x','C:/!PATH!/x','C:/../x','C:/x/..',("C:/x"+[char]0),("C:/x`n"))){$failed=$false;try{Assert-NativeShellConstant $bad}catch{$failed=$true};if(!$failed){throw 'Unsafe shell constant accepted'}}
# Source-matched w32-doexec: internal-sftp rewrite applies only at start; absolute path stays intact.
$forced=($lines|Where-Object {$_ -cmatch '^forcecommand '}).Substring(13)
if($forced.StartsWith('internal-sftp') -or $forced.Contains('"') -or $forced.Contains('&')){throw 'Absolute command not safe for reviewed cmd wrapper'}
$wrapped='"c:\windows\system32\cmd.exe" /c "'+$forced+'"'
if(($wrapped.ToCharArray()|Where-Object {$_ -eq '"'}).Count -ne 4){throw 'Unexpected shell quote structure'}
if(!$text.Contains('5F5DF08DFA1FB659D0F6EB96F0F942C2BD0852670BA79FF9107578142568E2CF') -or !$text.Contains('Absolute SFTP helper signature mismatch')){throw 'Explicit helper pin missing'}
'PASS: absolute helper/effective config, exact shell alphabet/wrapper, traversal/metacharacter/foreign-helper/duplicate/security negative cases. No shell or network executed.'
