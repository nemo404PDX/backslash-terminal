// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "Launch.h"
#include "Launch.g.cpp"
#include "EnumEntry.h"
#include "LaunchViewModel.h"
#include "../TerminalSettingsModel/SavedTabGroups.h"
#include "../TerminalSettingsModel/StartupGroupPolicy.h"

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Terminal::Settings::Model;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    Launch::Launch()
    {
        InitializeComponent();

        // BODGY
        // Xaml code generator for x:Bind to this will fail to find UnloadObject() on Launch class.
        // To work around, check it ourselves on construction and FindName to force load.
        // It's specified as x:Load=false in the XAML. So it only loads if this passes.
        if (CascadiaSettings::IsDefaultTerminalAvailable())
        {
            FindName(L"DefaultTerminalDropdown");
        }

        Automation::AutomationProperties::SetName(LaunchModeComboBox(), RS_(L"Globals_LaunchModeSetting/Text"));
        Automation::AutomationProperties::SetHelpText(LaunchModeComboBox(), RS_(L"Globals_LaunchModeSetting/[using:Windows.UI.Xaml.Controls]ToolTipService/ToolTip"));
        Automation::AutomationProperties::SetHelpText(PosXBox(), RS_(L"Globals_InitialPosXBox/[using:Windows.UI.Xaml.Controls]ToolTipService/ToolTip"));
        Automation::AutomationProperties::SetHelpText(PosYBox(), RS_(L"Globals_InitialPosYBox/[using:Windows.UI.Xaml.Controls]ToolTipService/ToolTip"));
        Automation::AutomationProperties::SetHelpText(UseDefaultLaunchPositionCheckbox(), RS_(L"Globals_DefaultLaunchPositionCheckbox/[using:Windows.UI.Xaml.Controls]ToolTipService/ToolTip"));
        Automation::AutomationProperties::SetName(CenterOnLaunchToggle(), RS_(L"Globals_CenterOnLaunch/Text"));
        Automation::AutomationProperties::SetHelpText(CenterOnLaunchToggle(), RS_(L"Globals_CenterOnLaunch/[using:Windows.UI.Xaml.Controls]ToolTipService/ToolTip"));
    }

    void Launch::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _ViewModel = args.ViewModel().as<Editor::LaunchViewModel>();
        BringIntoViewWhenLoaded(args.ElementToFocus());
        _RefreshSavedGroups();
        auto innerViewModel{ winrt::get_self<Editor::implementation::LaunchViewModel>(_ViewModel) };
        /* coroutine dispatch */ innerViewModel->PrepareStartOnUserLoginSettings();

        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "NavigatedToPage",
            TraceLoggingDescription("Event emitted when the user navigates to a page in the settings UI"),
            TraceLoggingValue("startup", "PageId", "The identifier of the page that was navigated to"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }
    void Launch::_RefreshStartupGroup()
    {
        if (!_ViewModel) { return; }
        const auto wasRefreshing = std::exchange(_refreshingStartup, true);
        auto reset = wil::scope_exit([&] { _refreshingStartup = wasRefreshing; });
        StartupModeChoice().SelectedIndex(_ViewModel.StartupWithSavedGroup() ? 1 : 0);
        StartupGroup().Visibility(_ViewModel.StartupWithSavedGroup() ? Visibility::Visible : Visibility::Collapsed);
        auto choices = winrt::single_threaded_vector<IInspectable>();
        winrt::hstring text;
        StartupGroupStatus().Text(L"");
        if (_ViewModel.StartupWithSavedGroup())
        {
            const auto state = ::Microsoft::Terminal::Backslash::StartupGroupPolicy::Picker(_ViewModel.SavedTabGroupsJson(), _ViewModel.StartupGroup());
            for (const auto& choice : state.choices) { choices.Append(winrt::make<EnumEntry>(choice.name, winrt::box_value(choice.id))); }
            if (state.selected) { text = state.selected->name; }
            if (!state.notice.empty()) { StartupGroupStatus().Text(winrt::to_hstring(state.notice)); }
            else if (!state.selected) { StartupGroupStatus().Text(RS_(L"StartupGroups_Choose")); }
        }
        StartupGroupPicker().ItemsSource(choices);
        StartupGroupPicker().Text(text);
    }

    void Launch::StartupMode_SelectionChanged(const IInspectable&, const Controls::SelectionChangedEventArgs&)
    {
        if (_refreshingStartup || !_ViewModel) { return; }
        _ViewModel.StartupWithSavedGroup(StartupModeChoice().SelectedIndex() == 1);
        _RefreshStartupGroup();
    }

    void Launch::StartupGroup_TextChanged(const Controls::AutoSuggestBox& sender, const Controls::AutoSuggestBoxTextChangedEventArgs& args)
    {
        if (_refreshingStartup || !_ViewModel || args.Reason() != Controls::AutoSuggestionBoxTextChangeReason::UserInput) { return; }
        const auto state = ::Microsoft::Terminal::Backslash::StartupGroupPolicy::Picker(_ViewModel.SavedTabGroupsJson(), _ViewModel.StartupGroup(), sender.Text());
        auto choices = winrt::single_threaded_vector<IInspectable>();
        for (const auto& choice : state.choices) { choices.Append(winrt::make<EnumEntry>(choice.name, winrt::box_value(choice.id))); }
        if (!state.notice.empty()) { StartupGroupStatus().Text(winrt::to_hstring(state.notice)); }
        sender.ItemsSource(choices);
    }

    void Launch::StartupGroup_QuerySubmitted(const Controls::AutoSuggestBox& sender, const Controls::AutoSuggestBoxQuerySubmittedEventArgs& args)
    {
        if (_refreshingStartup || !_ViewModel) { return; }
        const auto chosen = winrt::unbox_value_or<Editor::EnumEntry>(args.ChosenSuggestion(), nullptr);
        if (!chosen) { StartupGroupStatus().Text(RS_(L"StartupGroups_ChooseListed"));return; }
        const auto choice = ::Microsoft::Terminal::Backslash::StartupGroupPolicy::Choose(_ViewModel.SavedTabGroupsJson(), winrt::unbox_value_or<winrt::hstring>(chosen.EnumValue(), L""));
        if (!choice) { StartupGroupStatus().Text(RS_(L"StartupGroups_Stale"));return; }
        const auto wasRefreshing = std::exchange(_refreshingStartup, true);
        auto reset = wil::scope_exit([&] { _refreshingStartup = wasRefreshing; });
        _ViewModel.StartupGroup(choice->id);
        // AutoSuggestBox is still processing this selected item. Do not clear or
        // rebuild ItemsSource here; update display text explicitly, as the
        // established object-suggestion controls do, without TextMemberPath.
        sender.Text(choice->name);
        sender.IsSuggestionListOpen(false);
        StartupGroupStatus().Text(RS_(L"StartupGroups_SelectionChanged"));
    }

    void Launch::_RefreshSavedGroups(int selected)
    {
        _RefreshStartupGroup();
        SavedGroupList().Items().Clear();
        try
        {
            const auto store = ::Microsoft::Terminal::Backslash::SavedTabGroups::Read(_ViewModel.SavedTabGroupsJson());
            for (const auto& group : store.groups) { SavedGroupList().Items().Append(winrt::box_value(winrt::to_hstring(group.name))); }
            const auto count = static_cast<int>(store.groups.size());
            SavedGroupList().SelectedIndex(count ? std::clamp(selected, 0, count - 1) : -1);
            SavedGroupStatus().Text(count ? L"Changes apply only when you click Save. Reset discards unsaved edits." : L"Save a group from the Groups button next to the tabs.");
        }
        catch (const std::exception& error) { SavedGroupStatus().Text(winrt::to_hstring(error.what())); }
        const auto selectedItem = SavedGroupList().SelectedIndex() >= 0;
        SavedGroupUpdateButton().IsEnabled(selectedItem);SavedGroupDeleteButton().IsEnabled(selectedItem);
    }

    void Launch::SavedGroups_SelectionChanged(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::Controls::SelectionChangedEventArgs&)
    {
        if (!_ViewModel) { return; }
        try
        {
            const auto store = ::Microsoft::Terminal::Backslash::SavedTabGroups::Read(_ViewModel.SavedTabGroupsJson());
            const auto index = SavedGroupList().SelectedIndex();
            const auto selected = index >= 0 && static_cast<size_t>(index) < store.groups.size();
            SavedGroupUpdateButton().IsEnabled(selected);SavedGroupDeleteButton().IsEnabled(selected);
            if (selected) { SavedGroupName().Text(winrt::to_hstring(store.groups[index].name));SavedGroupFolderMode().SelectedIndex(store.groups[index].capturedFolders ? 1 : 0); }
        }
        catch (...) { SavedGroupUpdateButton().IsEnabled(false);SavedGroupDeleteButton().IsEnabled(false); }
    }

    void Launch::SavedGroups_Update(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        try
        {
            auto store = ::Microsoft::Terminal::Backslash::SavedTabGroups::Read(_ViewModel.SavedTabGroupsJson());
            const auto index = SavedGroupList().SelectedIndex();
            if (index < 0 || static_cast<size_t>(index) >= store.groups.size()) { return; }
            store.groups[index].name = ::Microsoft::Terminal::Backslash::SavedTabGroups::Name(winrt::to_string(SavedGroupName().Text()));
            store.groups[index].capturedFolders = SavedGroupFolderMode().SelectedIndex() == 1;
            _ViewModel.SavedTabGroupsJson(store.Json());_RefreshSavedGroups(index);
        }
        catch (const std::exception& error) { SavedGroupStatus().Text(winrt::to_hstring(error.what())); }
    }

    void Launch::SavedGroups_Delete(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        try
        {
            auto store = ::Microsoft::Terminal::Backslash::SavedTabGroups::Read(_ViewModel.SavedTabGroupsJson());
            const auto index = SavedGroupList().SelectedIndex();
            if (index < 0 || static_cast<size_t>(index) >= store.groups.size()) { return; }
            store.groups.erase(store.groups.begin() + index);
            _ViewModel.SavedTabGroupsJson(store.Json());_RefreshSavedGroups(index);
        }
        catch (const std::exception& error) { SavedGroupStatus().Text(winrt::to_hstring(error.what())); }
    }

}
