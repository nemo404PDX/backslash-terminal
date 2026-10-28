// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "AddProfile.h"
#include "AddProfile.g.cpp"
#include "AddProfilePageNavigationState.g.cpp"
#include "EnumEntry.h"
#include "SshScanUi.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Microsoft::Terminal::Settings::Model;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    AddProfile::AddProfile()
    {
        InitializeComponent();

        Automation::AutomationProperties::SetName(AddNewButton(), RS_(L"AddProfile_AddNewTextBlock/Text"));
        Automation::AutomationProperties::SetName(DuplicateButton(), RS_(L"AddProfile_DuplicateTextBlock/Text"));
    }

    void AddProfile::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _State = args.ViewModel().as<Editor::AddProfilePageNavigationState>();
        _hostRoot = args.WindowRoot();
        BringIntoViewWhenLoaded(args.ElementToFocus());

        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "NavigatedToPage",
            TraceLoggingDescription("Event emitted when the user navigates to a page in the settings UI"),
            TraceLoggingValue("addProfile", "PageId", "The identifier of the page that was navigated to"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void AddProfile::BackgroundSettingsClick(const IInspectable&, const RoutedEventArgs&)
    {
        if (const auto root = _hostRoot.get()) { root.OpenBackgroundSettings(); }
    }
    void AddProfile::ScanSshClick(const IInspectable&, const RoutedEventArgs&)
    {
        namespace ScanUi = ::Microsoft::Terminal::Backslash::SshScanUi;
        std::wstring_view stage = L"preparing scan results";
        try
        {
            ImportSelectedSsh().IsEnabled(false);
            SshCandidates().ItemsSource(nullptr);
            SshScanStatus().Text(L"Scanning this computer's SSH connections...");
            stage = L"reading SSH connection names";
            const auto candidates = _State.Settings().ScanSshProfiles();
            stage = L"displaying scan results";
            SshCandidates().ItemsSource(ScanUi::Rows(candidates));
            SshScanStatus().Text(candidates.Size() ? L"Select the connections to keep, then Import selected and Save Settings." : L"No new SSH connections found. Saved and hidden connections are already in Settings.");
            ImportSelectedSsh().IsEnabled(candidates.Size() != 0);
        }
        catch (const winrt::hresult_error& ex) { SshScanStatus().Text(ScanUi::Failure(stage, ex)); }
        catch (const std::exception& ex) { SshScanStatus().Text(ScanUi::Failure(stage, ex)); }
    }
    void AddProfile::ImportSelectedSshClick(const IInspectable&, const RoutedEventArgs&)
    {
        // Copy before navigation events change the selection/page.
        std::vector<winrt::guid> selected;
        for (const auto& item : SshCandidates().SelectedItems()) { selected.push_back(item.as<Model::Profile>().Guid()); }
        if (selected.empty()) { SshScanStatus().Text(L"Select at least one connection to import."); return; }
        try { for (const auto& id : selected) { _State.RequestRestoreSsh(id); } }
        catch (const std::exception& ex) { SshScanStatus().Text(winrt::to_hstring(ex.what())); }
        catch (const winrt::hresult_error& ex) { SshScanStatus().Text(ex.message()); }
    }

    void AddProfile::AddNewClick(const IInspectable& /*sender*/,
                                 const Windows::UI::Xaml::RoutedEventArgs& /*eventArgs*/)
    {
        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "AddNewProfile",
            TraceLoggingDescription("Event emitted when the user adds a new profile"),
            TraceLoggingValue("EmptyProfile", "Type", "The type of the creation method (i.e. empty profile, duplicate)"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));

        _State.RequestAddNew();
    }

    void AddProfile::DuplicateClick(const IInspectable& /*sender*/,
                                    const Windows::UI::Xaml::RoutedEventArgs& /*eventArgs*/)
    {
        if (const auto selected = Profiles().SelectedItem())
        {
            const auto selectedProfile = selected.as<Model::Profile>();
            TraceLoggingWrite(
                g_hTerminalSettingsEditorProvider,
                "AddNewProfile",
                TraceLoggingDescription("Event emitted when the user adds a new profile"),
                TraceLoggingValue("Duplicate", "Type", "The type of the creation method (i.e. empty profile, duplicate)"),
                TraceLoggingValue(!selectedProfile.Source().empty(), "SourceProfileHasSource", "True, if the source profile has a source (i.e. dynamic profile generator namespace, fragment). Otherwise, False, indicating it's based on a custom profile."),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));

            _State.RequestDuplicate(selectedProfile.Guid());
        }
    }

    void AddProfile::ProfilesSelectionChanged(const IInspectable& /*sender*/,
                                              const Windows::UI::Xaml::RoutedEventArgs& /*eventArgs*/)
    {
        if (!_IsProfileSelected)
        {
            IsProfileSelected(true);
        }
    }
}
