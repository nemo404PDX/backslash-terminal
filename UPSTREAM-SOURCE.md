# Upstream source provenance

Prepared 2026-10-13 UTC for a future renamed fork. Upstream files and branding are unchanged.

- Destination: D:\codex-workspace\backslash-terminal
- Official releases: https://github.com/microsoft/terminal/releases
- Release: Windows Terminal v1.25.2733.0, published 2026-10-02T20:27:27Z, prerelease=false. Verified as latest stable through the official releases page and GitHub latest-release API on preparation date.
- Release URL: https://github.com/microsoft/terminal/releases/tag/v1.25.2733.0
- Tag: v1.25.2733.0
- Annotated tag object: 643609c3f9e022c9fb44366a43f5ebb8567f9722
- Upstream commit: dc8ae365847d41f62d1ebca93815bb00959bc12f
- Official source archive: https://codeload.github.com/microsoft/terminal/zip/refs/tags/v1.25.2733.0
- Archive SHA-256: 1D2B9D701E66333D698412DBE4C8240E5239837B763E7DA79C8DEBE2B57CDAEA
- Download retained at C:\Users\nemo\AppData\Local\Temp\windows-terminal-v1.25.2733.0-source.zip (temporary storage).
- Metadata: https://api.github.com/repos/microsoft/terminal/releases/latest
- Tag resolution: https://api.github.com/repos/microsoft/terminal/git/tags/643609c3f9e022c9fb44366a43f5ebb8567f9722
- Tree inspection: https://api.github.com/repos/microsoft/terminal/git/trees/dc8ae365847d41f62d1ebca93815bb00959bc12f?recursive=1

## Dependencies and completeness

The official recursive commit tree was not truncated and contained zero gitlinks (mode 160000) and no .gitmodules. There are no pinned submodules to materialize in this release. Bundled source dependencies, oss content, attribution, LICENSE, NOTICE.md, NOTICE.html and component licenses were preserved from the archive.

This is a complete upstream source snapshot, not an offline build dependency bundle. No package restore or build scripts were executed. NuGet dependencies in dep/nuget/packages.config, build/packages.config and scratch project manifests remain unrestored. vcpkg.json declares fmt 12.1.0, ms-gsl 4.2.2, jsoncpp 1.9.6, cli11 2.6.1 and cmark 0.31.1 with baseline 927f62e4b8838bd7e441e9c45103a16ffd75007e; these packages and the vcpkg tool/registry were not downloaded. Future builds need the upstream documented Visual Studio/Windows SDK prerequisites and dependency restoration; package-feed accessibility has not been validated.

## Preparation constraints

No Git repository was initialized, no remote added, no history downloaded, and no build/install/branding changes performed. Local D:\AGENTS.md, D:\codex-workspace\AGENTS.md and D:\codex-workspace\.agents\skills were absent. No AGENTS.md was found in the extracted source.
## Verification

All 3,664 extracted archive files were checked against their ZIP-entry SHA-256 hashes: zero mismatches. Recursive inspection found zero files or directories named .git. That initial verification preceded the subsequent documentation preparation described below.


## Backslash documentation preparation (2026-10-13)

The source directory was renamed to D:\codex-workspace\backslash-terminal after confirming the destination was absent. The initial archive file verification above describes the source preparation before this documentation update. README.md now has a fork introduction with its complete original bytes retained as the trailing content. Added project documentation in docs/backslash and branding/concepts; application identifiers and source implementation are unchanged. LICENSE, NOTICE.md and component notices are unchanged. No Git initialization, remotes, builds, restoration, installs or publication occurred. Exact approved Library image materialization is blocked by the current helper's unsupported Windows extended-attribute call; see branding/concepts/README.md.
Documentation verification: checked all 3,664 original archive files after the rename. README.md is the only changed upstream file, and its complete original bytes remain as trailing content. All other original archive files, including licenses/notices and implementation source, are byte-identical. Verified 53 local Markdown links in the fork introduction/new docs with zero broken targets; recursive .git entry count is zero. Added seven docs/backslash documents and branding/concepts/README.md. The old source directory is absent. Concept image remains absent because materialization failed; no fonts were downloaded or installed.

