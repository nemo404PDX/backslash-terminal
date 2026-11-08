# Native authorization-file correction — offline candidate

Cause: diagnostic server rejected the newly generated fixture authorized_keys because Authenticated Users had unacceptable permissions. Positive expected host-key match occurred, but no user authentication/SFTP/transfer succeeded. This is a fixture-only setup correction; no product protocol change.

AFTER INDEPENDENT REVIEW AND USER CONSENT, ordinary non-admin PowerShell7:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\sftp-fixture\Invoke-NativeAuthFix.ps1' -RunFixture
```

Fresh fixed root artifacts/sftp/native-authfix-20261031-8a63e024 was absent at preparation. Failed combined/diagnostic roots and logs remain unchanged. Same signed native server, dependency pins, non-elevated token check, denied port API retained (no executor retry), one smoke file, exactly two replacement pairs, strict config/effective preflight, single loopback48223 connection,115-second server/120-second batch deadlines, bounded diagnostics and checked teardown. Normal user must supply old port cleanup verification; no kill-by-name. No services/firewall/extra account/global config/personal keys or Explorer/artwork changes.

## Exact added ACL scope

Immediately after OwnWrite creates and inventories ONLY new root/keys/authorized_keys, call FixtureKeyAcl.Protect(authorization,root,owned[authorization]). Existing helper requires current owner, exclusive READ_CONTROL|WRITE_DAC/content-read handle, non-reparse ancestor/file, one hardlink, unchanged hash/file identity, protected exact single current-SID FullControl ACE. No SetOwner, SACL, privilege change or directory ACL operation. Protection failure stops before either sshd preflight and triggers guarded cleanup. Result adds AuthorizationFileProtected=true only after the helper verifies its postconditions. Existing private-key protection remains. This is ONE additional newly owned PUBLIC authorization-file DACL restriction; requires explicit review/consent rather than silently changing user SSH security.

## StrictModes/source checks

Official guidance: https://github.com/PowerShell/Win32-OpenSSH/wiki/Security-protection-of-various-files-in-win32-openssh#authorized_keys describes the user's authorized_keys ownership/access restrictions including inherited access. Inspected native permission implementation https://raw.githubusercontent.com/PowerShell/openssh-portable/latestw_all/contrib/win32/win32compat/w32-sshfileperm.c reads OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION for the named resource, checks trusted owner/SIDs and disallowed ACE rights. It has no parent-directory ACL walk. Public-key caller https://raw.githubusercontent.com/PowerShell/openssh-portable/latestw_all/auth2-pubkey.c uses options.strict_modes when opening authorization files. This branch is contextual upstream source, not byte-for-byte attestation of release binary.

No evidence justifies Unix chmod-style parent directory changes, ownership takeover, SYSTEM/Administrators permissions, global installer scripts or StrictModes=no. Fixture's Guard already walks every existing ancestor and rejects reparse points before and after protection. It checks path/reparse containment, NOT a full parent ACL security proof or filesystem jail; do not overclaim. Prior actual run proved host private-key loading, expected host trust, account lookup reaching this authorization file and config parsing. It did not establish successful user login; any further explicit failure must still stop, not trigger speculative permission/account changes.

## Validation and hashes

Strict /W4 /WX native batch compilation passed (only root/marker constants differ from diagnostic source). Test-NativeAuthFixOffline.ps1 passed AST/order checks ensuring authorization creation -> verified protection -> result marker -> sshd -> batch; StrictModes=yes; no security expansion constructs; existing helper content/identity/owner/ACL checks and both ancestor guards; in-memory descriptor negative predicates and personal-path rejection. Default script mode ran memory-only and returned before creating root. No actual ACL/key/server/listener operations. CRLF-aware whitespace passed.

- Invoke-NativeAuthFix.ps1 E4D0F9DC307DBE578CBA234B8BBAF2D34EEFACEDC6DADFEAB02F136EA7FC93DD
- native-authfix-sshd_config E1F657A49CC7CF62248CA8FE91AECE338D024A04838B699188B4D57AC0549276
- artifacts/sftp/native-authfix-build-20261031-8a63e024/batch.exe430A8BD098CC0A8D4F2FA99A7CF4D155D27515247148FE832268F79F37EDE8F6

Result/diagnostics will be fresh root/evidence/preflight-result.json, server.stderr/client.stderr capped64KiB each, stage and checked teardown records. Auth fix passing alone does not establish transfer success. Unrun: live corrected fixture, trust rejection, cancellation/retry/reconnect/interruption, service/profile/UI/whole-app and release gates. No commit/tag/package/push or speculative product fix. Pause for review and user-run consent.
