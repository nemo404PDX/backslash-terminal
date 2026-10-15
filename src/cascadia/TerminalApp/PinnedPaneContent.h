// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "winrt/TerminalApp.h"
#include "BasicPaneEvents.h"

namespace winrt::TerminalApp::implementation
{
    class PinnedPaneContent : public winrt::implements<PinnedPaneContent, IPaneContent>, public BasicPaneEvents
    {
        Windows::UI::Xaml::Controls::Grid _root;
        Windows::UI::Xaml::Controls::TextBlock _message;
        winrt::hstring _identity;
        winrt::hstring _icon;
    public:
        PinnedPaneContent(winrt::hstring identity, winrt::hstring icon);
        void Message(winrt::hstring message);
        void Identity(winrt::hstring identity, winrt::hstring icon);
        void UpdateSettings(const Microsoft::Terminal::Settings::Model::CascadiaSettings&) {}
        Windows::UI::Xaml::FrameworkElement GetRoot() { return _root; }
        Windows::Foundation::Size MinimumSize() { return { 1, 1 }; }
        winrt::hstring Title() { return _identity; }
        uint64_t TaskbarState() { return 0; }
        uint64_t TaskbarProgress() { return 0; }
        bool ReadOnly() { return false; }
        winrt::hstring Icon() const { return _icon; }
        Windows::Foundation::IReference<Windows::UI::Color> TabColor() const noexcept { return nullptr; }
        Windows::UI::Xaml::Media::Brush BackgroundBrush();
        Microsoft::Terminal::Settings::Model::INewContentArgs GetNewTerminalArgs(BuildStartupKind) const;
        void Focus(Windows::UI::Xaml::FocusState) {}
        void Close() {} // No process, connection or content-core exists.
    };
}
