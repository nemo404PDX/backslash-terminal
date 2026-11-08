# Build this checkout

Use PowerShell 7 on Windows and an already installed Visual Studio with the upstream C++, Windows SDK and packaging prerequisites. The previously successful baseline used VS Community 2026 18.10.1, MSBuild 18.10.1, MSVC v145 14.51.36231 and SDK 10.0.26100.0. This script installs no tools.

From any directory:

```powershell
pwsh -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\Build.ps1' -Preflight
pwsh -NoProfile -File 'D:\codex-workspace\backslash-terminal\backslash\tools\backslash\Build.ps1'
```

The script derives the application root (`backslash/`) from PSScriptRoot, validates upstream source files, and runs restores/builds there. It never substitutes a temporary source copy. Preflight only checks source/tool paths without writes or developer-shell changes; it does not verify SDK completeness, write access, network access or compilation.

Defaults: `-Configuration Release -Platform x64 -MaxCpuCount 2`. Configuration accepts Release/Debug and platform accepts x64/ARM64/Win32. Only Release x64 has the recorded baseline evidence. Official installed vswhere selects the latest VS with MSBuild and C++ tools; override with `-VsInstallPath 'C:\path with spaces\Visual Studio'`. The script selects the installed amd64 MSBuild executable (matching the x64 host developer shell) to avoid accidentally using 32-bit MSBuild. The matching developer shell uses an x64 host compiler and disables automatic directory switching. Existing Git for Windows under Program Files is preferred on PATH.

NuGet restores the repository's dep/nuget/packages.config and OpenConsole.slnx using unchanged official configuration and pins. Dependencies may be fetched into normal caches/output directories; network and write access must already be permitted. A restore failure stops the build. Upstream MSBuild/vcpkg can also download and unpack its required build utilities into its caches; these are part of upstream build dependency resolution, with no system installer invoked by this script. MSBuild targets Terminal\CascadiaPackage with signing and symbol packaging disabled.

Each build creates a unique directory in the checkout-root ignored artifacts/build with session.log, restore-dependencies.log, restore-solution.log, build.log, build.binlog and package output under packages/. MSBuild prints the exact package paths. Standard executable/intermediate outputs are in backslash/bin and backslash/obj; the Dev executable is normally bin/x64/Release/WindowsTerminal/Backslash.exe (other build brands retain their configured names). No artifacts are deleted, and artifacts/baseline-v1.25.2733.0-x64 is untouched. Run one build at a time because bin/obj are shared. Repeat builds can reuse intermediates and dependency caches. Pass `-Rebuild` for a full package/dependency rebuild; the reorganized layout starts with fresh application-local bin/obj paths.

Native failures return their exit code; setup/script failures return 1. Only a zero MSBuild exit produces the completion message. Consult the current invocation's logs rather than old output files. Build success does not validate runtime behavior. Binlogs can contain environment/path information; review before sharing.

**Default upstream WindowsTerminalDev identity and Microsoft publisher subject remain. This is not an independently identified Backslash release.** The script does not sign, install, register or launch the app, or change default-terminal, security or permission settings. See the [baseline record](../../../docs/backslash/BUILD-BASELINE.md) and [identity prerequisites](../../../docs/backslash/APP-IDENTITY.md) before distribution or installation.



## Rendering visibility regression fixture

`Test-RenderVisibility.ps1 -EvidenceDirectory <fresh-directory>` compiles the real `RenderVisibility` and `RenderDiagnostics` helpers with the already installed VS/WIL toolchain. It verifies suspension acknowledgment, independent output, coalesced resume, rapid transitions, hidden shutdown and diagnostic opt-in/JSON roundtrip. It installs nothing and does not launch the app or change user settings.

