# Settings layout proposal

Status: review only, 2026-10-14. This does not reorganize the running UI or assign new settings keys. The machine-readable [proposal](settings-layout.proposed.jsonc) records current controls and future nodes separately; [profiles.schema.json](../../backslash/doc/cascadia/profiles.schema.json) remains the setting schema. The inventory is from the current uncommitted source over checkpoint `bfd6c420`, including saved-groups build `5666c86f`.

## Proposed navigation

Put Workspaces, Connections and Retro Appearance first. Keep inherited configuration in task-specific sections underneath Terminal configuration. Selecting a profile changes the editing context, with visible scope/inheritance and reset-to-parent affordances; selecting a saved group edits its recipe. No new per-workspace appearance inheritance is implied. Planned nodes should remain clearly marked or absent until implemented.

```text
Workspaces
  Saved tab groups
    Saved tab group [Backslash]
    Group name [Backslash]
    Starting folders [Backslash]
    Update group [Backslash]
    Delete group [Backslash]
    Pinned workspaces [planned]
  Commands and panes
    Actions
    Local app-command field [planned]
Connections
  Profiles
    Name
    Command line
    Starting directory
    Icon
    Tab title
    Tab color
    Run this profile as Administrator
    Hide profile from dropdown
    New empty profile
    Duplicate a profile
    Profile no longer detected
    Name
    Original Source
  SSH shells and host discovery
    SSH remote shell [Backslash]
    Custom remote shell command [Backslash]
    Apply custom command [Backslash]
    Scan SSH hosts [Backslash]
    Discovered SSH hosts [Backslash]
    Add / Restore [Backslash]
    Local and remote files [planned]
  Resumable sessions [initially collapsed]
    Normal or resumable [planned]
    Remembered sessions and results [planned]
    Viewing and control [planned]
    Require owner approval for takeover [planned]
  Presence and organization identity [initially collapsed]
    Who is connected [planned]
    Managed organization access [planned]
    Sign in and enroll this device [planned]
Retro Appearance
  Screen
    Retro terminal effects [Backslash]
    Retro glow intensity [Backslash]
    Retro scanline intensity [Backslash]
    Dark scanline thickness (DIP) [Backslash]
    Clear gap between scanlines (DIP) [Backslash]
    Screen phosphor color [Backslash]
    Screen tint intensity [Backslash]
    Animated static/noise [planned]
  Casing
    Retro bezel [Backslash]
    Casing width [Backslash]
    Casing finish [Backslash]
    Backslash badge [Backslash]
    Center aperture in fullscreen [Backslash]
Terminal configuration
  Startup and windows
    Default profile
    On startup [Backslash]
      Start with: Default profile / Saved group [Backslash]
      Startup group [searchable; Saved group only; Backslash]
    Default terminal application
    Language (requires relaunch)
    Default IME input mode
    Launch on machine startup
    When Terminal starts
    New instance behavior
    Launch size
    Launch parameters
  Window and tabs
    Show connection identity in tab titles [Backslash; default on]
    Application Theme
    Position of newly created tabs
    Hide the title bar (requires relaunch)
    Always show tabs
    Show tabs in full screen
    Use acrylic material in the tab row
    Use active terminal title as application title
    Always on top
    Tab width mode
    Pane animations
    Always display an icon in the notification area
    Hide Terminal in the notification area when it is minimized
    Automatically hide window
    Display a shield in the title bar when Windows Terminal is running as Administrator
    Allow acrylic material in unfocused windows
  Text and backgrounds
    Color scheme
      Foreground
      Background
      Selection background
    Font face
    Missing fonts:
    Non-monospace fonts:
    Font size
    Line height
    Cell width
    Font weight
    Variable font axes
    Font features
    Builtin Glyphs
    Full-color Emoji
    Automatically adjust lightness of indistinguishable text
    Cursor shape
    Cursor height
    Cursor color
    Background image path
    Background image stretch mode
    Background image alignment
    Background image opacity
    Intense text style
    Background opacity
    Enable acrylic material
    Padding
    Scrollbar visibility
    Color schemes
    This color scheme is part of the built-in settings or an installed extension
    Colors
    Set color scheme as default
    Rename color scheme
    Font discovery and download [planned]
  Input and interaction
    Automatically copy selection to clipboard
    Text formats to copy to the clipboard
    AltGr aliasing
    Remove trailing white-space in rectangular selection
    Scroll to input when typing
    Remove trailing white-space when pasting
    History size
    Word delimiters
    Profile termination behavior
    Snap window resizing to character grid
    Bell notification style
    Tab switcher interface style
    Bell sound
    Automatically focus pane on mouse hover
    Display a menu on right-click
    Adjust terminal font size by scrolling while holding the Ctrl key
    Display marks on the scrollbar
    Adjust terminal opacity by scrolling while holding the Ctrl and Shift keys
    Automatically mark prompts on pressing Enter
    Automatically detect URLs and make them clickable
    Launch this application with a new environment block
    Default URL to use for the "Search web" action
    Experimental: Reposition the cursor with mouse clicks
    Experimental: Add key bindings to color selected text
    Experimental: Display suggested input in rainbow colors
    Warn when closing more than one tab
    Warn when pasting more than 5 KiB
    Warn when pasting newlines
  Terminal protocols [initially collapsed]
    Suppress title changes
    Use the legacy input encoding
    Allow Kitty Keyboard Protocol
    Allow DECRQCRA (Request Checksum of Rectangular Area)
    Allow OSC 52 (Manipulate Selection Data) to write to the clipboard
    ENQ (Request Terminal Status) response
    Path translation
  Rendering and compatibility [initially collapsed]
    Allow Windows Terminal to run in the background
    Text antialiasing
    Graphics API
    Text measurement mode
    Disable partial Swap Chain invalidation
    Width of ambiguous characters
    Use software rendering (WARP)
  New tab menu
    Folder Name
    Folder Icon
    Allow inlining
    Allow empty
    Profile
    Separator
    Folder
    Profile matcher
    Remaining profiles
  Sources and maintenance [initially collapsed]
    Scope
    Enable debugging features
    Active Extensions
    Clear cache
    Reset to default settings
```

