# SSH profile controls and WSL discovery

Implemented in the current checkout after checkpoint `79cf9d34`, with the preceding uncommitted bezel slice preserved. No SSH configuration, remote DefaultShell, keys, credentials, installed Terminal, or user's portable settings were edited. Validation never connects to an SSH server.

## Remote shell

Settings → Profiles → generated SSH profile → Remote shell. Choose a built-in mode, **Save**, then open a **new SSH session**. Custom commands additionally use **Apply custom command** before Save. Cancel discards the settings clone. The existing Command line field/reset remains available. The generated parent supplies the original `Host` alias; changing the profile display name cannot redirect it. GUID and source stay unchanged. No new JSON setting is required: choices use the existing `commandline` override and its normal copy/save/reload behavior.

| Choice | Behavior |
| --- | --- |
| Use SSH configuration (default) | Clears the local commandline override and uses the generator's original SSH launch. Configured `RemoteCommand` and `RequestTTY` remain effective. |
| Server default | Requests a PTY and sets `RemoteCommand=none`, with no positional remote command. |
| Bash | Requests a PTY, clears configured RemoteCommand, and sends `bash -l`. |
| PowerShell 7 | Requests a PTY, clears configured RemoteCommand, and sends `pwsh -NoLogo`. |
| Custom | Requests a PTY, clears configured RemoteCommand, and sends the entered remote command line unchanged as one local argv argument. |

Example construction: `"%SystemRoot%\System32\OpenSSH\ssh.exe" -t -o RemoteCommand=none "original-alias" "pwsh -NoLogo"`. The command is interpreted by the **server login shell**; Windows argv quoting only transports it to SSH. Custom therefore uses that server shell's quoting rules, and cannot guarantee that a named shell is installed. Semicolons are ordinary command characters, without wt-specific escaping. Empty/whitespace-only Custom commands, NUL, CR/LF, and `%` are rejected. ConPTY expands environment variables before process creation, so `%` in a literal command/target is unsupported. Aliases beginning with `-` are rejected, preventing host-as-option ambiguity.

Only exact recognized generator/selector command lines are editable through this selector. An unfamiliar advanced command line is preserved; the selector is disabled with an explanation. Edit it manually or reset Command line before using the selector. Merely visiting the page, scanning, or selecting a candidate does not rewrite commands.

## Explicit scan and restoration

**Add profile → Scan SSH hosts → select → Add / Restore → Save**. Scan reads the existing system/user SSH configuration files; it does not reload settings, overwrite unsaved settings, change the seen-GUID set, or restore anything by itself. It respects `disabledProfileSources` and the existing policy filter. Direct concrete Host aliases, including multiple aliases per Host line and Host blocks without HostName, are supported. Wildcard/negated patterns and unsafe targets are omitted; duplicate aliases produce one GUID. This remains a small local-file scanner, not a complete OpenSSH Include/Match/configuration interpreter.

Restore materializes one saved profile with its existing stable GUID/source and a valid current generated parent. It clears runtime Deleted/Hidden/Orphaned flags and refreshes profile/menu collections. Deleted profiles return with generated defaults. Retained hidden/orphan profiles keep local customizations. A host still present but intentionally deleted stays deleted on ordinary reload; restoring one host leaves other deleted hosts alone. Successful Save adds saved SSH GUIDs to the seen set (including hosts first found while the editor was open). Failed Save and Cancel do not change that set. No seen GUID is erased, so a second deletion after restoration stays suppressed.

## Modern WSL discovery

The old `Modern=1` skip assumed an external distro fragment would supply those profiles. Backslash disables external fragment loading for portable isolation, so registry discovery now includes both modern and legacy distributions. Name-based GUID generation, system-directory `wsl.exe`, Docker/Rancher utility exclusions, source/policy filters, and ordinary deleted-profile suppression are retained. Repeated names are deduplicated; external fragments remain disabled.

## Validation and manual checks

`ProfileSourceTests` uses injected generators and temporary ApplicationState directories. Tests cover Windows argument quoting/remote command transport, environmental expansion, invalid inputs, shell mode round trips, renamed display names, advanced command preservation, remove Host → delete orphan → restore same Host, still-present deletions, second deletion, unique GUID/source/parent and active entries, hidden/orphan customization, scan/unsaved settings, Cancel/failed Save, newly scanned GUID tracking, disabled sources, and modern/legacy/excluded WSL distributions. No actual SSH config or WSL registry is modified.

