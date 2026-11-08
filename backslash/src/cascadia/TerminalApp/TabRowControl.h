// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "winrt/Microsoft.UI.Xaml.Controls.h"

#include "TabRowControl.g.h"
#include <winrt/Windows.UI.ViewManagement.h>

namespace winrt::TerminalApp::implementation
{
    struct TabRowControl : TabRowControlT<TabRowControl>
    {
        TabRowControl();

        void OnNewTabButtonClick(const Windows::Foundation::IInspectable& sender, const Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs& args);
        void OnNewTabButtonDrop(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::DragEventArgs& e);
        void OnNewTabButtonDragOver(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::DragEventArgs& e);

        void OnTabsViewportSizeChanged(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::SizeChangedEventArgs& args);
        void _UpdateActionsLayout();
        bool _measuringActions{false};
        double _actionsBudget{-1},_actionsScale{0},_actionsRasterScale{0},_groupsFontSize{0},_pinsFontSize{0};
        hstring _groupsText,_pinsText;
        Windows::UI::ViewManagement::UISettings _uiSettings;

        til::property_changed_event PropertyChanged;
        WINRT_OBSERVABLE_PROPERTY(bool, ShowElevationShield, PropertyChanged.raise, false);
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(TabRowControl);
}
