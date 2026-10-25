# Backslash application identity and coexistence plan

[Project entry point](README.md) · [Roadmap](ROADMAP.md) · [Provenance](UPSTREAM-SOURCE.md) · [Brand concept](../../backslash/branding/concepts/README.md)

Status: **docs-only proposal**, based on inspection of stable v1.25.2733.0, commit dc8ae365847d41f62d1ebca93815bb00959bc12f. No application identifiers, resources, manifest, source, keys, certificates or registration were changed by this task. A separate build task may run against the unchanged implementation; this document does not report or authorize its actions.

Display brand is Backslash; repository/folder is backslash-terminal. Folder naming is already established and does not migrate package/executable identity.

## Recommended first route: isolated portable distribution

Prefer a separate unpackaged output directory with the upstream .portable marker and adjacent settings directory. Launch by explicit executable path initially. This avoids package registration, execution-alias installation and default-terminal/Explorer changes. It is a recommended distribution/testing route, not a claim that an independently branded Backslash build already exists.

[New-UnpackagedTerminalDistribution.ps1](../../backslash/build/scripts/New-UnpackagedTerminalDistribution.ps1) has PortableMode and creates the .portable marker; its Layout parameter set defaults PortableMode on. The script was inspected, not executed. [FileUtils.cpp](../../backslash/src/cascadia/TerminalSettingsModel/FileUtils.cpp) IsPortableMode detects the marker beside the module; GetBaseSettingsPath uses the adjacent settings folder only when unpackaged and portable. [ApplicationState.cpp](../../backslash/src/cascadia/TerminalSettingsModel/ApplicationState.cpp) SharedInstance uses that same base for application state.

Portable mode is not total identity isolation: shared fragment discovery still points at Microsoft/Windows Terminal and extension-catalog behavior needs deliberate isolation/disable decisions. Product window-routing/name/icon isolation and a nonportable Backslash fallback path remain planned changes. Separate output must contain appropriate runtime files/licenses; portable does not mean dependency-free or universally launchable.

Do not run packaging/install/registration scripts as a consequence of reading this plan. An independently identified portable implementation should be reviewed as a separate small change after the current unchanged-baseline build work.

## Exact identity seams and proposed treatment

