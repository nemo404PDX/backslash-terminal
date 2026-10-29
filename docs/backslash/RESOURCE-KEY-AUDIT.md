# Resource fallback audit (post-preview.2 development)

The 60 PRI263 identities in the exact preview.2 build have no en-US fallback. Each was checked against tracked C++/headers/IDL/XAML/JSON/XML/RC sources, quoted resource literals, x:Uid roots, and the enum resource-name generator. All 93 localized instances are obsolete, rather than missing English for a current control. Only those data elements were removed; remaining translations, resource headers and valid localized siblings are retained. The published preview.2 tag preserves the original strings.

Absence of a substring alone was not the decision rule. Globals_DisableAnimationsReversed, InitialPosXBox/InitialPosYBox, LaunchModeSetting, ExistingWindow2, and Window*ButtonToolTip are distinct current IDs, not consumers of their obsolete prefixes. INITIALIZE_BINDABLE_ENUM_SETTING(LaunchMode) constructs Globals_LaunchMode_<enum>/Content; it cannot select the obsolete Header/HelpText. Rename and Cancel in SegoeFluentIconList are glyph labels, and literal Cancel button text is not a resource lookup. No runtime assembly of the other obsolete names was found. No new translations or English fallbacks were invented.

`Test-ResourceFallbacks.py` checks every localized resource in these three components against en-US and reports duplicate names/missing fallback by file. It never deletes resources automatically. A future failure requires checking use before deleting a translation or restoring authoritative English.

| Component | Obsolete resource name | Removed localized instances |
| --- | --- | --- |
| Microsoft.Terminal.Control | `NoticeFontNotFound` | 2 |
| Microsoft.Terminal.Control | `SearchBox_MatchesAvailable` | 2 |
| Microsoft.Terminal.Control | `SearchBox_NoMatches` | 2 |
| Microsoft.Terminal.Control | `UnexpectedRendererError` | 2 |
| Microsoft.Terminal.Settings.Editor | `AddProfile_AddNewButton.Tag` | 1 |
| Microsoft.Terminal.Settings.Editor | `AddProfile_DuplicateButton.Tag` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_DeleteButton2.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_DeleteButtonDisclaimerInBox.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_DeleteDisclaimerInBox` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_EditButton.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_RenameDisclaimerInBox` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_SystemColorsHeader.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `ColorScheme_TerminalColorsHeader.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_DisableAnimations.Header` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_InitialPosX.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_InitialPosY.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_InputServiceWarning.Header` | 15 |
| Microsoft.Terminal.Settings.Editor | `Globals_KeybindingsDisclaimer.Text` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_LaunchMode.Header` | 1 |
| Microsoft.Terminal.Settings.Editor | `Globals_LaunchMode.HelpText` | 1 |
| Microsoft.Terminal.Settings.Editor | `NullableColorPicker_ColorPickerContentDialog.PrimaryButtonText` | 1 |
| Microsoft.Terminal.Settings.Editor | `NullableColorPicker_ColorPickerContentDialog.SecondaryButtonText` | 1 |
| Microsoft.Terminal.Settings.Editor | `NullableColorPicker_ColorPickerContentDialog.Title` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_BellSoundBrowse.Content` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_BellStyleVisual.Content` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_BuiltInIcon.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_General.Header` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_HideIconCheckbox.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_HideIconCheckbox.Content` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconBox.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconBrowse.Content` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconEmojiBox.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconEmojiBox.PlaceholderText` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconType.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconTypeEmoji` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconTypeFontIcon` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconTypeImage` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_IconTypeNone` | 1 |
| Microsoft.Terminal.Settings.Editor | `Profile_PaddingSlider.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| Microsoft.Terminal.Settings.Editor | `Rename.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip` | 1 |
| TerminalApp | `AboutDialog_GettingStartedLink.Content` | 1 |
| TerminalApp | `Cancel` | 1 |
| TerminalApp | `CloseAll` | 1 |
| TerminalApp | `CouldNotOpenUriDialog.PrimaryButtonText` | 2 |
| TerminalApp | `ExportFailure` | 1 |
| TerminalApp | `ExportSuccess` | 1 |
| TerminalApp | `FeedbackMenuItem` | 1 |
| TerminalApp | `FeedbackUriValue` | 1 |
| TerminalApp | `KeyboardServiceDisabledDialog.PrimaryButtonText` | 1 |
| TerminalApp | `KeyboardServiceDisabledDialog.Title` | 1 |
| TerminalApp | `KeyboardServiceWarningText` | 15 |
| TerminalApp | `LegacyGlobalsProperty` | 1 |
| TerminalApp | `LegacyGlobalsPropertyHrefLabel` | 1 |
| TerminalApp | `LegacyGlobalsPropertyHrefUrl` | 1 |
| TerminalApp | `PlainText` | 1 |
| TerminalApp | `Quit` | 1 |
| TerminalApp | `TerminalPage_PaneMovedAnnouncement_ExistingWindow` | 1 |
| TerminalApp | `WindowCloseButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| TerminalApp | `WindowMaximizeButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |
| TerminalApp | `WindowMinimizeButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name` | 1 |

## Reproduce the semantic comparison

Run `python backslash/tools/backslash/Test-ResourceCleanupAudit.py` from the repository root. The default baseline is the immutable `v0.0.1-preview.2` tag. The checker reads Git stdout as bytes and parses XML directly with comments retained; no PowerShell string decoding or serialized-XML comparison occurs. It checks all 48 files, verifies the table's exact 60 identities / 93 removals, then compares all surviving translations/comments plus headers, non-data XML and element order. Formatting-only whitespace outside resource values is ignored; resource value/comment content is preserved. This is a pinned before/after audit; `Test-ResourceFallbacks.py` remains the ongoing consistency check.
