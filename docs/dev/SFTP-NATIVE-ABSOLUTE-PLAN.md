# Absolute native SFTP helper fixture — review candidate

No live retry, keys, ACL operations or downloaded server execution in this preparation. All previous failed roots/logs preserved. User artwork and separate startup-heading work untouched. Strong evidence identifies a bare-helper lookup dependency, but sole live root cause remains unproven until a successful test.

## Minimal correction

Fresh root: artifacts/sftp/native-absolute-20261031-f3407b61 (absent). Source NativeLiveAbsolute.cpp differs from prior diagnostic batch only by root/marker constants. New Invoke-NativeAbsolute.ps1 retains owner-only authorization-file protection, private keys, StrictModes, exact two pairs, one connection, loopback48223 and115/120-second watchdogs, capped diagnostics/checked teardown and exact cleanup. Prior reviewed runner files are not overwritten.

Fixture Subsystem and ForceCommand now both select this absolute helper:
D:/codex-workspace/backslash-terminal/artifacts/sftp/native-server-inspect-20261030-d581a2c7/layout/OpenSSH-Win64/sftp-server.exe
Both retain -d <freshroot>/remote -u077. No extra diagnostic option, no global PATH or shell registry modification. Helper hash5F5DF08DFA1FB659D0F6EB96F0F942C2BD0852670BA79FF9107578142568E2CF and exact Valid Microsoft Corporation signer are explicitly verified before setup; existing full dependency-inventory checks remain.

## Source-matched quoting/effective checks

Official v10.4.0.0 misc.c build_exec_command rewrites only internal-sftp/sftp-server prefixes; the absolute D:/... command is retained as CMD_OTHER. w32-doexec.c wraps no-PTY command as `<quoted cmd.exe> /c <quoted full command>`. Config fixed helper/root paths use forward slashes and a restricted no-whitespace/no-metacharacter ASCII alphabet, so no nested quoting/escaping or shell expansions occur. This is a deliberately fixed fixture, NOT a generic shell-quoting API. Spaces, quotes, Unicode/control characters, percent/exclamation expansion, ampersand/pipes, traversal and relative paths are refused rather than accepted through uncertain quoting. Existing helper path contains none of these.

Version-matched servconf.c parses Subsystem into argv, then argv_assemble reconstructs command/arguments for subsystem_args; simple no-space tokens dump unchanged. ForceCommand preserves its original command text. NativeAbsolutePolicy checks exact effective subsystem AND forcecommand command/args, single occurrence, absolute pinned helper, reviewed directory/umask and all prior security settings. It fails closed if actual dump formatting differs; do not relax or proceed on mismatch.

Source references:
- https://raw.githubusercontent.com/PowerShell/openssh-portable/v10.4.0.0/contrib/win32/win32compat/misc.c (build_exec_command1676-1726)
- https://raw.githubusercontent.com/PowerShell/openssh-portable/v10.4.0.0/contrib/win32/win32compat/w32-doexec.c (342-404)
- https://raw.githubusercontent.com/PowerShell/openssh-portable/v10.4.0.0/servconf.c (Subsystem1736-1781, ForceCommand2255-2263, dump4228-4230)

## Offline verification

Strict /W4 /WX batch compile passed. Test-NativeAbsoluteOffline.ps1 passed authorization protection ordering/helper in-memory predicates and exact positive effective config; negatives cover bare/internal/foreign helper, missing forced command, duplicate subsystem, changed umask, traversal and StrictModes weakening. Fixed shell-constant negatives cover spaces, quotes, &, |, percent/exclamation, NUL/newline, relative and parent paths. A negative test initially exposed regex $ accepting a trailing newline; changed to absolute \A/\z anchors and reran successfully. Tests model the source path/quote construction; no cmd.exe or downloaded binary was executed. Default runner memory-only mode and AST parsing passed, fresh root remains absent, CRLF-aware whitespace passed. No full application rebuild needed for fixture-only constants/policy; no product protocol change.

## Hashes and proposed normal-user action

- Invoke-NativeAbsolute.ps1 F02239E52C54223348FE77E48C5D5F300523CDE9440C1CEF803867FF06CC7FEF
- native-absolute-sshd_config41CCDAA9DDE6B7DAF8B2F5F03635ED659F6DF28C4D713D7A35AB97C58C12AC74
- NativeAbsolutePolicy.ps1 4CDADD9B2F029A5D25DEDE4D628663C0FD5BB71AF337DDC26DA06F5242D8C482
- batch.exe C5CA6710A79A49D1DABEE20B15C1EDC53865FF9BCC197C06B9557DA047A79B08 at artifacts/sftp/native-absolute-build-20261031-f3407b61

AFTER review and user consent, ordinary non-admin PowerShell only:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\sftp-fixture\Invoke-NativeAbsolute.ps1' -RunFixture
```

Do not run old fixture again or reuse its root. Same port API stays guarded; executor denial is not bypassed, so user normal-shell execution remains required. Stop if any denial/elevation/global-setting/account need arises. Fresh evidence/preflight-result.json and capped logs retain results/unknown files. Still no filesystem jail. Additional owned-file ACL scope beyond prior AuthFix: NONE. No new executable (helper already inventoried/signed), download/install, external server or extra listener.

Actual corrected auth/SFTP/browse/upload/download and all trust-rejection/cancel/retry/reconnect/interruption/production-service/profile/UI/whole-app gates remain unrun. Success in this single connection would not satisfy all release gates. No commit/tag/package/push or Release publication.