| Concern | Pinned source evidence | Proposed Backslash treatment |
| --- | --- | --- |
| Window routing | [WindowEmperor.cpp](../../backslash/src/cascadia/WindowsTerminal/WindowEmperor.cpp), HandleCommandlineArgs: branding-specific windowClassName, Admin suffix, unpackaged executable-path hash and user SID hash | Eventually use an independent Backslash routing identity. Separate unpackaged path already contributes to separation, but verify routing rather than infer coexistence from display name. |
| Portable/fallback settings | [FileUtils.cpp](../../backslash/src/cascadia/TerminalSettingsModel/FileUtils.cpp): UnpackagedSettingsFolderName currently Microsoft/Windows Terminal; IsPortableMode/GetBaseSettingsPath | Use adjacent portable settings first. Plan an independent Backslash local-AppData fallback if marker absent; exact path is a user/design choice. |
| Application state | [ApplicationState.cpp](../../backslash/src/cascadia/TerminalSettingsModel/ApplicationState.cpp): SharedInstance/GetBaseSettingsPath | Keep settings/state under the isolated Backslash base; inspect other future stores independently. |
| Stable automatic import | [CascadiaSettingsSerialization.cpp](../../backslash/src/cascadia/TerminalSettingsModel/CascadiaSettingsSerialization.cpp): LoadAll, _releaseSettingsPath; [FileUtils.cpp](../../backslash/src/cascadia/TerminalSettingsModel/FileUtils.cpp): GetReleaseSettingsPath | LoadAll only attempts stable import on first setup for Preview/Canary, and skips it in portable mode. Default Dev does not take that branch. Preserve an explicit no-automatic-import rule for the fork; any user-requested import is a separate previewed action. |
| Shared fragment files | [CascadiaSettingsSerialization.cpp](../../backslash/src/cascadia/TerminalSettingsModel/CascadiaSettingsSerialization.cpp): FragmentsPath, SettingsLoader::FindFragmentsAndMergeIntoUserSettings | Current constant is Microsoft/Windows Terminal/Fragments; discovery considers LocalAppData and ProgramData. Choose isolated Backslash roots or disable shared discovery initially. “SharedFragmentsPath” in an external summary is descriptive; actual pinned constant is FragmentsPath. |
| Extension fragments | Same file: AppExtensionHostName, AppExtensionCatalog::Open | Current host is com.microsoft.windows.terminal.settings. Isolate/disable for first route; later decide a fork-owned extension catalog with compatibility policy. Portable mode alone does not establish catalog independence. |
| Visible Dev labels | [package Resources.resw](../../backslash/src/cascadia/CascadiaPackage/Resources/en-US/Resources.resw), [ContextMenu.resw](../../backslash/src/cascadia/TerminalApp/Resources/en-US/ContextMenu.resw) | Audit product/menu/publisher display strings and localization; use Backslash labels while preserving accurate Microsoft upstream/legal attribution. Display names do not replace technical identities. |
| Icons/assets | [WindowsTerminal.rc](../../backslash/src/cascadia/WindowsTerminal/WindowsTerminal.rc), [CascadiaResources.build.items](../../backslash/src/cascadia/CascadiaResources.build.items), [images-Dev](../../backslash/res/terminal/images-Dev) | Produce licensed own icon assets, including contrast/small-size variants. Concept art is not a production ICO/SVG; do not wire an unverified concept PNG into app resources. |
| Build branding | [Branding.targets](../../backslash/build/rules/Branding.targets) | Explicit Release/Preview/Canary become their tokens; every other value falls back to Dev. A new “Backslash” property alone is insufficient and does not create independent strings/assets/package identities. |
| Actual solution | [OpenConsole.slnx](../../backslash/OpenConsole.slnx) | Keep source project/namespace renaming optional; solve deployment/settings/routing/product identity first. |

A proposed Backslash path or string above is not an applied edit. Do not blanket replace Microsoft identifiers across the repository.

## Packaged identity later

Only after a reviewed portable coexistence proof should packaged distribution be designed.

[Package-Dev.appxmanifest](../../backslash/src/cascadia/CascadiaPackage/Package-Dev.appxmanifest) uses Identity Name WindowsTerminalDev and a Microsoft product publisher subject. Backslash needs a unique package identity and a real selected publisher/certificate arrangement. Do not invent or imitate a publisher subject, create signing keys, trust a certificate or register a package in this documentation task.

The Dev execution alias is wtd.exe. [CascadiaPackage.wapproj](../../backslash/src/cascadia/CascadiaPackage/CascadiaPackage.wapproj) sets OCExecutionAliasName to wtd for Dev/default and uses it in packaging. A proposed fork alias is backslash.exe, pending collision/name/distribution decisions; never take over wt.exe. [WtExeUtils.h](../../backslash/src/cascadia/WinRTUtils/inc/WtExeUtils.h) IsDevBuild recognizes WindowsTerminalDev and GetWtExePath resolves wt/wtd or adjacent WindowsTerminal.exe. Those helpers must stay consistent with package name, aliases and executable paths; changing a manifest alias alone is insufficient.

The wapproj enables signing only when CascadiaPackage_TemporaryKey.pfx exists. An unsigned generated package is not proof it can be installed/launched on a user's machine. No key was created, read or installed here. [template.appinstaller](../../backslash/build/config/template.appinstaller) contains Microsoft publisher/update assumptions and an on-launch update policy unsuitable as-is for the fork. Its dependency publisher identities belong to their actual dependency owners; do not blindly replace every Publisher field.

