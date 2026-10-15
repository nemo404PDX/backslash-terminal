# Phased roadmap

[Project entry point](README.md) · [Feature scope](FEATURES.md)

This is a proposed implementation sequence. Source/documentation preparation and an unchanged upstream source-copy build are completed; see [build evidence](BUILD-BASELINE.md). Future changed-source builds/implementation and launch/install actions require their own authorization. Each phase should be a small reviewable vertical slice, with focused tests and no claim that the full roadmap is delivered.

## Phase 0 — prepared source and decisions

Completed: stable archive source, provenance, no upstream Git history, folder organization, project docs and a successful unchanged source-copy Release|x64 unsigned package build. [Build evidence](BUILD-BASELINE.md) is not runtime or Backslash identity proof. Blocked: approved concept image local materialization (see [brand record](../../branding/concepts/README.md)).

Next agent: read provenance and this doc; verify instructions; choose one next slice with the user. Resolve identity strings before editing them. Maintain a decision record for schema, controls and scope decisions. Do not initialize Git or add a remote on the user's behalf.

## Phase 1 — independent identity and coexistence

Read the [source-grounded application identity plan](APP-IDENTITY.md). Portable-first is recommended: separate output/.portable/adjacent settings, with explicit fragment/catalog isolation and no automatic import. The completed unchanged upstream build establishes a baseline; future identity edits and changed-source builds are a separate reviewed slice. Follow [minimal next prerequisites](BUILD-BASELINE.md).

Task: audit package identity/publisher, executable/app aliases, app IDs, resources, shortcuts, settings/cache locations, activation/default-terminal registration, update/release paths and attribution. Produce an explicit migration map; exact identifiers and signing/distribution model are open.

After separately authorized implementation/build: produce an independently identified Backslash build that coexists with installed Windows Terminal. Acceptance: launch each separately, isolated settings/activation/aliases, no accidental upgrade/replacement, preserved notices and correct upstream attribution. Folder naming alone does not accomplish this.

## Phase 2 — first saved workspace slice

Task: start with a named mixed-profile layout over existing action/layout infrastructure. Trace model serialization to UI and launch behavior. Choose schema/versioning and missing-reference handling.

Acceptance: save/reopen a WSL + PowerShell split with size/profile/directory references; missing profiles are recoverable; reopening starts sessions without promising process persistence. Extend to developer shells/SSH, then pin/group operations as separate slices. Evaluate upstream pinning work before porting any code.

## Phase 3 — SSH bookmarks and productive layouts

Task: choose OpenSSH config interoperability and key-reference storage; implement bookmark catalog/grouping then launch into workspace panes. Evaluate an existing terminal editor and preview/build workflow before proposing embedded editor architecture.

Acceptance: editable name/host/user/port/key reference, grouping, useful launch errors, clear machine identity; exported files contain no passwords/private-key bytes under the proposed safeguard. External file changes never silently overwrite modified editor buffers. Persistent detach/reattach requires a separate decision.

## Phase 3b — command composition and shared history

Preferred SSH investigation: familiar inline LOCAL editing/completion/multiline preview into one persistent remote shell, explicit Enter submission and reliable interactive pass-through. Retire pill/centered expansion from active scope. Preserve native local/remote providers as a supported choice. Start with the [technical research staged go/no-go plan](SSH-LOCAL-EDITOR-RESEARCH.md) and [independent audit](SSH-LOCAL-EDITOR-AUDIT.md), not product implementation. Critical gates: input ownership, exact multiline submission, echo reconciliation and trustworthy result correlation.

Validate local upward presentation shifting/reserved editor rows over the intact remote grid, no remote layout bytes, current-buffer/cursor edits, stale completion handling, Unicode, multiline/echo, Ctrl-C/password/REPL/TUI transitions, resize/asynchronous output, selection/copy/IME/accessibility and remote cwd/environment preservation. Completion acceptance must not execute; deliberate Enter submission is separate. Freeze revision/context, preserve exact submitted text, never auto-replay ambiguous delivery, and reject forged/uncorrelated outcome marks. Validate TSF/IME, device replies and no locks across blocking writes. If the preferred inline model fails, disclose the limitation rather than silently substituting a pill. Transport choice, remote completion data and optional integration/bootstrap remain decisions.

History slice: configurable storage/suggestion modes, with the user's preferred shared-persistent + Successful-only mode. Concurrent execution records survive tabs/windows/restarts, retain reliable outcomes/provenance and update promptly. Strict success eligibility applies when selected; failed/unknown records remain inspectable with markers and All/Successful/Failed/Unknown filters. No implicit deletion on mode changes; retention/archive/export/private-session policy remains open.

Default suggestions respect verified machine/connection, user and compatible shell; cross-directory reuse remains available within that scope. All connections search is deliberate/origin-labeled; cross-connection autosuggestion sharing is opt-in. Unknown/nested targets must not silently inherit local identity. SSH no-plugin manual recall/copy is supported fallback with Unknown entries, not proof of inline-editor/status feasibility.

