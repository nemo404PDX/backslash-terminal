# Native SFTP execution proposal — pending approval/review

No downloaded code, keys, ACL operations or listener ran while preparing this proposal. Prior evidence roots remain untouched. Product interoperability remains unverified.

## Runnable non-listening stage

PowerShell 7:

```powershell
& 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\sftp-fixture\Invoke-NativePreflight.ps1' -RunPreflight
```

DO NOT RUN until review/approval is confirmed. Default invocation runs only in-memory descriptor predicates; preparation used static PowerShell AST parsing only. The opt-in path independently refuses elevated parent token before any smoke ACL operation. Fresh fixed root `artifacts/sftp/native-preflight-20261030-6f2a1c93` was absent. It verifies Windows client/keygen, signed downloaded server and all 15 EXE/DLL dependencies against pinned inventory, ACL helper, process runner and static config; then creates one non-secret ACL smoke file and exactly two disposable host/client replacement pairs. It runs owned ten-second non-listening `-t` and `-T` commands with explicit fixture config; any error stops. It always removes matching owned generated files, preserving marker and evidence. No listener path exists. Partial keygen outputs are inventoried in finally. No MSI, install script, account, service, firewall, personal SSH config, agent, ProgramData or privilege changes.

Sources/hashes:

- Invoke-NativePreflight.ps1: `65E20C5AA049D640BD157F9077A626B55DC8CCF54B3A4B9CEF47EEC2E66B4032`
- native-sshd_config: `139D54F46D484EB9D6BE644B9E27BBCF3A6F0D8877C1EE0316A4ABE96F49EB3C`
- FixtureKeyAcl.cs: `88712FC4D5D3E3250641DF23EE6A4523CFD24E882C15215F97FDDA1EEB34A492`
- Existing Runner.exe: `858CF60453C70AB890DEF840756E683B13FBDD63F61FF66BD71DC86DF4DB55F6`
- Native sshd.exe: `00D61610A0965B42CC9A9CD401DAE8983FE77D5394697F074D776BB68CE6FF0A`, valid Microsoft signature, file version10.4.0.0. Exact path is pinned in script under artifacts/sftp/native-server-inspect-20261030-d581a2c7/layout/OpenSSH-Win64.

Config pins loopback IPv4 port48223, nemo-only/publickey-only, StrictModes, no forwarding/agent/TTY/userRC/Include/default config, internal-SFTP with disposable remote start directory and umask077. This is not a jail. Native acceptance of all options and account mapping remains a preflight question. Do not weaken options or change global state on failure.

## Conditional listener/client stage — not yet runnable or authorized by preparation

Only after reviewing effective config and actual preflight results, prepare one owned foreground debug server invocation `sshd.exe -d -e -f <reviewed fixture config>` with hard120-second total job budget, explicit non-elevated/current-user token, exact binary/dependency hashes and actual bind127.0.0.1:48223 verification. Debug mode allows ONE connection. No extra listener launches, duration extension or port substitution.

The client must be ready BEFORE server launch: a small allowlisted batch through the actual production ProcessTransport/Client/transfer adapter, fixture key/knownhosts only, `-F none`, IdentityAgent=none, IdentitiesOnly=yes, strict host-key checking and multiplexing disabled. Reject absolute/parent/separator/symlink paths; check exact ownership marker before any write. One connection could cover directory listing, binary upload and download with integrity comparison and non-overwrite rejection. No general command shell or arbitrary path UI. Actual batch client has NOT yet been implemented; existing native tests use fake children, not a real-endpoint switch. Do not launch a waiting server without this client ready/reviewed.

Unknown/mismatched host-key rejection ends a debug connection, and cancellation/disconnect may do likewise. Therefore full trust rejection, reconnect/retry, server interruption/publication ambiguity and real whole-app GUI shutdown cannot all be claimed under one debug listener. They remain explicit separate gates. Do not multiply listener attempts silently.