Font documentation follow-up: added requested prompt-theme glyph coverage and realistic prompt preview requirements, explicitly separating ligatures from Nerd Font/Powerline coverage. No font or prompt engine installation/configuration performed.


Shell productivity documentation follow-up: added optional shell-specific setup, multiple/older history matches, explicit suggestion exclusion, install target/privacy boundaries and acceptance examples. No shell installs/configuration, implementation, Git or remote changes performed.


Command suggestion requirement clarification: automatic history candidates use confirmed success as a strict eligibility filter, excluding failed and unknown outcomes by default. Documented separate diagnostic history, reliable shell status, edge semantics and exact older-success/newer-typo fixtures. Custom engine remains an open option. Documentation only.

History persistence follow-up: documented requested shared history across tabs/windows/restarts, concurrent completion-time ingestion, provenance/scoping, private/retention policies and fixtures. Proposed central storage is not an accepted architecture; no capture, implementation, installs or cloud sync performed.


History discovery follow-up: clarified cross-directory reuse, tomorrow persistence, context metadata as discovery aids, configurable broader successful suggestions with origin, and open retention/archive/export without a fixed 30-day policy or silent irreversible pruning. Documentation only.


History configurability follow-up: successful-only remains the original user's preferred mode, with strict confirmed-success eligibility scoped to that selected mode. Documented proposed independent storage/suggestion controls, independent shell history and no implicit deletion on mode changes. Earlier strict requirement entries describe prior design clarifications; current configurable requirements are in docs/backslash/SHELL-PRODUCTIVITY.md. Documentation only.

SSH history follow-up: documented connection-scoped local recall without remote autosuggestion plugins, manual Unknown entries, safe copy/insertion boundaries and separate opt-in remote integration. No host installations/modifications or implementation performed.


SSH editor research follow-up: added docs/backslash/SSH-LOCAL-EDITOR-RESEARCH.md and exact inventory. Inspected pinned input/ConPTY/parser/grid/cursor/marks/history/completion/settings seams; researched official public references. Preferred inline LOCAL composition supersedes pill exploration. No runtime experiments, builds, installs, host queries or implementation performed. Existing source/license files remain unchanged.

Research verification: 128 local Markdown targets checked with zero broken links. All 3,664 original archive files checked: only root README differs, retaining the complete original README bytes; all other original implementation/license/notice files unchanged. Zero .git entries. Exact inventory contains 13 project documentation paths. Source symbols were checked against the pinned tree; stale resize-comment name corrected to actual ControlCore::_refreshSizeUnderLock. Inline local expansion lifecycle and current preferred scope documented; no proof-of-concept stages executed.

Connector validation: actual Context7 resolve/query and Microsoft Learn search/full-page fetch succeeded. Main-branch Context7 excerpts kept separate from pinned source facts. No connector installation/configuration or host queries performed.

Independent audit reconciliation: read docs/backslash/SSH-LOCAL-EDITOR-AUDIT.md and accepted capability-first verdict; strengthened ownership, exact text/multiline delivery, no replay, echo, metadata trust/correlation, IME/device replies/locking and durable-record gates. Added audit links/inventory; audit SHA-256 DBAF298599DB3218116134E9F12C7DEF440DE929BE99451E1388ED6757324C04 remains unchanged. Current exact project inventory is 14 documentation paths. No implementation/build/host changes.

Final audit reconciliation verification: 14 documentation files, 160 local links, zero broken targets; audit report SHA-256 unchanged. Archive comparison still passed for all 3,664 original files (only root README differs and retains original bytes). A .git directory was newly observed at 2026-10-13T20:10:50Z in the shared workspace, after earlier zero-.git checks. This documentation writer did not create or modify it and left it untouched; earlier no-.git statements describe their recorded verification times, not the current checkout state.

