# Proposed bounded SFTP live interoperability fixture

PLAN ONLY. No server/client has been run, key generated, listener started, service installed or trust/configuration changed for this plan. The current authorization excludes live network/credential setup. This is separate from the passing mock/controller fixtures and is not release preparation.

## Available executables and prerequisites

- Client: `C:\Windows\System32\OpenSSH\ssh.exe`, version 9.5.6.4, valid Microsoft Windows Authenticode signature, SHA256 `98D447730047F5DF91373E852D057928A921C3D1E89938700AB656BA84B19DC8`.
- Native Windows OpenSSH directory inventory has no `sshd.exe`.
- Existing server candidate: `C:\devkitPro\msys2\usr\bin\sshd.exe`, SHA256 `4375123BA81CDE2EE13A2EE2561D9263DCA76BFE27126EE14AAF613F95B5A004`. Local package database identifies openssh 10.0p1-2. The executable is unsigned; package authenticity/integrity has NOT been independently verified. Do not call it a verified trusted server yet.
- No owned sshd_config/live server harness was found in project artifacts. Docker/WSL client executables exist, but no daemon/distribution/image/server availability was established or started. They are alternatives requiring separate setup, not a silent fallback.

Before server execution, independently verify signed package provenance OR obtain explicit user approval to use this precisely identified unsigned installation with its matching local package record. Local mtree agreement is integrity correspondence, not independent cryptographic authenticity. No download/install is part of this plan. Do not run it without one of those trust/authorization bases. After approval, use a non-listening syntax/effective-config preflight against only the fixture config. Abort if this OpenSSH build needs elevation, a new Windows/service account, privilege/separation directories outside the fixture, service registration, global permissions/policies or firewall changes. Do not retry through an elevated/alternate execution context. MSYS username/path mapping and unprivileged public-key authentication are prerequisites to prove, not assumed working.

## Fixed proposed scope

Fresh root: `D:\codex-workspace\backslash-terminal\artifacts\sftp\live-interop-20261030-8b3b7a4f`. It must be absent and all ancestors non-reparse; create an ownership marker and exact inventory before writing anything else. Existing user files are never copied in.

Endpoint: IPv4 `127.0.0.1:48223` only. Recheck that it is unused immediately before start; stop on collision. No wildcard/IPv6/LAN bind, tunnel, forwarding, discovery, external endpoint or port substitution. Launch exactly one foreground server in an owned kill-on-close job; validate the listener belongs to that process tree and the local address is exactly 127.0.0.1. A 120-second watchdog and finally cleanup stop the server/session tree even on failure/cancellation.

Fixture subdirectories: `keys/`, `config/`, `local/`, `remote/`, `evidence/`, plus an owned portable test runtime if needed. Generate exactly two disposable Ed25519 keypairs (server host and fixture client) inside keys. Restrict only those newly created private files to the current owner as required; no existing ACL, user account or security policy is changed. No real private key, password, agent identity or user's .ssh/config/authorized_keys/known_hosts is read or modified.

Server config uses the explicit fixture HostKey, AuthorizedKeysFile, PidFile and log path. Public-key authentication only; password/keyboard-interactive authentication, agent/TCP/X11 forwarding, tunnels and TTY are disabled. Restrict to the verified current test-account mapping and force internal-sftp starting in remote/. Do not install a service or create persistent access. The owned authorized-key file is referenced only by this short-lived server. If these constraints cannot be validated, abort before binding.

`internal-sftp -d` is a start directory, NOT an OS filesystem jail. Operations in this plan are constrained to fixture paths by the harness and run with the existing account's permissions. If a server-side jail is required, use a separately approved isolated environment; do not weaken host security or claim a chroot without proving one.

Client config is explicit `-F none`, fixture IdentityFile, IdentityAgent=none, IdentitiesOnly=yes, GlobalKnownHostsFile=none and the fixture UserKnownHostsFile, with the production strict/no-multiplexing/no-forwarding policy. Construct known_hosts directly from the freshly generated host PUBLIC key for `[127.0.0.1]:48223`; no TOFU, keyscan or user trust-store update. Bind the fake saved SSH profile only in the owned portable runtime/model fixture, not the user's profiles.

