param([switch]$RunFixture)
if((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'NativeAbsolutePolicy.ps1')).Hash -cne '4CDADD9B2F029A5D25DEDE4D628663C0FD5BB71AF337DDC26DA06F5242D8C482'){throw 'Effective config policy changed'}
. (Join-Path $PSScriptRoot 'NativeAbsolutePolicy.ps1')
$ErrorActionPreference='Stop'
if((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')).Hash -cne '88712FC4D5D3E3250641DF23EE6A4523CFD24E882C15215F97FDDA1EEB34A492'){throw 'ACL helper changed'}
Add-Type -Path (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')
[FixtureKeyAcl]::MemoryTests()
if(!$RunFixture){'Memory-only validation passed. No filesystem ACL/key/server operations.';return}
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
# Approved opt-in combined fixture; one owned listener after validated preflight only.
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest;$PSNativeCommandArgumentPassing='Standard';$PSNativeCommandUseErrorActionPreference=$false
$workspace='D:\codex-workspace\backslash-terminal';$root=Join-Path $workspace 'artifacts\sftp\native-absolute-20261031-f3407b61';$runner=Join-Path $workspace 'artifacts\sftp\live-runner-af8488db4b714d858903cbca3d059e13\Runner.exe';$owned=@{};$started=Get-Date;$result=[ordered]@{Phase='preflight';ListenerStarted=$false;GeneratedKeypairs=0;Commands=@();Cleanup=@();Error=$null}
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
$configSource=Join-Path $PSScriptRoot 'native-absolute-sshd_config'
if((Get-FileHash -LiteralPath $configSource).Hash -cne '41CCDAA9DDE6B7DAF8B2F5F03635ED659F6DF28C4D713D7A35AB97C58C12AC74'){throw 'Reviewed native config changed'}
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
$sftpHelper=Join-Path $workspace 'artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sftp-server.exe'
if((Get-FileHash -LiteralPath $sftpHelper).Hash -cne '5F5DF08DFA1FB659D0F6EB96F0F942C2BD0852670BA79FF9107578142568E2CF'){throw 'Reviewed absolute SFTP helper changed'}
$helperSignature=Get-AuthenticodeSignature -LiteralPath $sftpHelper
if($helperSignature.Status -ne 'Valid' -or $helperSignature.SignerCertificate.Subject -cne 'CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US'){throw 'Absolute SFTP helper signature mismatch'}
$batch=Join-Path $workspace 'artifacts\sftp\native-absolute-build-20261031-f3407b61\batch.exe'
if((Get-FileHash -LiteralPath $batch).Hash -cne 'C5CA6710A79A49D1DABEE20B15C1EDC53865FF9BCC197C06B9557DA047A79B08'){throw 'Reviewed batch changed'}
foreach($endpoint in [Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveTcpListeners()){if($endpoint.Port -eq 48223){throw 'Fixture port already in use'}}
try{
 New-Item -ItemType Directory -Path $root|Out-Null;foreach($name in 'keys','config','local','remote','evidence'){New-Item -ItemType Directory -Path (Join-Path $root $name)|Out-Null};OwnWrite (Join-Path $root 'owner.txt') 'Backslash native absolute fixture f3407b61'
 $smoke=Join-Path $root 'keys\acl-smoke.txt';OwnWrite $smoke 'Backslash non-secret ACL smoke c4a79d12'
 [FixtureKeyAcl]::Protect($smoke,$root,$owned[$smoke])
 Guard $smoke;if((Get-FileHash -LiteralPath $smoke).Hash -ne $owned[$smoke]){throw 'Smoke cleanup hash mismatch'}
 Remove-Item -LiteralPath $smoke;$owned.Remove($smoke)
 foreach($type in 'host','client'){$private=Join-Path $root "keys\${type}_ed25519";try{Run ("keygen-"+$type) 'C:\Windows\System32\OpenSSH\ssh-keygen.exe' @('-q','-t','ed25519','-N','','-C',("Backslash disposable "+$type+" fixture"),'-f',$private)}finally{Record $private;Record ($private+'.pub')};[FixtureKeyAcl]::Protect($private,$root,$owned[$private]);$result.GeneratedKeypairs++}
 OwnWrite (Join-Path $root 'keys\authorized_keys') ([IO.File]::ReadAllText((Join-Path $root 'keys\client_ed25519.pub')))
 $authorization=Join-Path $root 'keys\authorized_keys'
 [FixtureKeyAcl]::Protect($authorization,$root,$owned[$authorization])
 $result['AuthorizationFileProtected']=$true
 $hostKey=([IO.File]::ReadAllText((Join-Path $root 'keys\host_ed25519.pub'))).Trim().Split(' ');OwnWrite (Join-Path $root 'config\known_hosts') ("[127.0.0.1]:48223 "+$hostKey[0]+' '+$hostKey[1]+"`n")
 OwnWrite (Join-Path $root 'config\sshd_config') $config
 $cfg=(Join-Path $root 'config\sshd_config').Replace('\','/');Run 'sshd-syntax' 'D:\codex-workspace\backslash-terminal\artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sshd.exe' @('-e','-t','-f',$cfg)
 Run 'sshd-effective' 'D:\codex-workspace\backslash-terminal\artifacts\sftp\native-server-inspect-20261030-d581a2c7\layout\OpenSSH-Win64\sshd.exe' @('-e','-T','-f',$cfg,'-C','user=nemo,host=localhost,addr=127.0.0.1,laddr=127.0.0.1,lport=48223')
 # Validate every critical effective value BEFORE listener launch. Exact/no duplicate values.
 Assert-NativeEffectiveConfig -Lines (Get-Content -LiteralPath (Join-Path $root 'evidence/sshd-effective.stdout')) -root $root
 OwnWrite (Join-Path $root 'remote/owner.txt') 'Backslash native absolute fixture f3407b61'
 $source=Join-Path $root 'local/source.bin';Guard $source;if(Test-Path $source){throw 'Source collision'}
 $bytes=[byte[]]::new(65536);for($i=0;$i -lt $bytes.Length;$i++){$bytes[$i]=[byte]($i%256)}
 [IO.File]::WriteAllBytes($source,$bytes);Record $source;$dataHash=$owned[$source];$markerHash=$owned[(Join-Path $root 'remote/owner.txt')]
 $prefix=Join-Path $root 'evidence/batch';$result['ListenerAttempted']=$true
 try{& $runner $prefix 120000 $root $batch '--run-reviewed-single-connection';$code=$LASTEXITCODE;$result.Commands+=@{Name='single-connection-batch';ExitCode=$code};if($code){throw "Batch failed ($code); inspect owned evidence"};$result.Phase='single-connection-integrity-passed'}finally{
  if(Test-Path ($prefix+'.stdout')){$result.ListenerStarted=([IO.File]::ReadAllText($prefix+'.stdout').Contains('LISTENER_BOUND_127.0.0.1_48223'));$result['TeardownConfirmed']=([IO.File]::ReadAllText($prefix+'.stdout').Contains('confirmed=1'))}
  foreach($pair in @(@('local/marker-check.txt',$markerHash),@('local/download.bin',$dataHash),@('remote/uploaded.bin',$dataHash))){$path=Join-Path $root $pair[0];Guard $path;if(Test-Path -LiteralPath $path){if((Get-FileHash -LiteralPath $path).Hash -cne $pair[1]){$result.Cleanup+=@{Path=$path;Removed=$false;Reason='unexpected bytes retained'}}else{Record $path}}}
 }
}catch{$result.Error=$_.Exception.Message;$result.Phase='stopped'}finally{
 # Batch owns and stops its one listener; remove only inventoried unchanged fixture files.
 foreach($entry in @($owned.GetEnumerator())){if($entry.Key -like ($root+'\keys\*') -or $entry.Key -like ($root+'\config\*') -or $entry.Key -like ($root+'\local\*') -or $entry.Key -like ($root+'\remote\*')){Guard $entry.Key;if(Test-Path -LiteralPath $entry.Key){if((Get-FileHash -LiteralPath $entry.Key).Hash -eq $entry.Value){Remove-Item -LiteralPath $entry.Key;$result.Cleanup+=@{Path=$entry.Key;Removed=$true}}else{$result.Cleanup+=@{Path=$entry.Key;Removed=$false;Reason='changed bytes'}}}}}
 foreach($name in 'keys','config','local','remote'){ $dir=Join-Path $root $name;Guard $dir;if((Test-Path -LiteralPath $dir) -and @(Get-ChildItem -LiteralPath $dir -Force).Count -eq 0){Remove-Item -LiteralPath $dir}}
 $result['RetainedFiles']=@(Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object {$_.FullName.Substring($root.Length+1)})
 if(Test-Path -LiteralPath (Join-Path $root 'evidence')){$result|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'evidence\preflight-result.json')}
}
[pscustomobject]@{Phase=$result.Phase;GeneratedKeypairs=$result.GeneratedKeypairs;Error=$result.Error;ListenerStarted=$result.ListenerStarted;Evidence=(Join-Path $root 'evidence\preflight-result.json')}|ConvertTo-Json











