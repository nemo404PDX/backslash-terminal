# Backslash development build identity

Backslash has no declared public release version yet. Local builds use the numeric
version in `src/cascadia/CascadiaPackage/Package-Dev.appxmanifest` (currently
`0.0.1.0`), displayed explicitly as `0.0.1.0-dev`. The illustrative date-based
version was not adopted. Package and fixed EXE versions remain four numeric
components; each must fit 0–65535. The fourth component stays zero for Store
compatibility. Do not encode dates, Git hashes, or dirty state into that component.

`BuildIdentity.json` records the pinned upstream release and commit documented in
`docs/backslash/UPSTREAM-SOURCE.md`. This is separate from the Backslash Git revision.
`New-BuildIdentity.ps1` reads the Dev manifest and provenance, captures UTC time
and the actual checkout HEAD/status, then emits a resource header and JSON evidence
into the host's intermediate directory. It refuses missing Git provenance instead
of displaying a fabricated revision. Untracked, nonignored source files also make
the checkout dirty. Random suffixes on build directories are not Git revisions.

The WindowsTerminal resource compilation target invokes this generator automatically
for local Dev builds, including direct MSBuild builds. Each host resource build
receives a fresh timestamp; this identifies the compiled artifact, not its later
packaging/copy time. Incremental resource compilation observes the generated header.
Non-Dev branding is rejected until a separate Backslash release process is defined.
The upstream release-pipeline `custom.props` is upstream metadata, not a Backslash
release source, and must not be used to describe these local builds.

The EXE ProductVersion contains the complete identity. About and `-v`/`--version`
read that EXE resource, appending runtime portable/packaged/unpackaged state.
About formats the same details on separate selectable lines, with its existing
copy context menu. Shell and tab titles are unchanged. The Backslash project link
comes from this checkout's verified origin URL; Microsoft links are explicitly
labeled upstream. Notices and attribution remain intact. Local builds do not run
the upstream Dev update simulation or imply a Backslash update/support service.

For isolated verification, run the generator with `-OutputDirectory`, `-Architecture`
and `-Configuration`, or invoke the host's `BackslashGenerateBuildIdentity` MSBuild
target with a separate `IntDir`. Never install the upstream-identity MSIX merely
to inspect its version. The existing portable distribution script preserves the
compiled EXE identity while the `.portable` marker supplies runtime mode.

## Executable and Start naming

Local Dev builds produce `Backslash.exe` from the host project's TargetName;
this is not a post-build file rename. ProductName/FileDescription are Backslash,
InternalName is Backslash and OriginalFilename is Backslash.exe. The dev suffix,
channel and upstream attribution remain in the complete version identity.
Package display resources already use Backslash Dev. The Dev manifest's tokenized
application executable follows the build target, and its COM host path is updated
explicitly. Shared TerminalExecutable.h keeps unpackaged relaunch, elevation,
alias shim and shell-extension executable/icon paths consistent.

Package Identity/Publisher, application Id/AUMID, COM GUIDs, execution aliases,
signing and security settings are unchanged. Upstream release/preview/canary
filenames remain unchanged; current Backslash local build support is Dev only.
Project/internal DLL names and source filenames remain upstream-compatible.

The portable tooling creates no Start-menu shortcut or registration. Windows'
manual shortcut/pin action will now see Backslash.exe and its Backslash file
description, rather than WindowsTerminal.exe. Existing shortcuts/pins can retain
their old label and target; they are not silently migrated. Point a newly created
shortcut at the new candidate's Backslash.exe and use Backslash as its label.
Changing an existing real Start-menu item or registered package is a separate user
action, not part of building this repo. Moving a portable folder also requires
updating any absolute-path shortcuts. No compatibility WindowsTerminal.exe copy is
included: that could conceal stale helper paths or launch an older binary.