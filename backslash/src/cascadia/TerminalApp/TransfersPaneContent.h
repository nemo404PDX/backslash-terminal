// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "winrt/TerminalApp.h"
#include "BasicPaneEvents.h"
#include "../inc/TransferJobs.h"

namespace winrt::TerminalApp::implementation
{
    class TransfersPaneContent : public winrt::implements<TransfersPaneContent, IPaneContent>, public BasicPaneEvents
    {
    public:
        explicit TransfersPaneContent(const Microsoft::Terminal::Settings::Model::CascadiaSettings& settings);
        ~TransfersPaneContent();
        static bool IsContent(const IPaneContent& content) { return content && content.GetRoot().Name() == L"BackslashTransfersView"; }
        std::function<void()> OpenBackendSettings;
        void OwnershipLease(uint64_t lease) noexcept { _lease = lease; }
        Windows::UI::Xaml::UIElement::KeyDown_revoker KeyDownRevoker;
        Windows::UI::Xaml::FrameworkElement GetRoot() { return _root; }
        void UpdateSettings(const Microsoft::Terminal::Settings::Model::CascadiaSettings& settings);
        Windows::Foundation::Size MinimumSize() { return { 360, 300 }; }
        hstring Title() { return RS_(L"Transfers_Title"); }
        uint64_t TaskbarState() { return 0; }
        uint64_t TaskbarProgress() { return 0; }
        bool ReadOnly() { return false; }
        hstring Icon() const { return L"\xE8AB"; }
        Windows::Foundation::IReference<Windows::UI::Color> TabColor() { return nullptr; }
        Windows::UI::Xaml::Media::Brush BackgroundBrush();
        Microsoft::Terminal::Settings::Model::INewContentArgs GetNewTerminalArgs(BuildStartupKind) const;
        void Focus(Windows::UI::Xaml::FocusState reason);
        void Close();
    private:
        void _Refresh();
        void _UpdateFilterLayout();
        Windows::UI::Xaml::Controls::Grid _root{nullptr};
        Windows::UI::Xaml::Controls::Grid _filtersLayout{nullptr};
        Windows::UI::Xaml::Controls::TextBox _search{nullptr};
        Windows::UI::Xaml::Controls::ComboBox _filter{nullptr};
        Windows::UI::Xaml::Controls::ListView _list{nullptr};
        Windows::UI::Xaml::Controls::TextBlock _empty{nullptr};
        Windows::UI::Xaml::FrameworkElement::Loaded_revoker _loaded;
        Windows::UI::Xaml::FrameworkElement::SizeChanged_revoker _filtersSizeChanged;
        Windows::UI::Xaml::Controls::TextBox::TextChanged_revoker _searchChanged;
        Windows::UI::Xaml::Controls::Primitives::Selector::SelectionChanged_revoker _filterChanged;
        uint64_t _subscription = 0;
        uint64_t _lease = 0;
        bool _closed = false;
    };
}
