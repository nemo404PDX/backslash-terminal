// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
// Shared checks of actual applied native templates, not just XAML declarations.
static X::Controls::SplitView FindNavigationSplit(const X::DependencyObject& owner)
{
    if(const auto element=owner.try_as<X::FrameworkElement>();element&&element.Name()==L"RootSplitView")return element.as<X::Controls::SplitView>();
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(owner);++i)
        if(const auto split=FindNavigationSplit(X::Media::VisualTreeHelper::GetChild(owner,i)))return split;
    return nullptr;
}
static void CheckNavigationPane(const Mux::NavigationView& navigation,bool dark)
{
    const Mux::NavigationView defaults;
    if(navigation.MenuItems().Size())if(const auto first=navigation.MenuItems().GetAt(0).try_as<X::FrameworkElement>())
        std::wcout<<L"NAVIGATION_FIRST_ITEM top="<<first.TransformToVisual(navigation).TransformPoint({0,0}).Y<<L" height="<<first.ActualHeight()<<std::endl;
    Require(navigation.OpenPaneLength()==defaults.OpenPaneLength()&&navigation.CompactPaneLength()==defaults.CompactPaneLength(),L"Navigation retains shared Settings default logical expanded and collapsed widths");
    Require(navigation.CompactModeThresholdWidth()==defaults.CompactModeThresholdWidth()&&navigation.ExpandedModeThresholdWidth()==defaults.ExpandedModeThresholdWidth(),L"Navigation uses matching native adaptive thresholds");
    const auto split=FindNavigationSplit(navigation);
    Require(static_cast<bool>(split),L"Native navigation SplitView template applied");
    Require(split.CompactPaneLength()==defaults.CompactPaneLength()&&split.OpenPaneLength()>0&&split.OpenPaneLength()<=defaults.OpenPaneLength()+0.5,L"Applied pane widths preserve native DIP sizing and narrow-window clamping");
    const auto brush=split.PaneBackground().try_as<X::Media::SolidColorBrush>();
    Require(static_cast<bool>(brush),L"Both overlay and inline navigation panes use a solid semantic background");
    const auto color=brush.Color();
    const auto content=split.Content().as<X::Controls::Grid>();
    const auto corner=content.CornerRadius();
    const auto surface=content.Background().try_as<X::Media::SolidColorBrush>();
    Require(static_cast<bool>(surface),L"Navigation content has its own surface brush");
    std::wcout<<L"NAVIGATION_CORNER topLeft="<<corner.TopLeft<<L" topRight="<<corner.TopRight<<L" bottomLeft="<<corner.BottomLeft<<L" bottomRight="<<corner.BottomRight<<L" contentAlpha="<<unsigned(surface.Color().A)<<std::endl;
    if(navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal)Require(corner.TopLeft>0,L"Native top-left rounding belongs to the content surface");
    if(dark&&surface.Color().A==255)Require(surface.Color().R>color.R,L"Dark navigation remains behind the lighter rounded content surface");
    std::wcout<<L"NAVIGATION_PANE open="<<navigation.IsPaneOpen()<<L" logical="<<navigation.OpenPaneLength()<<L" compact="<<navigation.CompactPaneLength()<<L" effective="<<split.OpenPaneLength()<<L" scale="<<navigation.XamlRoot().RasterizationScale()<<L" dark="<<dark<<L" color="<<unsigned(color.A)<<L","<<unsigned(color.R)<<L","<<unsigned(color.G)<<L","<<unsigned(color.B)<<std::endl;
    Require(color.A==255,L"Expanded pane cannot blend into the lighter content background");
    Require(dark?(color.R<40&&color.G<40&&color.B<40):(color.R>180&&color.G>180&&color.B>180),L"Pane background follows the requested theme semantic base color");
}

inline X::Controls::TextBlock FindNavigationTitle(const X::DependencyObject& parent,const winrt::hstring& caption)
{
    if(const auto label=parent.try_as<X::Controls::TextBlock>();label&&label.Text()==caption)return label;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)
        if(const auto found=FindNavigationTitle(X::Media::VisualTreeHelper::GetChild(parent,i),caption))return found;
    return nullptr;
}
