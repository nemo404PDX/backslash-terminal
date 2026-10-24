# Appearance recipes: named starting looks

Status: future design only. No recipe catalog, recipe application, new export behavior or Explorer integration is implemented by this documentation task. Recipes are outside the current export/import implementation slice. This record owns only the future appearance idea; export/import implementation and validation belong to the separate active task.

## Purpose

Offer named starting looks so people can begin with a coherent appearance rather than designing every color, CRT control and bezel choice from scratch. A recipe is a starting point users can adjust, not a locked theme or a requirement to enable visual effects. Candidate names and included fields remain to be decided; no new built-in looks are promised here.

## Defaults and profile overrides

Application defaults provide the baseline. A future recipe could set appearance defaults or explicitly target a selected profile; scope must be clear before application. Explicit profile overrides continue to win in either direction: a profile can enable an effect that defaults disable, or disable/reduce an effect that defaults enable. Applying a default look must not silently clear profile overrides. Users should be able to inspect differences and deliberately reset an override to inherit again. Exact preview, merge and reset interactions remain design work.

For example, a quiet baseline could leave CRT effects off while one profile explicitly enables them. A retro baseline could enable CRT styling while a work profile explicitly keeps it off. Selecting a recipe should help establish that starting appearance without changing shell commands, destinations, connection credentials or running sessions. Existing baseline behavior is recorded in [curated defaults](CURATED-DEFAULTS.md); existing controls and limits are in [CRT appearance](CRT-APPEARANCE.md) and [bezel evidence](BEZEL-DESIGN.md).

## Distinct from full setup export/import

A full setup export/import is a separate configuration-transfer workflow; its actual supported fields, assets, exclusions and validation must come from the export/import task's final implementation record. A future appearance recipe is a narrower reusable look, not a full setup backup, session restore or transfer of machine-specific launch configuration. This document does not claim that recipes are included in the current export format or that any export/import tests have passed.

[Optional Explorer Open in Backslash integration](EXPLORER-INTEGRATION-DESIGN.md) is explicitly tracked on the [roadmap](ROADMAP.md) as separate machine-local future work. Installing or removing it would require its own opt-in and local path/registration handling. A portable setup transfer or appearance recipe must not silently register Explorer integration, copy another machine's registration or imply that such integration already exists.

## Future acceptance questions

- Decide which appearance fields and any referenced assets belong in a recipe, with clear missing-asset/font behavior.
- Show target scope and changed values before applying; preserve explicit profile overrides unless the user deliberately replaces or resets them.
- Keep named looks editable, with effects optional and a clear return to inherited defaults.
- Define recipe versioning and import/export separately from the full setup format; do not change the current export slice through this proposal.
- Treat Explorer registration as a separate optional local operation, never an implicit side effect of applying or transferring a look.