Final build/test/package evidence is appended below after validation. Native Computer Use's required `node_repl` is absent and the available CUA surface disables native APIs; desktop visual/interaction checks remain unrun. In a fresh portable extraction, check the five shell choices, Apply/Save/Cancel/reset, keyboard and accessible labels, scan-select-restore-save behavior, and WSL profile availability. SSH command behavior needs a later user-controlled connection check; this task performs no remote execution.

## Final integrated evidence (2026-10-13)

Checkpoint remains **`79cf9d34`**. All implementation remains uncommitted. The existing bezel work, FILE-TRANSFER-DESIGN.md, user-added concept images, and previous artifacts are preserved.

- Successful final app build: `artifacts/build/20261013-155957-526-Release-x64-96cd752b`, Release/x64, **0 errors / 134 warnings**, **1m34.61s**. Existing packaging/PRI/link warning categories remain. The prior integrated `82b80135` build (before centering correction) is preserved: 0 errors / 135 warnings; 157 settings tests and eight startup cases passed. Earlier compile attempts/logs are retained.
- Final settings DLL: **0 errors / 8 warnings**. TAEF host was `bin/x64/Release/UnitTests_SettingsModel/TE.exe`, adjacent to DLL and `resources.pri`. Focused `ProfileSourceTests`: **8/8 passed**. Full settings suite: **158/158 passed**, no failed/blocked/skipped/not-run tests. This includes existing CRT, bezel, profile duplication, inheritance and serialization regressions. Logs: `profile-source-tests.log`, `settings-full-suite.log`.
- Existing `Test-RetroShader.ps1 -Runtime`: compiled shader ABI/reflection and WARP pixel regressions passed (`retro-shader-test.log`). Shader source is unchanged.
- Fresh unsigned portable assembly and `Test-PortableIsolation.ps1` passed. No installation/registration was performed.
- Hidden smoke: **13 cases and 52 repeated center hot-reload samples passed** (`profile-portable-smoke.json`). Windowed starts were 80×24 and VT requests produced 90×30. Center toggles retained 90×30. Maximized grids stayed 169×50. Fullscreen with tabs retained 175 columns and toggled predictably between 50/49 rows; fullscreen without tabs toggled between 52/51 rows. Repeated on/off returned the same grid. This is runtime grid/mode-setting evidence, not visual inspection or a live tab-row accessibility assertion.
- Real read-only startup discovery generated exactly one `Ubuntu-24.04` profile, GUID `{963ff2f7-6aed-5ce3-9d91-90d99571f53a}`, and no Docker/Rancher utility entries. No WSL distro or SSH server was executed.
- **Four existing installed/user-portable settings/state files retained identical SHA256 hashes.** Only 13 processes matching the exact fresh portable executable were stopped. Smoke settings/state are preserved in the evidence directory, and the delivery folder is clean.
- Clean ZIP: `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-Profiles-x64-96cd752b.zip`, **258 entries**, `.portable` included, smoke settings excluded. SHA256: **`44FF7ED7C374641B09559D205316D0C2280F5918255E6CEA044F76A0A8C56EBF`**.