## Bounded checks

1. Real protocol handshake/listing with UTF-8, spaces, ampersand and semicolon fixture names.
2. Upload and download bounded generated data (at most 1 MiB per file); compare SHA256 using only local/remote fixture files. Check progress, actual terminal-independent cwd/profile isolation and visible results.
3. Unknown and mismatched host-key rejection using separate fixture known_hosts files (empty and wrong PUBLIC key). Verify trust files remain unchanged. No bypass or automatic enrollment.
4. Existing local destination: Cancel preserves bytes; deliberate identity-held replacement changes only the selected fixture. Existing remote destination remains unchanged and requires a new filename.
5. Cancel after remote temporary CREATE/partial WRITE, report exact retained path and unconfirmed cleanup. Retry creates a new job/temp; no blind append. Interrupt only the owned server/session to test disconnect/deadline/publication uncertainty. Do not report an unacknowledged rename as known unpublished.
6. Production controller and, where a reviewed whole-app harness can safely inject/observe this fixture, view close/reopen, X/last-tab/Quit, multiple windows and background process residence. Do not use existing user windows/jobs or claim controller/container fixtures prove whole-app process teardown.

## Stop and cleanup

Stop accepting clients, close the owned job/child handles and wait for the exact recorded server PID/start time/executable to exit. Never stop a service, kill by process name, remove another listener or restart Explorer. Verify port 48223 has no remaining listener from the owned tree.

Delete generated private keys, fixture authorized_keys and known_hosts/config using the exact owned inventory, file identities/hashes and marker guards. Map only reported temporary remote paths inside remote/ to cleanup candidates after the server stops. Never wildcard-delete .partial files or follow reparse paths. Remove fixture data/empty directories only when ownership is proven; preserve/report unexpected collisions instead of recursive deletion. Keep sanitized hashes/results and non-secret fixture logs locally under evidence; no credentials/private key bytes in logs, Git or uploads. If cleanup is uncertain, report exact retained state and keep the server stopped. Deleting the fixture cannot affect user SSH access because no global trust/authorization file was changed.

## Smallest proposed approval

“May I use the existing unsigned MSYS2 OpenSSH 10.0p1-2 server (matching its local package record), perform a non-listening preflight and, only if unprivileged isolation succeeds, run it once on 127.0.0.1:48223 for up to two minutes, generate two throwaway keypairs under the named fixture directory, transfer only generated fixture files, and stop/remove that owned test setup afterward? No installs, services, account changes, elevation, firewall changes, real credentials or user SSH configuration.”

If the installed server cannot be verified or safely run under these constraints, the next approval is for a named trusted server runtime or an existing disposable endpoint/profile/directory, not permission to improvise around this plan.

Configuration references: https://man.openbsd.org/sshd_config and https://github.com/PowerShell/Win32-OpenSSH/wiki . The implementation/privilege behavior of the available MSYS2 server remains a preflight question.

## Read-only assessment completed before any execution

The installed server and its `sshd-auth.exe`, `sshd-session.exe` and `sftp-server.exe` all exactly match SHA256 entries in the installed openssh 10.0p1-2 compressed mtree. Server hash remains `4375123BA81CDE2EE13A2EE2561D9263DCA76BFE27126EE14AAF613F95B5A004`. No cached openssh package/archive signature was found for independent verification. The available public keyring alone cannot authenticate a missing archive/signature. No package manager, server or helper was executed to reach these conclusions.

The installed sshd manual describes privilege separation and a root-owned, non-writable empty directory. Its installed `/var/empty` maps to `C:\devkitPro\msys2\var\empty`, currently owned by BUILTIN\Administrators. That is not proof of safe MSYS2-to-Windows account/token mapping. The current execution token's read-only privilege list did not list SeCreateToken, SeAssignPrimaryToken or SeTcb; do not infer a different ordinary host token's capabilities or try another context to obtain them.