Initial route omits default-terminal and Explorer registration. Later fork-owned implementation CLSIDs need synchronization between registration manifests and implementation headers, including [CTerminalHandoff.h](../../backslash/src/cascadia/TerminalConnection/CTerminalHandoff.h), [CConsoleHandoff.h](../../backslash/src/host/exe/CConsoleHandoff.h) and [OpenTerminalHere.h](../../backslash/src/cascadia/ShellExtension/OpenTerminalHere.h). Shared interface IIDs/contracts in [ITerminalHandoff.idl](../../backslash/src/host/proxy/ITerminalHandoff.idl) and [IConsoleHandoff.idl](../../backslash/src/host/proxy/IConsoleHandoff.idl) must not be blanket-renamed: implementation identity and interoperability contracts are different concerns.

No windows.protocol extension was found in the four inspected package manifests: Package.appxmanifest, Package-Pre.appxmanifest, Package-Can.appxmanifest and Package-Dev.appxmanifest. This is a scoped source observation, not proof that future protocols or every registration path are safe.

## Executable/project naming is a coordinated optional step

A differently named portable output directory can initially contain WindowsTerminal.exe while later display/product identity is migrated. If the main executable is renamed, update [WindowsTerminal.vcxproj](../../backslash/src/cascadia/WindowsTerminal/WindowsTerminal.vcxproj) TargetName, manifest COM executable paths, WtExeUtils helpers, [wt shim.cpp](../../backslash/src/cascadia/wt/shim.cpp)/[wt.vcxproj](../../backslash/src/cascadia/wt/wt.vcxproj), and [elevate-shim.cpp](../../backslash/src/cascadia/ElevateShim/elevate-shim.cpp)/[elevate-shim.vcxproj](../../backslash/src/cascadia/ElevateShim/elevate-shim.vcxproj) together. Validate elevation/new-window/shell-extension activation paths. Do not assume renaming the project or namespace performs this migration.

Preserve [LICENSE](../../LICENSE), [NOTICE.md](../../NOTICE.md), [NOTICE.html](../../backslash/src/cascadia/CascadiaPackage/NOTICE.html), component licenses/notices and accurate attribution in the distribution. Own product identity must coexist with upstream credit, not erase it.

## Remaining user/design choices

Portable-first acceptance; exact fallback settings directory; isolated fragments versus initial disable; executable/CLI alias names; timing of project/namespace renames; final icon/typeface production; unique package name; actual publisher/signing/distribution/update policy; whether/when default-terminal/Explorer integration is desired; explicit import UX; cleanup/retention behavior. No name/trademark clearance is claimed.

Package registration, enabling Developer Mode, certificate trust, alias installation/replacement, default-terminal changes and Explorer registration are system-affecting steps needing explicit approval before execution. Source preparation/documentation, an unchanged-baseline build, or an unsigned output does not authorize them. No keys, installs or registrations were performed by this task.

## Coexistence acceptance plan — not yet run

1. Start portable output by full path with .portable; verify settings/state are adjacent and installed Windows Terminal settings remain unchanged.
2. Start another independent output/installed Terminal concurrently, including elevated and unelevated cases; verify windows/commands route to the intended app, user and path.
3. First-run test with pre-existing stable/Preview/Canary settings: no automatic import or modification, including absent-marker fallback tests after independent fallback is implemented.
4. Put sentinel fragments in upstream shared roots and exercise catalog discovery: first Backslash route does not merge Microsoft Terminal fragments/extensions unintentionally. Test selected isolated roots/disable policy.
5. Verify title/menu/icon/product labels and legal attribution; no production asset claim from concept art.
6. Confirm no package/default-terminal/Explorer/alias registrations changed. Existing wt.exe/Windows Terminal launches still behave as before.
7. For packaged work only after approval: verify unique identity, real certificate/publisher, helper/alias/COM alignment, side-by-side install/update/uninstall and explicit settings retention/import behavior.

Retain before/after path and registration observations for future authorized tests. Static source inspection cannot certify coexistence or installability; no test was executed in this documentation task.