**Runnable executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-155957-526-Release-x64-96cd752b\WindowsTerminal.exe`.

Implemented source groups: model `Profile.*`, `SshCommandline.h`, `CascadiaSettings.*`/serialization; generators `SshHostGenerator.*`, `WslDistroGenerator.*`; editor `AddProfile.*`, `MainPage.*`, `ProfileViewModel.h`, `Profiles_Base.*`, English Resources.resw; tests `ProfileSourceTests.cpp` and test project; corrected geometry `RetroBezel.h`, `RetroBezelGeometry.h`, fullscreen handoff `TerminalPaneContent.cpp`, unchanged tab-policy extraction `TerminalTabPolicy.h`/`TabManagement.cpp`. Other modified source listed by Git belongs to the preserved preceding bezel slice. Supporting notes are in BEZEL-DESIGN.md and branding/concepts/README.md.

Remaining manual checks in the fresh portable copy: (1) SSH selector Apply/Save/Cancel/reset and keyboard/accessibility, plus explicit scan-select-restore-save; (2) square recess shoulder/rounded inside corner, text/selection/DPI/transparency; (3) Center aperture on/off with windowed/maximized/fullscreen tabs, actual fullscreen exit, and hot reload. Native `node_repl`/Computer Use is unavailable and CUA native APIs are disabled. The wallpaper-strip source defect is corrected; an interactive reproduction of centering itself hiding tabs is unrun. No production icon pack, remote shell availability check, hardware GPU timing, commit, or push is claimed.

## PowerShell selection regression follow-up

The initial SelectionChanged handler changed only the ComboBox/custom-field UI. The profile commandline changed only after a separate Apply click. Thus selecting PowerShell 7 and pressing Save alone serialized the original SSH configuration launch, which could still start the server's cmd shell. This is a source-confirmed UI persistence defect; remote pwsh availability is not inferred from the cmd output.

Built-in choices now update the profile/viewmodel as soon as the user selects them. Save persists that commandline. Guarded navigation and property-refresh events do not apply choices or clear advanced overrides. Custom retains its explicit validation/Apply step, visibly labeled Apply custom command. The page explains that a saved choice affects new SSH connections; it does not replace the shell of an already-running SSH session. No remote DefaultShell/config change is made.

A local fake ssh.exe fixture records real Windows-process argv after model selection, a saved-file round trip, generated reload layering and environment expansion. It never connects to a host. Live-server verification and native selector interaction remain unrun.

## Latest surround / badge / SSH-selection delivery (2026-10-13)

This delivery supersedes the `96cd752b` ZIP while preserving it. Source remains uncommitted atop `79cf9d34`.

Diagnosed/fixed: (1) the 6 DIP inner safety gutter exposed the host background; a fixed opaque-black hollow Border now covers it while its exact rectangular cutout leaves terminal opacity/acrylic intact, with colored material above it; (2) a fixed 8 DIP badge offset placed the plate off-center for different widths; the entire plate now centers vertically between rim and inner shoulder; (3) built-in SSH selection changed only UI until Apply; it now changes the profile commandline immediately, with guarded refresh and explicit Save/new-session text. The known user copy's `winblows-nuc` profile had no saved commandline override when inspected read-only. No remote pwsh availability conclusion is drawn.

- Final build: `artifacts/build/20261013-162825-371-Release-x64-f3e3a085`, **0 errors / 134 warnings**, **53.22 seconds**. Settings test DLL: **0 errors / 8 warnings**. Intermediate surround-only (`834f4f40`) and badge (`baca60a2`) build evidence is retained.
- Local adjacent-resource TAEF: focused **11/11 passed**; full suite **161/161 passed**, no failures/blocked/skipped/not-run tests. This includes the preserved SSH restoration/WSL/CRT/bezel tests and new surround, badge and real Windows-argv capture tests.
- Fake SSH client: all five modes selected, saved to fixture files, reloaded with generated parents, expanded and launched with CreateProcessW. PowerShell capture is `-t -o RemoteCommand=none sameHost "pwsh -NoLogo"`; custom quotes/semicolon remain one remote argument. `SshCaptureClient.cpp` cannot connect to a server and the test checks its identity before execution. It is not in the portable ZIP. Reproduce by building it with `Build-SshCaptureFixture.ps1` in a fresh evidence directory, setting process-local `BACKSLASH_TEST_SSH_EXE` to that fixture path, then running the local TAEF host. Existing fixture argv/JSON and compile logs are retained in the run directory.
- Shader ABI/reflection and WARP output regressions passed; shader source unchanged. Fresh portable isolation checks passed.
- **19 hidden startup cases / 76 repeated center hot-reloads passed**, including translucent and acrylic profiles in windowed, maximized and fullscreen modes, centering on/off, all bezel widths and top/bottom/omitted badge variants. Windowed/maximized grid dimensions stayed stable under centering; fullscreen toggles returned the same expected grids. This validates startup/settings/layout, not actual composited pixel appearance.
- **Four installed/user-portable settings/state hashes stayed unchanged**; only 19 owned test processes matching the fresh executable were stopped. Test settings/state are retained in evidence, and delivery has no settings folder.
- Previous ZIP and both user-added branding images retain exact SHA256 hashes. FILE-TRANSFER-DESIGN.md and all other profile/bezel work were preserved. No SSH connections, password handling, remote/config/key edits, installation/registration, commit or push occurred.

**Executable:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-20261013-162825-371-Release-x64-f3e3a085\WindowsTerminal.exe`.

**Clean ZIP:** `D:\codex-workspace\backslash-terminal\artifacts\portable\Backslash-Dev-0.0.1.0-Surround-x64-f3e3a085.zip` (258 entries); SHA256 **`0CD83CE7C44387C7DD471EB81244C4DCF3AC43CFBC8C1FC150A29DBEF48AE590`**.

Current change files for this follow-up: `inc/RetroBezel.h`, `RetroBezelGeometry.h`, `TerminalSettingsEditor/Profiles_Base.cpp/.h`, English Resources.resw, `UnitTests_SettingsModel/ProfileSourceTests.cpp`, `tools/backslash/SshCaptureClient.cpp`, `Build-SshCaptureFixture.ps1`, BEZEL-DESIGN.md and this document. Earlier uncommitted files remain as recorded in final-source-status.txt.

