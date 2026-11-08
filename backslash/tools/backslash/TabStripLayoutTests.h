// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
static X::FrameworkElement FindTabStripElement(const X::DependencyObject& owner,const winrt::hstring& name)
{
    if(const auto element=owner.try_as<X::FrameworkElement>();element&&element.Name()==name)return element;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(owner);++i)
        if(const auto found=FindTabStripElement(X::Media::VisualTreeHelper::GetChild(owner,i),name))return found;
    return nullptr;
}
static unsigned TestTabStripLayout(const std::filesystem::path& work)
{
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned tab-strip comparison",WS_OVERLAPPEDWINDOW,-32000,-32000,1800,300,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Require(hwnd!=nullptr,L"Owned tab-strip host created");
    X::Hosting::DesktopWindowXamlSource island;const auto native=island.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(native->AttachToWindow(hwnd));
    X::Controls::Grid root;root.RequestedTheme(X::ElementTheme::Dark);root.Background(X::Media::SolidColorBrush{winrt::Windows::UI::Color{255,32,32,32}});
    const auto row=winrt::get_activation_factory<winrt::TerminalApp::TabRowControl>().ActivateInstance<winrt::TerminalApp::TabRowControl>();
    const auto titlebar=winrt::get_activation_factory<winrt::TerminalApp::TitlebarControl,winrt::TerminalApp::ITitlebarControlFactory>().CreateInstance(reinterpret_cast<uint64_t>(hwnd));titlebar.Background(root.Background());titlebar.Content(row);root.Children().Append(titlebar);island.Content(root);
    HWND child{};winrt::check_hresult(native->get_WindowHandle(&child));
    SetWindowPos(child,nullptr,0,0,1600,240,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);Pump(250);
    const auto pins=row.FindName(L"PinsButton").as<X::Controls::Button>();
    const auto content=titlebar.FindName(L"ContentRoot").as<X::FrameworkElement>();
    const auto caption=titlebar.FindName(L"MinMaxCloseControl").as<X::FrameworkElement>();
    const auto external=row.FindName(L"TabActions").try_as<X::FrameworkElement>();
    const auto footer=external?external:row.TabView().TabStripFooter().as<X::FrameworkElement>();
    const auto right=external?footer:FindTabStripElement(row,L"RightContentPresenter");Require(static_cast<bool>(right),L"Actual action presenter found");
    // Supply the same localized captions when this isolated host has no app resource context.
    for(const auto name:{L"GroupsButton",L"PinsButton"}){
        const auto button=row.FindName(name).as<X::Controls::Button>();
        const auto label=button.Content().as<X::Controls::StackPanel>().Children().GetAt(1).as<X::Controls::TextBlock>();
        std::wcout<<L"TABSTRIP_CAPTION "<<name<<L" actual="<<label.Text().c_str()<<std::endl;
        if(label.Text().empty())label.Text(name==std::wstring_view(L"PinsButton")?L"Pins":L"Groups");
    }
    const auto rightEdge=[&](const X::FrameworkElement& element){const auto point=element.TransformToVisual(titlebar).TransformPoint({0,0});return double(point.X)+element.ActualWidth();};
    std::ofstream log(work/L"tabstrip.tsv");log<<"tabs\tmode\tmaximized\tphysical\tscale\troot\tcontent\tfooterPresenter\tfooter\tpinsRight\tcaptionLeft\tcontentMax\tclip\n";
    unsigned clips=0,cases=0;
    // IslandWindow's supported minimum is 460 DIP; convert through the actual island scale.
    const int minimumPhysical=static_cast<int>(std::ceil(460.0*root.XamlRoot().RasterizationScale()));
    const int maximumPhysical=static_cast<int>(std::floor(1200.0*root.XamlRoot().RasterizationScale()));
    for(const int count:{1,4,9})for(const auto mode:{Mux::TabViewWidthMode::Equal,Mux::TabViewWidthMode::SizeToContent,Mux::TabViewWidthMode::Compact})for(const bool maximized:{false,true}){
        row.TabView().TabItems().Clear();row.TabView().TabWidthMode(mode);
        for(int i=0;i<count;++i){Mux::TabViewItem item;item.Header(winrt::box_value(i%3==0?L"PowerShell":i%3==1?L"Settings":L"Transfers"));row.TabView().TabItems().Append(item);}
        row.TabView().SelectedIndex(count-1);
        titlebar.SetWindowVisualState(maximized?winrt::TerminalApp::WindowVisualState::WindowVisualStateMaximized:winrt::TerminalApp::WindowVisualState::WindowVisualStateNormal);
        for(int width=minimumPhysical;width<=maximumPhysical;width+=7){
            SetWindowPos(hwnd,nullptr,-32000,-32000,width+64,300,SWP_NOACTIVATE|SWP_NOZORDER);
            SetWindowPos(child,nullptr,0,0,width,240,SWP_NOACTIVATE|SWP_NOZORDER);root.UpdateLayout();
            double previous=-1;
            for(unsigned attempt=0;attempt<12;++attempt){
                Pump(20);root.UpdateLayout();const auto edge=rightEdge(pins);
                const bool settled=std::abs(root.ActualWidth()*root.XamlRoot().RasterizationScale()-width)<1&&std::abs(edge-previous)<0.05;
                previous=edge;if(settled)break;
            }
            Require(std::abs(root.ActualWidth()*root.XamlRoot().RasterizationScale()-width)<1,L"Comparison window settled at requested physical width");
            if(external&&width==minimumPhysical){
                const auto label=row.FindName(L"PinsButtonLabel").as<X::Controls::TextBlock>();
                const auto viewport=row.FindName(L"TabsViewport").as<X::FrameworkElement>();
                std::wcout<<L"COMPACT_ACTIONS labels="<<static_cast<int>(label.Visibility())<<L" tabsWidth="<<viewport.ActualWidth()<<std::endl;
                Require(label.Visibility()==X::Visibility::Collapsed&&viewport.ActualWidth()>50,L"Minimum titlebar compacts labels and retains usable tab space");
            }
            const auto pinRight=rightEdge(pins);const auto visible=std::min(rightEdge(content),std::min(rightEdge(right),rightEdge(footer)));
            const auto captionLeft=caption.TransformToVisual(titlebar).TransformPoint({0,0}).X;
            const bool clipped=pinRight>visible+0.1||pinRight>captionLeft+0.1||rightEdge(caption)>root.ActualWidth()+0.1;++cases;if(clipped)++clips;
            log<<count<<'\t'<<static_cast<int>(mode)<<'\t'<<maximized<<'\t'<<width<<'\t'<<root.XamlRoot().RasterizationScale()<<'\t'<<root.ActualWidth()<<'\t'<<rightEdge(content)<<'\t'<<rightEdge(right)<<'\t'<<rightEdge(footer)<<'\t'<<pinRight<<'\t'<<captionLeft<<'\t'<<content.MaxWidth()<<'\t'<<clipped<<'\n';
            if((count==4&&mode==Mux::TabViewWidthMode::Equal&&!maximized&&((width-minimumPhysical)%350==0))||(clipped&&clips==1)){
                X::VisualStateManager::GoToState(pins,L"PointerOver",false);Pump(20);
                CaptureSettings(root,work/(L"tabstrip-"+std::to_wstring(width)+(clipped?L"-clipped.png":L".png")));
                X::VisualStateManager::GoToState(pins,L"Normal",false);
            }
        }
    }
    log.close();
    if(external)Require(!X::Automation::AutomationProperties::GetName(pins).empty(),L"Compacted Pins retains its localized accessible name");
    X::Controls::MenuFlyout flyout;X::Controls::MenuFlyoutItem item;item.Text(L"Owned fixture pin");flyout.Items().Append(item);bool opened=false;flyout.Opened([&](auto&&,auto&&){opened=true;});pins.Flyout(flyout);
    X::Automation::Peers::ButtonAutomationPeer peer{pins};peer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);Require(opened,L"Pins dropdown still opens its owned fixture flyout");flyout.Hide();Pump(50);
    std::wcout<<L"TABSTRIP_OBSERVATION cases="<<cases<<L" clipped="<<clips<<L" scale="<<root.XamlRoot().RasterizationScale()<<L" flyoutOpened="<<opened<<std::endl;
    island.Content(nullptr);island.Close();DestroyWindow(hwnd);return clips;
}
