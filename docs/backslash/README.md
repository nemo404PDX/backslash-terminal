# Backslash Terminal project

Display brand: **Backslash**. Repository/folder: **backslash-terminal**.

Backslash is a planned independently branded, Windows-first fork of Microsoft Windows Terminal. The goal is approachable saved workspaces, useful inline terminal graphics, understandable palette editing, and optional configurable CRT styling. This is the existing C++/Windows source baseline, not a Rust rewrite or a cross-platform port.

**Status: source preparation and design documentation only.** No Backslash features, identity migration, build, installation or release have been completed. The baseline is stable [v1.25.2733.0](https://github.com/microsoft/terminal/releases/tag/v1.25.2733.0), commit `dc8ae365847d41f62d1ebca93815bb00959bc12f`. See [provenance](../../UPSTREAM-SOURCE.md).

## Read next

- [Exact documentation inventory](DOCUMENTATION.md): paths and purposes.
- [Independent SSH editor audit](SSH-LOCAL-EDITOR-AUDIT.md): capability gates and source-confirmed gaps.
- [SSH local editor research](SSH-LOCAL-EDITOR-RESEARCH.md): preferred inline composition feasibility, source seams and validation gates.
- [Shell productivity setup](SHELL-PRODUCTIVITY.md): opt-in provider setup and access to older history matches.
- [Feature scope](FEATURES.md): requested behavior, proposed safeguards and open choices.
- [Palette design](PALETTE.md): agreed copy behavior and precise color categories.
- [Font catalog](FONTS.md): requested opt-in font discovery and download design.
- [CRT Appearance design](CRT-APPEARANCE.md): controls, rendering constraints and quality requirements.
- [Phased roadmap](ROADMAP.md): bounded future tasks with acceptance checks.
- [Baseline evidence](BASELINE.md): reused capabilities, source paths and upstream research.
- [Brand concept](../../branding/concepts/README.md): approved visual direction and exact image transfer status.
- [Upstream README](../../README.md): preserved upstream background and build instructions.
- [MIT license](../../LICENSE) and [upstream notices](../../NOTICE.md).

“Requested” describes agreed product scope, “proposed” describes recommendations awaiting design decisions, and “open” describes choices still to resolve. None of these labels means implemented.

## Working boundaries

This preparation authorizes the folder rename, documentation and concept asset retrieval. It does not authorize code changes, builds, package restoration, installations, Git initialization, remotes, commits, pushes or publication. The user will configure their own remote later. Future implementation should start with a concrete, separately authorized slice of the roadmap.