## Implemented startup controls

Separate On startup controls are implemented within saved-groups scope. Default profile stays the existing single profile setting for ordinary tabs/windows. Start with chooses Default profile (default) or Saved group; only Saved group exposes a searchable group-only Startup group picker. Save/Reset uses the Settings draft; stable ID selection survives rename.

Only the initial ordinary launch uses the selected set, without an extra default tab and with saved focus. Additional windows, shortcuts while running, explicit requests and connection/content transfers preserve behavior. User-enabled previous-session restoration takes precedence without appending the startup set; absent a restorable layout, use the eligible initial starting set. Missing/deleted/invalid groups or profiles fall back to one default-profile tab with notice. Folder fallback stays explicit and startup-specific. No group autosave or automatic last-workspace recovery.

[Saved tab groups](SAVED-TAB-GROUPS.md) records keys, source, runtime evidence and remaining native checks. Keys are `backslash.startupWithSavedGroup` (false by default) and `backslash.startupGroup` (stable GUID, empty by default). The two historical `planned.startup.*` IDs stay stable, while their status/backing/source now record implementation. Generic startupActions is not rewritten. Generated index entries are confirmed; keyboard/screen-reader, conditional search/focus, notices and native folder confirmation remain unrun. Existing TeachingTip notes and Profiles collapse proposal remain separate.

## Profiles sidebar behavior

Required UX, proposed only: clicking the Profiles heading toggles only its profile list. `+ Add a new profile` is a separate sibling outside that collapsed list and stays accessible. The task categories above Profiles keep their space, reducing needless sidebar scrolling. This is a navigation requirement; the profile controls in the tree above describe page content.

```text
Workspaces / Connections / Retro Appearance / Terminal configuration
Profiles [expand/collapse header]
  Profile list [the only content hidden when collapsed]
+ Add a new profile [always accessible sibling]
```

Add, creating a profile, editing a profile and other navigation must preserve the list's collapse state and sidebar viewport. They must not autoexpand Profiles or force scrolling to the selected bottom entry. The selected profile and its editing content can remain active while the sidebar entry is hidden. When the user explicitly expands the list, its selected highlight reflects that active selection without an unnecessary viewport jump. This sidebar rule is independent of expanding containers inside a search result's destination content page.

Expose the Profiles header as a keyboard-accessible expand/collapse control with native automation state. Collapsed entries leave keyboard/automation navigation; collapsing a focused entry returns focus to the header without switching content. Add remains reachable in either state. Preserve collapse during navigation; initial state and persistence across application launches remain review decisions. If restart persistence is chosen, decide local UI-state storage separately; no settings.json key is assigned here.

Current [MainPage.xaml](../../backslash/src/cascadia/TerminalSettingsEditor/MainPage.xaml) has a Profiles item header, while [MainPage.cpp](../../backslash/src/cascadia/TerminalSettingsEditor/MainPage.cpp) initializes the profile list and creates/navigates to new profiles. This proposal does not claim the header already implements the requested collapse behavior. Future work must verify NavigationView selection and scroll behavior under the new structure.

Acceptance checklist:

- Collapse Profiles and verify only its list disappears; Add and categories above remain accessible.
- Click/keyboard-activate Add while collapsed; create a profile and verify content opens without list expansion or forced scroll.
- Edit/select/navigate through existing profiles and search deep links while collapsed; retain active content, selection and sidebar state.
- Explicitly expand and confirm selected highlight reflects active content without unnecessary viewport jumps.
- With many profiles and a selected last entry, repeat Add/create/edit and check viewport stays stable.
- Collapse/expand with keyboard and screen reader; verify expand state, focus recovery and hidden-entry exclusion.
- Verify Save/Reset/reload preserve profile data and navigation behavior; define restart-state policy before any persistence implementation.

## Search and expansion

The existing [generator](../../backslash/tools/GenerateSettingsIndex.ps1) reads authored `SettingContainer` names/UIDs, page routes and profile subpages, with explicit exclusions for diagnostics and several contextual controls. [MainPage](../../backslash/src/cascadia/TerminalSettingsEditor/MainPage.cpp) submits the chosen result to `_Navigate` with `ElementName`. [Utils](../../backslash/src/cascadia/TerminalSettingsEditor/Utils.h) finds the named control after load, scrolls it into view and focuses it. Appearance entries use `App.` targets. This is useful existing infrastructure, not a reason to create another search engine.

