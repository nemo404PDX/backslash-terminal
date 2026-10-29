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
#include <winrt/Windows.Graphics.Imaging.h>
#include "../../../src/cascadia/inc/ModernExplorerIntegration.h"
template<typename T> auto Await(T operation) { while(operation.Status()==winrt::Windows::Foundation::AsyncStatus::Started) Pump(10); return operation.GetResults(); }
static void Capture(E::MainPage const& owner,std::filesystem::path const& path) {
        X::Media::Imaging::RenderTargetBitmap bitmap;Await(bitmap.RenderAsync(owner));auto pixels=Await(bitmap.GetPixelsAsync());std::vector<uint8_t> bytes(pixels.Length());winrt::Windows::Storage::Streams::DataReader::FromBuffer(pixels).ReadBytes(bytes);
        winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;
        using namespace winrt::Windows::Graphics::Imaging;
        auto encoder=Await(BitmapEncoder::CreateAsync(BitmapEncoder::PngEncoderId(),stream));encoder.SetPixelData(BitmapPixelFormat::Bgra8,BitmapAlphaMode::Premultiplied,bitmap.PixelWidth(),bitmap.PixelHeight(),96,96,bytes);Await(encoder.FlushAsync());stream.Seek(0);
        winrt::Windows::Storage::Streams::DataReader reader{stream};Await(reader.LoadAsync(static_cast<uint32_t>(stream.Size())));std::vector<uint8_t> encoded(static_cast<size_t>(stream.Size()));reader.ReadBytes(encoded);std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(encoded.data()),static_cast<std::streamsize>(encoded.size()));output.close();
}
static X::Controls::ContentDialog FindDialog(X::DependencyObject const& node)
{
    if(auto dialog=node.try_as<X::Controls::ContentDialog>())return dialog;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(node);++i)if(auto found=FindDialog(X::Media::VisualTreeHelper::GetChild(node,i)))return found;
    return nullptr;
}
static X::Controls::Button FindClose(X::DependencyObject const& node)
{
    if(auto button=node.try_as<X::Controls::Button>())if(auto text=button.Content().try_as<winrt::Windows::Foundation::IPropertyValue>();text&&text.Type()==winrt::Windows::Foundation::PropertyType::String&&text.GetString()==L"Cancel")return button;
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(node);++i)if(auto found=FindClose(X::Media::VisualTreeHelper::GetChild(node,i)))return found;
    return nullptr;
}
static void Invoke(X::Controls::Button const& button)
{
    X::Automation::Peers::ButtonAutomationPeer peer{button};peer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();
}
static void CancelOwnedDialog(E::MainPage const& owner,X::Controls::Button const& button)
{
    Require(button.IsEnabled(),L"Expected action enabled before cancellation test");Invoke(button);Pump(200);
    X::Controls::ContentDialog dialog{nullptr};X::Controls::Button cancel{nullptr};
    for(auto popup:X::Media::VisualTreeHelper::GetOpenPopupsForXamlRoot(owner.XamlRoot())){if(auto found=FindDialog(popup.Child()))dialog=found;if(auto found=FindClose(popup.Child()))cancel=found;}
    Require(dialog&&cancel,L"Owned confirmation dialog and explicit Cancel found");Require(dialog.DefaultButton()==X::Controls::ContentDialogButton::Close,L"Cancel is the default confirmation action");
    Invoke(cancel);Pump(150);owner.UpdateLayout();
}
int wmain(int argc,wchar_t**argv)
{
    try {
        Require(argc==2,L"Evidence folder required");const std::filesystem::path work{argv[1]};
        const auto targetBefore=Backslash::ModernExplorer::Management::ReadTargetValues();
        const auto profile=std::filesystem::path(targetBefore.at(L"TargetExecutable")).parent_path()/L"settings"/L"settings.json";
        const auto profileBefore=Backslash::ModernExplorer::Management::Hash(profile);
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        auto app=winrt::make<ProbeApp>();int errors=0;
        app.UnhandledException([&](auto const&,auto const& args){++errors;std::wcerr<<args.Message().c_str()<<std::endl;args.Handled(true);});
        const auto settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings::LoadDefaults();
        wchar_t ownExecutable[32768]{};Require(GetModuleFileNameW(nullptr,ownExecutable,32768)>0,L"Read owned probe path");
        const auto ownRoot=std::filesystem::path(ownExecutable).parent_path();
        using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
        Require(Settings::IsPortableMode()&&std::filesystem::path(Settings::SettingsDirectory().c_str())==ownRoot/L"settings",L"All probe model state stays in the owned runtime clone");
        HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned Explorer Settings layout probe",WS_OVERLAPPEDWINDOW,-32000,-32000,1050,850,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Require(hwnd!=nullptr,L"Owned window");E::MainPage owner{settings};owner.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
        X::Hosting::DesktopWindowXamlSource island;auto native=island.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(native->AttachToWindow(hwnd));island.Content(owner);
        HWND child{};winrt::check_hresult(native->get_WindowHandle(&child));SetWindowPos(child,nullptr,0,0,1024,768,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        owner.Measure({1024,768});owner.Arrange({0,0,1024,768});owner.UpdateLayout();Pump(1500);
        auto page=owner.FindName(L"contentFrame").as<X::Controls::Frame>().Content().as<E::Launch>();Require(static_cast<bool>(page),L"Actual production Launch page");
        Capture(owner,work/L"launch-page.png");
        auto details=page.FindName(L"ExplorerDetails").as<Mux::Expander>();Require(!details.IsExpanded(),L"Details collapsed by default");
        details.IsExpanded(true);owner.UpdateLayout();Pump(150);Capture(owner,work/L"launch-page-details.png");
        details.IsExpanded(false);owner.Measure({640,768});owner.Arrange({0,0,640,768});SetWindowPos(child,nullptr,0,0,640,768,SWP_NOACTIVATE|SWP_NOZORDER);owner.UpdateLayout();Pump(150);Capture(owner,work/L"launch-page-narrow.png");
        details.IsExpanded(true);owner.UpdateLayout();Pump(100);
        CancelOwnedDialog(owner,page.FindName(L"ExplorerEnable").as<X::Controls::Button>());
        CancelOwnedDialog(owner,page.FindName(L"ExplorerRemove").as<X::Controls::Button>());
        CancelOwnedDialog(owner,page.FindName(L"ExplorerRestart").as<X::Controls::Button>());
        Require(Backslash::ModernExplorer::Management::ReadTargetValues()==targetBefore,L"Cancelled actions preserve exact target values");
        Require(Backslash::ModernExplorer::Management::Hash(profile)==profileBefore,L"Owned UI probe preserves existing user profile");
        Require(errors==0,L"No XAML errors");std::wcout<<L"PASS actual production Launch page captured; Details default/expanded and narrow layouts; Enable/Remove/Restart confirmations cancelled; target/profile unchanged\n"<<std::flush;
        island.Content(nullptr);island.Close();DestroyWindow(hwnd);ExitProcess(0);
    }catch(winrt::hresult_error const& error){std::wcerr<<error.message().c_str()<<L" HRESULT="<<std::hex<<static_cast<uint32_t>(error.code().value)<<std::endl;return 2;}
}
