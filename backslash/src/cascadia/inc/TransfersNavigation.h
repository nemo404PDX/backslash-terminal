// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <algorithm>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <functional>

namespace Microsoft::Terminal::Backslash::TransfersShell
{
    inline bool FocusActiveView(const winrt::Windows::UI::Xaml::Controls::Grid& root,
                                const winrt::Windows::UI::Xaml::Controls::Control& sftpTarget,
                                winrt::Windows::UI::Xaml::FocusState reason)
    {
        using namespace winrt::Windows::UI::Xaml;
        using namespace winrt::Windows::UI::Xaml::Controls;
        if(reason==FocusState::Unfocused)return false;
        const auto target=root.FindName(L"SftpWorkspace").as<UIElement>().Visibility()==Visibility::Visible?
            sftpTarget:root.FindName(L"Search").as<Control>();
        return target&&target.IsEnabled()&&target.Focus(reason);
    }
    // View selection only: no settings writes, connection, cancellation or job ownership.
    inline winrt::Microsoft::UI::Xaml::Controls::NavigationView Attach(
        const winrt::Windows::UI::Xaml::Controls::Grid& root, std::function<void()> openSettings)
    {
        using namespace winrt;
        using namespace winrt::Windows::UI::Xaml;
        using namespace winrt::Windows::UI::Xaml::Controls;
        namespace Mux=winrt::Microsoft::UI::Xaml::Controls;
        const auto navigation=root.FindName(L"TransfersNavigation").as<Mux::NavigationView>();
        const auto viewport=root.FindName(L"TransfersScrollViewer").as<ScrollViewer>();
        const auto sizeBody=[weak=make_weak(root)](auto&&,auto&&){if(const auto owner=weak.get()){
            const auto width=owner.FindName(L"TransfersScrollViewer").as<ScrollViewer>().ActualWidth();
            if(width>32){const auto bodyWidth=std::min(1000.0,width-32.0);owner.FindName(L"TransfersContentCard").as<FrameworkElement>().Width(bodyWidth);owner.FindName(L"Heading").as<FrameworkElement>().Width(bodyWidth);}
        }};
        viewport.SizeChanged(sizeBody);viewport.Loaded(sizeBody);
        navigation.SelectionChanged([weak=make_weak(root)](const Mux::NavigationView& sender, const Mux::NavigationViewSelectionChangedEventArgs& args) {
            const auto item=args.SelectedItem().try_as<Mux::NavigationViewItem>();
            if(const auto owner=weak.get();owner&&item) {
                const auto tag=unbox_value_or<hstring>(item.Tag(),{});
                if(tag!=L"activity"&&tag!=L"sftp")return;
                const bool sftp=tag==L"sftp";
                owner.FindName(L"SftpWorkspace").as<UIElement>().Visibility(sftp?Visibility::Visible:Visibility::Collapsed);
                owner.FindName(L"TransferActivity").as<UIElement>().Visibility(sftp?Visibility::Collapsed:Visibility::Visible);
                owner.FindName(L"BackendNotice").as<UIElement>().Visibility(sftp?Visibility::Visible:Visibility::Collapsed);
                owner.FindName(L"Heading").as<TextBlock>().Text(sftp?L"SFTP":L"Transfers");
                if(sender.IsLoaded()&&sender.DisplayMode()==Mux::NavigationViewDisplayMode::Minimal)sender.IsPaneOpen(false);
            }
        });
        const auto settingsLabel=root.FindName(L"BackendSettingsLabel").as<TextBlock>();
        const auto groupHeader=root.FindName(L"BackendGroupHeader").as<Mux::NavigationViewItemHeader>();
        navigation.RegisterPropertyChangedCallback(Mux::NavigationView::IsPaneOpenProperty(),[weak=make_weak(root)](const DependencyObject& sender,const DependencyProperty&) {
            if(const auto owner=weak.get()) {
                const auto visibility=sender.as<Mux::NavigationView>().IsPaneOpen()?Visibility::Visible:Visibility::Collapsed;
                owner.FindName(L"BackendSettingsLabel").as<TextBlock>().Visibility(visibility);
                const auto button=owner.FindName(L"BackendSettings").as<Button>();
                const bool expanded=visibility==Visibility::Visible;
                button.HorizontalContentAlignment(expanded?HorizontalAlignment::Left:HorizontalAlignment::Center);
                owner.FindName(L"BackendSettingsContent").as<StackPanel>().HorizontalAlignment(expanded?HorizontalAlignment::Left:HorizontalAlignment::Center);
                owner.FindName(L"BackendSettingsContent").as<StackPanel>().Spacing(expanded?14:0);
                button.Padding(expanded?Thickness{14,8,10,8}:Thickness{0,8,0,8});
                button.MinWidth(expanded?0:sender.as<Mux::NavigationView>().CompactPaneLength()-8);
                owner.FindName(L"BackendGroupHeader").as<Mux::NavigationViewItemHeader>().Visibility(visibility);
            }
        });
        settingsLabel.Visibility(navigation.IsPaneOpen()?Visibility::Visible:Visibility::Collapsed);
        const auto settingsButton=root.FindName(L"BackendSettings").as<Button>();
        settingsButton.HorizontalContentAlignment(navigation.IsPaneOpen()?HorizontalAlignment::Left:HorizontalAlignment::Center);
        root.FindName(L"BackendSettingsContent").as<StackPanel>().HorizontalAlignment(navigation.IsPaneOpen()?HorizontalAlignment::Left:HorizontalAlignment::Center);
        root.FindName(L"BackendSettingsContent").as<StackPanel>().Spacing(navigation.IsPaneOpen()?14:0);
        settingsButton.Padding(navigation.IsPaneOpen()?Thickness{14,8,10,8}:Thickness{0,8,0,8});
        settingsButton.MinWidth(navigation.IsPaneOpen()?0:navigation.CompactPaneLength()-8);
        groupHeader.Visibility(navigation.IsPaneOpen()?Visibility::Visible:Visibility::Collapsed);
        root.FindName(L"BackendSettings").as<Button>().Click([weak=make_weak(navigation),openSettings=std::move(openSettings)](auto&&,auto&&) {
            if(const auto sender=weak.get();sender&&sender.DisplayMode()==Mux::NavigationViewDisplayMode::Minimal)sender.IsPaneOpen(false);
            if(openSettings)openSettings();
        });
        navigation.SelectedItem(root.FindName(L"ActivityNavItem"));
        return navigation;
    }
}
