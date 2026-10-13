# Successful unchanged baseline build

[Project entry point](README.md) · [Application identity prerequisites](APP-IDENTITY.md) · [Roadmap](ROADMAP.md) · [Provenance](../../UPSTREAM-SOURCE.md)

Recorded 2026-10-13. **An unchanged upstream source copy built successfully; no independently identified Backslash application or runtime behavior has been validated.** This document records another authorized build task's result and read-only evidence checks. The documentation writer did not rerun the build.

## Result and scope

Build thread 01a13702-01b2-70ea-8920-ea42f73a703a reports exit 0 for stable Windows Terminal v1.25.2733.0, commit dc8ae365847d41f62d1ebca93815bb00959bc12f, configuration Release|x64, target Terminal\CascadiaPackage, with package signing disabled.

This was **not a direct build of D:\codex-workspace\backslash-terminal**. The build worker's declared writable roots covered Temp and its Documents task directory, not D:. It chose Temp on that permission basis; no D: write was attempted or rejected. The verified official archive was built under:

C:\Users\nemo\AppData\Local\Temp\backslash-baseline-build-20261013\terminal-1.25.2733.0

Independent documentation-time check compared all 3,664 original archive files in that source copy: zero missing/changed files. Dependency restores and build outputs are additional files, not source edits. The official archive SHA-256 remains 1D2B9D701E66333D698412DBE4C8240E5239837B763E7DA79C8DEBE2B57CDAEA.

The build task reports no source edit, system installation, signing, registration or application launch. Restoring build dependencies into the temporary source/cache was performed; it is distinct from installing/registering the resulting application. Runtime, portable launch and coexistence tests remain unrun.

The repository documentation checkpoint was read as 68d53a4 before these new doc edits. Source baseline identity is the upstream commit above; do not confuse the documentation commit with the upstream release commit.

## Toolchain and dependency record

| Component | Reported version / evidence |
| --- | --- |
| Visual Studio Community 2026 | 18.10.1, from build task |
| MSBuild | File version 18.10.1.42706, from build task; log banner 18.10.1-1.26427.6+3cd27c13e |
| MSVC / toolset | 14.51.36231 / v145; compiler path corroborated in log |
| Windows SDK | 10.0.26100.0; MIDL/SDK paths corroborated in log |
| NuGet | Official dependency restoration using unchanged upstream Azure feed/configuration; no feed rewrites |
| vcpkg pins | fmt 12.1.0, ms-gsl 4.2.2, jsoncpp 1.9.6, cli11 2.6.1, cmark 0.31.1 |

These versions describe this successful environment, not a new universal minimum-prerequisite claim. No toolchain or system dependency installation was performed by this documentation task.

## Artifact and evidence independently checked

Package:
C:\Users\nemo\AppData\Local\Temp\backslash-baseline-build-20261013\terminal-1.25.2733.0\src\cascadia\CascadiaPackage\AppPackages\CascadiaPackage_0.0.1.0_x64_Test\CascadiaPackage_0.0.1.0_x64.msix

- Size: **6,888,178 bytes**
- SHA-256: **E1E278CFDFE362AF703218C0780F7783D381B9A6149F089460025109C3C7EFD7**
- Inspected package manifest: Name WindowsTerminalDev, version 0.0.1.0, architecture x64, Microsoft publisher subject.
- AppxSignature.p7x is absent from the package; no package signature was produced.

Executable exists under the build source copy at bin\x64\Release\WindowsTerminal\WindowsTerminal.exe. It was not launched during evidence checks.

Original logs remain in the parent temporary directory; verified copies are now preserved in the repo artifact folder below:
- baseline-restored.log: text build log; inspected toolchain paths, warnings and emitted package path.
- baseline-restored.binlog: binary build log; presence/size verified, not decoded in this documentation pass.
- Earlier baseline.log/baseline.binlog and restore logs also exist; use baseline-restored files for the successful final build.

**This WindowsTerminalDev package is not a safe Backslash release.** Successful unsigned packaging does not establish installability, certificate trust, independent identity, settings/window-routing isolation or coexistence. The original temporary artifacts remain intact and verified copies are now preserved under the repository artifacts path below. The repository copies are ignored local deliverables, not committed or cloud-backed preservation.

## Recorded reproduction sequence

These are the reported commands for PowerShell 7 in the verified source-copy directory. They are documentation, not commands executed by this task or permission to install/register/launch anything. The described toolchain must already be present, and the chosen source/output directory must be writable.

