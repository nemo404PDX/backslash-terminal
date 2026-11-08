#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <appmodel.h>
#undef GetCurrentTime
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Core.h>
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

#include <fstream>
#include <set>
#include <winrt/Windows.Data.Json.h>

static void Pump(unsigned milliseconds)
{
    const auto end=GetTickCount64()+milliseconds;
    do{MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);}while(GetTickCount64()<end);
}
static void Require(bool condition,wchar_t const* message)
{
    if(!condition){std::wcerr<<L"FAIL: "<<message<<std::endl;ExitProcess(4);}
}
struct ProbeApp : X::ApplicationT<ProbeApp,X::Markup::IXamlMetadataProvider>
{
    std::vector<X::Markup::IXamlMetadataProvider> providers;
    X::Markup::IXamlMetadataProvider winui{nullptr};
    X::Hosting::WindowsXamlManager manager{nullptr};
    ProbeApp()
    {
        std::wcout<<L"app: providers"<<std::endl;
        providers.push_back(winrt::get_activation_factory<winrt::TerminalApp::XamlMetaDataProvider>().ActivateInstance<winrt::TerminalApp::XamlMetaDataProvider>());
        providers.push_back(E::XamlMetaDataProvider{});
        providers.push_back(winrt::Microsoft::Terminal::Control::XamlMetaDataProvider{});
        winui=winrt::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider{};providers.push_back(winui);
        UnhandledException([](auto const&,auto const& args){std::wcerr<<L"app-init XAML "<<args.Message().c_str()<<std::endl;args.Handled(true);});
        std::wcout<<L"app: manager"<<std::endl;
        manager=X::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        std::wcout<<L"app: resources"<<std::endl;
        Resources().MergedDictionaries().Append(winrt::Microsoft::UI::Xaml::Controls::XamlControlsResources{});
        std::wcout<<L"app: ready"<<std::endl;
    }
    X::Markup::IXamlType GetXamlType(X::Interop::TypeName const& type){return GetXamlType(type.Name);}
    X::Markup::IXamlType GetXamlType(winrt::hstring const& type){if(type.starts_with(L"Windows."))return nullptr;if(type.starts_with(L"Microsoft.UI.Xaml.")&&winui)return winui.GetXamlType(type);for(auto const& p:providers){if(auto t=p.GetXamlType(type))return t;}return nullptr;}
    winrt::com_array<X::Markup::XmlnsDefinition> GetXmlnsDefinitions(){return {};}
};
#include <winrt/Windows.Graphics.Imaging.h>
#include "../../src/cascadia/inc/SftpWorkspaceController.h"
#include "../../src/cascadia/inc/SftpService.h"
#include "../../src/cascadia/inc/SftpExit.h"
#include "../../src/cascadia/inc/TransfersNavigation.h"
#include "NavigationPaneTests.h"
template<typename T> auto Await(T operation) { while(operation.Status()==winrt::Windows::Foundation::AsyncStatus::Started) Pump(10); return operation.GetResults(); }
static void Capture(X::UIElement const& owner,std::filesystem::path const& path) {
        X::Media::Imaging::RenderTargetBitmap bitmap;Await(bitmap.RenderAsync(owner));auto pixels=Await(bitmap.GetPixelsAsync());std::vector<uint8_t> bytes(pixels.Length());winrt::Windows::Storage::Streams::DataReader::FromBuffer(pixels).ReadBytes(bytes);
        winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;
        using namespace winrt::Windows::Graphics::Imaging;
        auto encoder=Await(BitmapEncoder::CreateAsync(BitmapEncoder::PngEncoderId(),stream));encoder.SetPixelData(BitmapPixelFormat::Bgra8,BitmapAlphaMode::Premultiplied,bitmap.PixelWidth(),bitmap.PixelHeight(),96,96,bytes);Await(encoder.FlushAsync());stream.Seek(0);
        winrt::Windows::Storage::Streams::DataReader reader{stream};Await(reader.LoadAsync(static_cast<uint32_t>(stream.Size())));std::vector<uint8_t> encoded(static_cast<size_t>(stream.Size()));reader.ReadBytes(encoded);std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(encoded.data()),static_cast<std::streamsize>(encoded.size()));output.close();
}
namespace S=Microsoft::Terminal::Backslash::Transfers::Sftp;
static std::atomic<bool> MockHold=false,MockEntered=false,MockConflict=false;
struct MockSession:S::Session {
    std::atomic<bool> canceled=false;
    std::vector<S::Entry> Browse(std::string_view)override{MockEntered=true;while(MockHold&&!canceled)Sleep(1);return {{"example.bin",{4096,0100644,true,true}},{"folder",{0,0040755,true,true}}};}
    void Prepare(const S::Request&,const std::shared_ptr<S::Replacement>& authority)override{if(MockConflict&&!authority)throw S::Conflict(std::make_shared<S::Replacement>());}
    void Transfer(const S::Request&,const S::ProgressCallback& progress,const S::CancelCheck& cancel)override{cancel();progress({4096,4096});}
    void Publish(const S::Request&,const std::shared_ptr<S::Replacement>&)override{}
    void Cancel()noexcept override{canceled=true;}
};
void Invoke(X::Controls::Button const& button){X::Automation::Peers::ButtonAutomationPeer peer{button};peer.as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(50);}
X::Controls::Button FindButton(X::DependencyObject const& parent,winrt::hstring const& label)
{
    if(const auto button=parent.try_as<X::Controls::Button>())if(const auto content=button.Content().try_as<winrt::Windows::Foundation::IPropertyValue>();content&&content.Type()==winrt::Windows::Foundation::PropertyType::String&&content.GetString()==label)return button;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto found=FindButton(X::Media::VisualTreeHelper::GetChild(parent,i),label))return found;return nullptr;
}
X::Controls::ContentDialog FindDialog(X::DependencyObject const& parent)
{
    if(auto dialog=parent.try_as<X::Controls::ContentDialog>())return dialog;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto found=FindDialog(X::Media::VisualTreeHelper::GetChild(parent,i)))return found;return nullptr;
}
int wmain(int argc,wchar_t** argv)
{
    try{
        Require(argc==2,L"Evidence path");const std::filesystem::path work(argv[1]);winrt::init_apartment(winrt::apartment_type::single_threaded);auto app=winrt::make<ProbeApp>();int errors=0;
        app.UnhandledException([&](auto const&,auto const& args){++errors;std::wcerr<<args.Message().c_str()<<std::endl;args.Handled(true);});
        std::ifstream templateFile(work/L"template.xaml",std::ios::binary);std::string markup{std::istreambuf_iterator<char>(templateFile),{}};
        auto viewTemplate=X::Markup::XamlReader::Load(winrt::to_hstring(markup)).as<X::DataTemplate>();
        auto root=viewTemplate.LoadContent().as<X::Controls::Grid>();
        unsigned settingsRequests=0;
        auto navigation=Microsoft::Terminal::Backslash::TransfersShell::Attach(root,[&]{++settingsRequests;});
        root.FindName(L"Empty").as<X::Controls::TextBlock>().Text(L"No transfers yet.");
        root.FindName(L"Jobs").as<X::Controls::ListView>().Visibility(X::Visibility::Collapsed);
        S::Service service([](const auto&){return std::make_shared<MockSession>();});service.Enabled(true);
        using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
        const auto settings=Settings::LoadDefaults();Require(Settings::IsPortableMode(),L"Model remains in owned portable clone");
        std::filesystem::create_directories(work/L"owned-files");{std::ofstream file(work/L"owned-files"/L"example.bin");file<<"owned fixture";}{std::ofstream file(work/L"ssh.exe");file<<"INERT fixture - mock factory never executes";}
        SetEnvironmentVariableW(L"BACKSLASH_SSH_EXE",(work/L"ssh.exe").c_str());
        auto profile=settings.AllProfiles().GetAt(0);profile.Source(L"Backslash.Terminal.SSH");profile.Commandline(LR"("%BACKSLASH_SSH_EXE%" fixture)");profile.Name(L"SFTP - production callback fixture");
        namespace J=winrt::Windows::Data::Json;
        auto config=J::JsonObject::Parse(settings.GlobalSettings().TransferBackendsJson());config.GetNamedObject(L"sftp").SetNamedValue(L"enabled",J::JsonValue::CreateBooleanValue(true));
        auto reference=std::wstring(winrt::to_hstring(profile.Guid()));for(auto& c:reference)if(c>='a'&&c<='f')c-=32;
        config.GetNamedObject(L"sftp").SetNamedValue(L"profile",J::JsonValue::CreateStringValue(reference));settings.GlobalSettings().TransferBackendsJson(config.Stringify());
        auto controller=S::WorkspaceController::Create(root,service);controller->UpdateSettings(settings);auto view=controller->View();Require(view.connection.Text()==profile.Name(),L"Production binding accepts uppercase parsed GUID");
        const auto setReference=[&](const winrt::hstring& id){config.GetNamedObject(L"sftp").SetNamedValue(L"profile",J::JsonValue::CreateStringValue(id));settings.GlobalSettings().TransferBackendsJson(config.Stringify());controller->UpdateSettings(settings);};
        try{setReference(winrt::hstring(reference.substr(1,36)));Require(view.connection.Text()==profile.Name(),L"Equivalent unbraced GUID accepted when model permits");std::wcout<<L"PASS equivalent unbraced GUID accepted by production model/binding\n";}catch(const winrt::hresult_error&){setReference(winrt::hstring(reference));}
        setReference(L"{00000000-1111-2222-3333-444444444444}");Require(view.connection.Text()==L"SFTP",L"Missing production profile clears binding");Invoke(view.remoteGo);Require(service.Browser().generation==0,L"Missing profile cannot start a session");setReference(winrt::hstring(reference));
        profile.Source(L"not-owned");controller->UpdateSettings(settings);Require(view.connection.Text()==L"SFTP",L"Changed source profile rejected");profile.Source(L"Backslash.Terminal.SSH");controller->UpdateSettings(settings);
        view.localPath.Text(winrt::hstring((work/L"owned-files").wstring()));

        HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned SFTP mock workspace",WS_OVERLAPPEDWINDOW,-32000,-32000,1600,1250,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);Require(hwnd!=nullptr,L"Owned host");
        X::Hosting::DesktopWindowXamlSource island;auto native=island.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(native->AttachToWindow(hwnd));island.Content(root);HWND child{};winrt::check_hresult(native->get_WindowHandle(&child));
        SetWindowPos(child,nullptr,0,0,1536,1200,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);root.Measure({1024,900});root.Arrange({0,0,1536,1200});root.UpdateLayout();Pump(500);
        Require(navigation.PaneTitle().empty(),L"Sidebar has no redundant page title");
        const auto card=root.FindName(L"TransfersContentCard").as<X::Controls::Border>();
        const auto checkCard=[&] {
            CheckNavigationPane(navigation,root.ActualTheme()==X::ElementTheme::Dark);
            const auto spacer=root.FindName(L"NavigationSearchRowSpace").as<X::FrameworkElement>();
            const auto first=root.FindName(L"ActivityNavItem").as<X::FrameworkElement>();
            const auto firstY=first.TransformToVisual(root).TransformPoint({0,0}).Y;
            const auto spaceY=spacer.TransformToVisual(root).TransformPoint({0,0}).Y;
            std::wcout<<L"NAVIGATION_FIRST_ROW y="<<firstY<<L" spacer="<<spaceY<<L" height="<<spacer.ActualHeight()<<L" rootHeight="<<root.ActualHeight()<<std::endl;
            if(navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal||navigation.IsPaneOpen())Require(spacer.ActualHeight()>0,L"Visible navigation reserves native search-row space without a search control");
            if(navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal||navigation.IsPaneOpen())Require(firstY>=spaceY+spacer.ActualHeight(),L"First navigation entry follows reserved search-row space");
            if(!navigation.IsPaneOpen()&&navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal){
                const auto button=root.FindName(L"BackendSettings").as<X::FrameworkElement>();
                const auto gear=root.FindName(L"BackendSettingsIcon").as<X::FrameworkElement>();
                const auto x=gear.TransformToVisual(button).TransformPoint({0,0}).X;
                std::wcout<<L"FOOTER_COMPACT iconX="<<x<<L" iconWidth="<<gear.ActualWidth()<<L" buttonWidth="<<button.ActualWidth()<<std::endl;
                Require(std::abs(x+gear.ActualWidth()/2-button.ActualWidth()/2)<1,L"Collapsed footer icon centers inside its button");
            }
            if(navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal||navigation.IsPaneOpen()){
                const auto gear=root.FindName(L"BackendSettingsIcon").as<X::FrameworkElement>();
                const auto item=root.FindName(L"SftpNavItem").as<Mux::NavigationViewItem>();
                const auto icon=item.Icon().as<X::FrameworkElement>();
                const auto gearX=gear.TransformToVisual(root).TransformPoint({0,0}).X+gear.ActualWidth()/2;
                const auto iconX=icon.TransformToVisual(root).TransformPoint({0,0}).X+icon.ActualWidth()/2;
                std::wcout<<L"FOOTER_ALIGNMENT gear="<<gearX<<L" navigation="<<iconX<<std::endl;
                if(navigation.IsPaneOpen())Require(std::abs(gearX-iconX)<2,L"Expanded bottom settings gear aligns with navigation icons");
                if(navigation.IsPaneOpen()){
                    const auto caption=FindNavigationTitle(item,L"SFTP");Require(static_cast<bool>(caption),L"Expanded navigation caption rendered");
                    const auto label=root.FindName(L"BackendSettingsLabel").as<X::FrameworkElement>();
                    std::wcout<<L"FOOTER_TEXT label="<<label.TransformToVisual(root).TransformPoint({0,0}).X<<L" navigation="<<caption.TransformToVisual(root).TransformPoint({0,0}).X<<std::endl;
                    Require(std::abs(label.TransformToVisual(root).TransformPoint({0,0}).X-caption.TransformToVisual(root).TransformPoint({0,0}).X)<2,L"Bottom settings caption aligns with navigation text");
                }
            }
            const auto viewport=root.FindName(L"TransfersScrollViewer").as<X::Controls::ScrollViewer>();
            const auto offset=card.TransformToVisual(viewport).TransformPoint({0,0});
            std::wcout<<L"TRANSFERS_CARD viewport="<<viewport.ActualWidth()<<L" card="<<card.ActualWidth()<<L" x="<<offset.X<<L" root="<<root.ActualWidth()<<L" origin="<<viewport.TransformToVisual(root).TransformPoint({0,0}).X<<L" padding="<<card.Padding().Left<<std::endl;
            Require(card.ActualWidth()>0&&card.ActualWidth()<=1000.5,L"Standalone Transfers content is width constrained");
            Require(offset.X>=15&&offset.X+card.ActualWidth()<=viewport.ActualWidth()-15,L"Standalone Transfers retains outer breathing room");
            Require(std::abs(offset.X+card.ActualWidth()/2-viewport.ActualWidth()/2)<2,L"Standalone Transfers card is centered");
            const auto viewportOrigin=viewport.TransformToVisual(root).TransformPoint({0,0});
            Require(viewportOrigin.X>=0&&viewportOrigin.X+viewport.ActualWidth()<=root.ActualWidth()+1,L"Content viewport stays inside host after navigation transitions");
            Require(!card.Background(),L"Page has separate cards without an enclosing painted box");
            const auto heading=root.FindName(L"Heading").as<X::FrameworkElement>();
            const auto headingPosition=heading.TransformToVisual(root).TransformPoint({0,0});
            Require(headingPosition.Y>=0&&headingPosition.X>=0,L"Page heading stays inside the host after transitions");
            Require(std::abs(headingPosition.X-card.TransformToVisual(root).TransformPoint({0,0}).X)<2,L"Transfers heading aligns with its centered body column through pane and resize transitions");
            Require(heading.TransformToVisual(card).TransformPoint({0,0}).Y+heading.ActualHeight()<=1,L"Page heading is outside and above the separate content cards");
        };
        checkCard();
        {
            E::MainPage settingsReference{settings};settingsReference.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
            island.Content(settingsReference);Pump(200);
            Require(settingsReference.Breadcrumbs().Size()>0,L"Actual Settings breadcrumb rendered for typography comparison");
            const auto caption=settingsReference.Breadcrumbs().GetAt(0).as<E::Breadcrumb>().Label();
            const auto expected=FindNavigationTitle(settingsReference.FindName(L"NavigationBreadcrumbBar").as<X::DependencyObject>(),caption);
            Require(static_cast<bool>(expected),L"Actual Settings title TextBlock found");
            const auto heading=root.FindName(L"Heading").as<X::Controls::TextBlock>();
            std::wcout<<L"TITLE_METRICS transfers="<<heading.FontSize()<<L" settings="<<expected.FontSize()<<L" weight="<<heading.FontWeight().Weight<<L" family="<<heading.FontFamily().Source().c_str()<<std::endl;
            Require(heading.FontSize()==expected.FontSize()&&heading.FontWeight().Weight==expected.FontWeight().Weight&&heading.FontFamily().Source()==expected.FontFamily().Source()&&heading.LineHeight()==expected.LineHeight(),L"Transfers and SFTP titles match actual Settings title font metrics");
            island.Content(root);Pump(300);
        }
        E::MainPage verticalReference{settings};verticalReference.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
        const auto referenceMenu=verticalReference.FindName(L"SettingsNav").as<Mux::NavigationView>();
        const auto compareTitle=[&]{
            const bool open=navigation.IsPaneOpen();
            const auto mode=navigation.DisplayMode();
            const auto width=root.ActualWidth();
            const auto height=root.ActualHeight();
            const auto titleY=root.FindName(L"Heading").as<X::FrameworkElement>().TransformToVisual(root).TransformPoint({0,0}).Y;
            const auto bodyY=card.TransformToVisual(root).TransformPoint({0,0}).Y;
            island.Content(verticalReference);verticalReference.UpdateLayout();Pump(200);referenceMenu.IsPaneOpen(open);verticalReference.UpdateLayout();Pump(350);
            const auto text=FindNavigationTitle(verticalReference.FindName(L"NavigationBreadcrumbBar").as<X::DependencyObject>(),verticalReference.Breadcrumbs().GetAt(0).as<E::Breadcrumb>().Label());
            Require(static_cast<bool>(text),L"Settings reference title rendered at matching dimensions");
            const auto referenceY=text.TransformToVisual(verticalReference).TransformPoint({0,0}).Y;
            std::wcout<<L"TITLE_MATCH page="<<root.FindName(L"Heading").as<X::Controls::TextBlock>().Text().c_str()<<L" width="<<width<<L" height="<<height<<L" open="<<open<<L" mode="<<int(mode)<<L" title="<<titleY<<L" settings="<<referenceY<<L" body="<<bodyY<<L" referenceWidth="<<verticalReference.ActualWidth()<<L" referenceHeight="<<verticalReference.ActualHeight()<<L" referenceOpen="<<referenceMenu.IsPaneOpen()<<L" referenceMode="<<int(referenceMenu.DisplayMode())<<std::endl;
            Require(std::abs(verticalReference.ActualWidth()-width)<1&&std::abs(verticalReference.ActualHeight()-height)<1&&referenceMenu.DisplayMode()==mode&&referenceMenu.IsPaneOpen()==open,L"Title comparison uses matching viewport and navigation state");
            const auto tolerance=1.0/verticalReference.XamlRoot().RasterizationScale()+0.05;
            Require(std::abs(titleY-referenceY)<=tolerance,L"Transfers and SFTP title tops align within one raster pixel of Settings");
            const auto baseline=titleY+root.FindName(L"Heading").as<X::Controls::TextBlock>().BaselineOffset();
            const auto referenceBaseline=referenceY+text.BaselineOffset();
            std::wcout<<L"TITLE_BASELINE transfers="<<baseline<<L" settings="<<referenceBaseline<<L" scale="<<verticalReference.XamlRoot().RasterizationScale()<<std::endl;
            Require(std::abs(baseline-referenceBaseline)<=tolerance,L"Matched title font baselines align within one raster pixel");
            island.Content(root);navigation.IsPaneOpen(open);root.UpdateLayout();Pump(200);
        };
        for(const int width:{2400,1050,420,1920}) {
            SetWindowPos(hwnd,nullptr,-32000,-32000,width+64,1250,SWP_NOACTIVATE|SWP_NOZORDER);
            SetWindowPos(child,nullptr,0,0,width,1200,SWP_NOACTIVATE|SWP_NOZORDER);
            root.UpdateLayout();Pump(150);
            for(const auto page:{L"ActivityNavItem",L"SftpNavItem"}) {
                navigation.SelectedItem(root.FindName(page));
                for(const bool open:{false,true,false}) {navigation.IsPaneOpen(open);Pump(350);root.UpdateLayout();checkCard();compareTitle();}
            }
        }
        for(const int width:{1920,1050}) {
            SetWindowPos(child,nullptr,0,0,width,480,SWP_NOACTIVATE|SWP_NOZORDER);root.UpdateLayout();Pump(200);
            for(const bool open:{true,false}) {
                navigation.IsPaneOpen(open);Pump(250);checkCard();
                if(open||navigation.DisplayMode()!=Mux::NavigationViewDisplayMode::Minimal){
                    const auto footer=root.FindName(L"BackendSettings").as<X::FrameworkElement>();
                    const auto first=root.FindName(L"ActivityNavItem").as<X::FrameworkElement>();
                    Require(footer.TransformToVisual(root).TransformPoint({0,0}).Y>=first.TransformToVisual(root).TransformPoint({0,0}).Y+first.ActualHeight(),L"Short-height footer remains below usable first navigation item");
                }
            }
        }
        SetWindowPos(child,nullptr,0,0,1920,1200,SWP_NOACTIVATE|SWP_NOZORDER);root.UpdateLayout();Pump(200);
        navigation.SelectedItem(root.FindName(L"ActivityNavItem"));navigation.IsPaneOpen(true);Pump(150);
        root.FindName(L"Search").as<X::Controls::TextBox>().PlaceholderText(L"Search transfers");
        const auto statusFilter=root.FindName(L"StatusFilter").as<X::Controls::ComboBox>();
        X::Controls::ComboBoxItem all;all.Content(winrt::box_value(L"All"));statusFilter.Items().Append(all);statusFilter.SelectedIndex(0);
        Require(root.FindName(L"SftpWorkspace").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"Transfers opens on activity without SFTP controls");
        Require(root.FindName(L"TransferActivity").as<X::UIElement>().Visibility()==X::Visibility::Visible,L"Shared activity is the default page");
        Require(root.FindName(L"TransferActionError").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"Backend configuration does not leak into activity errors");
        Capture(root,work/L"workspace-transfers-default.png");
        navigation.IsPaneOpen(false);Pump(400);checkCard();
        Require(root.FindName(L"BackendGroupHeader").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"Collapsed menu does not truncate group labels");
        Require(root.FindName(L"BackendSettingsLabel").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"Collapsed footer retains icon without clipped text");
        for(int sample=0;sample<3;++sample){
            Pump(sample==0?100:800);root.UpdateLayout();
            for(const auto name:{L"ActivityNavItem",L"SftpNavItem"}){
                const auto item=root.FindName(name).as<Mux::NavigationViewItem>();
                const auto icon=item.Icon().as<X::FrameworkElement>();
                const auto point=icon.TransformToVisual(item).TransformPoint({0,0});
                std::wcout<<L"COLLAPSED_ICON sample="<<sample<<L" item="<<name<<L" x="<<point.X<<L" y="<<point.Y<<L" width="<<icon.ActualWidth()<<L" height="<<icon.ActualHeight()<<L" itemWidth="<<item.ActualWidth()<<std::endl;
                Require(point.X>=0&&point.X+icon.ActualWidth()<=item.ActualWidth(),L"Collapsed navigation icon remains within its item");
                Capture(item,work/(std::wstring{name}+L"-collapsed-"+std::to_wstring(sample)+L".png"));
                Capture(icon,work/(std::wstring{name}+L"-glyph-"+std::to_wstring(sample)+L".png"));
            }
            Capture(root,work/(L"workspace-menu-settled-"+std::to_wstring(sample)+L".png"));
        }
        Capture(root,work/L"workspace-menu-collapsed.png");navigation.IsPaneOpen(true);Pump(400);checkCard();
        std::wcout<<L"STAGE settings footer action"<<std::endl;
        Invoke(root.FindName(L"BackendSettings").as<X::Controls::Button>());Pump(100);
        Require(settingsRequests==1,L"Backend settings menu invokes the shared settings route once");
        Require(navigation.SelectedItem()==root.FindName(L"ActivityNavItem"),L"Settings action preserves selected transfer page");
        navigation.SelectedItem(root.FindName(L"SftpNavItem"));Pump(100);
        Require(root.FindName(L"TransferActivity").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"SFTP is separate from activity rather than appended below it");
        Require(service.Browser().generation==0,L"Menu navigation does not initiate a connection");
        for(int i=0;i<3;++i){
            navigation.SelectedItem(root.FindName(L"ActivityNavItem"));Pump(30);
            Require(Microsoft::Terminal::Backslash::TransfersShell::FocusActiveView(root,view.localPath,X::FocusState::Keyboard),L"Repeated activity open accepts keyboard focus");
            Require(root.FindName(L"Search").as<X::Controls::TextBox>().FocusState()==X::FocusState::Keyboard,L"Activity focuses visible search");
            navigation.SelectedItem(root.FindName(L"SftpNavItem"));Pump(30);
            Require(Microsoft::Terminal::Backslash::TransfersShell::FocusActiveView(root,view.localPath,X::FocusState::Keyboard),L"Repeated SFTP open accepts keyboard focus");
            Require(view.localPath.FocusState()==X::FocusState::Keyboard,L"SFTP focuses visible local directory rather than hidden activity search");
        }
        Require(navigation.SelectedItem()==root.FindName(L"SftpNavItem")&&service.Browser().generation==0,L"Focus does not change navigation or initiate browsing");
        // Capture the actual unconfigured controller state without initiating a browse.
        config.GetNamedObject(L"sftp").SetNamedValue(L"enabled",J::JsonValue::CreateBooleanValue(false));
        settings.GlobalSettings().TransferBackendsJson(config.Stringify());controller->UpdateSettings(settings);
        Require(view.localFiles.Items().Size()==0&&view.remoteFiles.Items().Size()==0,L"Unconfigured capture does not browse automatically");
        Require(view.localFiles.Visibility()==X::Visibility::Collapsed&&view.remoteFiles.Visibility()==X::Visibility::Collapsed,L"Unbrowsed lists do not reserve blank browser space");
        Require(view.localEmpty.Visibility()==X::Visibility::Visible&&view.remoteEmpty.Visibility()==X::Visibility::Visible,L"Unbrowsed panes explain their next action");
        Require(root.FindName(L"TransferActionError").as<X::UIElement>().Visibility()==X::Visibility::Collapsed,L"Backend configuration prompts stay separate from queue-action errors");
        Capture(root,work/L"workspace-unconfigured-card.png");
        config.GetNamedObject(L"sftp").SetNamedValue(L"enabled",J::JsonValue::CreateBooleanValue(true));
        settings.GlobalSettings().TransferBackendsJson(config.Stringify());controller->UpdateSettings(settings);
        for(const auto& [path,parent]:std::vector<std::pair<winrt::hstring,winrt::hstring>>{{L".",L".."},{L"..",L"../.."},{L"../..",L"../../.."},{L"namedrelative",L"."},{L"/",L"/"},{L"/folder///",L"/"}}){view.remotePath.Text(path);Invoke(view.remoteUp);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(50);Require(view.remotePath.Text()==parent,L"Actual production Up handler parent path");}
        view.remotePath.Text(L".");Invoke(view.remoteGo);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(100);Require(view.remoteFiles.Items().Size()==2,L"Production remote browse callback rows");Invoke(view.localGo);for(int i=0;i<100&&view.localFiles.Items().Size()==0;++i)Pump(10);Require(view.localFiles.Items().Size()==1,L"Production local browse callback");view.localFiles.SelectedIndex(0);view.remoteFiles.SelectedIndex(0);Invoke(view.upload);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(100);Require(service.Jobs().size()==1&&service.Jobs()[0].phase==S::JobPhase::Completed,L"Native invoke reaches mock service");
        Invoke(view.download);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(80);Require(service.Jobs().size()==2&&service.Jobs().back().request.direction==S::Direction::Download,L"Production download callback submits selected remote file");
        MockConflict=true;Invoke(view.download);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(80);const auto conflict=service.Jobs().back();Require(conflict.phase==S::JobPhase::AwaitingDecision,L"Production download conflict retained");
        for(bool approve:{false,true}){controller->ConfirmReplacement(conflict.id,conflict.decision);Pump(150);auto dialog=FindDialog(root.XamlRoot().Content());if(!dialog)for(const auto& popup:X::Media::VisualTreeHelper::GetOpenPopupsForXamlRoot(root.XamlRoot()))if((dialog=FindDialog(popup.Child())))break;Require(static_cast<bool>(dialog)&&dialog.DefaultButton()==X::Controls::ContentDialogButton::Close,L"Actual replacement confirmation defaults to cancel");const auto action=FindButton(dialog,approve?L"Replace":L"Cancel");Require(static_cast<bool>(action),L"Replacement action visible");Invoke(action);for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(80);Require(service.Jobs().back().phase==(approve?S::JobPhase::Completed:S::JobPhase::AwaitingDecision),L"Production replacement cancel/approve callback");}MockConflict=false;
        controller->ConfirmReplacement(conflict.id,conflict.decision);Pump(50);
        Require(root.FindName(L"TransferActionError").as<X::Controls::TextBlock>().Text()==L"That replacement decision expired",L"Expired replacement feedback reaches activity");controller->Error({});
        Require(X::Controls::Grid::GetRow(view.browsers.Children().GetAt(1).as<X::FrameworkElement>())==0,L"Wide browsers side by side");
        S::JobSnapshot retained;retained.phase=S::JobPhase::Canceled;retained.request.local=LR"(D:\owned-fixture\example.bin)";retained.request.remote="/destination.bin";retained.retained.push_back({{},"/.backslash-fixture.partial","Remote temporary may remain; cleanup not confirmed"});
        X::Controls::TextBlock retainedRow;retainedRow.Text(winrt::hstring(S::DisplayJob(retained)));retainedRow.TextWrapping(X::TextWrapping::Wrap);root.FindName(L"Jobs").as<X::Controls::ListView>().Items().Append(retainedRow);Require(std::wstring_view(retainedRow.Text()).find(L"/.backslash-fixture.partial")!=std::wstring_view::npos,L"Canceled job UI preserves exact retained remote path");
        Capture(root,work/L"workspace-wide.png");
        navigation.SelectedItem(root.FindName(L"ActivityNavItem"));Pump(100);
        root.FindName(L"Jobs").as<X::Controls::ListView>().Visibility(X::Visibility::Visible);
        root.FindName(L"Empty").as<X::Controls::TextBlock>().Visibility(X::Visibility::Collapsed);
        Capture(root,work/L"workspace-activity-wide.png");
        const auto previousTheme=root.RequestedTheme();root.RequestedTheme(X::ElementTheme::Light);Pump(100);CheckNavigationPane(navigation,false);Capture(root,work/L"workspace-activity-light.png");root.RequestedTheme(previousTheme);Pump(50);
        navigation.SelectedItem(root.FindName(L"SftpNavItem"));Pump(100);
        Require(view.remoteFiles.Items().Size()==2&&view.localFiles.Items().Size()==1,L"Switching pages retains browser state");
        SetWindowPos(child,nullptr,0,0,420,900,SWP_NOACTIVATE|SWP_NOZORDER);root.Measure({420,900});root.Arrange({0,0,420,900});root.UpdateLayout();Pump(200);checkCard();Require(X::Controls::Grid::GetRow(view.browsers.Children().GetAt(1).as<X::FrameworkElement>())==1,L"Narrow browser stacking");Capture(root,work/L"workspace-narrow.png");
        const auto scroll=root.FindName(L"TransfersScrollViewer").as<X::Controls::ScrollViewer>();scroll.ChangeView(nullptr,scroll.ScrollableHeight(),nullptr);Pump(200);Capture(root,work/L"workspace-narrow-actions.png");
        if(navigation.DisplayMode()==Mux::NavigationViewDisplayMode::Minimal)Require(scroll.TransformToVisual(root).TransformPoint({0,0}).Y>=48,L"Minimal navigation chrome cannot cover scrolling fields");
        const auto point=view.upload.TransformToVisual(root).TransformPoint({0,0});Require(point.Y>=0&&point.Y<root.ActualHeight(),L"Narrow transfer actions reachable by scrolling");const auto stop=view.cancelBrowse.TransformToVisual(root).TransformPoint({0,0});Require(stop.X+view.cancelBrowse.ActualWidth()<=root.ActualWidth(),L"Narrow stop action fits horizontally");
        const auto cardLeft=card.TransformToVisual(root).TransformPoint({0,0}).X;
        for(const auto& button:{view.upload,view.download,view.cancelBrowse}){const auto x=button.TransformToVisual(root).TransformPoint({0,0}).X;Require(x>=cardLeft+card.Padding().Left&&x+button.ActualWidth()<=cardLeft+card.ActualWidth()-card.Padding().Right,L"Every transfer action fits inside the padded card");}
        // Reparent the very same production-controller root into tab-like,
        // split and separate-window hosts without recreating session state.
        island.Content(nullptr);X::Controls::Frame tab;tab.Content(root);island.Content(tab);Pump(80);Require(view.remoteFiles.Items().Size()==2,L"Tab-like host retains production browser state");
        island.Content(nullptr);tab.Content(nullptr);X::Controls::Grid split;X::Controls::ColumnDefinition column;column.Width({1,X::GridUnitType::Star});split.ColumnDefinitions().Append(column);X::Controls::ColumnDefinition other;other.Width({1,X::GridUnitType::Star});split.ColumnDefinitions().Append(other);split.Children().Append(root);island.Content(split);Pump(80);Require(view.remoteFiles.Items().Size()==2,L"Split host retains production browser state");
        island.Content(nullptr);split.Children().Clear();HWND second=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned SFTP separate host",WS_OVERLAPPEDWINDOW,-32000,-32000,800,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);Require(second!=nullptr,L"Owned second host");X::Hosting::DesktopWindowXamlSource separate;auto separateNative=separate.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(separateNative->AttachToWindow(second));separate.Content(root);Pump(80);Require(view.remoteFiles.Items().Size()==2,L"Separate host retains production browser state");separate.Content(nullptr);separate.Close();DestroyWindow(second);island.Content(root);Pump(80);
        MockHold=true;MockEntered=false;Invoke(view.remoteGo);for(int i=0;i<100&&!MockEntered;++i)Pump(10);Require(service.Busy(),L"Owned mock browse in flight");const auto oldGeneration=service.Browser().generation;
        navigation.SelectedItem(root.FindName(L"ActivityNavItem"));Pump(50);navigation.SelectedItem(root.FindName(L"SftpNavItem"));Pump(50);
        Require(service.Busy()&&service.Browser().generation==oldGeneration,L"Navigation does not cancel or restart active work");
        const auto oldStatus=view.status.Text();controller->Close();Invoke(view.remoteGo);Require(service.Browser().generation==oldGeneration,L"Closed production callback cannot start a stale session");
        auto reopenedRoot=viewTemplate.LoadContent().as<X::Controls::Grid>();auto reopened=S::WorkspaceController::Create(reopenedRoot,service);reopened->UpdateSettings(settings);MockHold=false;for(int i=0;i<100&&service.Busy();++i)Pump(10);Pump(100);Require(view.status.Text()==oldStatus,L"Detached production view receives no late update");Require(reopened->View().remoteFiles.Items().Size()==2,L"Reopened production view receives process-owned result");Require(service.Jobs().size()==3,L"Completed job survives production view close/reopen");
        island.Content(reopenedRoot);Pump(100);MockHold=true;MockEntered=false;Invoke(reopened->View().remoteGo);for(int i=0;i<100&&!MockEntered;++i)Pump(10);Require(service.HasPendingWork(),L"Exit fixture has active owned work");
        for(bool quit:{false,true}){auto dialog=S::ExitDialog(quit);dialog.XamlRoot(reopenedRoot.XamlRoot());auto operation=dialog.ShowAsync();Pump(150);Require(dialog.DefaultButton()==X::Controls::ContentDialogButton::Close,L"Active-transfer exit defaults to keep working");auto keep=FindButton(dialog,L"Keep working");Require(static_cast<bool>(keep),L"Exit keep-working action visible");Invoke(keep);Pump(80);Require(operation.Status()==winrt::Windows::Foundation::AsyncStatus::Completed&&operation.GetResults()==X::Controls::ContentDialogResult::None,L"Keep working cancels exit");Require(service.HasPendingWork(),L"Canceling exit does not cancel transfer work");}
        auto quit=S::ExitDialog(true);quit.XamlRoot(reopenedRoot.XamlRoot());auto exit=quit.ShowAsync();Pump(150);auto confirm=FindButton(quit,L"Quit Backslash");Require(static_cast<bool>(confirm),L"Explicit quit action");Invoke(confirm);Pump(80);Require(exit.GetResults()==X::Controls::ContentDialogResult::Primary,L"Explicit quit accepted");service.BeginShutdown();MockHold=false;for(int i=0;i<100&&service.Busy();++i)Pump(10);Require(!service.Busy(),L"Confirmed exit stops owned work");reopened->Close();Pump(50);
        // Exercise the real projected bridge in the current TerminalApp DLL,
        // without AppLogic.Create/LoadAll, user settings or native SFTP jobs.
        auto actualLogic=winrt::get_activation_factory<winrt::TerminalApp::AppLogic>().ActivateInstance<winrt::TerminalApp::AppLogic>();
        const auto beginDrain=actualLogic.BeginTransferShutdown();const auto bridgeStart=GetTickCount64();
        while(beginDrain.Status()==winrt::Windows::Foundation::AsyncStatus::Started){Require(GetTickCount64()-bridgeStart<3000,L"Actual AppLogic shutdown begin bounded");Pump(10);}beginDrain.GetResults();Require(actualLogic.TransferShutdownComplete(),L"Actual owning DLL empty service is drained");Require(!actualLogic.TransferShutdownStatus().empty(),L"Actual DLL exposes shutdown status");std::wcout<<L"PASS actual TerminalApp AppLogic shutdown bridge ABI/empty ownership\n";
        Require(!X::Automation::AutomationProperties::GetName(view.localFiles).empty()&&!X::Automation::AutomationProperties::GetName(view.remoteFiles).empty(),L"Browser accessibility names");Require(errors==0,L"No unhandled XAML errors");controller->Close();Pump(50);island.Content(nullptr);island.Close();DestroyWindow(hwnd);std::wcout<<L"PASS actual production controller/profile bridge and current template: parsed GUID/missing/changed profile, Up navigation, local/remote browse and upload callbacks, wide/narrow layout and retained-path rendering. Mock session transport only; actual controller close/reopen, host reparenting and explicit exit dialogs/shutdown checked.\n"<<std::flush;ExitProcess(0);
    }catch(const winrt::hresult_error& error){std::wcerr<<error.message().c_str()<<L" HRESULT "<<std::hex<<static_cast<uint32_t>(error.code().value)<<std::endl;return 2;}
}