Unprivileged public-key login/isolation therefore remains unproven. A non-listening preflight and then an explicitly authorized loopback-only handshake are required to learn whether this installation works without new accounts, privileges or global directory changes. If it asks for any of those, stop; do not install/configure a Windows service or alter `/var/empty`, global sshd_config, user authorized_keys, policy or firewall. The smallest honest authorization names this unsigned, metadata-matching existing server and the preflight/abort conditions, rather than claiming it has already been independently authenticated or that unprivileged login is guaranteed.

## Exact proposed key generation and non-listening preflight

DO NOT RUN before the scoped user approval. Verified generator: `C:\Windows\System32\OpenSSH\ssh-keygen.exe`, version 9.5.6.4, valid Microsoft Windows signature (CN=Microsoft Windows, O=Microsoft Corporation), SHA256 `C9DC170CD161DB87E75EB6CF282F1325005B9E35628B9A36CB19883A8BE1A34F`. Recheck this signature/hash and the named server/helper hashes before execution; stop on changes.

After creating only the guarded fresh fixture root, marker and inventoried subdirectories, the exact PowerShell 7 commands would be:

```powershell
$PSNativeCommandArgumentPassing = 'Standard' # Process-local; preserves the empty -N argument.
$keygen = 'C:\Windows\System32\OpenSSH\ssh-keygen.exe'
$fixture = 'D:\codex-workspace\backslash-terminal\artifacts\sftp\live-interop-20261030-8b3b7a4f'
& $keygen -q -t ed25519 -N '' -C 'Backslash disposable host fixture' -f "$fixture\keys\host_ed25519"
& $keygen -q -t ed25519 -N '' -C 'Backslash disposable client fixture' -f "$fixture\keys\client_ed25519"
```

Each command requires zero exit status and only its two new key files. No agent import. Restrict only the newly generated private files to the current owner; stop if that cannot be done without broader permission changes. Build `keys/authorized_keys` from ONLY the new client public key and `config/known_hosts` from ONLY the new host public key for `[127.0.0.1]:48223`. Never use keyscan, existing user keys or a user's agent.

The owned `config/sshd_config` must contain the following effective values (paths deliberately use forward-slash drive syntax, not an assumed MSYS mount alias; abort if this build cannot resolve them):

```text
Port 48223
AddressFamily inet
UseDNS no
ListenAddress 127.0.0.1
HostKey D:/codex-workspace/backslash-terminal/artifacts/sftp/live-interop-20261030-8b3b7a4f/keys/host_ed25519
AuthorizedKeysFile D:/codex-workspace/backslash-terminal/artifacts/sftp/live-interop-20261030-8b3b7a4f/keys/authorized_keys
PidFile D:/codex-workspace/backslash-terminal/artifacts/sftp/live-interop-20261030-8b3b7a4f/config/sshd.pid
AllowUsers nemo
AuthenticationMethods publickey
PubkeyAuthentication yes
PasswordAuthentication no
KbdInteractiveAuthentication no
HostbasedAuthentication no
PermitEmptyPasswords no
PermitRootLogin no
PermitUserEnvironment no
PermitUserRC no
StrictModes yes
AllowAgentForwarding no
AllowTcpForwarding no
X11Forwarding no
PermitTunnel no
PermitTTY no
GatewayPorts no
MaxSessions 1
LoginGraceTime 10
Subsystem sftp internal-sftp
ForceCommand internal-sftp -d D:/codex-workspace/backslash-terminal/artifacts/sftp/live-interop-20261030-8b3b7a4f/remote -u 077
```

No Include directive or existing sshd_config is used. Login name `nemo` must map to the current local Windows test identity; abort if it does not or if domain/network account resolution would be required. These are the exact non-listening server preflight invocations, each in the owned job with a ten-second deadline and stderr captured only in fixture evidence:

```powershell
$sshd = 'C:\devkitPro\msys2\usr\bin\sshd.exe'
$cfg = 'D:/codex-workspace/backslash-terminal/artifacts/sftp/live-interop-20261030-8b3b7a4f/config/sshd_config'
& $sshd -e -t -f $cfg
& $sshd -e -T -f $cfg -C 'user=nemo,host=localhost,addr=127.0.0.1,laddr=127.0.0.1,lport=48223'
```

