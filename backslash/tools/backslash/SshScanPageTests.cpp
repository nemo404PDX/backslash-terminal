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
#include "SshScanUi.h"
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
static X::Controls::Button FindButton(X::DependencyObject const& parent, std::wstring_view caption)
{
    if(auto button=parent.try_as<X::Controls::Button>())
    {
        if(auto text=button.Content().try_as<winrt::Windows::Foundation::IPropertyValue>();text&&text.Type()==winrt::Windows::Foundation::PropertyType::String&&text.GetString()==caption)return button;
    }
    for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(auto result=FindButton(X::Media::VisualTreeHelper::GetChild(parent,i),caption))return result;
    return nullptr;
}
int wmain(int argc,wchar_t**argv)
{
    const wchar_t* stage=L"fixture isolation";
    try
    {
        Require(argc>1,L"Owned fixture root required");
        const auto work=std::filesystem::absolute(argv[1]).lexically_normal();
        const auto fixture=work/L"portable";
        wchar_t exePath[32768]{};Require(GetModuleFileNameW(nullptr,exePath,32768)>0,L"Read fixture executable path");
        Require(std::filesystem::path{exePath}.parent_path()==fixture,L"Probe executable inside owned fixture");
        Require(std::filesystem::is_regular_file(fixture/L".portable"),L"Owned portable marker exists");
        Require((GetFileAttributesW(fixture.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)==0,L"Owned fixture is not redirected");
        UINT32 packageLength=0;Require(GetCurrentPackageFullName(&packageLength,nullptr)==APPMODEL_ERROR_NO_PACKAGE,L"Probe is unpackaged");
        Require(std::filesystem::is_regular_file(work/L"synthetic-user"/L".ssh"/L"config"),L"Synthetic user config exists");
        Require(std::filesystem::is_regular_file(work/L"synthetic-system"/L"ssh"/L"ssh_config"),L"Synthetic system config exists");
        winrt::check_bool(SetEnvironmentVariableW(L"USERPROFILE",(work/L"synthetic-user").c_str()));
        winrt::check_bool(SetEnvironmentVariableW(L"ProgramData",(work/L"synthetic-system").c_str()));
        winrt::check_bool(SetEnvironmentVariableW(L"ProgramFiles",(work/L"synthetic-programs").c_str()));
        winrt::check_bool(SetEnvironmentVariableW(L"ProgramFiles(x86)",(work/L"synthetic-programs").c_str()));
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
        auto model=LoadLibraryExW((fixture/L"Microsoft.Terminal.Settings.Model.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        Require(model!=nullptr,L"Load owned model DLL");
        Require(Settings::IsPortableMode(),L"Model uses portable isolation");
        Require(std::filesystem::path{Settings::SettingsDirectory().c_str()}==fixture/L"settings",L"Model state is owned fixture/settings");
        stage=L"XAML app";auto app=winrt::make<ProbeApp>();
        for(auto const* component:{L"Microsoft.Terminal.Settings.Model.dll",L"Microsoft.Terminal.Settings.Editor.dll",L"TerminalApp.dll",L"Microsoft.UI.Xaml.dll"})
        {
            const auto module=GetModuleHandleW(component);Require(module!=nullptr,L"Production fixture component is loaded");
            wchar_t loaded[32768]{};Require(GetModuleFileNameW(module,loaded,32768)>0,L"Read loaded component path");
            Require(std::filesystem::path{loaded}.parent_path()==fixture,L"Production UI components resolve only inside the owned fixture");
        }
        int failures=0;
        app.UnhandledException([&](auto const&,auto const& args){++failures;std::wcerr<<L"UNHANDLED stage="<<stage<<L" "<<args.Message().c_str()<<std::endl;args.Handled(true);});
        // LoadDefaults reads the compiled inbox only: no user settings or host
        // discovery. Its built-in schemes are required by the real profile UI.
        const Settings settings=Settings::LoadDefaults();
        const auto sourceProfileCount=settings.AllProfiles().Size();
        Require(sourceProfileCount>0&&settings.GlobalSettings().ColorSchemes().Size()>0,L"Compiled inbox supplies complete profile-page defaults");
        const auto before=settings.DraftComparisonJson();
        stage=L"projected production scan API";
        const auto candidates=settings.ScanSshProfiles();
        std::wcout<<L"SERVICE count="<<candidates.Size()<<L" inspectableIterable="<<static_cast<bool>(candidates.try_as<winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>>())<<std::endl;
        Require(candidates.Size()==2,L"Actual production scan reads only two synthetic aliases");
        for(auto const& candidate:candidates)std::wcout<<L"CANDIDATE "<<candidate.Name().c_str()<<std::endl;
        const bool observe=argc>2&&std::wstring_view{argv[2]}==L"observe";
        if(observe)
        {
            try{X::Controls::ListView direct;direct.ItemsSource(candidates);std::wcout<<L"DIRECT typed ItemsSource accepted"<<std::endl;}
            catch(winrt::hresult_error const& error){std::wcout<<L"DIRECT typed ItemsSource HRESULT=0x"<<std::hex<<static_cast<uint32_t>(error.code().value)<<L" "<<error.message().c_str()<<std::dec<<std::endl;}
        }
        namespace ScanUi=Microsoft::Terminal::Backslash::SshScanUi;
        const auto rows=ScanUi::Rows(candidates);
        Require(static_cast<bool>(rows.try_as<winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>>()),L"Adapter exposes the XAML iterable boundary");
        X::Controls::ListView direct;direct.ItemsSource(rows);Require(direct.Items().Size()==2,L"XAML accepts the actual candidate adapter");
        const auto diagnostic=ScanUi::Failure(L"displaying scan results",winrt::hresult_error(E_INVALIDARG,L"synthetic binding detail"));
        Require(diagnostic.starts_with(L"Couldn't scan SSH connections.")&&std::wstring_view{diagnostic}.find(L"0x80070057")!=std::wstring_view::npos&&std::wstring_view{diagnostic}.find(L"displaying scan results")!=std::wstring_view::npos&&std::wstring_view{diagnostic}.find(L"synthetic binding detail")!=std::wstring_view::npos,L"HRESULT diagnostics include friendly text, phase, code and detail");
        Require(std::wstring_view{ScanUi::Failure(L"reading SSH connection names",std::invalid_argument{"synthetic config issue"})}.find(L"synthetic config issue")!=std::wstring_view::npos,L"Standard exception detail is retained");
        stage=L"production Add Profile navigation";
        auto hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned isolated SSH Scan regression",WS_OVERLAPPEDWINDOW,-32000,-32000,1050,850,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Require(hwnd!=nullptr,L"Owned offscreen host created");
        E::MainPage owner{settings};owner.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
        X::Hosting::DesktopWindowXamlSource island;auto native=island.as<IDesktopWindowXamlSourceNative>();winrt::check_hresult(native->AttachToWindow(hwnd));island.Content(owner);
        HWND child{};winrt::check_hresult(native->get_WindowHandle(&child));SetWindowPos(child,nullptr,0,0,1024,768,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        owner.Measure({1024,768});owner.Arrange({0,0,1024,768});owner.UpdateLayout();Pump(250);
        Require(owner.Breadcrumbs().Size()>0,L"Production owner initialized navigation");
        auto crumb=owner.Breadcrumbs().GetAt(0).as<E::Breadcrumb>();crumb.Tag(winrt::box_value(L"AddProfile"));crumb.SubPage(E::BreadcrumbSubPage::None);
        owner.Breadcrumbs().Append(crumb);owner.UpdateLayout();Pump(100);
        auto button=FindInvokableBreadcrumb(owner.FindName(L"NavigationBreadcrumbBar").as<X::DependencyObject>());Require(static_cast<bool>(button),L"Owned breadcrumb button exists");
        winrt::Microsoft::UI::Xaml::Automation::Peers::BreadcrumbBarItemAutomationPeer navigationPeer{button};navigationPeer.as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
        auto frame=owner.FindName(L"contentFrame").as<X::Controls::Frame>();
        const auto page=frame.Content().as<E::AddProfile>();
        const auto scan=FindButton(page,L"Scan this computer's SSH connections");Require(static_cast<bool>(scan),L"Actual production Scan button found");
        stage=L"invoke production Scan button";
        X::Automation::Peers::ButtonAutomationPeer scanPeer{scan};scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(200);owner.UpdateLayout();
        const auto list=page.FindName(L"SshCandidates").as<X::Controls::ListView>();
        const auto status=page.FindName(L"SshScanStatus").as<X::Controls::TextBlock>();
        const auto import=page.FindName(L"ImportSelectedSsh").as<X::Controls::Button>();
        std::wcout<<L"BUTTON items="<<list.Items().Size()<<L" importEnabled="<<import.IsEnabled()<<L" status="<<status.Text().c_str()<<std::endl;
        Require(settings.DraftComparisonJson()==before,L"Scan has not imported or persisted profiles");
        Require(page.State().Settings().AllProfiles().Size()==sourceProfileCount,L"Scan candidates stay outside Settings profiles");
        if(!observe)
        {
            Require(list.Items().Size()==2&&import.IsEnabled(),L"Production Scan displays two selectable candidates");
            for(auto const& item:list.Items())
            {
                const auto profile=item.as<winrt::Microsoft::Terminal::Settings::Model::Profile>();
                Require(static_cast<bool>(FindLabel(list,profile.Name())),L"Production typed item template renders candidate Name");
            }
            scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
            Require(list.Items().Size()==2,L"Repeated Scan replaces rows without duplicates or binding errors");
            std::wcout<<L"PASS repeated scan"<<std::endl;
            stage=L"empty production scan";
            std::ofstream{work/L"synthetic-user"/L".ssh"/L"config",std::ios::trunc};
            std::ofstream{work/L"synthetic-system"/L"ssh"/L"ssh_config",std::ios::trunc};
            scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
            Require(list.Items().Size()==0&&!import.IsEnabled(),L"Empty Scan clears previous candidates and disables Import");
            Require(status.Text().starts_with(L"No new SSH connections"),L"Empty Scan is a normal result, not an API error");
            std::wcout<<L"PASS empty scan"<<std::endl; stage=L"scan recovery";
            std::ofstream{work/L"synthetic-user"/L".ssh"/L"config"}<<"Host scan-fixture-one\nHostName never-connect.invalid\n";
            std::ofstream{work/L"synthetic-system"/L"ssh"/L"ssh_config"}<<"Host scan-fixture-two\nHostName never-connect.invalid\n";
            scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
            Require(list.Items().Size()==2&&import.IsEnabled(),L"Scan recovers normally after empty results");
            std::wcout<<L"PASS recovery scan"<<std::endl;
            stage=L"handled scan failure";
            // Deliberately append a detached scan DTO to this disposable draft.
            // It has no generated launch parent, so the real model rejects its
            // serialization. This exercises the production Scan error handler
            // without corrupt files, private model hooks or real SSH access.
            const auto detached=list.Items().GetAt(0).as<winrt::Microsoft::Terminal::Settings::Model::Profile>();
            const auto draftForFailure=page.State().Settings();
            draftForFailure.AllProfiles().Append(detached);
            const auto invalidDraft=draftForFailure.DraftComparisonJson();
            scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
            std::wcout<<L"HANDLED failure status="<<status.Text().c_str()<<std::endl;
            Require(list.Items().Size()==0&&!import.IsEnabled(),L"Handled scan failure clears stale rows and disables Import");
            Require(status.Text().starts_with(L"Couldn't scan SSH connections.")&&std::wstring_view{status.Text()}.find(L"Details: reading SSH connection names")!=std::wstring_view::npos&&std::wstring_view{status.Text()}.find(L"0x")!=std::wstring_view::npos,L"Actual Scan handler displays friendly phase/error details");
            Require(draftForFailure.DraftComparisonJson()==invalidDraft,L"Handled Scan failure preserves all draft edits");
            draftForFailure.AllProfiles().RemoveAt(draftForFailure.AllProfiles().Size()-1);
            scanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);owner.UpdateLayout();
            Require(list.Items().Size()==2&&import.IsEnabled(),L"Scan recovers after a handled error");
            std::wcout<<L"PASS handled failure and recovery"<<std::endl;
            stage=L"production selected import";
            const auto selected=list.Items().GetAt(0).as<winrt::Microsoft::Terminal::Settings::Model::Profile>();
            list.SelectedItems().Append(selected);Require(list.SelectedItems().Size()==1,L"Exactly one real candidate selected");
            X::Automation::Peers::ButtonAutomationPeer importPeer{import};importPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(200);owner.UpdateLayout();
            const auto draft=page.State().Settings();
            Require(draft.AllProfiles().Size()==sourceProfileCount+1&&settings.AllProfiles().Size()==sourceProfileCount,L"Actual Import button adds only one selected row to the draft");
            Require(!draft.FindProfile(selected.Guid()),L"Scan candidate does not become an extra transient launch row");
            unsigned owned=0;for(auto const& profile:draft.AllProfiles())if(profile.Source()==L"Backslash.Terminal.SSH"){++owned;Require(profile.HasSshShellSelector(),L"Imported alias retains its generated launch parent");}
            Require(owned==1,L"Exactly one durable alias draft owner created");
            Require(!std::filesystem::exists(fixture/L"settings"/L"settings.json"),L"Scan/import draft never writes settings.json");

            stage=L"two-selected production import";
            // A fresh Settings owner starts with no prior draft imports. It
            // still uses the actual production page, callbacks and model API.
            E::MainPage multiOwner{settings};multiOwner.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));island.Content(multiOwner);
            multiOwner.Measure({1024,768});multiOwner.Arrange({0,0,1024,768});multiOwner.UpdateLayout();Pump(200);
            const auto navigateAdd=[&](E::MainPage const& root) {
                Require(root.Breadcrumbs().Size()>0,L"Multi-case owner initialized navigation");
                auto destination=root.Breadcrumbs().GetAt(0).as<E::Breadcrumb>();destination.Tag(winrt::box_value(L"AddProfile"));destination.SubPage(E::BreadcrumbSubPage::None);
                root.Breadcrumbs().Append(destination);root.UpdateLayout();Pump(100);
                auto invoke=FindInvokableBreadcrumb(root.FindName(L"NavigationBreadcrumbBar").as<X::DependencyObject>());Require(static_cast<bool>(invoke),L"Multi-case navigation button exists");
                winrt::Microsoft::UI::Xaml::Automation::Peers::BreadcrumbBarItemAutomationPeer peer{invoke};peer.as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);root.UpdateLayout();
                return root.FindName(L"contentFrame").as<X::Controls::Frame>().Content().as<E::AddProfile>();
            };
            auto multiPage=navigateAdd(multiOwner);
            const auto multiState=multiPage.State();
            const auto multiBefore=multiState.Settings().DraftComparisonJson();
            const auto multiList=multiPage.FindName(L"SshCandidates").as<X::Controls::ListView>();
            const auto multiImport=multiPage.FindName(L"ImportSelectedSsh").as<X::Controls::Button>();
            X::Automation::Peers::ButtonAutomationPeer multiScanPeer{FindButton(multiPage,L"Scan this computer's SSH connections")};
            const auto scanMulti=[&] { multiScanPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);multiOwner.UpdateLayout(); };
            scanMulti();Require(multiList.Items().Size()==2,L"Fresh production owner scans two candidates");
            for(auto const& item:multiList.Items())multiList.SelectedItems().Append(item);
            Pump(50);Require(multiList.SelectedItems().Size()==2,L"Two actual candidate objects selected");
            scanMulti();
            Require(multiList.Items().Size()==2&&multiList.SelectedItems().Size()==0,L"Rescan clears both stale selections and refreshes rows");
            X::Automation::Peers::ButtonAutomationPeer multiImportPeer{multiImport};
            multiImportPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);multiOwner.UpdateLayout();
            Require(multiState.Settings().DraftComparisonJson()==multiBefore,L"Import after rescan requires a fresh selection and adds nothing");
            Require(multiPage.FindName(L"SshScanStatus").as<X::Controls::TextBlock>().Text().starts_with(L"Select at least one"),L"Rescan does not retain invisible selected GUIDs");
            std::vector<winrt::guid> selectedIds;
            for(auto const& item:multiList.Items()){selectedIds.push_back(item.as<winrt::Microsoft::Terminal::Settings::Model::Profile>().Guid());multiList.SelectedItems().Append(item);}
            Pump(50);Require(multiList.SelectedItems().Size()==2,L"Both refreshed candidates selected for import");
            multiImportPeer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(250);multiOwner.UpdateLayout();
            const auto multiDraft=multiState.Settings();
            Require(multiDraft.AllProfiles().Size()==sourceProfileCount+2&&settings.AllProfiles().Size()==sourceProfileCount,L"Two selected candidates imported exactly once into only the draft");
            std::set<std::wstring> identities;
            std::set<winrt::guid> ownerIds;
            for(auto const& profile:multiDraft.AllProfiles())if(profile.Source()==L"Backslash.Terminal.SSH")
            {
                Require(ownerIds.insert(profile.Guid()).second,L"Multi-import owner GUIDs are unique");
                identities.insert(std::wstring{profile.ConnectionIdentity(profile.Commandline())});
            }
            const std::set<std::wstring> expectedIdentities{L"SSH scan-fixture-one",L"SSH scan-fixture-two"};
            Require(ownerIds.size()==2&&identities==expectedIdentities,L"Both intended SSH aliases have one durable owner");
            for(auto const& id:selectedIds)Require(!multiDraft.FindProfile(id),L"No extra transient profile accompanies a selected import");
            const auto records=winrt::Windows::Data::Json::JsonArray::Parse(multiDraft.GlobalSettings().WorkspaceSshConnectionsJson());
            Require(records.Size()==2,L"Exactly two SSH connection records created");
            std::set<std::wstring> aliases;
            for(auto const& value:records){const auto record=value.GetObject();Require(record.GetNamedString(L"mode")==L"hostAlias",L"Multi-import records are alias references");aliases.insert(std::wstring{record.GetNamedString(L"alias")});}
            Require(aliases==std::set<std::wstring>{L"scan-fixture-one",L"scan-fixture-two"},L"No unwanted or duplicate SSH record imported");
            auto afterMulti=navigateAdd(multiOwner);
            X::Automation::Peers::ButtonAutomationPeer afterScan{FindButton(afterMulti,L"Scan this computer's SSH connections")};afterScan.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(150);multiOwner.UpdateLayout();
            const auto afterList=afterMulti.FindName(L"SshCandidates").as<X::Controls::ListView>();
            Require(afterList.Items().Size()==0&&afterList.SelectedItems().Size()==0&&!afterMulti.FindName(L"ImportSelectedSsh").as<X::Controls::Button>().IsEnabled(),L"Rescan suppresses imported aliases and retains no stale selection");
            Require(multiDraft.AllProfiles().Size()==sourceProfileCount+2,L"Post-import scan adds no duplicate owner");
            Require(!std::filesystem::exists(fixture/L"settings"/L"settings.json"),L"Multi-import remains an unsaved fixture draft");
            std::wcout<<L"PASS multi: selected=2, owners=2, records=2, stale selections after rescan=0, post-import candidates=0"<<std::endl;

            Require(failures==0,L"No unhandled XAML error");
            std::wcout<<L"PASS: projected production scan, XAML rows/template, repeated/empty/error/recovery scans, one selected import, clean source and HRESULT diagnostics"<<std::endl;
        }
        else std::wcout<<L"OBSERVATION only: service/direct binding/button boundary captured"<<std::endl;
        island.Content(nullptr);island.Close();DestroyWindow(hwnd);ExitProcess(0);
    }
    catch(winrt::hresult_error const& error){std::wcerr<<L"stage="<<stage<<L" HRESULT=0x"<<std::hex<<static_cast<uint32_t>(error.code().value)<<L" "<<error.message().c_str()<<std::endl;return 2;}
}