Manual checks remain: actual opaque-black gutter/rounded-edge composition while the screen is transparent/acrylic; badge plate vertical centering at DPI scales; native SSH selection/Save/Cancel/Custom interaction; and a user-controlled **new** SSH session confirming PowerShell. An existing cmd SSH session keeps its shell until disconnected/restarted. Native Computer Use/node_repl is unavailable and CUA native APIs are disabled. The surround screenshot was inspected here; badge positioning used the parent's inspected crop/clarification. No native/live-server verification is claimed.

## Current-checkout revalidation (2026-10-14)

The SSH selector/explicit restoration/modern WSL implementation was already committed in `36f73ec` when this delegated request resumed; current HEAD is `bfd6c420a12532e777dfc76fba7081a639e17c7c`. Earlier “uncommitted atop 79cf9d34” statements above describe those historical deliveries. This validation did not rewrite the implementation or commit/push anything. All pre-existing modified/untracked source, bezel work and FILE-TRANSFER-DESIGN.md retained their initial hashes.

Fresh current-source run: `artifacts/build/20261014-165317-601-Release-x64-aadc5e05`. App build: **0 errors / 134 warnings**, 21.91 seconds. Adjacent-resource TAEF: **184/184 settings**, **20/20 focused SSH/WSL/bezel**, and **1/1 focused production-backend WARP** passed. CRT compiled ABI/WARP pixel tests (including 1,200 tint cases), visibility concurrency checks and portable isolation passed. All five SSH modes used the local capture executable; no SSH server or WSL distribution was executed.

Full Control: **33/34 passed**. The sole failure, `BackslashPhosphorOpacityWARP`, was present and failing before this revalidation; its assertion requires white glyph tint through opacity changes. The existing test/source and shader were preserved for the separate renderer investigation. This package does not resolve that regression.

An owned copy passed **13 hidden local-shell startup cases / 52 center hot-reload samples**. Read-only registry startup discovery found exactly one modern Ubuntu-24.04 and excluded Docker/Rancher utility profiles. The four installed/user portable files tracked by the original isolation harness and all previous ZIPs stayed unchanged. The additional preservation check failed: settings.json/state.json in the already-running `de0a97ce` copy changed hashes during the window. The harness writes only the fresh owned copy, but the cause of those active-copy changes is unestablished; those files/processes were left alone. Both pre-existing app processes remained present. See the original mismatch evidence and `smoke-preservation-assessment.json`; broad user-file preservation is not claimed. The delivered portable directory remained pristine; smoke settings are retained in evidence. Native selector/Save/Cancel/accessibility and composited bezel/DPI checks remain unrun because no supported native desktop interaction tool is exposed.

Executable: `artifacts/portable/Backslash-Dev-20261014-165317-601-Release-x64-aadc5e05/WindowsTerminal.exe`.

Clean ZIP: `artifacts/portable/Backslash-Dev-0.0.1.0-Profiles-x64-aadc5e05.zip`; SHA256 `A596BF8A89B769136A77BB823CE345E3CD976BAC1F778495F8A580E34E70E555`.

Evidence: `source-before.json`, `delivery-summary.json`, `source-checkpoint.zip`, `source-checkpoint-inventory.json`, `focused-profile-bezel-tests.log`, `full-settings-tests.log`, `control-full-suite.log`, `profile-portable-smoke.json`, `modern-wsl-startup-discovery.json`, `smoke-additional-preservation.json`. Manual checklist: select each shell and Save/new session; Custom Apply/Save/Cancel/reset; scan alone then Add/Restore/Save, delete again and reload; retained customization; keyboard/accessibility; WSL profile visibility; bezel corners/gutter/centering across DPI and transparency. Remote execution remains outside this task.

Content-audit follow-up (2026-10-15 UTC): the earlier Control failure was an incorrect fixture expectation that white must become colored. The documented luminance-preserving ramp intentionally retains white/black endpoints. A corrected actual-backend probe now measures glyphs, explicit backgrounds, cursor and selection across 100/99/50/0/100 opacity; all match the ramp with zero BGRA8 channel error. **Full Control 34/34 passed**. See CRT-RENDERING-PLAN.md's content audit and `artifacts/phosphor-content-audit`. Production shader/binary behavior is unchanged; the ZIP above is not an opacity fix. Historical failing logs and source snapshot remain intact.
