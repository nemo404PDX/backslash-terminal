// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "Launch.h"
#include "Launch.g.cpp"
#include "EnumEntry.h"
#include "LaunchViewModel.h"
#include "../TerminalSettingsModel/SavedTabGroups.h"
#include "../TerminalSettingsModel/StartupGroupPolicy.h"
#include "../inc/ModernExplorerIntegration.h"
#include "../inc/ExplorerRestart.h"
#include <appmodel.h>

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

    void Launch::ExplorerActions_SizeChanged(const IInspectable& sender, const SizeChangedEventArgs&)
    {
        const auto element=sender.as<FrameworkElement>();
        const auto panel=element.try_as<Controls::Grid>()?element.as<Controls::Grid>():element.Parent().try_as<Controls::Grid>();
        if(!panel||panel.Children().Size()!=3||panel.ActualWidth()<=0)return;
        double widths[3]{};
        for(uint32_t i=0;i<3;++i){
            const auto button=panel.Children().GetAt(i).as<FrameworkElement>();
            button.Measure({std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()});
            widths[i]=button.DesiredSize().Width;
            button.HorizontalAlignment(HorizontalAlignment::Left);
        }
        const auto available=std::isfinite(panel.Width())?panel.Width():panel.Parent().as<FrameworkElement>().ActualWidth();
        if(available<=0)return;
        const int columns=widths[0]+widths[1]+widths[2]+16<=available?3:widths[0]+widths[1]+8<=available?2:1;
        if(panel.ColumnDefinitions().Size()!=static_cast<uint32_t>(columns)){
            panel.ColumnDefinitions().Clear();panel.RowDefinitions().Clear();
            for(int i=0;i<columns;++i){Controls::ColumnDefinition c;c.Width(GridLengthHelper::Auto());panel.ColumnDefinitions().Append(c);}
            for(int i=0;i<(3+columns-1)/columns;++i){Controls::RowDefinition r;r.Height(GridLengthHelper::Auto());panel.RowDefinitions().Append(r);}
        }
        for(int i=0;i<3;++i){const auto child=panel.Children().GetAt(i).as<FrameworkElement>();Controls::Grid::SetColumn(child,i%columns);Controls::Grid::SetRow(child,i/columns);Controls::Grid::SetColumnSpan(child,columns==2&&i==2?2:1);}
    }

    void Launch::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _ViewModel = args.ViewModel().as<Editor::LaunchViewModel>();
        BringIntoViewWhenLoaded(args.ElementToFocus());
        _RefreshSavedGroups();
        _RefreshExplorer();
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
    void Launch::_ExplorerControls(bool enabled)
    {
        ExplorerEnable().IsEnabled(enabled); ExplorerRepair().IsEnabled(enabled); ExplorerRemove().IsEnabled(enabled);
        ExplorerCheckStatus().IsEnabled(enabled); ExplorerNotify().IsEnabled(enabled); ExplorerRestart().IsEnabled(enabled);
    }

    void Launch::_ExplorerError(const std::wstring& message)
    {
        ExplorerError().Text(message); ExplorerError().Visibility(message.empty() ? Visibility::Collapsed : Visibility::Visible);
    }

    void Launch::_RenderExplorer(const ::Backslash::ModernExplorer::Management::State& state)
    {
        namespace Modern = ::Backslash::ModernExplorer::Management;
        _explorerState = state;
        ExplorerDiagnostics().Text(state.details);
        ExplorerLegacy().Text(state.legacy);
        std::wstring status = !state.safe || !state.supported ? L"Modern integration unavailable" :
            state.package && state.configComplete && state.filesComplete && state.targetValid ?
            (Modern::Same(state.target, state.current) ? L"Modern folder menu enabled for this copy" : L"Modern folder menu enabled for another copy") :
            state.package || state.config || state.supportExists ? L"Modern integration needs repair" : L"Modern folder menu is not enabled";
        ExplorerStatus().Text(status);
        _ExplorerError(state.problem);
        _ExplorerControls(!_explorerBusy);
        const auto allowed = [&](Modern::Action action) {
            try { const auto plan = Modern::MakePlan(action, state, state.target); return action == Modern::Action::Repair ? state.package || state.config || state.supportExists : plan.Changes(); }
            catch (...) { return false; }
        };
        ExplorerEnable().Content(winrt::box_value(state.package ? L"Use this copy" : L"Enable"));
        ExplorerEnable().IsEnabled(!_explorerBusy && allowed(Modern::Action::Enable));
        ExplorerRepair().IsEnabled(!_explorerBusy && allowed(Modern::Action::Repair));
        ExplorerRemove().IsEnabled(!_explorerBusy && allowed(Modern::Action::Remove));
    }

    winrt::fire_and_forget Launch::_RefreshExplorer()
    {
        const auto lifetime = get_strong();
        if (_explorerBusy) { co_return; }
        _explorerBusy = true; _ExplorerControls(false); ExplorerStatus().Text(L"Checking modern integration…");
        const auto ui = winrt::apartment_context();
        const auto exe = wil::GetModuleFileNameW<std::wstring>(nullptr);
        ::Backslash::ModernExplorer::Management::State state; state.current = exe;
        co_await winrt::resume_background();
        try { ::Backslash::ModernExplorer::Management::NativeBackend backend{exe}; state = backend.Read(); }
        catch (...) { state.safe = false; state.problem = ::Backslash::ModernExplorer::Management::DescribeError(); }
        co_await ui;
        _explorerBusy = false; _RenderExplorer(state);
    }

    void Launch::Explorer_Refresh(const IInspectable&, const RoutedEventArgs&) { _RefreshExplorer(); }

    winrt::fire_and_forget Launch::Explorer_Change(const IInspectable& sender, const RoutedEventArgs&)
    {
        namespace Modern = ::Backslash::ModernExplorer::Management;
        const auto lifetime = get_strong();
        if (_explorerBusy) { co_return; }
        _explorerBusy = true; _ExplorerControls(false); _ExplorerError(L"");
        ExplorerResult().Visibility(Visibility::Collapsed);
        const auto action = sender.as<Controls::Button>() == ExplorerRemove() ? Modern::Action::Remove :
                            sender.as<Controls::Button>() == ExplorerRepair() ? Modern::Action::Repair : Modern::Action::Enable;
        const auto previous = _explorerState;
        try
        {
            const auto plan = Modern::MakePlan(action, previous, previous.target);
            const auto heading = action == Modern::Action::Remove ? L"Remove modern Explorer integration?" : action == Modern::Action::Repair ? L"Repair modern Explorer integration?" : L"Use this Backslash copy?";
            std::wstring message = action == Modern::Action::Remove ? L"Remove the owned current-user helper package, target selection and verified support files. Classic menu entries are left unchanged." :
                L"Manage the current-user development helper without changing Windows policy, certificates or profile starting folders.";
            if (plan.registration) { message += L"\n\nRegister the development helper using the already-enabled Developer Mode."; }
            if (plan.files) { message += L"\n\nCopy the eight reviewed helper files into:\n" + previous.support; }
            message += L"\n\nSelected copy:\n" + (previous.target.empty() ? L"None" : previous.target);
            if (action != Modern::Action::Remove) { message += L"\n\nTarget after this action:\n" + plan.selected; }
            Controls::ContentDialog dialog; dialog.XamlRoot(XamlRoot()); dialog.Title(winrt::box_value(heading));
            Controls::TextBlock text; text.Text(message); text.TextWrapping(TextWrapping::Wrap);
            Controls::ScrollViewer scroll; scroll.Content(text); scroll.MaxHeight(360); dialog.Content(scroll);
            dialog.PrimaryButtonText(action == Modern::Action::Remove ? L"Remove" : action == Modern::Action::Repair ? L"Repair" : L"Use this copy");
            dialog.CloseButtonText(L"Cancel"); dialog.DefaultButton(Controls::ContentDialogButton::Close);
            if (co_await dialog.ShowAsync() != Controls::ContentDialogResult::Primary) { _explorerBusy = false; _RenderExplorer(previous); co_return; }
        }
        catch (...) { _explorerBusy = false; _RenderExplorer(previous); _ExplorerError(Modern::DescribeError()); co_return; }
        const auto ui = winrt::apartment_context(); Modern::State state; Modern::Result result;
        co_await winrt::resume_background();
        try { Modern::NativeBackend backend{previous.current}; result = Modern::Execute(backend, action, previous.target); state = backend.Read(); }
        catch (...) { state = previous; result.error = Modern::DescribeError(); }
        co_await ui;
        _explorerBusy = false; _RenderExplorer(state);
        if (!result.success) { _ExplorerError(result.error + (state.problem.empty() ? L"" : L"\n" + state.problem)); }
        else
        {
            ExplorerResult().Text(result.changed ? action == Modern::Action::Remove ? L"Modern integration removed. Classic entries were left unchanged." : L"Changes applied. A shell update was requested." : L"Already up to date. No changes were needed.");
            ExplorerResult().Visibility(Visibility::Visible);
        }
    }

    void Launch::Explorer_Notify(const IInspectable&, const RoutedEventArgs&)
    {
        if (_explorerBusy) { return; }
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, nullptr, nullptr);
        ExplorerResult().Text(L"Shell update requested. This does not restart Explorer or guarantee an immediate icon refresh.");
        ExplorerResult().Visibility(Visibility::Visible);
    }

    winrt::fire_and_forget Launch::Explorer_Restart(const IInspectable&, const RoutedEventArgs&)
    {
        namespace Modern = ::Backslash::ModernExplorer::Management;
        const auto lifetime = get_strong();
        if (_explorerBusy) { co_return; }
        _explorerBusy = true; _ExplorerControls(false);
        bool confirmed = false;
        try
        {
            Controls::ContentDialog dialog; dialog.XamlRoot(XamlRoot()); dialog.Title(winrt::box_value(L"Restart Explorer?"));
            Controls::TextBlock text; text.Text(L"This can interrupt the desktop and taskbar, close File Explorer windows, and disrupt file operations. Finish any Explorer work first. Windows may refuse a safe restart; Backslash will not force termination or elevate."); text.TextWrapping(TextWrapping::Wrap); dialog.Content(text);
            dialog.PrimaryButtonText(L"Restart Explorer"); dialog.CloseButtonText(L"Cancel"); dialog.DefaultButton(Controls::ContentDialogButton::Close);
            confirmed = co_await dialog.ShowAsync() == Controls::ContentDialogResult::Primary;
        }
        catch (...) { _ExplorerError(Modern::DescribeError()); }
        std::wstring failure;
        if (confirmed)
        {
            const auto ui = winrt::apartment_context(); co_await winrt::resume_background();
            try { Modern::AfterRestartConfirmation(confirmed, [] { Modern::RestartExplorerGracefully(); }); }
            catch (...) { failure = Modern::DescribeError(); }
            co_await ui;
        }
        _explorerBusy = false; _RenderExplorer(_explorerState);
        if (!failure.empty()) { _ExplorerError(failure); }
        else if (confirmed) { ExplorerResult().Text(L"Windows completed the Explorer restart request. Check the desktop and folder menu."); ExplorerResult().Visibility(Visibility::Visible); }
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
