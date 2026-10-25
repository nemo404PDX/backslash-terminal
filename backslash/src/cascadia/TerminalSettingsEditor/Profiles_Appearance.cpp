// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <cmath>
#include "Profiles_Appearance.h"
#include "Appearances.h"

#include "ProfileViewModel.h"
#include "PreviewConnection.h"

#include "Profiles_Appearance.g.cpp"

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;

static constexpr std::wstring_view AppearanceSettingPrefix{ L"App." };

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    Profiles_Appearance::Profiles_Appearance()
    {
        InitializeComponent();
        _previewConnection = winrt::make_self<PreviewConnection>();
    }

    void Profiles_Appearance::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _Profile = args.ViewModel().as<Editor::ProfileViewModel>();
        _weakWindowRoot = args.WindowRoot();

        // Settings are stored in Profiles_Appearance and Appearances.
        // We use the "App." prefix to indicate if it's in Appearances,
        // and remove it on the way to Appearances object.
        const auto elementToFocus = args.ElementToFocus();
        if (elementToFocus.starts_with(AppearanceSettingPrefix))
        {
            std::wstring correctedName{ elementToFocus.c_str() };
            get_self<implementation::Appearances>(DefaultAppearanceView())->BringIntoViewWhenLoaded(hstring{ correctedName.substr(AppearanceSettingPrefix.size()) });
        }
        else
        {
            BringIntoViewWhenLoaded(elementToFocus);
        }

        const auto settings = winrt::get_self<implementation::ProfileViewModel>(_Profile)->TermSettings();
        _previewConnection->DisplayPowerlineGlyphs(_Profile.DefaultAppearance().HasPowerlineCharacters());
        if (!_previewControl)
        {
            _previewControl = Control::TermControl(settings, settings, *_previewConnection);
            _previewControl.CursorVisibility(Control::CursorDisplayState::Shown);
            _previewControl.IsEnabled(false);
            _previewControl.AllowFocusWhenDisabled(false);
            _previewBezel.Content(_previewControl);
            ControlPreview().Child(_previewBezel.Root());
        }
        else
        {
            _previewControl.UpdateControlSettings(settings, settings);
        }

        _updateBezelPreview();
        _subscribePreviewRootChanges();

        // Subscribe to some changes in the view model
        // These changes should force us to update our own set of "Current<Setting>" members,
        // and propagate those changes to the UI
        _ViewModelChangedRevoker = _Profile.PropertyChanged(winrt::auto_revoke, { this, &Profiles_Appearance::_onProfilePropertyChanged });
        // The Appearances object handles updating the values in the settings UI, but
        // we still need to listen to the changes here just to update the preview control
        _AppearanceViewModelChangedRevoker = _Profile.DefaultAppearance().PropertyChanged(winrt::auto_revoke, { this, &Profiles_Appearance::_onProfilePropertyChanged });

        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "NavigatedToPage",
            TraceLoggingDescription("Event emitted when the user navigates to a page in the settings UI"),
            TraceLoggingValue("profile.appearance", "PageId", "The identifier of the page that was navigated to"),
            TraceLoggingValue(_Profile.IsBaseLayer(), "IsProfileDefaults", "If the modified profile is the profile.defaults object"),
            TraceLoggingValue(static_cast<GUID>(_Profile.Guid()), "ProfileGuid", "The guid of the profile that was navigated to"),
            TraceLoggingValue(_Profile.Source().c_str(), "ProfileSource", "The source of the profile that was navigated to"),
            TraceLoggingValue(_Profile.DefaultAppearance().BackgroundImageSettingsVisible(), "HasBackgroundImage", "If the profile has a background image defined"),
            TraceLoggingValue(_Profile.HasUnfocusedAppearance(), "HasUnfocusedAppearance", "If the profile has an unfocused appearance defined"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void Profiles_Appearance::OnNavigatedFrom(const NavigationEventArgs& /*e*/)
    {
        _previewRootChangedRevoker.revoke();
        _ViewModelChangedRevoker.revoke();
        _AppearanceViewModelChangedRevoker.revoke();
    }

    void Profiles_Appearance::CreateUnfocusedAppearance_Click(const IInspectable& /*sender*/, const RoutedEventArgs& /*e*/)
    {
        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "CreateUnfocusedAppearance",
            TraceLoggingDescription("Event emitted when the user creates an unfocused appearance for a profile"),
            TraceLoggingValue(_Profile.IsBaseLayer(), "IsProfileDefaults", "If the modified profile is the profile.defaults object"),
            TraceLoggingValue(static_cast<GUID>(_Profile.Guid()), "ProfileGuid", "The guid of the profile that was navigated to"),
            TraceLoggingValue(_Profile.Source().c_str(), "ProfileSource", "The source of the profile that was navigated to"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));

        _Profile.CreateUnfocusedAppearance();
    }

    void Profiles_Appearance::DeleteUnfocusedAppearance_Click(const IInspectable& /*sender*/, const RoutedEventArgs& /*e*/)
    {
        _Profile.DeleteUnfocusedAppearance();
    }

    void Profiles_Appearance::_updateBezelPreview()
    {
        const auto appearance = _Profile.DefaultAppearance();
        _previewBezel.Update(appearance.RetroBezelEnabled(), static_cast<int>(appearance.RetroBezelSize()),
                             static_cast<int>(appearance.RetroBezelColor()), static_cast<int>(appearance.RetroBezelBadge()), false, false,
                             static_cast<int>(appearance.RetroBezelFiller()), static_cast<int>(appearance.RetroBezelBadgeStyle()),
                             std::wstring_view{ appearance.RetroBezelBadgeLettering() }, std::wstring_view{ appearance.RetroBezelBadgeBacking() },
                             static_cast<int>(appearance.RetroBezelTopSize()), static_cast<int>(appearance.RetroBezelRightSize()), static_cast<int>(appearance.RetroBezelBottomSize()), static_cast<int>(appearance.RetroBezelLeftSize()));
        _updatePreviewLayout();
    }

    void Profiles_Appearance::PreviewViewport_Loaded(const IInspectable&, const RoutedEventArgs&)
    {
        _subscribePreviewRootChanges();
        _updatePreviewLayout();
    }

    void Profiles_Appearance::PreviewViewport_SizeChanged(const IInspectable&, const SizeChangedEventArgs&)
    {
        _updatePreviewLayout();
    }

    void Profiles_Appearance::_subscribePreviewRootChanges()
    {
        if (const auto root = XamlRoot())
        {
            _previewRootChangedRevoker = root.Changed(winrt::auto_revoke, [weak = get_weak()](const auto&, const auto&) {
                if (const auto page = weak.get()) { page->_updatePreviewLayout(); }
            });
        }
    }

    void Profiles_Appearance::_updatePreviewLayout()
    {
        if (!_Profile || !_previewControl) { return; }
        using namespace ::Microsoft::Terminal::Backslash;
        const auto root = XamlRoot();
        const auto rawScale = root ? root.RasterizationScale() : 1.0;
        const auto scale = std::isfinite(rawScale) && rawScale > 0 ? rawScale : 1.0;
        const auto dpi = static_cast<uint32_t>(std::lround(scale * 96.0));
        const auto settings = get_self<ProfileViewModel>(_Profile)->TermSettings();
        const auto required = Control::TermControl::GetProposedDimensions(settings, dpi,
                                                                          AppearancePreviewMinimumColumns,
                                                                          AppearancePreviewMinimumRows);
        const auto layout = GetAppearancePreviewLayout(static_cast<float>(PreviewViewport().ActualWidth()),
                                                        _previewBezel.Insets, required.Width, required.Height,
                                                        static_cast<float>(scale));
        const auto border = ControlPreview();
        // Width/Height start as NaN (Auto). Avoid layout feedback after setting them.
        if (!std::isfinite(border.Width()) || std::abs(border.Width() - layout.width) > 0.1) { border.Width(layout.width); }
        if (!std::isfinite(border.Height()) || std::abs(border.Height() - layout.height) > 0.1) { border.Height(layout.height); }
    }

    void Profiles_Appearance::_onProfilePropertyChanged(const IInspectable&, const PropertyChangedEventArgs&)
    {
        if (!_updatePreviewControl)
        {
            _updatePreviewControl = std::make_shared<ThrottledFunc<>>(
                winrt::Windows::System::DispatcherQueue::GetForCurrentThread(),
                til::throttled_func_options{
                    .delay = std::chrono::milliseconds{ 100 },
                    .debounce = true,
                    .trailing = true,
                },
                [this]() {
                    const auto settings = winrt::get_self<implementation::ProfileViewModel>(_Profile)->TermSettings();
                    _previewConnection->DisplayPowerlineGlyphs(_Profile.DefaultAppearance().HasPowerlineCharacters());
                    _previewControl.UpdateControlSettings(settings, settings);
                    _updateBezelPreview();
                });
        }

        _updatePreviewControl->Run();
    }
}
