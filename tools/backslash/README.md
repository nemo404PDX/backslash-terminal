# Build this checkout

Use PowerShell 7 on Windows and an already installed Visual Studio with the upstream C++, Windows SDK and packaging prerequisites. The previously successful baseline used VS Community 2026 18.10.1, MSBuild 18.10.1, MSVC v145 14.51.36231 and SDK 10.0.26100.0. This script installs no tools.

From any directory:

```powershell
pwsh -NoProfile -File 'D:\codex-workspace\backslash-terminal\tools\backslash\Build.ps1' -Preflight
pwsh -NoProfile -File 'D:\codex-workspace\backslash-terminal\tools\backslash\Build.ps1'
```

The script derives the checkout root from PSScriptRoot, validates upstream source files, and runs restores/builds there. It never substitutes a temporary source copy. Preflight only checks source/tool paths without writes or developer-shell changes; it does not verify SDK completeness, write access, network access or compilation.

Defaults: `-Configuration Release -Platform x64 -MaxCpuCount 2`. Configuration accepts Release/Debug and platform accepts x64/ARM64/Win32. Only Release x64 has the recorded baseline evidence. Official installed vswhere selects the latest VS with MSBuild and C++ tools; override with `-VsInstallPath 'C:\path with spaces\Visual Studio'`. The script selects the installed amd64 MSBuild executable (matching the x64 host developer shell) to avoid accidentally using 32-bit MSBuild. The matching developer shell uses an x64 host compiler and disables automatic directory switching. Existing Git for Windows under Program Files is preferred on PATH.

NuGet restores the repository's dep/nuget/packages.config and OpenConsole.slnx using unchanged official configuration and pins. Dependencies may be fetched into normal caches/output directories; network and write access must already be permitted. A restore failure stops the build. Upstream MSBuild/vcpkg can also download and unpack its required build utilities into its caches; these are part of upstream build dependency resolution, with no system installer invoked by this script. MSBuild targets Terminal\CascadiaPackage with signing and symbol packaging disabled.

Each build creates a unique directory in the already ignored artifacts/build with session.log, restore-dependencies.log, restore-solution.log, build.log, build.binlog and package output under packages/. MSBuild prints the exact package paths. Standard executable/intermediate outputs stay in upstream bin/obj; the default executable is normally bin/x64/Release/WindowsTerminal/WindowsTerminal.exe. No artifacts are deleted, and artifacts/baseline-v1.25.2733.0-x64 is untouched. Run one build at a time because bin/obj are shared. Repeat builds can reuse intermediates and dependency caches.

Native failures return their exit code; setup/script failures return 1. Only a zero MSBuild exit produces the completion message. Consult the current invocation's logs rather than old output files. Build success does not validate runtime behavior. Binlogs can contain environment/path information; review before sharing.

**Default upstream WindowsTerminalDev identity and Microsoft publisher subject remain. This is not an independently identified Backslash release.** The script does not sign, install, register or launch the app, or change default-terminal, security or permission settings. See the [baseline record](../../docs/backslash/BUILD-BASELINE.md) and [identity prerequisites](../../docs/backslash/APP-IDENTITY.md) before distribution or installation.



## Rendering visibility regression fixture

`Test-RenderVisibility.ps1 -EvidenceDirectory <fresh-directory>` compiles the real `RenderVisibility` and `RenderDiagnostics` helpers with the already installed VS/WIL toolchain. It verifies suspension acknowledgment, independent output, coalesced resume, rapid transitions, hidden shutdown and diagnostic opt-in/JSON roundtrip. It installs nothing and does not launch the app or change user settings.

For app diagnostics, set process-local `BACKSLASH_RENDER_DIAGNOSTICS` to an absolute writable JSONL path and launch a fresh portable copy by its full executable path. Leave the variable unset for normal use. Lifecycle snapshots and one-second active samples contain counters only; file size grows with the bounded opt-in run. See [the CRT rendering report](../../docs/backslash/CRT-RENDERING-PLAN.md#implemented-follow-up-hidden-controls-and-opt-in-counters-2026-10-14) for counter meanings, the reparenting fix, exact evidence and limits. Actual GPU residency/occlusion is not inferred from these counters.

## Native bezel badge SVG decoding

`Test-BezelBadgeSvg.ps1 -EvidenceDirectory <existing-directory>` compiles an owned offscreen XAML decoder using installed VS/SDK/WIL dependencies. It creates a fresh probe directory, copies exactly the three working SVGs, uses the actual `BezelBadgeSvg.h` asset/stream helper and the application's existing Windows target manifest, and requires all three `SvgImageSource.SetSourceAsync` results to succeed. It performs no desktop input, screenshot, registration or installation. This checks decoding and explicit-white currentColor transport, not composed badge pixels or arbitrary recoloring. [Badge implementation and validation](../../docs/backslash/BEZEL-BADGE-IMPLEMENTATION.md) records placement/settings and accepted live-sizing behavior.

## Composed bezel badge regression

`Test-BezelBadgeComposition.ps1 -EvidenceDirectory <existing-directory>` uses the production `RetroBezel` in native live/wide-preview/narrow-preview containers. It verifies painted lettering/backing, wide aspect ratio, all styles/positions/thicknesses and aperture-span anchors, then saves PNGs and `composed-results.json` in a new owned directory. It briefly shows its own native host and captures only its XAML elements, without installation or input into other applications. The screen is a placeholder; the actual-app captures are separate evidence. Decoder-only success is insufficient for this regression. See [the rendering follow-up](../../docs/backslash/BEZEL-BADGE-RENDER-FIX.md).
