// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "TransfersPaneContent.h"
#include "SettingsViewHost.h"
#include "Utils.h"
#include "../inc/TransfersViewLayout.h"
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Microsoft::Terminal::Settings::Model;
namespace T = ::Microsoft::Terminal::Backslash::Transfers;
namespace B = ::Microsoft::Terminal::Backslash;

namespace winrt::TerminalApp::implementation
{
    TransfersPaneContent::TransfersPaneContent(const CascadiaSettings& settings)
    {
        const auto viewTemplate = Application::Current().Resources().Lookup(box_value(L"TransfersViewTemplate")).as<DataTemplate>();
        _root = viewTemplate.LoadContent().as<Grid>();
        _search = _root.FindName(L"Search").as<TextBox>();
        _filter = _root.FindName(L"StatusFilter").as<ComboBox>();
        _list = _root.FindName(L"Jobs").as<ListView>();
        _empty = _root.FindName(L"Empty").as<TextBlock>();
        _filtersLayout = _root.FindName(L"FiltersLayout").as<Grid>();
        _root.FindName(L"Heading").as<TextBlock>().Text(Title());
        const auto gear = _root.FindName(L"BackendSettings").as<Button>();
        Automation::AutomationProperties::SetName(gear, RS_(L"Transfers_BackendSettings"));
        ToolTipService::SetToolTip(gear, box_value(RS_(L"Transfers_BackendSettings")));
        gear.Click([weak = get_weak()](auto&&, auto&&) { if (const auto self = weak.get(); self && self->OpenBackendSettings) { self->OpenBackendSettings(); } });
        _root.FindName(L"BackendNotice").as<TextBlock>().Text(RS_(L"Transfers_NoBackend"));
        _search.PlaceholderText(RS_(L"Transfers_Search"));
        Automation::AutomationProperties::SetName(_search, RS_(L"Transfers_Search"));
        Automation::AutomationProperties::SetName(_filter, RS_(L"Transfers_Filter"));
        Automation::AutomationProperties::SetName(_list, Title());
        for (const auto key : { USES_RESOURCE(L"Transfers_All"), USES_RESOURCE(L"Transfers_Active"), USES_RESOURCE(L"Transfers_Completed"), USES_RESOURCE(L"Transfers_Failed"), USES_RESOURCE(L"Transfers_Canceled") })
        {
            ComboBoxItem item; item.Content(box_value(GetLibraryResourceString(key))); _filter.Items().Append(item);
        }
        _filter.SelectedIndex(0);
        _filtersSizeChanged = _filtersLayout.SizeChanged(winrt::auto_revoke, [weak = get_weak()](auto&&, auto&&) {
            if (const auto self = weak.get()) { self->_UpdateFilterLayout(); }
        });
        _searchChanged = _search.TextChanged(winrt::auto_revoke, [weak = get_weak()](auto&&, auto&&) { if (const auto self = weak.get()) { self->_Refresh(); } });
        _filterChanged = _filter.SelectionChanged(winrt::auto_revoke, [weak = get_weak()](auto&&, auto&&) { if (const auto self = weak.get()) { self->_Refresh(); } });
        const auto dispatcher = _root.Dispatcher();
        _subscription = T::Store::Shared().Subscribe([weak = get_weak(), dispatcher] {
            try
            {
                dispatcher.RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, [weak] {
                    if (const auto self = weak.get(); self && !self->_closed) { self->_Refresh(); }
                });
            }
            catch (...) { LOG_CAUGHT_EXCEPTION(); }
        });
        _loaded = _root.Loaded(winrt::auto_revoke, [weak = get_weak()](auto&&, auto&&) { if (const auto self = weak.get()) { self->_Refresh(); } });
        UpdateSettings(settings);
        _UpdateFilterLayout();
        _Refresh();
    }
    TransfersPaneContent::~TransfersPaneContent() { Close(); }
    void TransfersPaneContent::Close()
    {
        if (_closed) { return; }
        _closed = true;
        T::Store::Shared().Unsubscribe(_subscription);
        SettingsViewHost::Instance(B::BuiltinViewId::Transfers).Release(_lease);
        _lease = 0;
        // Detaching the view never changes the process-owned job records.
    }
    void TransfersPaneContent::UpdateSettings(const CascadiaSettings& settings)
    {
        _root.RequestedTheme(settings.GlobalSettings().CurrentTheme().RequestedTheme());
    }
    Windows::UI::Xaml::Media::Brush TransfersPaneContent::BackgroundBrush()
    {
        return _root.Background();
    }
    INewContentArgs TransfersPaneContent::GetNewTerminalArgs(BuildStartupKind) const { return BaseContentArgs(L"transfers"); }
    void TransfersPaneContent::Focus(FocusState reason)
    {
        if (!_closed && reason != FocusState::Unfocused) { _search.Focus(reason); }
    }
    void TransfersPaneContent::_UpdateFilterLayout()
    {
        if (_closed) { return; }
        // Measure localized labels using the actual native control font. The
        // policy caps status width and keeps at least 240 DIPs for wide search.
        TextBlock measurement;
        measurement.FontFamily(_filter.FontFamily());
        measurement.FontSize(_filter.FontSize());
        measurement.FontWeight(_filter.FontWeight());
        measurement.FontStyle(_filter.FontStyle());
        double longestLabel = 0;
        for (const auto& value : _filter.Items())
        {
            const auto item = value.as<ComboBoxItem>();
            measurement.Text(winrt::unbox_value<hstring>(item.Content()));
            measurement.Measure({std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()});
            longestLabel = std::max(longestLabel, static_cast<double>(measurement.DesiredSize().Width));
        }
        namespace L = ::Microsoft::Terminal::Backslash::TransfersViewLayout;
        const auto statusWidth = L::StatusWidthForLabel(longestLabel);
        const bool wide = L::UseSideBySide(_filtersLayout.ActualWidth(), statusWidth);
        Grid::SetColumnSpan(_search, wide ? 1 : 2);
        Grid::SetRow(_filter, wide ? 0 : 1);
        Grid::SetColumn(_filter, wide ? 1 : 0);
        Grid::SetColumnSpan(_filter, wide ? 1 : 2);
        _filter.Width(wide ? statusWidth : std::numeric_limits<double>::quiet_NaN());
        _filtersLayout.RowSpacing(wide ? 0 : 12);
    }
    void TransfersPaneContent::_Refresh()
    {
        if (_closed) { return; }
        const auto jobs = T::Store::Shared().Snapshot();
        const auto index = _filter.SelectedIndex();
        const auto filter = index >= 0 && index <= 4 ? static_cast<T::Filter>(index) : T::Filter::All;
        _list.Items().Clear();
        for (const auto& job : jobs)
        {
            if (!T::Matches(job, filter, std::wstring_view{_search.Text()})) { continue; }
            const wchar_t* statusKey = USES_RESOURCE(L"Transfers_Queued");
            switch (job.status)
            {
            case T::Status::Connecting: statusKey = USES_RESOURCE(L"Transfers_Connecting"); break;
            case T::Status::Running: statusKey = USES_RESOURCE(L"Transfers_Running"); break;
            case T::Status::Verifying: statusKey = USES_RESOURCE(L"Transfers_Verifying"); break;
            case T::Status::Canceling: statusKey = USES_RESOURCE(L"Transfers_Canceling"); break;
            case T::Status::Completed: statusKey = USES_RESOURCE(L"Transfers_Completed"); break;
            case T::Status::Failed: statusKey = USES_RESOURCE(L"Transfers_Failed"); break;
            case T::Status::Canceled: statusKey = USES_RESOURCE(L"Transfers_Canceled"); break;
            default: break;
            }
            TextBlock row;
            row.Text(winrt::hstring{job.name} + L" — " + GetLibraryResourceString(statusKey) + L"\n" + winrt::hstring{job.source} + L" → " + winrt::hstring{job.destination} + L"\n" + winrt::hstring{job.detail});
            row.TextWrapping(TextWrapping::Wrap);
            row.IsTextSelectionEnabled(true);
            _list.Items().Append(row);
        }
        _empty.Text(jobs.empty() ? RS_(L"Transfers_Empty") : RS_(L"Transfers_NoMatches"));
        _empty.Visibility(_list.Items().Size() ? Visibility::Collapsed : Visibility::Visible);
        _list.Visibility(_list.Items().Size() ? Visibility::Visible : Visibility::Collapsed);
    }
}
