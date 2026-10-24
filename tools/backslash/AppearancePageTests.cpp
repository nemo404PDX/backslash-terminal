#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <appmodel.h>
#undef GetCurrentTime
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Data.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Automation.Peers.h>
#include <winrt/Windows.UI.Xaml.Automation.Provider.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Microsoft.Terminal.Settings.Editor.h>
#include <winrt/Microsoft.Terminal.Settings.Model.h>
#include <winrt/Microsoft.Terminal.Control.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/TerminalApp.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <array>
#include <cmath>
#include <wincodec.h>
#include <wrl/client.h>
namespace X=winrt::Windows::UI::Xaml;
namespace E=winrt::Microsoft::Terminal::Settings::Editor;
namespace Mux=winrt::Microsoft::UI::Xaml::Controls;
static void CollectNavigation(winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable> const& source,std::vector<Mux::NavigationViewItem>& result)
{
    for(auto const& item:source)if(auto nav=item.try_as<Mux::NavigationViewItem>())
    {
        result.push_back(nav);
        if(auto children=nav.MenuItemsSource().try_as<winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>>())CollectNavigation(children,result);
        else CollectNavigation(nav.MenuItems(),result);
    }
}
static std::vector<Mux::NavigationViewItem> NavigationItems(Mux::NavigationView const& nav)
{
    std::vector<Mux::NavigationViewItem> result;
    CollectNavigation(nav.MenuItemsSource().as<winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>>(),result);
    return result;
}
static void Pump(unsigned milliseconds)
{
    const auto end=GetTickCount64()+milliseconds;
    do{MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);}while(GetTickCount64()<end);
}
template<typename T> static auto Wait(T const& operation)
{
    const auto end=GetTickCount64()+15000;
    while(operation.Status()==winrt::Windows::Foundation::AsyncStatus::Started){Pump(10);if(GetTickCount64()>end)throw winrt::hresult_error(HRESULT_FROM_WIN32(WAIT_TIMEOUT));}
    return operation.GetResults();
}
static void Require(bool condition,wchar_t const* message)
{
    if(!condition){std::wcerr<<L"FAIL: "<<message<<std::endl;ExitProcess(4);}
}
static X::FrameworkElement FindElement(X::DependencyObject const& parent,std::wstring_view name)
{
    if(auto element=parent.try_as<X::FrameworkElement>();element&&element.Name()==name)return element;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto found=FindElement(X::Media::VisualTreeHelper::GetChild(parent,i),name))return found;
    return nullptr;
}
static X::Controls::TextBlock FindLabel(X::DependencyObject const& parent,winrt::hstring const& label)
{
    if(auto text=parent.try_as<X::Controls::TextBlock>();text&&text.Text()==label)return text;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto found=FindLabel(X::Media::VisualTreeHelper::GetChild(parent,i),label))return found;
    return nullptr;
}
static Mux::BreadcrumbBarItem FindInvokableBreadcrumb(X::DependencyObject const& parent)
{
    if(auto item=parent.try_as<Mux::BreadcrumbBarItem>();item&&item.Visibility()==X::Visibility::Visible&&item.IsHitTestVisible()&&item.Content().try_as<E::Breadcrumb>())return item;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto found=FindInvokableBreadcrumb(X::Media::VisualTreeHelper::GetChild(parent,i)))return found;
    return nullptr;
}
static void SavePng(std::filesystem::path const& path,X::UIElement const& element)
{
    X::Media::Imaging::RenderTargetBitmap bitmap;Wait(bitmap.RenderAsync(element));
    auto buffer=Wait(bitmap.GetPixelsAsync());std::vector<uint8_t> pixels(buffer.Length());
    winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer).ReadBytes(pixels);
    Require(bitmap.PixelWidth()>0&&bitmap.PixelHeight()>0,L"Native render has dimensions");
    size_t visible=0;for(size_t i=3;i<pixels.size();i+=4)if(pixels[i])++visible;
    Require(visible>static_cast<size_t>(bitmap.PixelWidth()*bitmap.PixelHeight())/2,L"Native render has painted pixels");
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;winrt::check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)));
    ComPtr<IWICStream> stream;winrt::check_hresult(factory->CreateStream(&stream));winrt::check_hresult(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE));
    ComPtr<IWICBitmapEncoder> encoder;winrt::check_hresult(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder));winrt::check_hresult(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache));
    ComPtr<IWICBitmapFrameEncode> frame;winrt::check_hresult(encoder->CreateNewFrame(&frame,nullptr));winrt::check_hresult(frame->Initialize(nullptr));winrt::check_hresult(frame->SetSize(bitmap.PixelWidth(),bitmap.PixelHeight()));
    auto format=GUID_WICPixelFormat32bppBGRA;winrt::check_hresult(frame->SetPixelFormat(&format));
    winrt::check_hresult(frame->WritePixels(bitmap.PixelHeight(),bitmap.PixelWidth()*4,static_cast<UINT>(pixels.size()),pixels.data()));winrt::check_hresult(frame->Commit());winrt::check_hresult(encoder->Commit());
    std::wcout<<L"RENDER "<<path.c_str()<<L" "<<bitmap.PixelWidth()<<L"x"<<bitmap.PixelHeight()<<std::endl;
}
struct ProbeArgs : winrt::implements<ProbeArgs,E::INavigateToPageArgs>
{
    winrt::Windows::Foundation::IInspectable vm;
    E::IHostedInWindow root;
    ProbeArgs(winrt::Windows::Foundation::IInspectable const& value,E::IHostedInWindow const& owner):vm(value),root(owner){}
    winrt::Windows::Foundation::IInspectable ViewModel(){return vm;}
    E::IHostedInWindow WindowRoot(){return root;}
    winrt::hstring ElementToFocus(){return {};}
};
struct ProbeApp : X::ApplicationT<ProbeApp,X::Markup::IXamlMetadataProvider>
{
    std::vector<X::Markup::IXamlMetadataProvider> providers;
    X::Hosting::WindowsXamlManager manager{nullptr};
    ProbeApp()
    {
        std::wcout<<L"app: providers"<<std::endl;
        providers.push_back(winrt::get_activation_factory<winrt::TerminalApp::XamlMetaDataProvider>().ActivateInstance<winrt::TerminalApp::XamlMetaDataProvider>());
        providers.push_back(E::XamlMetaDataProvider{});
        providers.push_back(winrt::Microsoft::Terminal::Control::XamlMetaDataProvider{});
        providers.push_back(winrt::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider{});
        UnhandledException([](auto const&,auto const& args){std::wcerr<<L"app-init XAML "<<args.Message().c_str()<<std::endl;args.Handled(true);});
        std::wcout<<L"app: manager"<<std::endl;
        manager=X::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        std::wcout<<L"app: resources"<<std::endl;
        Resources().MergedDictionaries().Append(winrt::Microsoft::UI::Xaml::Controls::XamlControlsResources{});
        std::wcout<<L"app: ready"<<std::endl;
    }
    X::Markup::IXamlType GetXamlType(X::Interop::TypeName const& type){return GetXamlType(type.Name);}
    X::Markup::IXamlType GetXamlType(winrt::hstring const& type){if(type.starts_with(L"Windows."))return nullptr;for(auto const& p:providers){if(auto t=p.GetXamlType(type))return t;}return nullptr;}
    winrt::com_array<X::Markup::XmlnsDefinition> GetXmlnsDefinitions(){return {};}
};
int wmain(int argc,wchar_t**argv)
{
    const wchar_t* stage=L"init";
    try
    {
        std::wcout<<L"probe start"<<std::endl;winrt::init_apartment(winrt::apartment_type::single_threaded);
        // Fail closed before constructing any Settings UI (and its UI-state
        // singleton): require an unpackaged process and the copied model DLL,
        // marker and executable in the disposable evidence/portable directory.
        Require(argc>2,L"Disposable evidence directory is required");
        const auto fixture=std::filesystem::absolute(std::filesystem::path{argv[2]}/L"portable").lexically_normal();
        UINT32 packageLength=0;
        Require(GetCurrentPackageFullName(&packageLength,nullptr)==APPMODEL_ERROR_NO_PACKAGE,L"Fixture must be unpackaged");
        wchar_t exePath[32768]{};Require(GetModuleFileNameW(nullptr,exePath,32768)>0,L"Read fixture executable path");
        Require(std::filesystem::path{exePath}.parent_path()==fixture,L"Executable is inside disposable fixture");
        Require((GetFileAttributesW(fixture.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)==0,L"Fixture is not a redirected directory");
        Require(std::filesystem::is_regular_file(fixture/L".portable"),L"Fixture has portable state marker");
        auto model=LoadLibraryExW((fixture/L"Microsoft.Terminal.Settings.Model.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        Require(model!=nullptr,L"Load fixture model DLL");
        wchar_t modelPath[32768]{};Require(GetModuleFileNameW(model,modelPath,32768)>0,L"Read model DLL path");
        Require(std::filesystem::path{modelPath}.parent_path()==fixture,L"Model DLL is inside disposable fixture");
        using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
        Require(Settings::IsPortableMode(),L"Model confirms portable mode before UI state access");
        Require(std::filesystem::path{Settings::SettingsDirectory().c_str()}==fixture/L"settings",L"Model state directory is the disposable fixture/settings");
        std::wcout<<L"ISOLATION: all ApplicationState files rooted at "<<(fixture/L"settings")<<std::endl;
        stage=L"app";auto app=winrt::make<ProbeApp>();int failures=0;
        app.UnhandledException([&](auto const&,auto const& args){++failures;std::wcerr<<L"XAML UNHANDLED stage="<<stage<<L" "<<args.Message().c_str()<<std::endl;args.Handled(true);});
        stage=L"construct page";X::FrameworkElement page{nullptr};
        const std::wstring_view kind=argc>1?argv[1]:L"global";
        auto hwnd=CreateWindowExW(0,L"STATIC",L"Owned offscreen Appearance regression",WS_OVERLAPPEDWINDOW,0,0,1024,768,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        const auto settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings::LoadDefaults();
        std::wcout<<L"settings: synthetic defaults"<<std::endl;
        E::MainPage owner{settings};owner.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
        std::wcout<<L"settings: owner initialized"<<std::endl;
        if(kind==L"sidebar")
        {
            auto group=owner.FindName(L"ProfilesGroupNavItem").as<Mux::NavigationViewItem>();
            std::wcout<<L"Profiles before layout expanded="<<group.IsExpanded()<<L" selectable="<<group.SelectsOnInvoked()<<std::endl;
        }
        X::Controls::Frame frame;
        E::ProfileViewModel testedProfile{nullptr};
        if(kind==L"sidebar")page=owner;
        else if(kind==L"global")
        {
            auto vm=E::GlobalAppearanceViewModel{settings.GlobalSettings()};
            frame.Navigate(winrt::xaml_typename<E::GlobalAppearance>(),winrt::make<ProbeArgs>(vm,owner));
        }
        else
        {
            auto nav=owner.FindName(L"SettingsNav").as<winrt::Microsoft::UI::Xaml::Controls::NavigationView>();
            E::ProfileViewModel vm{nullptr};
            for(auto const& n:NavigationItems(nav))
            {
                if(auto candidate=n.Tag().try_as<E::ProfileViewModel>()){vm=candidate;break;}
            }
            if(!vm)throw winrt::hresult_error(E_FAIL,L"No fixture profile view model");
            testedProfile=vm;
            frame.Navigate(winrt::xaml_typename<E::Profiles_Appearance>(),winrt::make<ProbeArgs>(vm,owner));
        }
        if(kind!=L"sidebar")page=frame;
        stage=L"attach page";
        X::Hosting::DesktopWindowXamlSource island;auto native=island.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(native->AttachToWindow(hwnd));island.Content(page);
        HWND initialChild{};winrt::check_hresult(native->get_WindowHandle(&initialChild));
        const auto scale=static_cast<double>(GetDpiForWindow(hwnd))/96.0;
        SetWindowLongPtrW(hwnd,GWL_EXSTYLE,WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE);
        SetWindowPos(hwnd,nullptr,-32000,-32000,static_cast<int>(1024*scale)+40,static_cast<int>(768*scale)+80,SWP_NOACTIVATE|SWP_NOZORDER);
        SetWindowPos(initialChild,nullptr,0,0,static_cast<int>(1024*scale),static_cast<int>(768*scale),SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        stage=L"layout page";page.Measure({1024,768});page.Arrange({0,0,1024,768});page.UpdateLayout();
        const auto end=GetTickCount64()+500;while(GetTickCount64()<end){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);}
        if(kind==L"sidebar")
        {
            stage=L"native Profiles collapse and navigation";
            auto nav=owner.FindName(L"SettingsNav").as<Mux::NavigationView>();
            auto group=owner.FindName(L"ProfilesGroupNavItem").as<Mux::NavigationViewItem>();
            auto content=owner.FindName(L"contentFrame").as<X::Controls::Frame>();
            std::wcout<<L"Profiles after layout expanded="<<group.IsExpanded()<<L" selectable="<<group.SelectsOnInvoked()<<L" navwidth="<<nav.ActualWidth()<<std::endl;
            Require(group.IsExpanded()&&!group.SelectsOnInvoked(),L"Profiles starts expanded and does not navigate");
            const std::array<const wchar_t*,4> utilityNames{L"ProfilesGroupNavItem",L"ExportSetupNavItem",L"ImportSetupNavItem",L"RecoverSetupNavItem"};
            const auto checkUtilityItems=[&](bool compact){
                for(const auto name:utilityNames)
                {
                    auto item=owner.FindName(name).as<Mux::NavigationViewItem>();
                    const auto label=winrt::unbox_value<winrt::hstring>(item.Content());
                    Require(!label.empty()&&static_cast<bool>(item.Icon().try_as<X::Controls::FontIcon>()),L"Utility navigation has full label and native glyph");
                    Require(winrt::unbox_value<winrt::hstring>(X::Controls::ToolTipService::GetToolTip(item))==label,L"Utility tooltip retains localized label");
                    Require(X::Automation::AutomationProperties::GetName(item)==label,L"Utility accessible name retains localized label");
                    if(compact)
                    {
                        auto icon=item.Icon().as<X::Controls::FontIcon>();const auto origin=icon.TransformToVisual(nav).TransformPoint({0,0});
                        Require(origin.X>=0&&origin.X+icon.ActualWidth()<=nav.CompactPaneLength(),L"Native icon fits compact pane width");
                        std::wcout<<L"ICON "<<name<<L" x="<<origin.X<<L" width="<<icon.ActualWidth()<<L" pane="<<nav.CompactPaneLength()<<std::endl;
                    }
                    if(auto text=FindLabel(item,label))
                    {
                        bool visible=true;
                        for(X::DependencyObject node=text;node&&node!=nav;node=X::Media::VisualTreeHelper::GetParent(node))
                        {
                            if(auto element=node.try_as<X::UIElement>())visible=visible&&element.Visibility()==X::Visibility::Visible&&element.Opacity()>0;
                        }
                        const auto origin=text.TransformToVisual(nav).TransformPoint({0,0});
                        if(compact)Require(!visible||text.ActualWidth()<1||origin.X>=nav.CompactPaneLength()-1,L"Compact rail has no visible label fragment");
                        else Require(visible&&text.ActualWidth()>0,L"Expanded pane shows full localized utility label");
                        std::wcout<<L"UTILITY "<<name<<L" compact="<<compact<<L" textX="<<origin.X<<L" visible="<<visible<<std::endl;
                    }
                    else Require(compact,L"Expanded utility label is rendered");
                }
            };
            checkUtilityItems(false);SavePng(std::filesystem::path{argv[2]}/L"settings-navigation-wide.png",nav);
            group.IsExpanded(false);nav.IsPaneOpen(false);Pump(150);owner.UpdateLayout();
            Require(!nav.IsPaneOpen(),L"Native Settings pane collapses");checkUtilityItems(true);
            SavePng(std::filesystem::path{argv[2]}/L"settings-navigation-compact.png",nav);
            nav.IsPaneOpen(true);Pump(150);owner.UpdateLayout();checkUtilityItems(false);
            group.IsExpanded(true);

            Mux::NavigationViewItem childItem{nullptr};E::ProfileViewModel childVM{nullptr};
            for(auto const& item:NavigationItems(nav))if(auto vm=item.Tag().try_as<E::ProfileViewModel>()){childItem=item;childVM=vm;break;}
            Require(static_cast<bool>(childItem),L"Nested profile is reachable");
            // NavigationView's SelectionItem pattern does not raise ItemInvoked.
            // Use the owned breadcrumb Invoke pattern to exercise MainPage's
            // real navigation dispatch and its recursive profile lookup.
            const auto navigate=[&](winrt::Windows::Foundation::IInspectable const& destination){
                Require(owner.Breadcrumbs().Size()>0,L"Initial Settings breadcrumb exists");
                auto crumb=owner.Breadcrumbs().GetAt(0).as<E::Breadcrumb>();crumb.Tag(destination);crumb.SubPage(E::BreadcrumbSubPage::None);
                owner.Breadcrumbs().Append(crumb);owner.UpdateLayout();Pump(100);
                auto button=FindInvokableBreadcrumb(owner.FindName(L"NavigationBreadcrumbBar").as<X::DependencyObject>());
                Require(static_cast<bool>(button),L"Owned breadcrumb navigation button materializes");
                winrt::Microsoft::UI::Xaml::Automation::Peers::BreadcrumbBarItemAutomationPeer peer{button};
                peer.as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);owner.UpdateLayout();
            };
            navigate(childVM);Pump(100);owner.UpdateLayout();
            Require(static_cast<bool>(content.Content().try_as<E::Profiles_Base>()),L"Invoking nested profile navigates its page");
            childVM.Name(L"Unsaved fixture profile draft");
            auto selected=nav.SelectedItem();auto before=content.Content();const auto crumbs=owner.Breadcrumbs().Size();
            winrt::Microsoft::UI::Xaml::Automation::Peers::NavigationViewItemAutomationPeer groupPeer{group};
            auto expansion=groupPeer.as<X::Automation::Provider::IExpandCollapseProvider>();
            Require(!groupPeer.GetName().empty(),L"Profiles heading has an accessible name");
            expansion.Collapse();Pump(100);owner.UpdateLayout();
            Require(!group.IsExpanded()&&expansion.ExpandCollapseState()==X::Automation::ExpandCollapseState::Collapsed,L"Native collapsed accessibility state");
            Require(nav.SelectedItem()==selected&&content.Content()==before&&owner.Breadcrumbs().Size()==crumbs,L"Collapse preserves selected profile and content");
            Require(childVM.Name()==L"Unsaved fixture profile draft",L"Collapse preserves the unsaved profile draft");
            expansion.Expand();Pump(100);Require(group.IsExpanded()&&expansion.ExpandCollapseState()==X::Automation::ExpandCollapseState::Expanded,L"Native expanded accessibility state");
            expansion.Collapse();
            navigate(owner.FindName(L"AppearanceNavItem").as<Mux::NavigationViewItem>().Tag());Pump(100);owner.UpdateLayout();
            Require(static_cast<bool>(content.Content().try_as<E::GlobalAppearance>()),L"Other settings navigation works while Profiles collapsed");
            Require(!group.IsExpanded(),L"Other page navigation retains collapsed state");
            expansion.Expand();navigate(childVM);Pump(100);Require(childVM.Name()==L"Unsaved fixture profile draft",L"Returning to profile retains draft");
            for(auto size:{winrt::Windows::Foundation::Size{640,360},winrt::Windows::Foundation::Size{1024,768}})
            {
                SetWindowPos(initialChild,nullptr,0,0,static_cast<int>(size.Width*scale),static_cast<int>(size.Height*scale),SWP_NOACTIVATE|SWP_NOZORDER);
                owner.Width(size.Width);owner.Height(size.Height);owner.Measure(size);owner.Arrange({0,0,size.Width,size.Height});owner.UpdateLayout();Pump(100);
                nav.IsPaneOpen(true);Pump(100);
                expansion.Collapse();Pump(50);Require(!group.IsExpanded()&&content.Content().try_as<E::Profiles_Base>(),L"Short/narrow collapse retains active profile");
                expansion.Expand();Pump(50);
            }
            Require(childItem.IsTabStop(),L"Profile remains a keyboard navigation target");
            for(const bool collapsed:{true,false})
            {
                if(collapsed)expansion.Collapse();else expansion.Expand();Pump(50);
                E::MainPage reopened{settings};reopened.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
                island.Content(reopened);reopened.Measure({1024,768});reopened.Arrange({0,0,1024,768});reopened.UpdateLayout();Pump(100);
                Require(reopened.FindName(L"ProfilesGroupNavItem").as<Mux::NavigationViewItem>().IsExpanded()!=collapsed,L"Reopened Settings restores group expansion choice");
                island.Content(owner);owner.UpdateLayout();Pump(100);
            }
            expansion.Collapse();Pump(50);
            auto roots=nav.MenuItemsSource().as<winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable>>();
            uint32_t groupIndex=0;Require(roots.IndexOf(group,groupIndex)&&groupIndex+1<roots.Size(),L"Profiles group has root sibling below it");
            const auto addItem=roots.GetAt(groupIndex+1).as<Mux::NavigationViewItem>();
            Require(winrt::unbox_value<winrt::hstring>(addItem.Tag())==L"AddProfile"&&static_cast<bool>(addItem.Icon()),L"Existing Add profile action and plus icon remain root siblings");
            navigate(addItem.Tag());
            auto addPage=content.Content().as<E::AddProfile>();
            auto addButton=FindElement(addPage,L"AddNewButton").as<X::Controls::Button>();
            X::Automation::Peers::ButtonAutomationPeer addPeer{addButton};
            addPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);owner.UpdateLayout();
            Require(group.IsExpanded()&&static_cast<bool>(content.Content().try_as<E::Profiles_Base>()),L"Add from collapsed group creates and selects a profile");
            Require(nav.SelectedItem()!=selected,L"New profile becomes selected");
            navigate(childVM);Require(childVM.Name()==L"Unsaved fixture profile draft",L"Adding a profile preserves another profile's unsaved draft");
            std::wcout<<L"PASS: native Profiles collapse/expand, selected content and draft retention, other-page navigation, accessible state, narrow/short layout"<<std::endl;
        }
#if defined(APPEARANCE_EDGE_VM_TESTS)
        if(testedProfile)
        {
            stage=L"mixed edge selections and reset";
            using Size=winrt::Microsoft::Terminal::Settings::Model::RetroBezelEdgeSize;
            auto vm=testedProfile.DefaultAppearance();
            auto choice=[](auto const& list,Size value){for(auto const& e:list){if(winrt::unbox_value<Size>(e.EnumValue())==value)return e;}throw winrt::hresult_error(E_FAIL,L"Missing edge dropdown choice");};
            size_t notifications=0;
            auto changed=vm.PropertyChanged([&](auto const&,auto const& args){if(args.PropertyName().starts_with(L"CurrentRetroBezel"))++notifications;});
            vm.CurrentRetroBezelTopSize(choice(vm.RetroBezelTopSizeList(),Size::Slim));
            vm.CurrentRetroBezelRightSize(choice(vm.RetroBezelRightSizeList(),Size::Wide));
            vm.CurrentRetroBezelBottomSize(choice(vm.RetroBezelBottomSizeList(),Size::Medium));
            vm.CurrentRetroBezelLeftSize(choice(vm.RetroBezelLeftSizeList(),Size::Wide));
            if(vm.RetroBezelTopSize()!=Size::Slim||vm.RetroBezelRightSize()!=Size::Wide||vm.RetroBezelBottomSize()!=Size::Medium||vm.RetroBezelLeftSize()!=Size::Wide)throw winrt::hresult_error(E_FAIL,L"Mixed edge selections lost");
            vm.ClearRetroBezelRightSize();
            if(vm.HasRetroBezelRightSize()||winrt::unbox_value<Size>(vm.CurrentRetroBezelRightSize().as<E::EnumEntry>().EnumValue())!=Size::Default||notifications<5)throw winrt::hresult_error(E_FAIL,L"Edge reset/inheritance notifications failed");
            vm.PropertyChanged(changed);page.UpdateLayout();
            std::wcout<<L"PASS: mixed edges, four choices, inherited reset, Current notifications="<<notifications<<std::endl;
            // Restore a useful mixed example for native rendering.
            vm.CurrentRetroBezelRightSize(choice(vm.RetroBezelRightSizeList(),Size::Wide));
        }
#endif
        if(testedProfile&&argc>2)
        {
            stage=L"responsive production thickness template";
            auto appearances=frame.Content().as<X::FrameworkElement>().FindName(L"DefaultAppearanceView").as<X::FrameworkElement>();
            auto contentTemplate=appearances.Resources().Lookup(winrt::box_value(L"BezelEdgeControlsTemplate")).as<X::DataTemplate>();
            X::Controls::ContentControl host;host.Content(testedProfile.DefaultAppearance());host.ContentTemplate(contentTemplate);
            host.HorizontalContentAlignment(X::HorizontalAlignment::Stretch);host.VerticalContentAlignment(X::VerticalAlignment::Top);
            host.VerticalAlignment(X::VerticalAlignment::Top);host.RequestedTheme(X::ElementTheme::Dark);
            X::Controls::Grid canvas;canvas.Background(X::Media::SolidColorBrush{winrt::Windows::UI::Color{255,32,32,32}});canvas.Children().Append(host);
            island.Content(canvas);HWND child{};winrt::check_hresult(native->get_WindowHandle(&child));
            SetWindowLongPtrW(hwnd,GWL_EXSTYLE,WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE);
            const std::array<int,3> widths{720,320,720};int resize=0;
            for(auto width:widths)
            {
                SetWindowPos(hwnd,nullptr,-32000,-32000,static_cast<int>(width*scale)+40,static_cast<int>(950*scale)+80,SWP_NOACTIVATE|SWP_NOZORDER);
                SetWindowPos(child,nullptr,0,0,static_cast<int>(width*scale),static_cast<int>(950*scale),SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);
                canvas.Width(width);canvas.Height(950);canvas.Measure({static_cast<float>(width),950});canvas.Arrange({0,0,static_cast<float>(width),950});canvas.UpdateLayout();Pump(200);canvas.UpdateLayout();
                auto group=FindElement(canvas,L"BezelThicknessGroup");Require(static_cast<bool>(group),L"Thickness group materializes");
                Require(std::abs(group.ActualWidth()-width)<1,L"Thickness group fills available width");
                auto grid=FindElement(group,L"BezelEdgesGrid").as<X::Controls::Grid>();
                auto overall=FindElement(group,L"RetroBezelSize").as<E::SettingContainer>();
                auto overallCombo=overall.Content().as<X::Controls::ComboBox>();
                Require(std::abs(overall.ActualWidth()-grid.ActualWidth())<1&&std::abs(overallCombo.ActualWidth()-overall.ActualWidth())<1,L"Overall field fills the top of the group");
                const auto overallOrigin=overall.TransformToVisual(group).TransformPoint({0,0});const auto edgesOrigin=grid.TransformToVisual(group).TransformPoint({0,0});
                Require(overallOrigin.Y+overall.ActualHeight()<=edgesOrigin.Y,L"Overall size sits above the edge controls");
                if(resize==0)
                {
                    auto vm=testedProfile.DefaultAppearance();const auto inherited=vm.RetroBezelSize();
                    for(auto const& entry:vm.RetroBezelSizeList())if(winrt::unbox_value<winrt::Microsoft::Terminal::Settings::Model::RetroBezelSize>(entry.EnumValue())==winrt::Microsoft::Terminal::Settings::Model::RetroBezelSize::Wide)overallCombo.SelectedItem(entry);
                    Pump(100);Require(vm.RetroBezelSize()==winrt::Microsoft::Terminal::Settings::Model::RetroBezelSize::Wide,L"Actual overall dropdown updates its value");
                    auto reset=FindElement(overall,L"ResetButton").as<X::Controls::Button>();X::Automation::Peers::ButtonAutomationPeer peer{reset};
                    peer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);canvas.UpdateLayout();
                    Require(!vm.HasRetroBezelSize()&&vm.RetroBezelSize()==inherited,L"Overall reset restores inherited value");
                    std::wcout<<L"PASS: actual overall dropdown and inherited Reset binding"<<std::endl;
                }
                double firstWidth=-1;
                const std::array<wchar_t const*,4> names{L"RetroBezelTopSize",L"RetroBezelLeftSize",L"RetroBezelRightSize",L"RetroBezelBottomSize"};
                const std::array<int,4> wideRows{0,1,1,2},wideColumns{0,0,1,0};
                Require(grid.Children().Size()==4,L"Exactly four edge fields");
                for(size_t index=0;index<names.size();++index)
                {
                    const auto name=names[index];
                    auto cell=FindElement(group,name).as<E::SettingContainer>();
                    auto combo=cell.Content().as<X::Controls::ComboBox>();
                    Require(grid.Children().GetAt(static_cast<uint32_t>(index)).as<X::FrameworkElement>().Name()==name,L"Tab/declaration order Top Left Right Bottom");
                    Require(cell.ActualWidth()>0&&std::abs(combo.ActualWidth()-cell.ActualWidth())<1,L"Edge dropdown stretches across field");
                    if(firstWidth<0)firstWidth=cell.ActualWidth();else Require(std::abs(firstWidth-cell.ActualWidth())<1,L"Four edge fields have equal widths");
                    auto origin=cell.TransformToVisual(group).TransformPoint({0,0});
                    Require(origin.X>=0&&origin.X+cell.ActualWidth()<=group.ActualWidth()+1,L"Edge field remains inside card");
                    if(width<480)Require(X::Controls::Grid::GetRow(cell)==static_cast<int>(index)&&X::Controls::Grid::GetColumn(cell)==0&&X::Controls::Grid::GetColumnSpan(cell)==2,L"Narrow edge fields stack in reading order");
                    else Require(X::Controls::Grid::GetRow(cell)==wideRows[index]&&X::Controls::Grid::GetColumn(cell)==wideColumns[index],L"Wide spatial Top Left/Right Bottom arrangement");
                    auto label=FindLabel(cell,winrt::unbox_value<winrt::hstring>(cell.Header()));Require(static_cast<bool>(label),L"Edge field has visible label");
                    const auto labelTop=label.TransformToVisual(group).TransformPoint({0,0});const auto comboTop=combo.TransformToVisual(group).TransformPoint({0,0});
                    Require(labelTop.Y+label.ActualHeight()<=comboTop.Y+1,L"Labels sit above dropdowns");
                    if(width>=480)Require(std::abs((labelTop.X+label.ActualWidth()/2)-(comboTop.X+combo.ActualWidth()/2))<1,L"Wide edge label centered over dropdown");
                    else Require(std::abs(labelTop.X-comboTop.X)<1,L"Stacked edge label left aligned");
                    Require(!X::Automation::AutomationProperties::GetName(combo).empty(),L"Dropdown has accessible name");
                    std::wcout<<L"LAYOUT width="<<width<<L" "<<name<<L" row="<<X::Controls::Grid::GetRow(cell)<<L" col="<<X::Controls::Grid::GetColumn(cell)<<L" field="<<cell.ActualWidth()<<L" combo="<<combo.ActualWidth()<<std::endl;
                }
                if(resize<2)SavePng(std::filesystem::path{argv[2]}/(width>480?L"bezel-thickness-wide.png":L"bezel-thickness-narrow.png"),group);
                if(resize==2)
                {
                    auto top=FindElement(group,L"RetroBezelTopSize").as<E::SettingContainer>();
                    auto reset=FindElement(top,L"ResetButton").as<X::Controls::Button>();
                    Require(reset.Visibility()==X::Visibility::Visible,L"Local edge override exposes reset");
                    X::Automation::Peers::ButtonAutomationPeer peer{reset};
                    peer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);canvas.UpdateLayout();
                    Require(!testedProfile.DefaultAppearance().HasRetroBezelTopSize(),L"Native reset button clears local override");
                    auto selected=top.Content().as<X::Controls::ComboBox>().SelectedItem().as<E::EnumEntry>();
                    Require(winrt::unbox_value<winrt::Microsoft::Terminal::Settings::Model::RetroBezelEdgeSize>(selected.EnumValue())==winrt::Microsoft::Terminal::Settings::Model::RetroBezelEdgeSize::Default,L"Reset refreshes actual dropdown to inherited choice");
                    std::wcout<<L"PASS: actual native edge Reset button and dropdown binding"<<std::endl;
                }
                ++resize;
            }
            std::wcout<<L"PASS: production thickness card full width, equal dropdowns, wide/narrow/wide transitions"<<std::endl;
        }
        std::wcout<<kind<<L" layout complete failures="<<failures<<std::endl;
        island.Content(nullptr);island.Close();DestroyWindow(hwnd);
        // The owned one-page test process has no app run loop. Stop after the
        // explicit fixture teardown instead of exercising Application shutdown.
        ExitProcess(failures?3:0);
    }
    catch(winrt::hresult_error const&e){std::wcerr<<L"stage="<<stage<<L" HRESULT=0x"<<std::hex<<static_cast<unsigned long>(e.code().value)<<L" "<<e.message().c_str()<<std::endl;return 2;}
}
