// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "TabRowControl.h"
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <algorithm>
#include <cmath>
#include <limits>

#include "TabRowControl.g.cpp"

using namespace winrt::Windows::ApplicationModel::DataTransfer;

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::UI::Text;

namespace winrt
{
    namespace MUX = Microsoft::UI::Xaml;
    namespace WUX = Windows::UI::Xaml;
}

namespace winrt::TerminalApp::implementation
{
    TabRowControl::TabRowControl()
    {
        InitializeComponent();
        LayoutUpdated([weak=get_weak()](auto&&,auto&&){if(const auto self=weak.get())self->_UpdateActionsLayout();});
    }

    void TabRowControl::OnTabsViewportSizeChanged(const IInspectable&, const WUX::SizeChangedEventArgs& args)
    {
        WUX::Media::RectangleGeometry clip;clip.Rect({0,0,args.NewSize().Width,args.NewSize().Height});TabsViewport().Clip(clip);
    }

    void TabRowControl::_UpdateActionsLayout()
    {
        if(_measuringActions)return;
        auto parent=WUX::Media::VisualTreeHelper::GetParent(*this).try_as<WUX::FrameworkElement>();if(!parent)return;
        auto available=parent.ActualWidth();
        // TitlebarControl constrains its content with MaxWidth. Read that
        // allocation, rather than the toolbar's potentially clipped desired size.
        for(auto owner=parent;owner;owner=WUX::Media::VisualTreeHelper::GetParent(owner).try_as<WUX::FrameworkElement>())
            if(std::isfinite(owner.MaxWidth())){available=owner.MaxWidth();break;}
        if(available<=0)return;
        const auto scale=_uiSettings.TextScaleFactor();const auto raster=XamlRoot()?XamlRoot().RasterizationScale():0;
        const auto groupsText=GroupsButtonLabel().Text(),pinsText=PinsButtonLabel().Text();
        const auto groupsFont=GroupsButtonLabel().FontSize(),pinsFont=PinsButtonLabel().FontSize();
        if(available==_actionsBudget&&scale==_actionsScale&&raster==_actionsRasterScale&&groupsText==_groupsText&&pinsText==_pinsText&&groupsFont==_groupsFontSize&&pinsFont==_pinsFontSize)return;
        _actionsBudget=available;_actionsScale=scale;_actionsRasterScale=raster;_groupsText=groupsText;_pinsText=pinsText;_groupsFontSize=groupsFont;_pinsFontSize=pinsFont;
        if(!groupsText.empty()){
            if(WUX::Automation::AutomationProperties::GetName(GroupsButton()).empty())WUX::Automation::AutomationProperties::SetName(GroupsButton(),groupsText);
            if(!WUX::Controls::ToolTipService::GetToolTip(GroupsButton()))WUX::Controls::ToolTipService::SetToolTip(GroupsButton(),box_value(groupsText));
        }
        if(!pinsText.empty()){
            if(WUX::Automation::AutomationProperties::GetName(PinsButton()).empty())WUX::Automation::AutomationProperties::SetName(PinsButton(),pinsText);
            if(!WUX::Controls::ToolTipService::GetToolTip(PinsButton()))WUX::Controls::ToolTipService::SetToolTip(PinsButton(),box_value(pinsText));
        }
        _measuringActions=true;const auto reset=wil::scope_exit([&]{_measuringActions=false;});
        GroupsButtonLabel().Visibility(WUX::Visibility::Visible);PinsButtonLabel().Visibility(WUX::Visibility::Visible);
        const Windows::Foundation::Size unbounded{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        TabActions().Measure(unbounded);
        // Leave the tab strip the measured footprint of one new-tab control.
        const bool compact=TabActions().DesiredSize().Width+NewTabButton().DesiredSize().Width>available;
        GroupsButtonLabel().Visibility(compact?WUX::Visibility::Collapsed:WUX::Visibility::Visible);
        PinsButtonLabel().Visibility(compact?WUX::Visibility::Collapsed:WUX::Visibility::Visible);
        TabActions().Measure(unbounded);
        TabsViewport().MaxWidth(std::max(0.0,available-TabActions().DesiredSize().Width));
    }

    // Method Description:
    // - Bound in the Xaml editor to the [+] button.
    // Arguments:
    // <unused>
    void TabRowControl::OnNewTabButtonClick(const IInspectable&, const Controls::SplitButtonClickEventArgs&)
    {
    }

    // Method Description:
    // - Bound in Drag&Drop of the Xaml editor to the [+] button.
    // Arguments:
    // <unused>
    void TabRowControl::OnNewTabButtonDrop(const IInspectable&, const winrt::Windows::UI::Xaml::DragEventArgs&)
    {
    }

    // Method Description:
    // - Bound in Drag-over of the Xaml editor to the [+] button.
    // Allows drop of 'StorageItems' which will be used as StartingDirectory
    // Arguments:
    //  - <unused>
    //  - e: DragEventArgs which hold the items
    void TabRowControl::OnNewTabButtonDragOver(const IInspectable&, const winrt::Windows::UI::Xaml::DragEventArgs& e)
    {
        // We can only handle drag/dropping StorageItems (files).
        // If the format on the clipboard is anything else, returning
        // early here will prevent the drag/drop from doing anything.
        if (!e.DataView().Contains(StandardDataFormats::StorageItems()))
        {
            return;
        }

        // Make sure to set the AcceptedOperation, so that we can later receive the path in the Drop event
        e.AcceptedOperation(DataPackageOperation::Copy);

        const auto modifiers = static_cast<uint32_t>(e.Modifiers());
        if (WI_IsFlagSet(modifiers, static_cast<uint32_t>(DragDrop::DragDropModifiers::Alt)))
        {
            e.DragUIOverride().Caption(RS_(L"DropPathTabSplit/Text"));
        }
        else if (WI_IsFlagSet(modifiers, static_cast<uint32_t>(DragDrop::DragDropModifiers::Shift)))
        {
            e.DragUIOverride().Caption(RS_(L"DropPathTabNewWindow/Text"));
        }
        else
        {
            e.DragUIOverride().Caption(RS_(L"DropPathTabRun/Text"));
        }

        // Sets if the caption is visible
        e.DragUIOverride().IsCaptionVisible(true);
        // Sets if the dragged content is visible
        e.DragUIOverride().IsContentVisible(false);
        // Sets if the glyph is visible
        e.DragUIOverride().IsGlyphVisible(false);
    }
}
