# Backslash application and build tree

This directory contains the application source, resources, dependency manifests, solutions and build tools. The [repository README](../README.md), [MIT license](../LICENSE), [third-party notice](../NOTICE.md), [project documentation/gallery](../docs/README.md) and [.github community guides](../.github/CONTRIBUTING.md) remain at the checkout level.

From the checkout root, using existing Windows prerequisites:

```powershell
pwsh -NoProfile -File .\backslash\tools\backslash\Build.ps1 -Preflight
pwsh -NoProfile -File .\backslash\tools\backslash\Build.ps1 -Rebuild
```

Open `backslash/OpenConsole.slnx` in Visual Studio. When running MSBuild directly from this directory, use this directory as `SolutionDir`. NuGet restores into `backslash/packages`; compilation writes `backslash/bin` and `backslash/obj`. Build evidence and isolated portable deliveries stay in the checkout-root `artifacts/` directory. Existing old artifacts are not moved or replaced.

See the [Windows build guide](tools/backslash/README.md) and [repository layout/migration guide](../docs/backslash/REPOSITORY-LAYOUT.md). No installer or user settings changes are part of the build.
