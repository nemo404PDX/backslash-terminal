# Backslash portable identity implementation and evidence

Current Dev builds use `Backslash.exe`, complete Backslash Dev artwork, and the generated About/CLI build identity described in [BUILD-IDENTITY.md](../../backslash/tools/backslash/BUILD-IDENTITY.md). The dated first-slice record below preserves earlier artifact names and metadata.

Implemented 2026-10-13 in the selected checkout. This records the first implementation slice of the historical [identity proposal](APP-IDENTITY.md); it does not make the retained upstream package identity installable as Backslash.

## Identity changes

- `WindowEmperor.cpp`: Backslash/Preview/Canary/Dev routing prefixes; elevation, executable-path and user-SID hashes remain intact.
- `FileUtils.cpp`: unpackaged fallback is `%LOCALAPPDATA%\Backslash\Terminal`; `.portable` still selects the adjacent `settings` directory. `ApplicationState` already uses that same base and was not changed.
- `CascadiaSettingsSerialization.cpp`: stable-settings automatic migration is removed for every branding mode. External fragment discovery is an empty function for both runtime settings and extension-list discovery; upstream file roots, extension catalog identity and discovery-only helpers are removed. Explicit fragment parser tests remain supported. Built-in dynamic profile generators remain available.
- Package resource strings and TerminalApp context resources: existing Dev product-name/short-name/store-name/description keys identify Backslash Dev, including localized keys that existed. Microsoft legal attribution and internal DLL/namespace names remain.
- `WindowsTerminal.rc`, `.vcxproj`, and `CascadiaSettings.cpp`: executable product/description metadata and portable About/CLI version label identify `0.0.1.0 (Backslash Dev; upstream 1.25)`. This is a local development version, not an official upstream release version. Existing executable name is retained for launch/elevation helper compatibility.
- Temporary upstream Dev icons remain. No production Backslash art or new assets were downloaded.

## Reproduce and run

Run `backslash/tools/backslash/Build.ps1` (default Release/x64), then `backslash/tools/backslash/New-PortableDistribution.ps1 -BuildRun <successful-run-directory>`. The latter unpacks the exact unsigned build payload **without installation**, adapts upstream resource-merging support, merges WinUI XAML resources, copies app-local x64 VC145 CRT DLLs from the installed toolchain, retains notices/licenses, writes `.portable`, and emits a file/hash inventory. Fresh output is required; previous directories are never overwritten. Resource-staging evidence is retained, with no recursive deletion.

Run `backslash/tools/backslash/Test-PortableIsolation.ps1 -PortableDirectory <output-directory>` for focused source and distribution checks. Launch `Backslash.exe` by full path inside a current Dev portable directory; the historical artifacts below retain `WindowsTerminal.exe`. No new CLI alias is registered.

Delivery directory:
`artifacts/portable/Backslash-Dev-20261013-142327-974-Release-x64-d6d855bb`

Clean ZIP:
`artifacts/portable/Backslash-Dev-0.0.1.0-x64-d6d855bb.zip`

ZIP has 259 entries and includes `.portable`; smoke-generated settings/state and shell marker files are excluded. SHA256:
`E8D9B2F3456B5EA4BF268B53EAFD5575FA83AE47AB112E959C4765CF12FB899F`

## Validation actually performed

Evidence root: `artifacts/build/20261013-142327-974-Release-x64-d6d855bb`.

- Repository build script, amd64 MSBuild: **0 errors, 135 warnings**, elapsed 37.79 seconds. Existing baseline also reported 135 warnings. Warning categories include PRI263 missing-neutral-resource warnings, WMC1004/WMC1506, APPX4001 and LNK4068. These are retained limitations, not evidence of a warning-free build.
- Settings test DLL compiled using `SettingsModel.UnitTests.vcxproj`, Release/x64, with explicit `/p:SolutionDir=D:\codex-workspace\backslash-terminal\`. Direct TAEF run of `SettingsModelUnitTests::DeserializationTests::BackslashDisablesExternalFragments`: **1 passed, 0 failed/blocked/skipped**. Both discovery modes leave empty profiles and extension map unchanged. Full test suite was not run or packaged/registered.
- Static isolation/completeness checks passed, including adjacent settings/state base, fallback, all-branding migration removal, disabled fragment discovery, routing prefix/path/elevation separation, runtime/resources/notices, absent package manifest, executable description and version.
- Authorized hidden smoke launch of the exact delivery directory executed its PowerShell test shell and created adjacent `settings/settings.json` and `settings/state.json`. Second `-w 0` invocation exited through same-distribution handoff and executed its shell. The resident process was verified by exact executable path and stopped after testing; no smoke app is intentionally left running.
- Two discovered existing Windows Terminal settings/state files retained identical SHA256 hashes before launch, after initial smoke, and after handoff/cleanup. Hash-only evidence is retained; no contents were logged or edited.
- Initial assembly caught a missing HTML notice, corrected before delivery. Initial standalone test-build command lacked SolutionDir, corrected and subsequently passed. Initial static metadata check found blank upstream local-build version resources, corrected with explicit development metadata and a fresh final build. Earlier builds and incomplete distribution attempts were preserved.

## Limits and scope

Visible UI/icon review, elevated/unelevated coexistence, concurrent installed-Terminal or second-copy routing, actual absent-marker fallback runtime, Preview/Canary runtime, and shared-root sentinel tests were not performed. Migration/fallback/routing coverage beyond the Dev smoke is static. Final production icons, packaged identity/signing/update policy and portability to a clean Windows machine remain future work.

No package registration, certificate trust, security setting, default-terminal/Explorer integration, alias installation, toolchain installation, commit or push was performed. The upstream-identity unsigned MSIX under build output is only an intermediate payload and must not be installed as this deliverable. Existing documentation/build script changes and baseline artifacts were preserved. Glow/scanline controls are outside this slice.