The SSH shell panel and saved-group editor are plain panels rather than indexed `SettingContainer` entries. Their individual controls need explicit index coverage. Some collection pages already receive page/add-button entries, which does not prove item/field deep links. `SettingContainer::SetExpanded` exists, but this source trace does not establish automatic expansion of every ancestor in the search path. A future layout change must resolve stable control IDs to route/context, expand outer-to-inner containers, then scroll/focus the editable widget. Search must not apply or save a value. Handle hidden capability controls with an explanatory unavailable result, not silent navigation failure.

Acceptance: localized labels/help/keywords; profile/defaults/color-scheme context; collapsed nested colors/fonts/effects; groups and SSH fields; keyboard Enter/click versus suggestion arrows; focus/automation announcement; unavailable/planned controls; Save/Reset preserved. Native UI search/expansion tests remain unrun.

## Inventory and validation

- 138 authored root-editor `SettingContainer` nodes, plus 15 explicitly selected non-container editors/actions and 12 planned nodes (165 controls total).
- 185 unique category/control IDs; all category/parent IDs and source file/line references resolved.
- Assigned JSON keys matched existing schema property/path components or model macros. Existing model keys absent from the schema: `compatibility.kittyKeyboardMode`, `debugFeatures`, `defaultInputScope`, `font.builtinGlyphs`, `font.colorGlyphs`, `rightClickContextMenu`, `searchWebDefaultQueryUrl`. Existing en-US localization keys resolved; proposed category/future-feature localization keys are intentionally not yet resources.
- Unresolved backing controls: 0.
- Coverage is container-level, not every child widget, dialog or JSON-only option. Dynamic collections have editor nodes; generated XAML was excluded. Key matching does not prove type/range semantics or a full serialization trace.
- Dependencies record current binding expressions where present; capability, policy, inheritance and custom-shader precedence still need integration tests when the layout is implemented.

## Control reference

Labels and descriptions reuse current resources where available. Stable IDs, widget types, exact XAML bindings, localization keys, hierarchy, collapse hints, scopes, search routes and dependencies are in JSONC. Empty keys with `external-or-derived` mean an OS setting, action or diagnostic rather than an invented JSON setting. `planned-unimplemented` entries have no assigned persisted keys.

