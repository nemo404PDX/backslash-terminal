// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <algorithm>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Automation.Peers.h>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    struct WorkspaceView
    {
        winrt::Windows::UI::Xaml::Controls::TextBox localPath{nullptr},remotePath{nullptr},filename{nullptr};
        winrt::Windows::UI::Xaml::Controls::TextBlock connection{nullptr},status{nullptr},localEmpty{nullptr},remoteEmpty{nullptr};
        winrt::Windows::UI::Xaml::Controls::ListView localFiles{nullptr},remoteFiles{nullptr};
        winrt::Windows::UI::Xaml::Controls::Button localGo{nullptr},remoteGo{nullptr},localUp{nullptr},remoteUp{nullptr},upload{nullptr},download{nullptr},cancelBrowse{nullptr};
        winrt::Windows::UI::Xaml::Controls::Grid browsers{nullptr};
    };
    // Shared by the production content and native mock UI fixture. Construction
    // has no settings, filesystem, networking or child-process side effects.
    inline WorkspaceView BuildWorkspace(const winrt::Windows::UI::Xaml::Controls::Grid& host,const winrt::hstring& initialLocal)
    {
        using namespace winrt;using namespace winrt::Windows::UI::Xaml;using namespace winrt::Windows::UI::Xaml::Controls;
        WorkspaceView view;StackPanel layout;layout.Spacing(8);host.Children().Append(layout);
        view.connection=TextBlock{};view.connection.Text(L"SFTP");view.connection.Visibility(Visibility::Collapsed);
        view.status=TextBlock{};view.status.TextWrapping(TextWrapping::Wrap);Automation::AutomationProperties::SetLiveSetting(view.status,Automation::Peers::AutomationLiveSetting::Polite);layout.Children().Append(view.status);
        const auto card=[] { return Markup::XamlReader::Load(LR"(<Border xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Padding="16" Background="{ThemeResource ExpanderHeaderBackground}" BorderBrush="{ThemeResource ExpanderHeaderBorderBrush}" BorderThickness="1" CornerRadius="{ThemeResource ControlCornerRadius}" />)").as<Border>(); };
        view.browsers=Grid{};view.browsers.ColumnSpacing(12);view.browsers.RowSpacing(8);
        for(int i=0;i<2;++i){ColumnDefinition c;c.Width({1,GridUnitType::Star});view.browsers.ColumnDefinitions().Append(c);RowDefinition r;r.Height(GridLengthHelper::Auto());view.browsers.RowDefinitions().Append(r);}
        layout.Children().Append(view.browsers);
        for(bool remote:{false,true}){
            StackPanel panel;panel.Spacing(8);TextBlock label;label.Text(remote?L"Remote":L"Local");panel.Children().Append(label);
            TextBox path;path.PlaceholderText(remote?L"Remote directory":L"Local directory");path.Text(remote?L".":initialLocal);Automation::AutomationProperties::SetName(path,remote?L"Remote directory":L"Local directory");panel.Children().Append(path);
            StackPanel actions;actions.Orientation(Orientation::Horizontal);actions.Spacing(8);
            Button go;go.Content(box_value(remote?L"Connect / Go":L"Go"));actions.Children().Append(go);Button up;up.Content(box_value(L"Up"));actions.Children().Append(up);panel.Children().Append(actions);
            ListView files;files.SelectionMode(ListViewSelectionMode::Single);files.Height(160);files.Visibility(Visibility::Collapsed);Automation::AutomationProperties::SetName(files,remote?L"Remote files":L"Local files");panel.Children().Append(files);
            TextBlock empty;empty.Text(remote?L"Connect to browse remote files.":L"Choose Go to browse local files.");empty.TextWrapping(TextWrapping::Wrap);panel.Children().Append(empty);
            files.Items().VectorChanged([weak=make_weak(files),hint=make_weak(empty)](auto&&,auto&&){if(const auto list=weak.get()){const bool hasFiles=list.Items().Size()!=0;list.Visibility(hasFiles?Visibility::Visible:Visibility::Collapsed);if(const auto text=hint.get())text.Visibility(hasFiles?Visibility::Collapsed:Visibility::Visible);}});
            if(remote){view.remoteEmpty=empty;view.remotePath=path;view.remoteFiles=files;view.remoteGo=go;view.remoteUp=up;}else{view.localEmpty=empty;view.localPath=path;view.localFiles=files;view.localGo=go;view.localUp=up;}
            auto browserCard=card();browserCard.Child(panel);if(remote)Grid::SetColumn(browserCard,1);view.browsers.Children().Append(browserCard);
        }
        view.browsers.SizeChanged([weak=winrt::make_weak(view.browsers)](auto&&,auto&&){if(const auto grid=weak.get()){
            const bool narrow=grid.ActualWidth()<700;const auto remote=grid.Children().GetAt(1).as<FrameworkElement>();Grid::SetColumn(remote,narrow?0:1);Grid::SetRow(remote,narrow?1:0);
            for(const auto& child:grid.Children()){const auto panel=child.as<FrameworkElement>();Grid::SetColumnSpan(panel,narrow?2:1);}
        }});
        view.filename=TextBox{};view.filename.PlaceholderText(L"Destination filename (optional)");Automation::AutomationProperties::SetName(view.filename,L"Destination filename");auto transferCard=card();StackPanel transferControls;transferControls.Spacing(8);transferControls.Children().Append(view.filename);transferCard.Child(transferControls);layout.Children().Append(transferCard);
        Grid actions;actions.ColumnSpacing(8);actions.RowSpacing(8);
        for(int i=0;i<3;++i){ColumnDefinition column;column.Width(GridLengthHelper::Auto());actions.ColumnDefinitions().Append(column);}
        RowDefinition actionRow;actionRow.Height(GridLengthHelper::Auto());actions.RowDefinitions().Append(actionRow);
        view.upload=Button{};view.upload.Content(box_value(L"Upload"));actions.Children().Append(view.upload);
        view.download=Button{};view.download.Content(box_value(L"Download"));Grid::SetColumn(view.download,1);actions.Children().Append(view.download);
        view.cancelBrowse=Button{};view.cancelBrowse.Content(box_value(L"Stop"));Automation::AutomationProperties::SetName(view.cancelBrowse,L"Cancel remote directory browsing");ToolTipService::SetToolTip(view.cancelBrowse,box_value(L"Cancel remote directory browsing"));Grid::SetColumn(view.cancelBrowse,2);actions.Children().Append(view.cancelBrowse);transferControls.Children().Append(actions);
        const auto reflow=[weak=make_weak(actions),available=make_weak(host)](auto&&,auto&&){const auto viewport=available.get();if(const auto panel=weak.get();panel&&viewport&&viewport.ActualWidth()>0){
            const auto width=std::max(0.0,viewport.ActualWidth()-34.0);panel.Width(width);
            const auto first=panel.Children().GetAt(0).as<FrameworkElement>();const auto second=panel.Children().GetAt(1).as<FrameworkElement>();const auto third=panel.Children().GetAt(2).as<FrameworkElement>();
            const auto pair=first.DesiredSize().Width+second.DesiredSize().Width+8;
            const unsigned columns=width>=pair+third.DesiredSize().Width+8?3u:width>=pair?2u:1u;
            if(panel.ColumnDefinitions().Size()!=columns){panel.ColumnDefinitions().Clear();for(unsigned i=0;i<columns;++i){ColumnDefinition c;c.Width(GridLengthHelper::Auto());panel.ColumnDefinitions().Append(c);}}
            const unsigned rows=columns==3?1u:columns==2?2u:3u;
            if(panel.RowDefinitions().Size()!=rows){panel.RowDefinitions().Clear();for(unsigned i=0;i<rows;++i){RowDefinition r;r.Height(GridLengthHelper::Auto());panel.RowDefinitions().Append(r);}}
            for(unsigned i=0;i<3;++i){const auto button=panel.Children().GetAt(i).as<FrameworkElement>();Grid::SetColumn(button,static_cast<int32_t>(i%columns));Grid::SetRow(button,static_cast<int32_t>(i/columns));}
        }};
        host.SizeChanged(reflow);actions.SizeChanged(reflow);
        return view;
    }
}
