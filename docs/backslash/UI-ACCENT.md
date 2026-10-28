# Backslash application accent

Implemented in the uncommitted development checkout; not part of the published `v0.0.1-preview.1` tag. See [current status and validation](DEVELOPMENT-STATUS.md).

The source anchor is the existing Backslash color scheme's cursor and bright cyan, `#14FFCA`, in [defaults.json](../../backslash/src/cascadia/TerminalSettingsModel/defaults.json). The supplied [icon export](../images/icon-exports/100.png) has a related mint-green gradient; the older approximate concept green and CRT phosphor default are separate colors.

[App.xaml](../../backslash/src/cascadia/TerminalApp/App.xaml) defines all seven accent tones in application-level Light and Dark dictionaries above merged control resources. This gives the native controls a complete palette for fills, text and selection states instead of retaining host-blue variants. Settings tab, split and separate-window hosts use the same application resources. No OS accent or user setting is changed.

| Resource | RGB | Purpose |
| --- | --- | --- |
| SystemAccentColor | `#008469` | Balanced selection background; white text contrast 4.66:1 |
| SystemAccentColorDark1 | `#00614E` | Light-theme accent fill; white text remains at least 4.65:1 through tested 80% opacity |
| SystemAccentColorDark2 | `#004F40` | Light-theme accent text |
| SystemAccentColorDark3 | `#003D31` | Deeper family tone |
| SystemAccentColorLight1 | `#0DE0BB` | Brighter family tone, also present in the Backslash scheme |
| SystemAccentColorLight2 | `#14FFCA` | Brand anchor and dark-theme accent fill |
| SystemAccentColorLight3 | `#70FFE0` | Dark-theme accent text |

The seven HighContrast entries explicitly resolve to `SystemColorHighlightColor`; omission would risk a normal-theme fallback. Existing semantic warning/error and disabled resources, and native dual-color focus rings, are retained. Accent-driven focus/selection states inherit the palette where the native control uses it. Terminal color schemes and phosphor rendering have no dependency on these resources.

The checks follow the native [WinUI 2.8.4 accent resource mappings](https://github.com/microsoft/microsoft-ui-xaml/blob/v2.8.4/dev/CommonStyles/Common_themeresources_any.xaml). Eighteen source/contrast checks passed, including selected text and light/dark fill at 100%, 90% and 80% opacity. The final candidate builds successfully. Actual visual resolution, hover/pressed appearance, high-contrast transitions and all Settings hosting modes remain GUI QA, not proven by arithmetic or XAML compilation alone.