Stop immediately after either nonzero exit/timeout, unsupported security option, unsafe effective value, wrong account/path, or any request for elevation, extra account/privilege, user key/agent, global directory/configuration change or weakened StrictModes. Successful -t/-T is not proof of successful unprivileged authentication. Only after all checks AND the scoped live approval, the single foreground listener command would be `& $sshd -D -e -f $cfg`, subject to the 120-second job watchdog and verified loopback bind.

### Enforced request boundary, not a server jail

The standalone fixture runner must expose only `LIST('.')` and generated allowlisted leaf filenames (including its recorded random temporary names). Reject absolute paths, slash/backslash, NUL, `..`, symlinks/reparse entries and any operation not tied to the fixture inventory. No parent navigation, arbitrary UI path entry, remote command, directory creation outside the inventory or user-server action is allowed. Before ANY write, read and hash a uniquely generated remote ownership-marker leaf; a wrong starting directory/marker aborts without writes. Hold/verify the owned local fixture directory against reparse substitution.

These checks enforce OUR REQUESTS. `ForceCommand internal-sftp -d` only selects the start directory and does NOT jail the account's filesystem permissions. The short-lived loopback server would still run with the existing test identity's permissions, protected by the new owner-only client key. If server-side confinement is required, this plan is insufficient: use a separately approved isolated environment. If authentication requires elevation or access to a user's agent/keys, abort and clean up rather than falling back.

## Proposed ACL preflight revision — pending review, not executed

The 2026-10-30 run stopped at `Set-Acl -LiteralPath $private -AclObject $acl`, after `$acl.SetOwner($identity.User)`, protected DACL construction and owner-only FullControl. Error was an OS/.NET privilege exception (`SeSecurityPrivilege`), not an automatic approval-review denial. The saved result lacks a stack trace; the precise internal failing branch is inferred from provider source, not directly captured.

PowerShell's FileSystemProvider.SetSecurityDescriptor first copies AccessControlSections.All. Its PrivilegeNotHeld fallback excludes Audit only when no audit rules AND source AreAuditRulesProtected equals destination AreAccessRulesProtected. A newly constructed descriptor has unprotected audit rules; a protected destination DACL can therefore retain Audit in the fallback. The exact failed key's descriptor was not preserved, so this condition is a supported explanation rather than proven inspection of that deleted file. Explicit SetOwner was also unnecessary for an already current-user-owned generated file.

Source: https://github.com/PowerShell/PowerShell/blob/master/src/System.Management.Automation/namespaces/FileSystemSecurity.cs (SetSecurityDescriptor overloads). API: https://learn.microsoft.com/en-us/dotnet/api/system.io.filesystemaclextensions.setaccesscontrol?view=net-10.0 .

Proposed replacement below is REVIEW TEXT ONLY, not a command executed in this investigation. Run under the already identified PowerShell 7.6.9; no Set-Acl provider or privilege-enabling calls. Before it, retain existing exact path/hash/owner-marker/non-reparse guards. Open only the newly generated private file with a handle denying write/delete sharing; verify its owner is exactly current SID using the FileStream GetAccessControl overload (Access, Owner and Group; no Audit). Abort on mismatch; do not take ownership. Use FileStream overloads for subsequent read/write of security metadata to keep the verified object pinned.

```powershell
# $keyHandle is an exclusively pinned handle to the exact newly created private file.
$ownerBefore = [IO.FileSystemAclExtensions]::GetAccessControl($keyHandle)
if ($ownerBefore.GetOwner([Security.Principal.SecurityIdentifier]) -ne $identity.User) {
    throw 'Generated private key owner mismatch'
}
$dacl = [Security.AccessControl.FileSecurity]::new()
$dacl.SetAccessRuleProtection($true, $false)
$dacl.AddAccessRule([Security.AccessControl.FileSystemAccessRule]::new(
    $identity.User, [Security.AccessControl.FileSystemRights]::FullControl,
    [Security.AccessControl.AccessControlType]::Allow))
[IO.FileSystemAclExtensions]::SetAccessControl($keyHandle, $dacl)
# Read the default Access|Owner|Group back through the same handle. Require unchanged current owner,
# protected DACL, exactly one non-inherited Allow ACE for current SID, FullControl,
# no other entries, and unchanged content hash before server/key use.
```

