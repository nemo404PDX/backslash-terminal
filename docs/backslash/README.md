# Backslash guides

Backslash is an experimental Windows terminal based on Microsoft Windows Terminal. Start with the [project overview and download instructions](../../README.md). Release notes describe the features included in each downloadable version; source documentation may also describe development features.

## Using Backslash

- [Portable operation and storage](PORTABLE-IMPLEMENTATION.md)
- [Saved tab groups](SAVED-TAB-GROUPS.md) and [startup groups](STARTUP-GROUP-REOPEN.md)
- [Pinned destinations](PINNED-TABS-IMPLEMENTATION.md) and [supported folder reporting](LIVE-CWD-AND-SESSION-METADATA.md)
- [Settings placement](SETTINGS-PLACEMENT.md) and [pane close behavior](FOCUSED-PANE-CLOSE.md)
- [SSH import and authentication requirements](SSH-PORTABLE-IMPORT.md), [SSH/WSL profiles](SSH-PROFILES.md) and [connection titles](CONNECTION-TITLES.md)
- [CRT appearance](CRT-APPEARANCE.md), [bezel and badges](BEZEL-DESIGN.md), [badge colors](CURRENTCOLOR-BADGE-CONTROLS.md) and [bezel finishes](BEZEL-FINISH-EXTENSIONS.md)
- [Default appearance and geometry](CURATED-DEFAULTS.md)

Saved groups reopen layouts with fresh sessions. SSH aliases depend on each computer's configuration. Experimental SFTP supports individual files with noninteractive authentication and an already trusted host key. Resumable remote sessions are not implemented. Read each guide's restrictions before relying on a feature.

## Development and design references

- [Build guide](../../backslash/tools/backslash/README.md), [repository layout](REPOSITORY-LAYOUT.md) and [documentation catalog](DOCUMENTATION.md)
- [Feature scope](FEATURES.md), [roadmap](ROADMAP.md) and [file-transfer design](FILE-TRANSFER-DESIGN.md)
- [Explorer integration](EXPLORER-INTEGRATION.md) (development feature; explicit opt-in)
- [Transfers and SFTP](TRANSFERS-FOUNDATION.md) and [backend settings](TRANSFERS-BACKENDS.md) (experimental)
- [Rendering design and validation](CRT-RENDERING-PLAN.md), [Pins validation](../dev/PINS-VALIDATION.md) and [build baseline](BUILD-BASELINE.md)
- [Maintainer handoff notes](../dev/HANDOFF.md) for machine-local artifacts, historical verification and operational context
- [Upstream source provenance](UPSTREAM-SOURCE.md), [MIT license](../../LICENSE) and [third-party notices](../../NOTICE.md)

Technical validation records are dated contributor references. Their test results apply to the identified builds and do not guarantee compatibility for every release or machine. Plans describe proposed behavior and are not release commitments.
