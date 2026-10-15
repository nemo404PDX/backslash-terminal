// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "PinnedPaneContent.h"
namespace winrt::TerminalApp::implementation
{
    PinnedPaneContent::PinnedPaneContent(winrt::hstring identity, winrt::hstring icon) : _identity{ std::move(identity) }, _icon{ std::move(icon) }
    {
        using namespace Windows::UI::Xaml;
        _message.TextWrapping(TextWrapping::Wrap);
        _message.Margin({ 24, 24, 24, 24 });
        _message.HorizontalAlignment(HorizontalAlignment::Center);
        _message.VerticalAlignment(VerticalAlignment::Center);
        _root.Children().Append(_message);
        Message(RS_(L"Pins_Opening"));
    }
    void PinnedPaneContent::Message(winrt::hstring message)
    {
        _message.Text(_identity + L"\n\n" + message);
        Windows::UI::Xaml::Automation::AutomationProperties::SetName(_root, _message.Text());
    }
    void PinnedPaneContent::Identity(winrt::hstring identity, winrt::hstring icon)
    {
        _identity = std::move(identity); _icon = std::move(icon); Message(RS_(L"Pins_Opening"));
    }
    Windows::UI::Xaml::Media::Brush PinnedPaneContent::BackgroundBrush()
    {
        return Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::Transparent() };
    }
    Microsoft::Terminal::Settings::Model::INewContentArgs PinnedPaneContent::GetNewTerminalArgs(BuildStartupKind) const
    {
        return Microsoft::Terminal::Settings::Model::NewTerminalArgs{};
    }
}
