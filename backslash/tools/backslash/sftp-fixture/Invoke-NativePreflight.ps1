param([switch]$RunPreflight)
$ErrorActionPreference='Stop'
if((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')).Hash -cne '88712FC4D5D3E3250641DF23EE6A4523CFD24E882C15215F97FDDA1EEB34A492'){throw 'ACL helper changed'}
Add-Type -Path (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')
[FixtureKeyAcl]::MemoryTests()
if(!$RunPreflight){'Memory-only validation passed. No filesystem ACL/key/server operations.';return}
if($PSVersionTable.PSVersion.Major -ne 7){throw 'Reviewed PowerShell 7 required'}
Add-Type -TypeDefinition @"
using System;using System.Runtime.InteropServices;
public static class NativeFixtureToken {
 [DllImport("advapi32.dll",SetLastError=true)]static extern bool OpenProcessToken(IntPtr p,uint a,out IntPtr t);
 [DllImport("advapi32.dll",SetLastError=true)]static extern bool GetTokenInformation(IntPtr t,int c,out int e,int n,out int r);
 [DllImport("kernel32.dll")]static extern IntPtr GetCurrentProcess();
 [DllImport("kernel32.dll")]static extern bool CloseHandle(IntPtr h);
 public static void RequireNonElevated(){IntPtr t;if(!OpenProcessToken(GetCurrentProcess(),8,out t))throw new Exception("Token query failed");try{int e,r;if(!GetTokenInformation(t,20,out e,4,out r)||e!=0)throw new Exception("Non-elevated token required");}finally{CloseHandle(t);}}
}
"@
[NativeFixtureToken]::RequireNonElevated()
# Approved bounded fixture preflight only; no listener is started by this script.
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest;$PSNativeCommandArgumentPassing='Standard';$PSNativeCommandUseErrorActionPreference=$false
$workspace='D:\codex-workspace\backslash-terminal';$root=Join-Path $workspace 'artifacts\sftp\native-preflight-20261030-6f2a1c93';$runner=Join-Path $workspace 'artifacts\sftp\live-runner-af8488db4b714d858903cbca3d059e13\Runner.exe';$owned=@{};$started=Get-Date;$result=[ordered]@{Phase='preflight';ListenerStarted=$false;GeneratedKeypairs=0;Commands=@();Cleanup=@();Error=$null}
function Guard([string]$path){$full=[IO.Path]::GetFullPath($path);if(!$full.StartsWith(($workspace+'\artifacts\sftp\'),[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture escaped workspace'};$cursor=$full;while($cursor){if(Test-Path -LiteralPath $cursor){if((Get-Item -Force -LiteralPath $cursor).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse path refused'}};$parent=Split-Path -Parent $cursor;if($parent -eq $cursor){break};$cursor=$parent}}
function Record([string]$path){Guard $path;if(Test-Path -LiteralPath $path -PathType Leaf){$owned[$path]=(Get-FileHash -LiteralPath $path).Hash}}
function OwnWrite([string]$path,[string]$text){Guard $path;if(Test-Path -LiteralPath $path){throw 'Refusing overwrite'};[IO.File]::WriteAllText($path,$text,[Text.UTF8Encoding]::new($false));Record $path}
function Run([string]$name,[string]$exe,[string[]]$arguments){$prefix=Join-Path $root "evidence\$name";& $runner $prefix 10000 $root $exe @arguments;$code=$LASTEXITCODE;Record ($prefix+'.stdout');Record ($prefix+'.stderr');$result.Commands+=@{Name=$name;ExitCode=$code};if($code){$detail=Get-Content -Raw -LiteralPath ($prefix+'.stderr');throw "$name failed ($code): $detail"}}
if((Get-FileHash -LiteralPath $runner).Hash -cne '858CF60453C70AB890DEF840756E683B13FBDD63F61FF66BD71DC86DF4DB55F6'){throw 'Runner changed'}
Guard $root;if(Test-Path -LiteralPath $root){throw 'Approved fixture root already exists'}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent();if($identity.Name -ine ($env:COMPUTERNAME+'\nemo')){throw 'Current local identity does not match approved fixture'}
$checks=@{'C:\Windows\System32\OpenSSH\ssh-keygen.exe'='C9DC170CD161DB87E75EB6CF282F1325005B9E35628B9A36CB19883A8BE1A34F';'C:\Windows\System32\OpenSSH\ssh.exe'='98D447730047F5DF91373E852D057928A921C3D1E89938700AB656BA84B19DC8';'D:\codex-workspace\backslash-terminal\artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sshd.exe'='00D61610A0965B42CC9A9CD401DAE8983FE77D5394697F074D776BB68CE6FF0A'}
foreach($entry in $checks.GetEnumerator()){if((Get-FileHash -LiteralPath $entry.Key).Hash -ine $entry.Value){throw 'Reviewed executable changed'}}
if((Get-AuthenticodeSignature -LiteralPath 'C:\Windows\System32\OpenSSH\ssh-keygen.exe').Status -ne 'Valid'){throw 'Key generator signature invalid'}
$configSource=Join-Path $PSScriptRoot 'native-sshd_config'
if((Get-FileHash -LiteralPath $configSource).Hash -cne '139D54F46D484EB9D6BE644B9E27BBCF3A6F0D8877C1EE0316A4ABE96F49EB3C'){throw 'Reviewed native config changed'}
$config=[IO.File]::ReadAllText($configSource)
$inventoryPath=Join-Path $workspace 'artifacts\sftp\native-server-inspect-20261030-d581a2c7\inventory.json'
if((Get-FileHash -LiteralPath $inventoryPath).Hash -cne '964EE31D09886815F2744058A89E81BCF59260A769A9D0DE67762750EE3182E8'){throw 'Native inventory changed'}
$nativeRoot=Join-Path $workspace 'artifacts\sftp\native-server-inspect-20261030-d581a2c7'
foreach($entry in (Get-Content -Raw -LiteralPath $inventoryPath|ConvertFrom-Json)) {
 if($entry.Path -match '\.(exe|dll)$') {
  $path=Join-Path $nativeRoot $entry.Path
  if((Get-FileHash -LiteralPath $path).Hash -cne $entry.SHA256 -or (Get-AuthenticodeSignature -LiteralPath $path).Status -ne 'Valid'){throw 'Native executable dependency changed'}
 }
}
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
 $cfg=(Join-Path $root 'config\sshd_config').Replace('\','/');Run 'sshd-syntax' 'D:\codex-workspace\backslash-terminal\artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sshd.exe' @('-e','-t','-f',$cfg)
 Run 'sshd-effective' 'D:\codex-workspace\backslash-terminal\artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sshd.exe' @('-e','-T','-f',$cfg,'-C','user=nemo,host=localhost,addr=127.0.0.1,laddr=127.0.0.1,lport=48223')
 $result.Phase='preflight-passed-needs-effective-config-validation';
}catch{$result.Error=$_.Exception.Message;$result.Phase='stopped'}finally{
 # This script never starts a listener. Remove exact generated keys/config only.
 foreach($entry in @($owned.GetEnumerator())){if($entry.Key -like ($root+'\keys\*') -or $entry.Key -like ($root+'\config\*')){Guard $entry.Key;if(Test-Path -LiteralPath $entry.Key){if((Get-FileHash -LiteralPath $entry.Key).Hash -eq $entry.Value){Remove-Item -LiteralPath $entry.Key;$result.Cleanup+=@{Path=$entry.Key;Removed=$true}}else{$result.Cleanup+=@{Path=$entry.Key;Removed=$false;Reason='changed bytes'}}}}}
 foreach($name in 'keys','config','local','remote'){ $dir=Join-Path $root $name;Guard $dir;if((Test-Path -LiteralPath $dir) -and @(Get-ChildItem -LiteralPath $dir -Force).Count -eq 0){Remove-Item -LiteralPath $dir}}
 if(Test-Path -LiteralPath (Join-Path $root 'evidence')){$result|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'evidence\preflight-result.json')}
}
[pscustomobject]@{Phase=$result.Phase;GeneratedKeypairs=$result.GeneratedKeypairs;Error=$result.Error;ListenerStarted=$false;Evidence=(Join-Path $root 'evidence\preflight-result.json')}|ConvertTo-Json




