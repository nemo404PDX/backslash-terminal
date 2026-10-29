# Optional Explorer integration: Open in Backslash

Original design background; see the [current development feature guide](EXPLORER-INTEGRATION.md) for modern Windows 11 integration and legacy-entry recovery. The checklist below describes the design goals, not a current test report.

## Requested behavior

Provide an optional Windows Explorer **Open in Backslash** action that launches a chosen portable Backslash copy in the selected folder or folder background. Keep its identity separate from the existing **Open in Terminal** action. Do not replace that action, take over default-terminal registration or require an installed package merely to run the portable application.

## Portable-compatible lifecycle

- Registration is an explicit machine-local opt-in, with a clear chosen executable path and scope. Prefer a per-user option where feasible; determine the exact supported Windows integration mechanism during implementation.
- Quote executable and folder arguments correctly, including spaces, Unicode and shell metacharacters; define which folder contexts are supported and how they select the working directory. Avoid passing folder text through an unnecessary command shell.
- Moving, renaming or removing the portable folder can invalidate a registered absolute executable path. Define detection and a clear repair/re-register action pointing to the new location; do not silently bind to another copy.
- Updating a portable copy at the same location should preserve the user's explicit choice. Updating to a new location must offer deliberate retargeting and identify the old registration. Define behavior for multiple portable copies and stale paths.
- Provide an explicit unregister action that removes only Backslash-owned entries for the selected registration, including cleanup when the former executable is missing. Leave other terminal registrations and unrelated context-menu items intact.
- Treat registration as local machine state. Full setup export/import or a future appearance recipe must not silently install, copy or activate Explorer registration from another machine.

## Design acceptance checklist

- [ ] Opt-in registration creates a distinct Open in Backslash action and leaves Open in Terminal intact.
- [ ] Selected-folder and folder-background launches resolve supported paths correctly, including spaces and Unicode, without command injection.
- [ ] An unregistered portable copy still runs normally; multiple-copy selection is explicit.
- [ ] Same-location update, moved executable, missing executable and repair/re-register behavior are tested.
- [ ] Unregister removes only owned entries and works after a move/removal of the original copy.
- [ ] Setup transfer and appearance-recipe application do not change machine registration automatically.

See [application identity planning](APP-IDENTITY.md), [portable delivery](PORTABLE-IMPLEMENTATION.md) and [appearance recipes](APPEARANCE-RECIPES-DESIGN.md). Current behavior is described in the [feature guide](EXPLORER-INTEGRATION.md); this original design checklist is not validation evidence.