For app diagnostics, set process-local `BACKSLASH_RENDER_DIAGNOSTICS` to an absolute writable JSONL path and launch a fresh portable copy by its full executable path. Leave the variable unset for normal use. Lifecycle snapshots and one-second active samples contain counters only; file size grows with the bounded opt-in run. See [the CRT rendering report](../../../docs/backslash/CRT-RENDERING-PLAN.md#implemented-follow-up-hidden-controls-and-opt-in-counters-2026-10-14) for counter meanings, the reparenting fix, exact evidence and limits. Actual GPU residency/occlusion is not inferred from these counters.

## Native bezel badge SVG decoding

`Test-BezelBadgeSvg.ps1 -EvidenceDirectory <existing-directory>` compiles an owned offscreen XAML decoder using installed VS/SDK/WIL dependencies. It creates a fresh probe directory, copies exactly the three working SVGs, uses the actual `BezelBadgeSvg.h` asset/stream helper and the application's existing Windows target manifest, and requires all three `SvgImageSource.SetSourceAsync` results to succeed. It performs no desktop input, screenshot, registration or installation. This checks decoding and explicit-white currentColor transport, not composed badge pixels or arbitrary recoloring. [Badge implementation and validation](../../../docs/backslash/BEZEL-BADGE-IMPLEMENTATION.md) records placement/settings and accepted live-sizing behavior.

## Composed bezel badge regression

`Test-BezelBadgeComposition.ps1 -EvidenceDirectory <existing-directory>` uses the production `RetroBezel` in native live/wide-preview/narrow-preview containers. It verifies painted lettering/backing, wide aspect ratio, all styles/positions/thicknesses and aperture-span anchors, then saves PNGs and `composed-results.json` in a new owned directory. It briefly shows its own native host and captures only its XAML elements, without installation or input into other applications. The screen is a placeholder; the actual-app captures are separate evidence. Decoder-only success is insufficient for this regression. See [the rendering follow-up](../../../docs/backslash/BEZEL-BADGE-RENDER-FIX.md).

`Test-BezelBadgeComposition.ps1 -LayoutOnly -EvidenceDirectory <existing-directory>` runs the fast 12-case native edge layout/toggle matrix. It checks whole-terminal margins and dimensions, independent left/right values, zero bezel-off margins, restored settings, and unchanged outer layout size in live/wide-preview/narrow-preview hosts. The normal pixel matrix includes all three legacy widths plus Slim top/Wide sides/Medium bottom (216 cases). These fixtures use a placeholder terminal, not live terminal cells.


## Native SSH Scan UI regression

`Test-SshScanPage.ps1 -PortableDirectory <runtime-matching-current-built-DLLs>` builds a fresh owned offscreen XAML host and invokes the actual production Scan/Import controls. It requires current Editor/Model DLL hashes, copies no settings directory, and redirects user/system SSH config to synthetic fixtures. A non-executable client placeholder is used only for scanner existence checks; no SSH connection, key, ProxyCommand, terminal launch, installation or interaction with existing windows occurs.

The default run is a strict gate for typed row rendering, repeat/empty/handled-error/recovery scans, single and two selected imports, stale selection clearing, exact owner/record counts and duplicate suppression. Its UI model uses compiled-only inbox defaults so appearance previews have the schemes present in the real app. All unhandled XAML errors fail the gate. `-Observe` is diagnostic capture only, not a passing regression gate. Evidence is retained under checkout `artifacts/ssh-scan-ui/`. In development, use a disposable runtime copy containing the current built components; never overlay a user's running portable folder.

## Resource fallback consistency

Run `python tools/backslash/Test-ResourceFallbacks.py --self-test` from the app root. This validates the three localized UI resource trees against en-US, rejects duplicates/missing fallbacks and exercises synthetic invalid inputs. It never deletes translations automatically. Check use before removing an obsolete entry or restoring authoritative English. The owned `Test-SshScanPage.ps1` probe also checks mutable NewTabMenu folder notifications and row lifetime.

For the pinned preview.2 cleanup audit, run `python tools/backslash/Test-ResourceCleanupAudit.py` from the app root (or `python backslash/tools/backslash/Test-ResourceCleanupAudit.py` from the repository root). It compares raw Git/XML bytes without console encoding or serializer artifacts; see `docs/backslash/RESOURCE-KEY-AUDIT.md`. The native Scan probe also exercises real NewTabMenu A/B navigation, notifications, bound/rendered list replacement and old-model isolation with synthetic models.

For the local C1902 workaround, explicitly invoke `Build-SerialTerminalApp.ps1` (Release/x64 only). It uses the tracked, hash-pinned `build/SerialTerminalApp.targets` in a child-process environment, preserving ordinary build defaults and any existing import. `-Preflight` checks prerequisites without building. See [reproduction and unresolved debug-symbol limitations](../../../docs/dev/BUILD-PDB-RECOVERY.md).