Lifecycle/budget issue to resolve before end-to-end execution: standalone preflight always cleans its two keypairs. A later separately run listener setup would need two additional keys and fresh root/config, which must be stated/reviewed as additional scope. To keep the overall native test to exactly two keypairs, revise the orchestration before execution to perform preflight, validated client batch and guarded cleanup in one finally-bounded invocation after all gates are approved; do not execute this standalone preflight then regenerate keys under an assumed budget. This document deliberately exposes that choice rather than silently retaining private keys or adding a listener to the reviewed script.

Current release gates remain real auth/interoperability, trust/error/cancel/retry, interactive app-exit, Settings regressions/blank split, Explorer lifecycle and packaging review. No release/tag/push occurs from these scripts.

## Combined single-connection proposal (supersedes standalone sequencing)

Use `backslash/tools/backslash/sftp-fixture/Invoke-NativeCombined.ps1 -RunFixture` ONLY after review and approval. No live path has been executed. Fresh root is `artifacts/sftp/native-combined-20261030-71d4b908`, confirmed absent. It replaces the standalone-preflight sequencing for the approved test: one smoke file, exactly two replacement keypairs, -t/-T, strict effective-value validation, one batch/listener, final hash-guarded cleanup in the same invocation. Original roots and the previous standalone runner remain evidence; do not execute both scripts.

`NativeLiveBatch.cpp` compiled with /W4 /WX to artifacts/sftp/native-combined-build-20261030-71d4b908/batch.exe. It uses production NativeSession/ProcessTransport/protocol/local publication, not a fake SFTP server and not the GUI/service/profile-selection layer. Only one NativeSession/SSH child is constructed. The owned native debug server is started suspended, assigned to kill-on-close job, resumed, and observed with GetExtendedTcpTable: its PID must listen only on127.0.0.1:48223. Readiness uses no probe connection. A115-second watchdog kills only that job, while the entire batch runs under a separate reviewed120-second job deadline. All destructor/exception paths stop owned server; outer job also bounds unexpected stalls. No process is killed by name. No reconnect/listener retry.

The batch uses fixture-only client identity and known_hosts, -F none, IdentityAgent=none, IdentitiesOnly=yes, GlobalKnownHostsFile=none and production strict/multiplexing policy. It browses `.`, downloads and verifies exact remote ownership marker, uploads a generated65536-byte payload containing every byte value, downloads it again and compares both local/remote bytes to the original. Remote protocol paths are hardcoded marker/upload names and production random temporary names; no arbitrary user paths. Data writes follow marker verification. No filesystem jail is claimed. Output only reports stage/results, not keys or server contents. Existing user profiles/configuration remain unused.

PowerShell prepares fixture contents, hash-verifies batch/dependencies/config/helper, refuses an occupied port/elevated parent/reparse/colliding root, validates critical -T values exactly, launches one batch, and inventories retained paths. Cleanup deletes only exact inventoried unchanged known files. Unexpected bytes, protocol partials or unknown files remain with evidence rather than wildcard deletion. The batch's successful upload/download paths are adopted for cleanup only if their hashes match the pre-generated expected payload/marker. Failure is not called completion. Keys/config are removed on normal script failures; external host termination cannot guarantee script finally, and job ownership stops child processes only.

Offline checks executed: PowerShell AST parse; in-memory ACL positive/negative predicates; effective config exact-positive and missing StrictModes, duplicate StrictModes, wildcard bind, password auth, personal authorized_keys and missing ForceCommand negative cases; default runner exits before filesystem effects; root still absent; CRLF-aware whitespace check. Build initially found a Bytes symbol collision then missing advapi32 linkage; corrected before strict final compile. No downloaded server, keygen, ACL write or network operation executed during preparation. These are offline preparation checks, not real interoperability evidence.

UNRUN: all live batch operations; unknown/changed host-key rejection on a real peer; cancellation/retry/reconnect/server interruption/lost publication acknowledgment; service/UI saved-profile flow against real server; whole-app X/Quit/last-tab/multiwindow; theme/DPI/accessibility; full Settings/Explorer acceptance. One debug connection cannot prove these multi-connection gates. Separate approval required for additional listeners/duration, credentials or privileged/global changes. Pause for independent review and user execution approval; no commit/tag/package/push.
