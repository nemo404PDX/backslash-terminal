# These are fixed fixture constants, NOT arbitrary accepted shell text.
# v10.4.0.0 w32-doexec wraps the full command in cmd /c double quotes.
# No inner quote/escape is required or allowed for this fixed no-space alphabet.
function Assert-NativeShellConstant([string]$value) {
 if($value -cnotmatch '\A[A-Za-z]:/[A-Za-z0-9._/-]+\z' -or (($value -split '/') -contains '..') -or (($value -split '/') -contains '.') -or $value.Contains('//')){throw 'Unsafe fixture shell constant'}
}
function Assert-NativeEffectiveConfig {
param([string[]]$Lines,[string]$root)
 $helper='D:/codex-workspace/backslash-terminal/artifacts/sftp/native-server-inspect-20261030-d581a2c7/layout/OpenSSH-Win64/sftp-server.exe'
 Assert-NativeShellConstant $helper
 Assert-NativeShellConstant ($root.Replace('\','/')+'/remote')
 $effective=@{};foreach($line in $Lines){if($line -match '^(\S+)\s+(.*)$'){$key=$Matches[1].ToLowerInvariant();if(!$effective.ContainsKey($key)){$effective[$key]=@()};$effective[$key]+=,$Matches[2]}}
 $required=@{port='48223';addressfamily='inet';listenaddress='127.0.0.1:48223';allowusers='nemo';authenticationmethods='publickey';pubkeyauthentication='yes';passwordauthentication='no';kbdinteractiveauthentication='no';hostbasedauthentication='no';permitemptypasswords='no';permitrootlogin='no';permituserenvironment='no';permituserrc='no';strictmodes='yes';allowagentforwarding='no';allowtcpforwarding='no';x11forwarding='no';permittunnel='no';permittty='no';gatewayports='no';maxsessions='1';logingracetime='10';hostkey=($root.Replace('\','/')+'/keys/host_ed25519');authorizedkeysfile=($root.Replace('\','/')+'/keys/authorized_keys');pidfile=($root.Replace('\','/')+'/config/sshd.pid');subsystem=('sftp '+$helper+' -d '+$root.Replace('\','/')+'/remote -u 077');forcecommand=($helper+' -d '+$root.Replace('\','/')+'/remote -u 077')}
 foreach($entry in $required.GetEnumerator()){if(!$effective.ContainsKey($entry.Key) -or $effective[$entry.Key].Count -ne 1 -or $effective[$entry.Key][0] -cne $entry.Value){throw ('Unsafe/unsupported effective setting: '+$entry.Key)}}

}




