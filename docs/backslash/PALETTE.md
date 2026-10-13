# Palette and profile design

[Project entry point](README.md) · [Baseline evidence](BASELINE.md)

## Agreed behavior

When copying a palette/profile, fill missing destination color slots **once**, using existing base colors and each destination slot's hue-family and brightness/intensity constraints. Preserve dark/normal/bright or faint relationships where those categories apply; a neutral ramp stays neutral.

Existing supplied destination colors must remain intact. Once copying finishes, every generated color is independently editable in the destination. There is no live link to the original and no automatic recalculation when a source/base color changes. Copying a profile must have clear ownership semantics for its copied palette; the eventual schema must satisfy this behavior.

Do not gate extended colors on Windows versus WSL/SSH profiles: they share terminal renderer capabilities. Application output may still depend on terminal negotiation or environment configuration.

## Keep these categories distinct

| Category | Meaning and editing boundary |
| --- | --- |
| Named ANSI 0–15 | Eight normal and eight bright entries; upstream “purple” is ANSI magenta, with magenta aliases. |
| Indexed 0–255 | Output selects an index. 16–231 start as a 6×6×6 RGB cube, 232–255 as grayscale. They are not 240 known application roles. |
| Direct RGB | Application supplies the color itself. Do not automatically recolor truecolor output. |
| Default roles | Foreground, background, cursor and selection background have explicit roles outside the named ANSI scheme entries. |
| Semantic colors | Application/UI concepts such as error or success require an explicit role contract; arbitrary indexed slots cannot be assigned that meaning by inference. |

Old Windows ConsoleColor integer ordering differs from ANSI ordering. Importers must map by documented semantics, not blindly copy numeric positions. Faint variants in other terminals are not automatically extra ANSI slots.

## Verified baseline and implications

[ColorScheme.cpp](../../src/cascadia/TerminalSettingsModel/ColorScheme.cpp) serializes 16 named ANSI entries and the four default roles; its loader currently requires a valid 16-color scheme. [colorTable.cpp](../../src/types/colorTable.cpp) already initializes the extended 256-color table. OSC 4 and 104 modify/reset live entries through the parser and adapter. Persisted 256-color editing therefore needs changes across the settings model, serialization, settings-to-renderer adapter, editor, reset behavior and tests; it does not require inventing 256-color rendering.

The new fill operation needs a deliberate incomplete-input path rather than relying on the current strict scheme loader. Decide whether generated values become explicit stored entries and how imported incomplete palettes are validated.

There is no universal “Unix palette.” [Kitty](https://sw.kovidgoyal.net/kitty/conf/#color-scheme) exposes color0–255 and additional roles. [Konsole](https://github.com/KDE/konsole/blob/master/src/colorscheme/ColorScheme.cpp) uses a 30-slot scheme with normal/intense/faint variants, including foreground/background. These are comparison models, not a selected Backslash storage format.

## Proposed UI and unresolved design

A grouped ANSI/indexed/default-role editor with previews is recommended. A lock or manual-regenerate control is optional and not selected; it must never introduce automatic source linking.

Open: exact generation algorithm, color space, hue-family treatment for indexed cube slots, gamut mapping, contrast targets, handling absent base colors, schema/version migration, shared-versus-profile overrides, undo and reset semantics. Do not commit to a specific HSL/OKLCH algorithm yet.

## Future acceptance checks

- Supplied colors are preserved; missing values are generated only during copying.
- Neutral entries stay neutral and documented intensity relationships hold.
- Source edits do not mutate the destination; destination edits do not mutate the source.
- All generated entries remain individually editable after save/reload.
- ANSI, indexed, RGB and default-role output retain their distinct behavior.
- OSC set/reset and persisted reset rules are defined and tested.
- Windows, WSL and SSH profiles expose the same renderer palette editor.

