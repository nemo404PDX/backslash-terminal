function Assert-NativeEffectiveConfig {
param([string[]]$Lines,[string]$root)
 $effective=@{};foreach($line in $Lines){if($line -match '^(\S+)\s+(.*)$'){$key=$Matches[1].ToLowerInvariant();if(!$effective.ContainsKey($key)){$effective[$key]=@()};$effective[$key]+=,$Matches[2]}}
 $required=@{port='48223';addressfamily='inet';listenaddress='127.0.0.1:48223';allowusers='nemo';authenticationmethods='publickey';pubkeyauthentication='yes';passwordauthentication='no';kbdinteractiveauthentication='no';hostbasedauthentication='no';permitemptypasswords='no';permitrootlogin='no';permituserenvironment='no';permituserrc='no';strictmodes='yes';allowagentforwarding='no';allowtcpforwarding='no';x11forwarding='no';permittunnel='no';permittty='no';gatewayports='no';maxsessions='1';logingracetime='10';hostkey=($root.Replace('\','/')+'/keys/host_ed25519');authorizedkeysfile=($root.Replace('\','/')+'/keys/authorized_keys');pidfile=($root.Replace('\','/')+'/config/sshd.pid');subsystem='sftp internal-sftp';forcecommand=('internal-sftp -d '+$root.Replace('\','/')+'/remote -u 077')}
 foreach($entry in $required.GetEnumerator()){if(!$effective.ContainsKey($entry.Key) -or $effective[$entry.Key].Count -ne 1 -or $effective[$entry.Key][0] -cne $entry.Value){throw ('Unsafe/unsupported effective setting: '+$entry.Key)}}

}

