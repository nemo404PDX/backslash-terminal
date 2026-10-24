# Inherited Windows Terminal documentation

For Backslash features and development, start at [docs/README.md](../docs/README.md). This directory came from the Microsoft Windows Terminal [source baseline](../UPSTREAM-SOURCE.md). Retention does not make every proposal implemented or Microsoft workflow applicable to Backslash.

## Useful technical references

- [Terminal settings development](cascadia/AddASetting.md), [JSON utilities](cascadia/Json-Utility-API.md), [XAML unit testing](cascadia/Unittesting-CppWinRT-Xaml.md), [feature flags](feature_flags.md), [debugging](Debugging.md), [exceptions](EXCEPTIONS.md) and [WIL](WIL.md) explain inherited implementation techniques. Check older details against current source.
- [profiles.schema.json](cascadia/profiles.schema.json) is maintained, including Backslash saved groups, connection titles and CRT/bezel settings. It must not be archived as untouched upstream documentation.
- [Console control events](ConsoleCtrlEvent.md), [console settings](ConsoleHostSettings.md) and [cooked input](COOKED_READ_DATA.md) describe inherited console behavior.
- [reference/](reference/) contains VT data, Unicode fixtures, layout material and the sequence-index generator. The generator defaults to a solution root two directories above its working directory.
- [Color nudging](color_nudging.html) is referenced by src/types/ColorFix.cpp. [Organization](ORGANIZATION.md), [building](building.md), [style](STYLE.md) and [fuzzing](fuzzing.md) supply upstream context; use the [Backslash build guide](../tools/backslash/README.md) for fork builds.

## Original specifications and historical process

- [specs/](specs/) preserves original designs for panes, settings UI, process management, shell integration marks, ConPTY and other features. These remain useful architectural references; a spec alone does not establish completion or current behavior.
- [2022](upstream/history/roadmap-2022.md), [2023](upstream/history/roadmap-2023.md), [v1](upstream/history/terminal-v1-roadmap.md), [v2](upstream/history/terminal-v2-roadmap.md) roadmaps and the [2023 accessibility plan](terminal-a11y-2023.md) are historical upstream plans, not Backslash commitments.
- [Bot workflow](upstream/history/bot.md), [submitting code](upstream/history/submitting_code.md), [Windows test passes](WindowsTestPasses.md), [TAEF](TAEF.md), [UniversalTest](UniversalTest.md) and [Niksa](Niksa.md) preserve upstream process/test context, including Microsoft environment assumptions. Review applicability before following operational instructions.
- [Legacy AddASetting](AddASetting.md) mentions private OS-repository steps and differs from the [Terminal settings guide](cascadia/AddASetting.md). [SettingsSchema.md](cascadia/SettingsSchema.md) is an upstream documentation redirect.

## Stable paths and attribution

Source comments name doc/feature_flags.md and doc/color_nudging.html; Backslash records link to the schema. CI path filters and spelling exclusions also name doc/. Keep technical references at stable paths. Six superseded planning/process files have moved to [upstream/history/](upstream/history/README.md); its index records the mapping for future upstream updates. New Backslash guidance belongs in docs/backslash/. Preserve all original attribution and legal notices.