// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <ThrottledFunc.h>

#include "Profiles_Appearance.g.h"
#include "PreviewConnection.h"
#include "../inc/RetroBezel.h"
#include "../inc/AppearancePreviewLayout.h"
#include "Utils.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct Profiles_Appearance : public HasScrollViewer<Profiles_Appearance>, Profiles_AppearanceT<Profiles_Appearance>
    {
    public:
        Profiles_Appearance();

        void OnNavigatedTo(const Windows::UI::Xaml::Navigation::NavigationEventArgs& e);
        void OnNavigatedFrom(const Windows::UI::Xaml::Navigation::NavigationEventArgs& e);

        void CreateUnfocusedAppearance_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& e);
        void DeleteUnfocusedAppearance_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& e);

        void PreviewViewport_Loaded(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void PreviewViewport_SizeChanged(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::SizeChangedEventArgs&);

        Editor::IHostedInWindow WindowRoot() const noexcept { return _weakWindowRoot.get(); };

        til::property_changed_event PropertyChanged;
        WINRT_PROPERTY(Editor::ProfileViewModel, Profile, nullptr);

    private:
        void _onProfilePropertyChanged(const IInspectable&, const PropertyChangedEventArgs&);

        winrt::com_ptr<PreviewConnection> _previewConnection{ nullptr };
        Microsoft::Terminal::Control::TermControl _previewControl{ nullptr };
        ::Microsoft::Terminal::Backslash::RetroBezel _previewBezel;
        void _updateBezelPreview();
        void _updatePreviewLayout();
        void _subscribePreviewRootChanges();
        Windows::UI::Xaml::XamlRoot::Changed_revoker _previewRootChangedRevoker;
        std::shared_ptr<ThrottledFunc<>> _updatePreviewControl;
        Windows::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker _ViewModelChangedRevoker;
        Windows::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker _AppearanceViewModelChangedRevoker;
        winrt::weak_ref<Editor::IHostedInWindow> _weakWindowRoot;
    };
};

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(Profiles_Appearance);
}
