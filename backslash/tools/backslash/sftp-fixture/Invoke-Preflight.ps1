param([switch]$RunPreflight)
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')
[FixtureKeyAcl]::MemoryTests()
if(!$RunPreflight){'Memory-only validation passed. No filesystem ACL/key/server operations.';return}
if($PSVersionTable.PSVersion.Major -ne 7){throw 'Reviewed PowerShell 7 required'}
# Approved bounded fixture preflight only; no listener is started by this script.
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest;$PSNativeCommandArgumentPassing='Standard';$PSNativeCommandUseErrorActionPreference=$false
$workspace='D:\codex-workspace\backslash-terminal';$root=Join-Path $workspace 'artifacts\sftp\live-interop-20261030-c4a79d12';$runner=Join-Path $workspace 'artifacts\sftp\live-runner-af8488db4b714d858903cbca3d059e13\Runner.exe';$owned=@{};$started=Get-Date;$result=[ordered]@{Phase='preflight';ListenerStarted=$false;GeneratedKeypairs=0;Commands=@();Cleanup=@();Error=$null}
function Guard([string]$path){$full=[IO.Path]::GetFullPath($path);if(!$full.StartsWith(($workspace+'\artifacts\sftp\'),[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture escaped workspace'};$cursor=$full;while($cursor){if(Test-Path -LiteralPath $cursor){if((Get-Item -Force -LiteralPath $cursor).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse path refused'}};$parent=Split-Path -Parent $cursor;if($parent -eq $cursor){break};$cursor=$parent}}
function Record([string]$path){Guard $path;if(Test-Path -LiteralPath $path -PathType Leaf){$owned[$path]=(Get-FileHash -LiteralPath $path).Hash}}
function OwnWrite([string]$path,[string]$text){Guard $path;if(Test-Path -LiteralPath $path){throw 'Refusing overwrite'};[IO.File]::WriteAllText($path,$text,[Text.UTF8Encoding]::new($false));Record $path}
function Run([string]$name,[string]$exe,[string[]]$arguments){$prefix=Join-Path $root "evidence\$name";& $runner $prefix 10000 $root $exe @arguments;$code=$LASTEXITCODE;Record ($prefix+'.stdout');Record ($prefix+'.stderr');$result.Commands+=@{Name=$name;ExitCode=$code};if($code){$detail=Get-Content -Raw -LiteralPath ($prefix+'.stderr');throw "$name failed ($code): $detail"}}
if((Get-FileHash -LiteralPath $runner).Hash -cne '858CF60453C70AB890DEF840756E683B13FBDD63F61FF66BD71DC86DF4DB55F6'){throw 'Runner changed'}
Guard $root;if(Test-Path -LiteralPath $root){throw 'Approved fixture root already exists'}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent();if($identity.Name -ine ($env:COMPUTERNAME+'\nemo')){throw 'Current local identity does not match approved fixture'}
$checks=@{'C:\Windows\System32\OpenSSH\ssh-keygen.exe'='C9DC170CD161DB87E75EB6CF282F1325005B9E35628B9A36CB19883A8BE1A34F';'C:\Windows\System32\OpenSSH\ssh.exe'='98D447730047F5DF91373E852D057928A921C3D1E89938700AB656BA84B19DC8';'C:\devkitPro\msys2\usr\bin\sshd.exe'='4375123BA81CDE2EE13A2EE2561D9263DCA76BFE27126EE14AAF613F95B5A004';'C:\devkitPro\msys2\usr\lib\ssh\sshd-auth.exe'='292553E0468F8E728CA9A67486653F46809E0EAEc864E2C9F06EE7091EA24D80';'C:\devkitPro\msys2\usr\lib\ssh\sshd-session.exe'='952328BA5F59F1D630F89E236BB76C3FC86503F4583B664ABA303A98909585CB'}
foreach($entry in $checks.GetEnumerator()){if((Get-FileHash -LiteralPath $entry.Key).Hash -ine $entry.Value){throw 'Reviewed executable changed'}}
if((Get-AuthenticodeSignature -LiteralPath 'C:\Windows\System32\OpenSSH\ssh-keygen.exe').Status -ne 'Valid'){throw 'Key generator signature invalid'}
$plan=Join-Path $workspace 'docs\dev\SFTP-LIVE-INTEROP-PLAN.md';if((Get-FileHash -LiteralPath $plan).Hash -cne 'BCF30F648862C9594E3824839B4A5C1408574D15A19187EF1852D305F912EE7B'){throw 'Reviewed plan changed'}
$config=[regex]::Match((Get-Content -Raw -LiteralPath $plan),'(?s)```text\r?\n(Port 48223.*?)```').Groups[1].Value;if(!$config){throw 'Exact config not found'};$config=$config.Replace('live-interop-20261030-8b3b7a4f','live-interop-20261030-c4a79d12')
try{
 New-Item -ItemType Directory -Path $root|Out-Null;foreach($name in 'keys','config','local','remote','evidence'){New-Item -ItemType Directory -Path (Join-Path $root $name)|Out-Null};OwnWrite (Join-Path $root 'owner.txt') 'Backslash live fixture 20261030-8b3b7a4f'
 $smoke=Join-Path $root 'keys\acl-smoke.txt';OwnWrite $smoke 'Backslash non-secret ACL smoke c4a79d12'
 [FixtureKeyAcl]::Protect($smoke,$root,$owned[$smoke])
 Guard $smoke;if((Get-FileHash -LiteralPath $smoke).Hash -ne $owned[$smoke]){throw 'Smoke cleanup hash mismatch'}
 Remove-Item -LiteralPath $smoke;$owned.Remove($smoke)
 foreach($type in 'host','client'){$private=Join-Path $root "keys\${type}_ed25519";try{Run ("keygen-"+$type) 'C:\Windows\System32\OpenSSH\ssh-keygen.exe' @('-q','-t','ed25519','-N','','-C',("Backslash disposable "+$type+" fixture"),'-f',$private)}finally{Record $private;Record ($private+'.pub')};[FixtureKeyAcl]::Protect($private,$root,$owned[$private]);$result.GeneratedKeypairs++}
 OwnWrite (Join-Path $root 'keys\authorized_keys') ([IO.File]::ReadAllText((Join-Path $root 'keys\client_ed25519.pub')))
 $hostKey=([IO.File]::ReadAllText((Join-Path $root 'keys\host_ed25519.pub'))).Trim().Split(' ');OwnWrite (Join-Path $root 'config\known_hosts') ("[127.0.0.1]:48223 "+$hostKey[0]+' '+$hostKey[1]+"`n")
 OwnWrite (Join-Path $root 'config\sshd_config') $config
 $cfg=(Join-Path $root 'config\sshd_config').Replace('\','/');Run 'sshd-syntax' 'C:\devkitPro\msys2\usr\bin\sshd.exe' @('-e','-t','-f',$cfg)
 Run 'sshd-effective' 'C:\devkitPro\msys2\usr\bin\sshd.exe' @('-e','-T','-f',$cfg,'-C','user=nemo,host=localhost,addr=127.0.0.1,laddr=127.0.0.1,lport=48223')
 $result.Phase='preflight-passed-needs-effective-config-validation';
}catch{$result.Error=$_.Exception.Message;$result.Phase='stopped'}finally{
 # This script never starts a listener. Remove exact generated keys/config only.
 foreach($entry in @($owned.GetEnumerator())){if($entry.Key -like ($root+'\keys\*') -or $entry.Key -like ($root+'\config\*')){Guard $entry.Key;if(Test-Path -LiteralPath $entry.Key){if((Get-FileHash -LiteralPath $entry.Key).Hash -eq $entry.Value){Remove-Item -LiteralPath $entry.Key;$result.Cleanup+=@{Path=$entry.Key;Removed=$true}}else{$result.Cleanup+=@{Path=$entry.Key;Removed=$false;Reason='changed bytes'}}}}}
 foreach($name in 'keys','config','local','remote'){ $dir=Join-Path $root $name;Guard $dir;if((Test-Path -LiteralPath $dir) -and @(Get-ChildItem -LiteralPath $dir -Force).Count -eq 0){Remove-Item -LiteralPath $dir}}
 if(Test-Path -LiteralPath (Join-Path $root 'evidence')){$result|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'evidence\preflight-result.json')}
}
[pscustomobject]@{Phase=$result.Phase;GeneratedKeypairs=$result.GeneratedKeypairs;Error=$result.Error;ListenerStarted=$false;Evidence=(Join-Path $root 'evidence\preflight-result.json')}|ConvertTo-Json



