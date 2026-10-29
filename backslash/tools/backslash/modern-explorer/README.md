# Modern Explorer helper prototype

Windows x64 native prototype for an independently identified Explorer command that launches an explicitly chosen portable Backslash copy. It is not installed by building it. Actual Windows 11 menu placement, packaged activation and update/removal behavior require a separately approved live test.

## Build and review

Use the repository's installed VS 2026 C++/Windows SDK prerequisites:

```powershell
pwsh -NoProfile -File .\backslash\tools\backslash\modern-explorer\Build-Prototype.ps1 -TargetExecutable 'D:\path\to\Backslash.exe'
```

This creates a fresh `artifacts/modern-explorer/<run>/` with `/W4 /WX` host/DLL/fixture builds, a Windows manifest-reader identity result, an unsigned identity-only MSIX, file hashes and `deployment-plan.json`. It copies existing Backslash Dev artwork and notices. MakeAppx uses `/nv` for the externally referenced sparse payload; this is not a claim that deployment/COM activation has been validated. Identity-aware host startup can require AppModel RPC access unavailable in a restricted sandbox.

The plan lists the proposed support files/destinations, exact package name/publisher/family/CLSID, explicit target selection and ownership checks, per-user registration and removal boundaries. It performs none of those live actions. Do not install the unsigned review MSIX or copy a development certificate into trust stores as part of building.

## Pinned host-side developer deployment

`Deploy-HostPrototype.ps1` is a one-attempt workflow for the reviewed artifacts already present in this checkout. Run it from ordinary **non-elevated, 64-bit Windows PowerShell 5.1 on the host**, after reviewing the script. It is pinned to this developer's paths and artifact hashes; it is not a redistributable installer or a way to bypass execution isolation.

```powershell
# Read-only verification and exact plan:
powershell.exe -NoProfile -File .\backslash\tools\backslash\modern-explorer\Deploy-HostPrototype.ps1
# Only after separately authorizing the printed plan:
powershell.exe -NoProfile -File .\backslash\tools\backslash\modern-explorer\Deploy-HostPrototype.ps1 -Register
```

The opt-in copies eight verified support files, registers the exact current-user development identity using existing Developer Mode, and selects the reviewed portable target. Existing package/configuration/support state is refused. Failures trigger rollback of only this invocation's owned changes; changed, malformed or foreign state is preserved with an error instead of broad deletion. Evidence is written under a fresh `artifacts/modern-explorer/host-deploy-*/` directory. Interrupted runs require reviewing that evidence, not blindly repeating the command. The script never launches the terminal or verifies menu placement; folder/background visibility, folder launch and portable settings behavior remain manual acceptance gates. It neither changes policy/certificates nor elevates/restarts applications.

Rollback checks raw registry bytes (including termination), not normalized .NET strings. Package removal also requires a loose development registration at the exact support directory with the reviewed manifest hash; an identical package identity elsewhere is preserved. `HostDeploymentGuards.ps1` supplies these guards. Run `powershell.exe -NoProfile -File .\backslash\tools\backslash\modern-explorer\Test-HostDeploymentGuards.ps1` for in-memory malformed-value and mocked package/rollback tests; these do not deploy or touch live registry state.

## Isolated gates

Run `ModernExplorerTests.exe` with five arguments: a fresh existing fixture directory, a compatible built `Backslash.exe`, the built command DLL, the staged AppxManifest.xml, and the built LaunchSink.exe. It writes/removes only its generated `HKCU\Software\Backslash.ExplorerIntegration.Tests\Modern-<pid>-<tick>` root. It instantiates COM without registering it, uses synthetic shell selection/background providers and captured launches, then invokes one owned test child for actual argv/cwd/unpackaged-identity evidence. Existing Explorer entries and the production target key are never written. A restricted executor may need separate approval for these disposable registry writes.

## Runtime contract

`Backslash.ExplorerHost.exe --status` is read-only and returns configuration plus the helper's package-identity signal. It never installs packages, changes certificates, edits classic verbs, changes profiles or starts automatically.

Explicit configuration commands require the current target path and the literal `--apply target-only` suffix:

```text
--use-copy <absolute Backslash.exe> --expected-current <current path or <none>> --apply target-only
--remove-target --expected-current <current path or <none>> --apply target-only
```

These commands write/remove the bounded owned production target key and therefore belong after approval, not in build verification. They do not deploy/remove the package. Foreign/unmarked keys, unsupported schema, unknown data and malformed registry strings are rejected. Expected-copy checks are best-effort race detection, not a transaction against concurrent external registry editors. An interrupted initial write can leave an owned partial record that can be repaired or removed.

The command obtains exactly one selected filesystem folder or resolves the folder background through the site. Nonfolders and multiple selections are hidden/rejected. It invokes the exact executable through CreateProcess with one literal folder argument, no shell intermediary and no fallback target. Missing targets produce a visible failure rather than launching another copy. The launcher checks its new suspended child for package identity before resuming; it refuses identity-bearing children so portable storage cannot silently change.

Targets must be compatible Backslash builds implementing `--backslash-explorer-folder`. Validation checks absolute filesystem syntax, the Backslash.exe name, x64 PE shape and a sibling TerminalApp.dll; it does not authenticate the selected binary or silently probe protocol support. UNC argument boundaries are tested; real UNC shares and shell-specific UNC behavior remain unverified.

Startup Settings uses the native modern controller in `src/cascadia/inc/ModernExplorerIntegration.h`, with policy/lifecycle logic in `ModernExplorerPolicy.h`. It reads exact package/location/development ownership, verifies helper hashes and raw target values, and offers explicitly confirmed enable/repoint, repair and removal. Classic entries are reported separately and left untouched. The portable assembly includes the eight pinned helper files in `ExplorerIntegration/`; a different helper payload requires a reviewed hash/identity update. Signed distribution remains separate from this local development route.

`Test-Management.ps1` compiles the controller with `/W4 /WX`, exercises fake lifecycle/notification/restart-confirmation behavior and checks only owned file fixtures. `Inspect-LaunchPage.ps1 -PortableDirectory <matching runtime>` renders the actual production Startup page in an owned offscreen clone, with compiled inbox defaults and no copied user settings. It checks collapsed/expanded/narrow layout and cancels enable/remove/restart dialogs; it never confirms a live mutation or invokes a terminal. A compatible owned registration is required for the action-cancellation cases. [Architecture, source evidence and approval boundary](../../../../docs/dev/EXPLORER-MODERN-INVESTIGATION.md).
