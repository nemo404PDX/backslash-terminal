// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "Launch.g.h"
#include "Utils.h"
#include "../inc/ModernExplorerPolicy.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct Launch : public HasScrollViewer<Launch>, LaunchT<Launch>
    {
    public:
        Launch();

        void OnNavigatedTo(const winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs& e);
        void Explorer_Refresh(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        winrt::fire_and_forget Explorer_Change(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        winrt::fire_and_forget _RefreshExplorer();
        void Explorer_Notify(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        winrt::fire_and_forget Explorer_Restart(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void _RenderExplorer(const ::Backslash::ModernExplorer::Management::State&);
        void _ExplorerControls(bool enabled);
        void _ExplorerError(const std::wstring&);
        ::Backslash::ModernExplorer::Management::State _explorerState;
        bool _explorerBusy{ false };

        void SavedGroups_SelectionChanged(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::Controls::SelectionChangedEventArgs&);
        void SavedGroups_Update(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void SavedGroups_Delete(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void _RefreshSavedGroups(int selected = -1);
        void _RefreshStartupGroup();
        void StartupMode_SelectionChanged(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::Controls::SelectionChangedEventArgs&);
        void StartupGroup_TextChanged(const Windows::UI::Xaml::Controls::AutoSuggestBox&, const Windows::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs&);
        void StartupGroup_QuerySubmitted(const Windows::UI::Xaml::Controls::AutoSuggestBox&, const Windows::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs&);
        bool _refreshingStartup{ false };
        til::property_changed_event PropertyChanged;
        WINRT_OBSERVABLE_PROPERTY(Editor::LaunchViewModel, ViewModel, PropertyChanged.raise, nullptr);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(Launch);
}