| Stable ID | Current/planned label | Scope | Backing key or operation | Source |
| --- | --- | --- | --- | --- |
| `control.global-appearance.show-connection-identity` | Show connection identity in tab titles | app | `backslash.showConnectionIdentity` | [GlobalAppearance.xaml:30](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L30) |
| `control.add-profile.duplicate-profile` | Duplicate a profile | app | `profiles.list` | [AddProfile.xaml:51](../../backslash/src/cascadia/TerminalSettingsEditor/AddProfile.xaml#L51) |
| `control.appearances.color-scheme` | Color scheme | profile/defaults/appearance | `colorScheme` | [Appearances.xaml:78](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L78) |
| `control.appearances.foreground` | Foreground | profile/defaults/appearance | `foreground` | [Appearances.xaml:215](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L215) |
| `control.appearances.background` | Background | profile/defaults/appearance | `background` | [Appearances.xaml:231](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L231) |
| `control.appearances.selection-background` | Selection background | profile/defaults/appearance | `selectionBackground` | [Appearances.xaml:247](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L247) |
| `control.appearances.font-face-container` | Font face | profile/defaults/appearance | `font.face` | [Appearances.xaml:265](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L265) |
| `control.appearances.missing-font-faces` | Missing fonts: | profile/defaults/appearance | external-or-derived | [Appearances.xaml:288](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L288) |
| `control.appearances.proportional-font-faces` | Non-monospace fonts: | profile/defaults/appearance | external-or-derived | [Appearances.xaml:293](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L293) |
| `control.appearances.font-size` | Font size | profile/defaults/appearance | `font.size` | [Appearances.xaml:300](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L300) |
| `control.appearances.line-height` | Line height | profile/defaults/appearance | `font.cellHeight` | [Appearances.xaml:317](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L317) |
| `control.appearances.cell-width` | Cell width | profile/defaults/appearance | `font.cellWidth` | [Appearances.xaml:334](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L334) |
| `control.appearances.font-weight` | Font weight | profile/defaults/appearance | `font.weight` | [Appearances.xaml:351](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L351) |
| `control.appearances.font-axes` | Variable font axes | profile/defaults/appearance | `font.axes` | [Appearances.xaml:388](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L388) |
| `control.appearances.font-features` | Font features | profile/defaults/appearance | `font.features` | [Appearances.xaml:416](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L416) |
| `control.appearances.enable-builtin-glyphs` | Builtin Glyphs | profile/defaults/appearance | `font.builtinGlyphs` | [Appearances.xaml:446](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L446) |
| `control.appearances.enable-color-glyphs` | Full-color Emoji | profile/defaults/appearance | `font.colorGlyphs` | [Appearances.xaml:456](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L456) |
| `control.appearances.retro-terminal-effect` | Retro terminal effects | profile/defaults/appearance | `experimental.retroTerminalEffect` | [Appearances.xaml:466](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L466) |
| `control.appearances.retro-terminal-glow-intensity` | Retro glow intensity | profile/defaults/appearance | `experimental.retroTerminalGlowIntensity` | [Appearances.xaml:475](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L475) |
| `control.appearances.retro-terminal-scanline-intensity` | Retro scanline intensity | profile/defaults/appearance | `experimental.retroTerminalScanlineIntensity` | [Appearances.xaml:494](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L494) |
| `control.appearances.retro-terminal-scanline-thickness` | Dark scanline thickness (DIP) | profile/defaults/appearance | `experimental.retroTerminalScanlineThickness` | [Appearances.xaml:513](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L513) |
| `control.appearances.retro-terminal-scanline-gap` | Clear gap between scanlines (DIP) | profile/defaults/appearance | `experimental.retroTerminalScanlineGap` | [Appearances.xaml:532](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L532) |
| `control.appearances.retro-terminal-tint-color` | Screen phosphor color | profile/defaults/appearance | `experimental.retroTerminalTintColor` | [Appearances.xaml:553](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L553) |
| `control.appearances.retro-terminal-tint-intensity` | Screen tint intensity | profile/defaults/appearance | `experimental.retroTerminalTintIntensity` | [Appearances.xaml:567](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L567) |
| `control.appearances.adjust-indistinguishable-colors` | Automatically adjust lightness of indistinguishable text | profile/defaults/appearance | `adjustIndistinguishableColors` | [Appearances.xaml:586](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L586) |
| `control.appearances.retro-bezel-enabled` | Retro bezel | profile/defaults | `experimental.retroBezelEnabled` | [Appearances.xaml:602](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L602) |
| `control.appearances.retro-bezel-size` | Casing width | profile/defaults | `experimental.retroBezelSize` | [Appearances.xaml:608](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L608) |
| `control.appearances.retro-bezel-color` | Casing finish | profile/defaults | `experimental.retroBezelColor` | [Appearances.xaml:618](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L618) |
| `control.appearances.retro-bezel-badge` | Backslash badge | profile/defaults | `experimental.retroBezelBadge` | [Appearances.xaml:638](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L638) |
| `control.appearances.retro-bezel-center-fullscreen` | Center aperture in fullscreen | profile/defaults | `experimental.retroBezelCenterFullscreen` | [Appearances.xaml:648](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L648) |
| `control.appearances.cursor-shape` | Cursor shape | profile/defaults/appearance | `cursorShape` | [Appearances.xaml:662](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L662) |
| `control.appearances.cursor-height` | Cursor height | profile/defaults/appearance | `cursorHeight` | [Appearances.xaml:675](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L675) |
| `control.appearances.cursor-color` | Cursor color | profile/defaults/appearance | `cursorColor` | [Appearances.xaml:698](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L698) |
| `control.appearances.background-image-container` | Background image path | profile/defaults/appearance | `backgroundImage` | [Appearances.xaml:720](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L720) |
| `control.appearances.background-image-stretch-mode` | Background image stretch mode | profile/defaults/appearance | `backgroundImageStretchMode` | [Appearances.xaml:748](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L748) |
| `control.appearances.background-image-alignment` | Background image alignment | profile/defaults/appearance | `backgroundImageAlignment` | [Appearances.xaml:762](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L762) |
| `control.appearances.background-image-opacity` | Background image opacity | profile/defaults/appearance | `backgroundImageOpacity` | [Appearances.xaml:951](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L951) |
| `control.appearances.intense-text-style` | Intense text style | profile/defaults/appearance | `intenseTextStyle` | [Appearances.xaml:978](../../backslash/src/cascadia/TerminalSettingsEditor/Appearances.xaml#L978) |
| `control.compatibility.allow-headless` | Allow Windows Terminal to run in the background | app | `compatibility.allowHeadless` | [Compatibility.xaml:29](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L29) |
| `control.compatibility.text-measurement` | Text measurement mode | app | `compatibility.textMeasurement` | [Compatibility.xaml:36](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L36) |
| `control.compatibility.ambiguous-width` | Width of ambiguous characters | app | `compatibility.ambiguousWidth` | [Compatibility.xaml:46](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L46) |
| `control.compatibility.debug-features-enabled` | Enable debugging features | app | `debugFeatures` | [Compatibility.xaml:56](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L56) |
| `control.compatibility.reset-application-state` | Clear cache | app | external-or-derived | [Compatibility.xaml:64](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L64) |
| `control.compatibility.reset-to-default-settings` | Reset to default settings | app | external-or-derived | [Compatibility.xaml:86](../../backslash/src/cascadia/TerminalSettingsEditor/Compatibility.xaml#L86) |
| `control.edit-color-scheme.inbox-scheme-duplicate` | This color scheme is part of the built-in settings or an installed extension | color-scheme | `schemes` | [EditColorScheme.xaml:203](../../backslash/src/cascadia/TerminalSettingsEditor/EditColorScheme.xaml#L203) |
| `control.edit-color-scheme.colors-header` | Colors | color-scheme | `schemes` | [EditColorScheme.xaml:212](../../backslash/src/cascadia/TerminalSettingsEditor/EditColorScheme.xaml#L212) |
| `control.edit-color-scheme.set-as-default-container` | Set color scheme as default | color-scheme | `profiles.defaults.colorScheme` | [EditColorScheme.xaml:431](../../backslash/src/cascadia/TerminalSettingsEditor/EditColorScheme.xaml#L431) |
| `control.edit-color-scheme.rename-container` | Rename color scheme | color-scheme | `schemes` | [EditColorScheme.xaml:439](../../backslash/src/cascadia/TerminalSettingsEditor/EditColorScheme.xaml#L439) |
| `control.extensions.scope` | Scope | app | external-or-derived | [Extensions.xaml:503](../../backslash/src/cascadia/TerminalSettingsEditor/Extensions.xaml#L503) |
| `control.global-appearance.theme` | Application Theme | app | `theme` | [GlobalAppearance.xaml:31](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L31) |
| `control.global-appearance.new-tab-position` | Position of newly created tabs | app | `newTabPosition` | [GlobalAppearance.xaml:46](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L46) |
| `control.global-appearance.show-titlebar` | Hide the title bar (requires relaunch) | app | `showTabsInTitlebar` | [GlobalAppearance.xaml:56](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L56) |
| `control.global-appearance.always-show-tabs` | Always show tabs | app | `alwaysShowTabs` | [GlobalAppearance.xaml:64](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L64) |
| `control.global-appearance.show-tabs-fullscreen` | Show tabs in full screen | app | `showTabsFullscreen` | [GlobalAppearance.xaml:72](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L72) |
| `control.global-appearance.acrylic-tab-row` | Use acrylic material in the tab row | app | `useAcrylicInTabRow` | [GlobalAppearance.xaml:79](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L79) |
| `control.global-appearance.show-title-in-titlebar` | Use active terminal title as application title | app | `showTerminalTitleInTitlebar` | [GlobalAppearance.xaml:86](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L86) |
| `control.global-appearance.always-on-top` | Always on top | app | `alwaysOnTop` | [GlobalAppearance.xaml:93](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L93) |
| `control.global-appearance.tab-width-mode` | Tab width mode | app | `tabWidthMode` | [GlobalAppearance.xaml:100](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L100) |
| `control.global-appearance.disable-animations` | Pane animations | app | `disableAnimations` | [GlobalAppearance.xaml:111](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L111) |
| `control.global-appearance.always-show-notification-icon` | Always display an icon in the notification area | app | `alwaysShowNotificationIcon` | [GlobalAppearance.xaml:118](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L118) |
| `control.global-appearance.minimize-to-notification-area` | Hide Terminal in the notification area when it is minimized | app | `minimizeToNotificationArea` | [GlobalAppearance.xaml:125](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L125) |
| `control.global-appearance.auto-hide-window` | Automatically hide window | app | `autoHideWindow` | [GlobalAppearance.xaml:132](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L132) |
| `control.global-appearance.show-admin-shield` | Display a shield in the title bar when Windows Terminal is running as Administrator | app | `showAdminShield` | [GlobalAppearance.xaml:139](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L139) |
| `control.global-appearance.enable-unfocused-acrylic` | Allow acrylic material in unfocused windows | app | `compatibility.enableUnfocusedAcrylic` | [GlobalAppearance.xaml:146](../../backslash/src/cascadia/TerminalSettingsEditor/GlobalAppearance.xaml#L146) |
| `control.interaction.copy-on-select` | Automatically copy selection to clipboard | app | `copyOnSelect` | [Interaction.xaml:30](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L30) |
| `control.interaction.copy-format` | Text formats to copy to the clipboard | app | `copyFormatting` | [Interaction.xaml:37](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L37) |
| `control.interaction.trim-block-selection` | Remove trailing white-space in rectangular selection | app | `trimBlockSelection` | [Interaction.xaml:47](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L47) |
| `control.interaction.trim-paste` | Remove trailing white-space when pasting | app | `trimPaste` | [Interaction.xaml:54](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L54) |
| `control.interaction.word-delimiters` | Word delimiters | app | `wordDelimiters` | [Interaction.xaml:61](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L61) |
| `control.interaction.snap-to-grid-on-resize` | Snap window resizing to character grid | app | `snapToGridOnResize` | [Interaction.xaml:71](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L71) |
| `control.interaction.tab-switcher-mode` | Tab switcher interface style | app | `tabSwitcherMode` | [Interaction.xaml:78](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L78) |
| `control.interaction.focus-follow-mouse` | Automatically focus pane on mouse hover | app | `focusFollowMouse` | [Interaction.xaml:88](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L88) |
| `control.interaction.scroll-to-zoom` | Adjust terminal font size by scrolling while holding the Ctrl key | app | `experimental.scrollToZoom` | [Interaction.xaml:95](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L95) |
| `control.interaction.scroll-to-change-opacity` | Adjust terminal opacity by scrolling while holding the Ctrl and Shift keys | app | `experimental.scrollToChangeOpacity` | [Interaction.xaml:102](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L102) |
| `control.interaction.detect-urls` | Automatically detect URLs and make them clickable | app | `experimental.detectURLs` | [Interaction.xaml:109](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L109) |
| `control.interaction.search-web-default-query-url` | Default URL to use for the "Search web" action | app | `searchWebDefaultQueryUrl` | [Interaction.xaml:116](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L116) |
| `control.interaction.enable-color-selection` | Experimental: Add key bindings to color selected text | app | `experimental.enableColorSelection` | [Interaction.xaml:126](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L126) |
| `control.interaction.confirm-close-all-tabs` | Warn when closing more than one tab | app | `warning.confirmCloseAllTabs` | [Interaction.xaml:139](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L139) |
| `control.interaction.warn-about-large-paste` | Warn when pasting more than 5 KiB | app | `warning.largePaste` | [Interaction.xaml:146](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L146) |
| `control.interaction.warn-about-multi-line-paste` | Warn when pasting newlines | app | `warning.multiLinePaste` | [Interaction.xaml:153](../../backslash/src/cascadia/TerminalSettingsEditor/Interaction.xaml#L153) |
| `control.launch.default-profile` | Default profile | app | `defaultProfile` | [Launch.xaml:58](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L58) |
| `control.launch.default-terminal-dropdown` | Default terminal application | app | external-or-derived | [Launch.xaml:222](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L222) |
| `control.launch.language` | Language (requires relaunch) | app | `language` | [Launch.xaml:222](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L222) |
| `control.launch.default-input-scope` | Default IME input mode | app | `defaultInputScope` | [Launch.xaml:236](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L236) |
| `control.launch.start-on-user-login` | Launch on machine startup | app | external-or-derived | [Launch.xaml:246](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L246) |
| `control.launch.first-window-preference` | When Terminal starts | app | `firstWindowPreference` | [Launch.xaml:256](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L256) |
| `control.launch.windowing-behavior` | New instance behavior | app | `windowingBehavior` | [Launch.xaml:266](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L266) |
| `control.launch.launch-size` | Launch size | app | `initialCols`, `initialRows` | [Launch.xaml:276](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L276) |
| `control.launch.launch-parameters` | Launch parameters | app | `launchMode`, `initialPosition`, `centerOnLaunch` | [Launch.xaml:320](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L320) |
| `control.new-tab-menu.current-folder-name` | Folder Name | app | `newTabMenu` | [NewTabMenu.xaml:323](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L323) |
| `control.new-tab-menu.current-folder-icon` | Folder Icon | app | `newTabMenu` | [NewTabMenu.xaml:332](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L332) |
| `control.new-tab-menu.current-folder-inlining` | Allow inlining | app | `newTabMenu` | [NewTabMenu.xaml:359](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L359) |
| `control.new-tab-menu.current-folder-allow-empty` | Allow empty | app | `newTabMenu` | [NewTabMenu.xaml:366](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L366) |
| `control.new-tab-menu.add-profile` | Profile | app | `newTabMenu` | [NewTabMenu.xaml:379](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L379) |
| `control.new-tab-menu.add-separator` | Separator | app | `newTabMenu` | [NewTabMenu.xaml:429](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L429) |
| `control.new-tab-menu.add-folder` | Folder | app | `newTabMenu` | [NewTabMenu.xaml:446](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L446) |
| `control.new-tab-menu.add-match-profiles` | Profile matcher | app | `newTabMenu` | [NewTabMenu.xaml:473](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L473) |
| `control.new-tab-menu.add-remaining-profiles` | Remaining profiles | app | `newTabMenu` | [NewTabMenu.xaml:501](../../backslash/src/cascadia/TerminalSettingsEditor/NewTabMenu.xaml#L501) |
| `control.profiles-advanced.antialiasing-mode` | Text antialiasing | profile/defaults | `antialiasingMode` | [Profiles_Advanced.xaml:40](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L40) |
| `control.profiles-advanced.alt-gr-aliasing` | AltGr aliasing | profile/defaults | `altGrAliasing` | [Profiles_Advanced.xaml:53](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L53) |
| `control.profiles-advanced.snap-on-input` | Scroll to input when typing | profile/defaults | `snapOnInput` | [Profiles_Advanced.xaml:63](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L63) |
| `control.profiles-advanced.history-size` | History size | profile/defaults | `historySize` | [Profiles_Advanced.xaml:73](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L73) |
| `control.profiles-advanced.close-on-exit` | Profile termination behavior | profile/defaults | `closeOnExit` | [Profiles_Advanced.xaml:87](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L87) |
| `control.profiles-advanced.bell-style` | Bell notification style | profile/defaults | `bellStyle` | [Profiles_Advanced.xaml:100](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L100) |
| `control.profiles-advanced.bell-sound` | Bell sound | profile/defaults | `bellSound` | [Profiles_Advanced.xaml:118](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L118) |
| `control.profiles-advanced.right-click-context-menu` | Display a menu on right-click | profile/defaults | `rightClickContextMenu` | [Profiles_Advanced.xaml:200](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L200) |
| `control.profiles-advanced.show-marks` | Display marks on the scrollbar | profile/defaults | `showMarksOnScrollbar` | [Profiles_Advanced.xaml:210](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L210) |
| `control.profiles-advanced.auto-mark-prompts` | Automatically mark prompts on pressing Enter | profile/defaults | `autoMarkPrompts` | [Profiles_Advanced.xaml:221](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L221) |
| `control.profiles-advanced.reload-env-vars` | Launch this application with a new environment block | profile/defaults | `compatibility.reloadEnvironmentVariables` | [Profiles_Advanced.xaml:232](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L232) |
| `control.profiles-advanced.reposition-cursor-with-mouse` | Experimental: Reposition the cursor with mouse clicks | profile/defaults | `experimental.repositionCursorWithMouse` | [Profiles_Advanced.xaml:242](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L242) |
| `control.profiles-advanced.rainbow-suggestions` | Experimental: Display suggested input in rainbow colors | profile/defaults | `experimental.rainbowSuggestions` | [Profiles_Advanced.xaml:253](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L253) |
| `control.profiles-advanced.path-translation-style` | Path translation | profile/defaults | `pathTranslationStyle` | [Profiles_Advanced.xaml:263](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Advanced.xaml#L263) |
| `control.profiles-appearance.opacity` | Background opacity | profile/defaults | `opacity` | [Profiles_Appearance.xaml:96](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Appearance.xaml#L96) |
| `control.profiles-appearance.use-acrylic` | Enable acrylic material | profile/defaults | `useAcrylic` | [Profiles_Appearance.xaml:118](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Appearance.xaml#L118) |
| `control.profiles-appearance.padding` | Padding | profile/defaults | `padding` | [Profiles_Appearance.xaml:135](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Appearance.xaml#L135) |
| `control.profiles-appearance.scrollbar-visibility` | Scrollbar visibility | profile/defaults | `scrollbarState` | [Profiles_Appearance.xaml:203](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Appearance.xaml#L203) |
| `control.profiles-base.name` | Name | profile/defaults | `name` | [Profiles_Base.xaml:43](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L43) |
| `control.profiles-base.commandline` | Command line | profile/defaults | `commandline` | [Profiles_Base.xaml:71](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L71) |
| `control.profiles-base.starting-directory` | Starting directory | profile/defaults | `startingDirectory` | [Profiles_Base.xaml:92](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L92) |
| `control.profiles-base.icon` | Icon | profile/defaults | `icon` | [Profiles_Base.xaml:121](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L121) |
| `control.profiles-base.tab-title` | Tab title | profile/defaults | `tabTitle` | [Profiles_Base.xaml:151](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L151) |
| `control.profiles-base.tab-color` | Tab color | profile/defaults | `tabColor` | [Profiles_Base.xaml:163](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L163) |
| `control.profiles-base.elevate` | Run this profile as Administrator | profile/defaults | `elevate` | [Profiles_Base.xaml:179](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L179) |
| `control.profiles-base.hidden` | Hide profile from dropdown | profile/defaults | `hidden` | [Profiles_Base.xaml:189](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L189) |
| `control.profiles-base-orphaned.delete-orphaned` | Profile no longer detected | profile/defaults | `profiles.list` | [Profiles_Base_Orphaned.xaml:26](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base_Orphaned.xaml#L26) |
| `control.profiles-base-orphaned.name` | Name | profile/defaults | `name` | [Profiles_Base_Orphaned.xaml:44](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base_Orphaned.xaml#L44) |
| `control.profiles-base-orphaned.source` | Original Source | profile/defaults | `source` | [Profiles_Base_Orphaned.xaml:53](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base_Orphaned.xaml#L53) |
| `control.profiles-terminal.suppress-application-title` | Suppress title changes | profile/defaults | `suppressApplicationTitle` | [Profiles_Terminal.xaml:35](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L35) |
| `control.profiles-terminal.force-vtinput` | Use the legacy input encoding | profile/defaults | `compatibility.input.forceVT` | [Profiles_Terminal.xaml:45](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L45) |
| `control.profiles-terminal.allow-kitty-keyboard-mode` | Allow Kitty Keyboard Protocol | profile/defaults | `compatibility.kittyKeyboardMode` | [Profiles_Terminal.xaml:55](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L55) |
| `control.profiles-terminal.allow-vt-checksum-report` | Allow DECRQCRA (Request Checksum of Rectangular Area) | profile/defaults | `compatibility.allowDECRQCRA` | [Profiles_Terminal.xaml:65](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L65) |
| `control.profiles-terminal.allow-vt-clipboard-write` | Allow OSC 52 (Manipulate Selection Data) to write to the clipboard | profile/defaults | `compatibility.allowOSC52` | [Profiles_Terminal.xaml:75](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L75) |
| `control.profiles-terminal.answerback-message` | ENQ (Request Terminal Status) response | profile/defaults | `answerbackMessage` | [Profiles_Terminal.xaml:85](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Terminal.xaml#L85) |
| `control.rendering.graphics-api` | Graphics API | app | `rendering.graphicsAPI` | [Rendering.xaml:27](../../backslash/src/cascadia/TerminalSettingsEditor/Rendering.xaml#L27) |
| `control.rendering.disable-partial-invalidation` | Disable partial Swap Chain invalidation | app | `rendering.disablePartialInvalidation` | [Rendering.xaml:36](../../backslash/src/cascadia/TerminalSettingsEditor/Rendering.xaml#L36) |
| `control.rendering.software-rendering` | Use software rendering (WARP) | app | `rendering.software` | [Rendering.xaml:42](../../backslash/src/cascadia/TerminalSettingsEditor/Rendering.xaml#L42) |
| `control.launch.saved-group-list` | Saved tab group | workspace-template | `backslash.savedTabGroups` | [Launch.xaml:400](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L400) |
| `control.launch.saved-group-name` | Group name | workspace-template | `backslash.savedTabGroups` | [Launch.xaml:401](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L401) |
| `control.launch.saved-group-folder-mode` | Starting folders | workspace-template | `backslash.savedTabGroups` | [Launch.xaml:402](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L402) |
| `control.launch.saved-group-update-button` | Update group | workspace-template | `backslash.savedTabGroups` | [Launch.xaml:407](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L407) |
| `control.launch.saved-group-delete-button` | Delete group | workspace-template | `backslash.savedTabGroups` | [Launch.xaml:408](../../backslash/src/cascadia/TerminalSettingsEditor/Launch.xaml#L408) |
| `control.profiles-base.ssh-shell-choice` | SSH remote shell | profile | `commandline` | [Profiles_Base.xaml:55](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L55) |
| `control.profiles-base.ssh-custom-command` | Custom remote shell command | profile | `commandline` | [Profiles_Base.xaml:63](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L63) |
| `control.profiles-base.apply-ssh-shell-button` | Apply custom command | profile | `commandline` | [Profiles_Base.xaml:65](../../backslash/src/cascadia/TerminalSettingsEditor/Profiles_Base.xaml#L65) |
| `control.add-profile.add-profile-scan-ssh` | Scan SSH hosts | app/profile | `profiles.list` | [AddProfile.xaml:40](../../backslash/src/cascadia/TerminalSettingsEditor/AddProfile.xaml#L40) |
| `control.add-profile.ssh-hosts` | Discovered SSH hosts | app/profile | `profiles.list` | [AddProfile.xaml:43](../../backslash/src/cascadia/TerminalSettingsEditor/AddProfile.xaml#L43) |
| `control.add-profile.restore-ssh-button` | Add / Restore | app/profile | `profiles.list` | [AddProfile.xaml:47](../../backslash/src/cascadia/TerminalSettingsEditor/AddProfile.xaml#L47) |
| `control.add-profile.add-new-button` | New empty profile | profile | `profiles.list` | [AddProfile.xaml:26](../../backslash/src/cascadia/TerminalSettingsEditor/AddProfile.xaml#L26) |
| `control.actions.commands-list-view` | Actions | app | `actions`, `keybindings` | [Actions.xaml:223](../../backslash/src/cascadia/TerminalSettingsEditor/Actions.xaml#L223) |
| `control.color-schemes.color-scheme-list-view` | Color schemes | app/color-scheme | `schemes` | [ColorSchemes.xaml:64](../../backslash/src/cascadia/TerminalSettingsEditor/ColorSchemes.xaml#L64) |
| `control.extensions.active-extensions-list` | Active Extensions | app | `disabledProfileSources` | [Extensions.xaml:460](../../backslash/src/cascadia/TerminalSettingsEditor/Extensions.xaml#L460) |
| `planned.workspace.pin` | Pinned workspaces (planned) | workspace-template | undecided | [FEATURES.md:1](FEATURES.md#L1) |
| `planned.commands.field` | Local app-command field (planned) | app/session | undecided | [APP-COMMANDS-DESIGN.md:1](APP-COMMANDS-DESIGN.md#L1) |
| `planned.files.view` | Local and remote files (planned) | connection/pane | undecided | [FILE-TRANSFER-DESIGN.md:1](FILE-TRANSFER-DESIGN.md#L1) |
| `planned.sessions.mode` | Normal or resumable (planned) | connection/session | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.sessions.registry` | Remembered sessions and results (planned) | installation/session | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.sessions.control` | Viewing and control (planned) | attachment/session | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.sessions.lock` | Require owner approval for takeover (planned) | session | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.presence.opt-in` | Who is connected (planned) | connection/account | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.presence.managed` | Managed organization access (planned) | organization/account | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.identity.enroll` | Sign in and enroll this device (planned) | account/device | undecided | [RESUMABLE-SESSIONS-DESIGN.md:1](RESUMABLE-SESSIONS-DESIGN.md#L1) |
| `planned.retro.noise` | Animated static/noise (planned) | profile/appearance | undecided | [CRT-RENDERING-PLAN.md:1](CRT-RENDERING-PLAN.md#L1) |
| `planned.fonts.catalog` | Font discovery and download (planned) | app/profile | undecided | [FONTS.md:1](FONTS.md#L1) |

| planned.startup.mode | Start with: Default profile / Saved group (implemented) | app | `backslash.startupWithSavedGroup` | [SAVED-TAB-GROUPS.md](SAVED-TAB-GROUPS.md) |
| planned.startup.group | Startup group (implemented; searchable, conditional) | app | `backslash.startupGroup`; stable GUID | [SAVED-TAB-GROUPS.md](SAVED-TAB-GROUPS.md) |

## Implemented naming default

[Connection identity titles](CONNECTION-TITLES.md) are now an implemented default-on Appearance toggle, with explicit title precedence, captured launch metadata and ordinary dynamic application titles as details. This adds one source-backed control to the inventory. Startup mode/group nodes are also implemented and retain their historical stable proposal IDs; Profiles sidebar collapse behavior is unchanged. Exact build/test/manual limits are recorded in the naming document and delivery evidence.

## Review decisions still open

Exact labels/order, compact versus expanded presentation, profile context placement and responsive layout remain review choices. Reuse the current inheritance model and localization conventions. No runtime migration or authoritative new layout schema is selected. A small renderer/mock UI can derive a tree from this proposal after review; translating the current source into an entire new UI before that would commit to unreviewed structure.

Future app commands, file views, remote persistence/presence and identity are described in [app commands](APP-COMMANDS-DESIGN.md), [file transfer](FILE-TRANSFER-DESIGN.md) and [resumable sessions](RESUMABLE-SESSIONS-DESIGN.md). They remain separate implementation slices.
