// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "../TerminalSettingsModel/PinnedStore.h"
#include "PinnedPaneContent.h"
namespace winrt::TerminalApp::implementation
{
    struct PinTabView
    {
        std::string id;
        std::optional<::Microsoft::Terminal::Backslash::Pins::Destination> destination;
        std::optional<::Microsoft::Terminal::Backslash::Pins::Lease> lease;
        winrt::com_ptr<PinnedPaneContent> placeholder;
        winrt::hstring icon;
        bool dormant = true;
        bool busy = false;
        std::string transfer;
    };
    inline constexpr double PinnedTabWidth = 88.0;
}