~~~powershell
$env:PATH = 'C:\Program Files\Git\cmd;' + $env:PATH
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
.\dep\nuget\nuget.exe restore .\dep\nuget\packages.config -NonInteractive
.\dep\nuget\nuget.exe restore .\OpenConsole.slnx -NonInteractive
msbuild.exe .\OpenConsole.slnx /t:Terminal\CascadiaPackage /p:Configuration=Release /p:Platform=x64 /p:AppxSymbolPackageEnabled=false /p:AppxPackageSigningEnabled=false /m:2
~~~

The build task retained text/binlogs separately. A future reproduction should explicitly choose output/log paths and capture its process exit code; do not assume the old artifact was regenerated just because it still exists.

## Warnings and practical limits

Reported warning classes: PRI263 default/neutral resource issues, XAML binding warnings, APPX1707 WinMD implementation metadata, archive Git metadata and temporary/incremental-build considerations. PRI263 and APPX1707 are visible in the inspected final text log. Preserve warnings for later runtime investigation; exit 0 does not dismiss their effect.

No application launch, UI behavior, accessibility, SSH ownership, CRT performance, portable settings or packaged installation was tested. Do not mark feature/audit acceptance gates passed based on this compilation.

## Next minimal portable identity slice

Prerequisites are the choices and source-tied plan in [APP-IDENTITY.md](APP-IDENTITY.md): agree separate portable output and exact independent fallback/routing identity; choose isolated/disabled shared fragment/catalog discovery; explicitly preserve no automatic upstream settings import; decide which visible labels/assets enter the first slice. Concept art is not production icon material.

Implement and review a minimal portable identity change only under separate authorization, leaving package registration, Developer Mode, certificate trust, execution aliases, default-terminal and Explorer changes out initially. Then build that changed source and, only if authorized, perform portable launch/coexistence tests. Keep source-copy/hash/build evidence distinct from those later identity and runtime results.


## Verified repository-owned artifact preservation

Destination: D:\codex-workspace\backslash-terminal\artifacts\baseline-v1.25.2733.0-x64

- [Unsigned package](../../artifacts/baseline-v1.25.2733.0-x64/package/CascadiaPackage_0.0.1.0_x64.msix): 6,888,178 bytes; SHA-256 E1E278CFDFE362AF703218C0780F7783D381B9A6149F089460025109C3C7EFD7.
- [Final text log](../../artifacts/baseline-v1.25.2733.0-x64/logs/baseline-restored.log) and [binary log](../../artifacts/baseline-v1.25.2733.0-x64/logs/baseline-restored.binlog).
- [Complete executable build output](../../artifacts/baseline-v1.25.2733.0-x64/executable-build-output), including [WindowsTerminal.exe](../../artifacts/baseline-v1.25.2733.0-x64/executable-build-output/WindowsTerminal.exe) and every original runtime sibling/resource directory/debug-symbol file.
- [Artifact scope README](../../artifacts/baseline-v1.25.2733.0-x64/README.md), [per-file hash manifest](../../artifacts/baseline-v1.25.2733.0-x64/DELIVERABLE-HASHES.json) and [notices](../../artifacts/baseline-v1.25.2733.0-x64/notices).

Copied 81 deliverable files (75 executable-output files, package, two logs, three notices), totaling 707,067,772 bytes. Every source/destination SHA-256 matched. Two additional local metadata files are the artifact README and hash manifest; 83 files total in the preserved folder. Source Temp originals were retained. The new destination was absent before copying; no existing files/source tree were overwritten or merged.

The full executable build output is not a coherent prepared portable distribution. Microsoft.UI.Xaml.dll, merged resources.pri, .portable and OpenConsole.exe were absent in that raw output. Upstream unpackaged packaging combines XAML/resource assets; that step has not been executed. Preserving runtime siblings avoids handing over an isolated executable, but does not establish runnable completeness. Do not launch/install/register this upstream WindowsTerminalDev artifact.

The existing artifacts/ rule in root .gitignore covers both preserved deliverables and artifacts/build/ for the separately authorized repo-build task's logs. Redundant temporary rules added during copying were removed; .gitignore is unchanged from the checkpoint. This preservation task did not modify any build script, rerun a build, change permissions, commit or push. A separate worker owns the new reproducible build script/usage doc; links can be connected after its final paths are known.
