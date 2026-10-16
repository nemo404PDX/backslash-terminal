# Backslash Terminal project

Display brand: **Backslash**. Repository/folder: **backslash-terminal**.

Backslash is a planned independently branded, Windows-first fork of Microsoft Windows Terminal. The goal is approachable saved workspaces, useful inline terminal graphics, understandable palette editing, and optional configurable CRT styling. This is the existing C++/Windows source baseline, not a Rust rewrite or a cross-platform port.

**Current status: isolated portable Backslash Dev builds with configurable CRT/bezel controls and SSH profile controls; modern WSL discovery is corrected.** See [portable implementation](PORTABLE-IMPLEMENTATION.md), [CRT evidence](CRT-APPEARANCE.md), [bezel evidence](BEZEL-DESIGN.md), and [SSH/WSL evidence](SSH-PROFILES.md) for completed scope and unrun visual checks. The unsigned portable delivery is not an installed production release. [Baseline build evidence](BUILD-BASELINE.md) records the earlier preparation stage. The baseline is stable [v1.25.2733.0](https://github.com/microsoft/terminal/releases/tag/v1.25.2733.0), commit `dc8ae365847d41f62d1ebca93815bb00959bc12f`. See [provenance](../../UPSTREAM-SOURCE.md).

## Read next

- [Curated application defaults](CURATED-DEFAULTS.md): requested grid, font, CRT/bezel fallbacks, native centering and accepted usable-grid geometry.
- [Clarified pre-pin/latest-pin phosphor comparison](PHOSPHOR-COMPARISON-13ebff74-35414271.md): resolved build identities, independent saved appearances and picker shortcut RGBs.
- [Badge composed-render fix](BEZEL-BADGE-RENDER-FIX.md): corrected square, aperture-span anchors, actual app/preview evidence and latest portable.
- [Implemented SVG bezel badge](BEZEL-BADGE-IMPLEMENTATION.md): six placements, independent filler, removed plate and fresh portable validation.
- [Badge SVG assets](BACKSLASH-BADGE-ASSET.md): exact backups, conservative cleanup, currentColor/cutout validation and future integration constraints.
- [Frozen phosphor reference](PHOSPHOR-COLOR-REFERENCE-dc027cae.md), [frozen bezel reference](BEZEL-COLOR-REFERENCE-dc027cae.md), and [cross-build color audit](PHOSPHOR-REGRESSION-AUDIT-960a9f9.md): literal backups, swatches and source/package evidence.

- [Baseline build evidence](BUILD-BASELINE.md): verified source-copy artifact/logs, toolchain and runtime limits.

- [Application identity plan](APP-IDENTITY.md): portable-first isolation and later package/alias/registration choices.

- [Exact documentation inventory](DOCUMENTATION.md): paths and purposes.
- [Implemented SSH profiles and modern WSL discovery](SSH-PROFILES.md): shell choices, explicit restoration, and validation evidence.
- [Independent SSH editor audit](SSH-LOCAL-EDITOR-AUDIT.md): capability gates and source-confirmed gaps.
- [SSH local editor research](SSH-LOCAL-EDITOR-RESEARCH.md): preferred inline composition feasibility, source seams and validation gates.
- [Shell productivity setup](SHELL-PRODUCTIVITY.md): opt-in provider setup and access to older history matches.
- [Saved tab groups](SAVED-TAB-GROUPS.md): implemented named terminal layouts, folder modes, management and validation limits.
- [Connection identity titles](CONNECTION-TITLES.md): curated default and Appearance opt-out, explicit title precedence and safe launch metadata.
- [Internal app commands](APP-COMMANDS-DESIGN.md): future dedicated command field, dispatch reuse, slash/collision rules and native pane actions.
- [Resumable remote sessions](RESUMABLE-SESSIONS-DESIGN.md): future opt-in ownership, missed output/results, lifecycle and shared-session identity.
- [Settings layout proposal](SETTINGS-LAYOUT-PROPOSAL.md) and [JSONC inventory](settings-layout.proposed.jsonc): reviewable task categories, actual controls, key/resource references and search gaps.
- [Feature scope](FEATURES.md): requested behavior, proposed safeguards and open choices.
- [Palette design](PALETTE.md): agreed copy behavior and precise color categories.
- [Font catalog](FONTS.md): requested opt-in font discovery and download design.
- [CRT Appearance design](CRT-APPEARANCE.md): controls, rendering constraints and quality requirements.
- [CRT rendering plan and performance audit](CRT-RENDERING-PLAN.md): visibility/resource findings, isolated measurements, implemented tint/scanline controls and acceptance gates.
- [Bezel implementation and future material design](BEZEL-DESIGN.md): implemented frame controls, corner/centering fixes, validation and remaining visual checks.
- [Keyboard sounds design](KEYBOARD-SOUNDS-DESIGN.md): future optional coherent press/release audio, three sound profiles, overall Weight and silent paste/output.
- [Terminal status bars design](TERMINAL-STATUS-BARS-DESIGN.md): optional top/bottom rows, four anchored slots, reserved layout space and metadata limits; not implemented.
- [Phased roadmap](ROADMAP.md): bounded future tasks with acceptance checks.
- [Baseline evidence](BASELINE.md): reused capabilities, source paths and upstream research.
- [Brand concept](../../branding/concepts/README.md): approved visual direction and exact image transfer status.
- [Main project README](../../README.md): concise Backslash introduction, implemented features and planned additions.
- [MIT license](../../LICENSE) and [upstream notices](../../NOTICE.md).

“Requested” describes agreed product scope, “proposed” describes recommendations awaiting design decisions, and “open” describes choices still to resolve. None of these labels means implemented.

## Historical preparation boundaries

The following paragraph records the original preparation scope, before the separately authorized implementation slices linked above.

This preparation authorizes the folder rename, documentation and concept asset retrieval. It does not authorize code changes, builds, package restoration, installations, Git initialization, remotes, commits, pushes or publication. The user will configure their own remote later. Future implementation should start with a concrete, separately authorized slice of the roadmap.