Local shell productivity providers are candidates to research, not selected packages. Any future setup must preview target/source/dependencies/config changes and preserve configuration. No silent remote modifications/history transmission. See [shell acceptance fixtures](SHELL-PRODUCTIVITY.md) and [research go/no-go evidence](SSH-LOCAL-EDITOR-RESEARCH.md). No implementation, setup, build or host probe is authorized by this roadmap.

## Phase 4 — palette copy/fill and extended editor

Task: resolve generation algorithm, storage/versioning and override semantics in [palette design](PALETTE.md). Implement copy/fill as the first slice; add persisted indexed editing separately through model/adapter/editor/reset paths.

Acceptance: supplied colors preserved, fill once, independent destination edits, neutral/intensity constraints, save/reload round-trip, correct ANSI/indexed/RGB distinctions and OSC reset coverage across Windows/WSL/SSH profiles.

## Phase 5 — useful inline graphics

Task: build a user-facing Sixel output demonstration and compatibility matrix before considering Kitty graphics. Do not equate background-image settings with output images.

Acceptance: scrolling/alternate screen/resize/clipping behavior, bounded image resources and graceful unsupported output. Record local, WSL and SSH test environments. Decide additional protocol scope separately.

## Phase 6 — configurable CRT Appearance

Task: first parameterize existing sampled glow and DPI-scaled scanlines in Retro.hlsl/atlas custom_shader_ps.hlsl through profile controls, saved settings and live preview. Reuse BackendD3D shader hooks; include neutral/Nop comparison, reset, readability and GPU/DPI/resize checks. Saved control persistence is distinct from phosphor afterimage history. Validate time wrap/continuous redraw and diagnostic errors. Higher-quality bloom/multiple passes, curvature with input mapping, temporal history and bezel composition are later separate proofs; animation/background samples do not deliver them. See [sample map](BASELINE.md) and [concrete first CRT slice](CRT-APPEARANCE.md).

Acceptance: [CRT quality bar](CRT-APPEARANCE.md), saved profile state, reset/compare, reduced motion, readable text, stable resize, aligned input/cursor/selection, bounded GPU/animation/memory. Intentional glitch must never excuse instability. Review cool-retro-term component licenses before any copying.

## Phase 6b — curated fonts

Task: research specific font versions, ligature/patched glyph coverage and licenses, choose maintainable catalog data and installation scope, then implement explicit official-asset Install/apply and representative previews. Reuse verified font feature support. Acceptance: [font catalog checks](FONTS.md), accurate independent badges, operator/ambiguous glyph/box drawing/prompt previews, explicit downloads, pinned version/hash verification, offline/error recovery and retained notices.

## Deferred follow-up - optional keyboard sounds

[Keyboard sounds design](KEYBOARD-SOUNDS-DESIGN.md) preserves the future bounded slice: coherent mechanical press/release audio with exactly Normal, Silent (dampened, not mute) and Mixed sound profiles; separate candidate switch styles, one overall Weight and independent Volume. Resolve key classification, listening model and settings scope before separately authorized implementation. Acceptance includes silent paste/output, actual down/up pairing, held-key repeat, focus cleanup, bounded latency/resources and disabled no-work behavior. No keyboard-sounds implementation is approved; groups and current fixes remain the priority.

## Release-readiness update (2026-10-15)

User-confirmed checks: group creation, startup and saving while Settings is open work. This is user-reported interactive evidence, not a substitute for remaining automated/native verification. The user subsequently confirmed background opacity and phosphor tint passed after the hue-retaining correction; see CRT-RENDERING-PLAN.md. Branding must be fixed before any preview release. Version and tag remain undecided; no release, tag or publication is authorized by this note.

[Terminal status bars](TERMINAL-STATUS-BARS-DESIGN.md) remain a separate future design: optional top/bottom rows reserve space inside the screen/bezel, with two anchored items per row. Path/host/user/time first and folder counts later are proposals, not implemented scope.

## Phase 7 — release readiness

Only after authorized builds and implementation: test upgrades/migrations, accessibility, packaging/signing, coexistence, graphics/performance and recovery; finalize vector/ICO assets and typeface rights, review name/trademark clearance, licenses and notices. Publication, remote setup and installs remain separately authorized actions.


## Follow-up designs: commands, session ownership and settings navigation

Review [internal app commands](APP-COMMANDS-DESIGN.md), [resumable sessions](RESUMABLE-SESSIONS-DESIGN.md) and the [source-backed settings layout proposal](SETTINGS-LAYOUT-PROPOSAL.md) as separate bounded slices. Dedicated local app-command ownership/typed dispatch precedes any optional inline interception. A first resumable job must survive disconnect and return the same session's result/missed output; viewers, input locks and opt-in presence follow later. Presence then progresses to organization-enforced identity/access, with server/gateway policy covering alternate clients. Saved tab groups are implemented layout recipes and do not provide these persistence or identity features.

The settings proposal places Workspaces, Connections and Retro Appearance prominently and inventories inherited controls without changing runtime navigation. Search should reuse existing indexing/navigation and gain exact-control coverage plus ancestor expansion. Animated background/noise optimization requires the source/runtime distinctions and measured comparison in [CRT rendering research](CRT-RENDERING-PLAN.md); no benchmark result is implied.
