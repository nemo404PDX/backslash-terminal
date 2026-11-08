# Native fixture diagnostics — offline prepared, pending review/user execution

Old failed fixture native-combined-20261030-71d4b908 and evidence remain unchanged. No speculation-driven product fix. The previous generic SSH close cannot establish trust/auth/subsystem/protocol cause. Old port cleanup query result from user remains pending; executor's read-only port inventory was denied, so no automatic live attempts are permitted.

Next proposed user command AFTER REVIEW:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\sftp-fixture\Invoke-NativeDiagnostic.ps1' -RunFixture
```

Use ordinary non-admin shell as nemo. No policy bypass/elevation. Same API port guard retained; if any guard denies/fails, stop. Fresh fixed root artifacts/sftp/native-diagnostic-20261031-59ca7e20 was absent after offline validation. Exactly two new disposable keypairs plus one non-secret ACL smoke; same explicit native signed server/dependency hashes, current-user-only ACL, -t/-T/effective settings, one debug listener127.0.0.1:48223,115-second owned server watchdog and120-second outer job; final exact hash-guarded cleanup. No services/firewall/accounts/global config/real credentials/Explorer state changes. No listener retry, port substitution or wider filesystem permissions. Server is still not jailed.

## Narrow diagnostic difference

NativeLiveDiagnostic.cpp uses the same production NativeSession/ProcessTransport operations and single connection. It defines BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS only in the fixture translation unit. Conditional constructor/diagnostic callback branches in SftpProcess.h and SftpNativeSession.h are absent from ordinary builds; default diagnostics discard remains unchanged. No speculative trust/auth/protocol correction is made. The existing callback sink runs on the normal stderr drain thread, catches sink exceptions and continues draining. Native client adds only -v for debug diagnostics; all strict trust, knownhosts, key/agent isolation and multiplexing guards remain.

Server starts with explicit STARTUPINFOEX stdio and inherited-handle whitelist: NUL stdin/stdout and one owned stderr pipe; no broad handle inheritance. A dedicated drain writes through FixtureLog, capped at65536 bytes per server/client file. After cap or sink write error it keeps draining and discards excess, preventing pipe backpressure. File creation is CREATE_NEW at exact evidence path, not inherited; no append/overwrite or global log path. Local bounded synchronous disk writes remain subject to outer job timeout. Debug1 logging records connection negotiation/fingerprints/fixture paths, not private-key file contents; logs remain local, are not automatically uploaded, and may include username/public fingerprint/path metadata. Inspect only relevant excerpts before sharing. No arbitrary remote endpoint/banner or personal SSH config is used.

Stages include SERVER_PID, loopback-ready, SSH start, browse begin/complete, marker download, upload, download. Server teardown records TerminateJobObject result, WaitForSingleObject result, actual exit code, log status/truncation and confirmed flag. Successful batch requires confirmed server teardown. Destruction also records teardown on exception. Drain completion gets2 seconds then CancelSynchronousIo before joining. Outer120-second job remains final process-tree bound; if forcibly killed, confirmation may be absent and must be reported uncertain. Independent host PID/port check still required after run; no automatic kill-by-name.

## Result locations and interpretation

Fresh root evidence/server.stderr and client.stderr (each<=64KiB); batch.stdout/stderr; sshd syntax/effective logs; preflight-result.json with Phase/Error/ListenerStarted/Commands/Cleanup/RetainedFiles and TeardownConfirmed when batch output exists. Truncation/failure is shown by teardown/client status where reached. Cleanup removes only exact inventoried known fixture keys/config/data with unchanged hashes; diagnostics remain. Unknown temporary files/mismatches remain rather than wildcard deletion. Guard failure before root creation can produce console error with no JSON. Error/stopped is failure even if shell exitcode0.

This one connection may prove browse, marker identity, upload/download integrity only if it passes. Real host-key rejection/cancel/retry/reconnect/interruption/uncertain publication and service/profile/UI/whole-app routes remain unrun. No release claims or actions follow this fixture alone.

## Offline evidence

Strict /W4 /WX builds: artifacts/sftp/native-diagnostic-build-20261031-59ca7e20 (batch-build.log, diagnostic-tests-build.log). DiagnosticTests.cpp executed owned fake child only:256KiB stderr flood drained, exact64KiB output cap, protocol byte retained, throwing diagnostic sink still drains and owned teardown completes. No sockets/downloaded server/keygen/ACL operations ran. Normal no-macro Process and Native fake-child regressions passed: artifacts/sftp/process-f3b899061e5d4328a542e89166ba295e and native-3b8299c470804d2c9656500357ce557f. Diagnostic script AST/default memory-only mode and CRLF-aware whitespace passed. Full application rebuild not run; normal code path is unchanged by conditional fixture-only seam.

Reviewed-candidate hashes (SHA256):
- Invoke-NativeDiagnostic.ps1:748AD610F0B79EB12BE79CCA82954DF3F893C7A3A2E68D4C303A8431C17B6076
- native-diagnostic-sshd_config:70A9BBA49B571D03159C75D02B67B34E8A84DA87C95C24CECDEC544C806845DB
- batch.exe:DAF1FC367CDAD09FF2528521CBEE55E5B26C4F617EA5D446CB99F74FA8E974D6

Pause for independent review and user-run approval. No server execution, key creation, ACL mutation, commit/tag/package/push in preparation.