Only AccessRulesModified is set on this fresh descriptor: no SetOwner, SetGroup, audit mutation or binary All-section transfer. The security scope is narrower than the original operation and keeps the same owner-only private-key protection. Do not request SeSecurityPrivilege, elevation, ownership changes, SACL access, inheritance changes on directories, or permission changes on existing files. An OS denial still aborts and triggers exact guarded cleanup; this proposal does not guarantee filesystem permission support before a reviewed retry.

Read-only/in-memory validation completed: PowerShell 7.6.9 has FileStream SetAccessControl overload; constructed binary descriptor has DiscretionaryAclPresent, DiscretionaryAclProtected, SelfRelative; owner/group/SACL absent; exactly one current-SID ACE, mask 2032127 (FullControl). No security-setting operation or new key/server execution occurred. Before retry, compile/parse revised harness and test descriptor construction/verification predicates entirely in memory (wrong owner, inherited/extra/deny ACE, unprotected DACL all refused). Only after review, exercise on one newly owned non-secret fixture file; verify before/after owner and exact DACL, then guarded cleanup. Do not resume server if that check fails.

The original fixed root now contains retained evidence, so a retry must explicitly preserve that evidence and use a newly reviewed unique fixture root with consistently regenerated config and ownership guards; do not bypass the existing-root refusal or overwrite the prior runner/result. Any revised plan hash must be reviewed and pinned by the new runner. Still no listener attempt has been consumed; original two-key budget was interrupted after the first key was generated and removed. The retry's key count/root must be stated explicitly for review before regeneration.

## Runnable revised preflight for review — not executed

Sources: `backslash/tools/backslash/sftp-fixture/{Invoke-Preflight.ps1,FixtureKeyAcl.cs,Runner.cpp,Test-KeyAclMemory.ps1}`. Default invocation only compiles the helper and runs in-memory predicates. Explicit `-RunPreflight` is required for any fixture/security/key/server-preflight operation; DO NOT run that switch before review. No listener code is present in this runner. The original executable runner remains hash-pinned and its source is copied alongside for review.

Fresh fixed root: `D:\codex-workspace\backslash-terminal\artifacts\sftp\live-interop-20261030-c4a79d12`, confirmed absent at preparation time. Prior `8b3b7a4f` evidence remains unchanged. This revision budgets exactly TWO replacement keypairs (host/client), following verified deletion of the original interrupted host pair. No extra key retries. Extract the same configuration above and substitute only the old fixture-root suffix with `c4a79d12`; retain port, account and all security options.

Before key generation, write exactly one non-secret `keys/acl-smoke.txt`, protect and verify it with the same production helper, then hash-guarded cleanup. On any error, stop and clean only inventoried unchanged owned files; never weaken protection or retry elevated. The helper opens GENERIC_READ|READ_CONTROL|WRITE_DAC (0x80060000), share=0, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, no WRITE_OWNER/ACCESS_SYSTEM_SECURITY. It refuses directory/reparse/multiple-hardlink objects, verifies current owner and content hash through the pinned handle, writes only DACL changes and checks unchanged volume/file index, owner, hash and exactly one protected explicit current-user FullControl allow ACE. Ancestors are checked for reparse points before/after. No descriptor is printed or key contents logged. GetAccessControl(FileStream) reads default Access/Owner/Group, not Audit. No SetOwner or Set-Acl remains in executable source.

Memory-only compile/predicate validation passed: correct descriptor, wrong SID, inherited ACE, unprotected DACL, extra ACE and deny ACE. The existing owner mismatch is checked before any mutation; actual filesystem operation and OS postcondition verification remain UNRUN. Script default path returned before creating the fresh root. This is a narrower DACL-only implementation of the previously approved owner-only-file protection, not expanded privilege authorization. No server handshake/listener or live acceptance is established by these preparation checks.
