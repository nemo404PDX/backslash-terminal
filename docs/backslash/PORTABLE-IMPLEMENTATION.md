# Portable operation and build reference

Extract the complete Windows x64 portable ZIP into a writable folder and run `Backslash.exe`. Keep the supporting DLLs, resources and `.portable` marker beside the executable. A normal local folder works; USB storage is optional. The preview is unsigned, so Windows may show an unsigned-application warning.

## Settings and updates

With `.portable` present, settings and state live in the adjacent `settings` directory. Keep that directory when moving the application and back it up before updates. Without the marker, unpackaged builds use `%LOCALAPPDATA%\Backslash\Terminal`.

Backslash does not automatically import Windows Terminal settings, and external settings fragments are not discovered automatically. Built-in profile generators remain available. SSH aliases still depend on each computer's configuration and authentication; the portable folder does not bundle credentials.

Split App/Data roots, automatic workspace discovery, sync and interrupted-drive reconnection are not implemented. Signing, independent installed-package identity and broad clean-machine compatibility coverage remain outstanding. Only Release x64 has recorded build validation; no ARM or Win32 compatibility is established here.

## Build a portable copy

From the repository root, with the [Windows build prerequisites](../../backslash/tools/backslash/README.md):

```powershell
pwsh -NoProfile -File .\backslash\tools\backslash\Build.ps1
pwsh -NoProfile -File .\backslash\tools\backslash\New-PortableDistribution.ps1 -BuildRun '.\artifacts\build\<successful-run-directory>'
```

Assembly unpacks the unsigned build payload without installing it, includes the app-local runtime and notices, writes `.portable`, and emits a file/hash inventory. It creates a fresh output directory rather than overwriting an existing portable copy. Run `Backslash.exe` by full path in that directory.

The unsigned MSIX in build output retains upstream package identity and is an intermediate payload; do not install it as the Backslash portable deliverable. The build does not register an executable alias or Explorer menu. Explorer integration, where available, is a separate explicit opt-in.

Contributor source details and dated build/isolation results are in [portable identity implementation and verification](../dev/FEATURE-RECORDS.md#portable-identity-implementation-and-verification).
