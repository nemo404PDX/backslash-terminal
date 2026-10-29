// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <winrt/Windows.UI.Xaml.Navigation.h>
#include <memory>

// Synthetic navigation contracts: no dialog, discovery, import or network calls.
struct NavigationProbeHost : winrt::implements<NavigationProbeHost, E::IHostedInWindow>
{
    explicit NavigationProbeHost(uint64_t hwnd) : _hwnd{hwnd} {}
    bool TryPropagateHostingWindow(winrt::Windows::Foundation::IInspectable const&) { return false; }
    uint64_t GetHostingWindow() const { return _hwnd; }
    bool SetHostSshDiscovery(bool) { throw winrt::hresult_illegal_method_call(); }
    void ImportSshConnection(winrt::guid const&) { throw winrt::hresult_illegal_method_call(); }
    void OpenBackgroundSettings() { throw winrt::hresult_illegal_method_call(); }
private:
    uint64_t _hwnd;
};
struct NavigationProbeArgs : winrt::implements<NavigationProbeArgs, E::INavigateToPageArgs>
{
    NavigationProbeArgs(E::NewTabMenuViewModel const& model, E::IHostedInWindow const& host) : _model{model}, _host{host} {}
    winrt::Windows::Foundation::IInspectable ViewModel() const { return _model; }
    E::IHostedInWindow WindowRoot() const { return _host.get(); }
    winrt::hstring ElementToFocus() const { return {}; }
private:
    E::NewTabMenuViewModel _model;
    winrt::weak_ref<E::IHostedInWindow> _host;
};
static void TestNewTabMenuNavigation(X::Hosting::DesktopWindowXamlSource const& island, HWND hwnd,
                                     winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings const& settings)
{
    using ModelFolder = winrt::Microsoft::Terminal::Settings::Model::FolderEntry;
    E::FolderEntryViewModel folderA{ModelFolder{L"folder-A"}, settings}, folderB{ModelFolder{L"folder-B"}, settings};
    E::FolderEntryViewModel rowA{ModelFolder{L"row-A"}, settings}, rowB{ModelFolder{L"row-B"}, settings};
    folderA.Entries().Append(rowA); folderB.Entries().Append(rowB);
    E::NewTabMenuViewModel modelA{settings.Copy()}, modelB{settings.Copy()};
    modelA.CurrentFolder(folderA); modelB.CurrentFolder(folderB);
    auto hostA=winrt::make<NavigationProbeHost>(reinterpret_cast<uint64_t>(hwnd));
    auto hostB=winrt::make<NavigationProbeHost>(reinterpret_cast<uint64_t>(hwnd));
    struct Notifications { int models=0, roots=0; };
    std::vector<std::shared_ptr<Notifications>> observed;
    std::vector<X::Data::INotifyPropertyChanged::PropertyChanged_revoker> subscriptions;
    X::Controls::Frame frame;
    // Frame.Navigated exposes each production page before OnNavigatedTo assigns
    // its parameters. Observe real navigation notifications, not manual setters.
    auto navigation=frame.Navigated(winrt::auto_revoke,[&](auto&&, X::Navigation::NavigationEventArgs const& args) {
        if(auto page=args.Content().try_as<E::NewTabMenu>())
        {
            auto counts=std::make_shared<Notifications>(); observed.push_back(counts);
            subscriptions.push_back(page.as<X::Data::INotifyPropertyChanged>().PropertyChanged(winrt::auto_revoke,[counts](auto&&, auto const& changed) {
                if(changed.PropertyName()==L"ViewModel") ++counts->models;
                if(changed.PropertyName()==L"WindowRoot") ++counts->roots;
            }));
        }
    });
    island.Content(frame);
    auto layout=[&] { frame.Measure({1024,768}); frame.Arrange({0,0,1024,768}); frame.UpdateLayout(); Pump(100); frame.UpdateLayout(); };
    auto navigate=[&](auto const& model, auto const& host) {
        Require(frame.Navigate(winrt::xaml_typename<E::NewTabMenu>(),winrt::make<NavigationProbeArgs>(model,host)),L"Production NewTabMenu navigation succeeds"); layout();
        Require(observed.back()->models==1 && observed.back()->roots==1,L"Actual navigation raises ViewModel and WindowRoot exactly once");
    };
    navigate(modelA,hostA);
    auto page=frame.Content().as<E::NewTabMenu>();
    auto list=page.FindName(L"NewTabMenuListView").as<X::Controls::ListView>();
    Require(page.ViewModel()==modelA && page.WindowRoot()==hostA,L"Initial A model/root assigned");
    Require(list.Items().Size()==1 && list.Items().GetAt(0).as<E::FolderEntryViewModel>()==rowA && FindLabel(page,L"row-A"),L"A ItemsSource and rendered row are production-bound");
    auto sourceA=list.ItemsSource();
    navigate(modelB,hostB);
    page=frame.Content().as<E::NewTabMenu>(); list=page.FindName(L"NewTabMenuListView").as<X::Controls::ListView>();
    Require(page.ViewModel()==modelB && page.WindowRoot()==hostB,L"B navigation reads distinct model/root identities");
    Require(list.ItemsSource()!=sourceA && list.Items().Size()==1 && list.Items().GetAt(0).as<E::FolderEntryViewModel>()==rowB,L"B replaces actual bound ItemsSource");
    Require(FindLabel(page,L"row-B") && !FindLabel(page,L"row-A"),L"B replaces rendered A row");
    rowA.Name(L"old-A-mutated");
    modelA.CurrentView().Append(E::FolderEntryViewModel{ModelFolder{L"old-A-only"},settings});
    modelA.CurrentFolderName(L"old-A-folder-mutated"); layout();
    Require(observed.back()->models==1 && observed.back()->roots==1 && list.Items().Size()==1 && FindLabel(page,L"row-B") && !FindLabel(page,L"old-A-mutated") && !FindLabel(page,L"old-A-only"),L"Old A values and collection cannot affect B");
    rowB.Name(L"live-B-mutated"); layout();
    Require(FindLabel(page,L"live-B-mutated") && !FindLabel(page,L"row-B"),L"Current B still updates rendered binding");
    for(int cycle=0;cycle<3;++cycle)
    {
        navigate(modelA,hostA); page=frame.Content().as<E::NewTabMenu>(); list=page.FindName(L"NewTabMenuListView").as<X::Controls::ListView>();
        Require(page.ViewModel()==modelA && page.WindowRoot()==hostA && list.Items().Size()==2 && FindLabel(page,L"old-A-mutated"),L"Repeated A navigation renders current A");
        navigate(modelB,hostB); page=frame.Content().as<E::NewTabMenu>(); list=page.FindName(L"NewTabMenuListView").as<X::Controls::ListView>();
        Require(page.ViewModel()==modelB && page.WindowRoot()==hostB && list.Items().Size()==1 && FindLabel(page,L"live-B-mutated"),L"Repeated B navigation renders only B");
    }
    Require(observed.size()==8,L"Eight actual A/B navigations observed");
    navigation.revoke(); for(auto& subscription:subscriptions) subscription.revoke(); subscriptions.clear();
    Require(frame.Navigate(winrt::xaml_typename<X::Controls::Page>()),L"Navigate away for teardown");
    frame.BackStack().Clear(); frame.ForwardStack().Clear(); island.Content(nullptr);
    sourceA=nullptr; list=nullptr; page=nullptr; frame=nullptr; Pump(100);
    rowA.Name(L"A-after-teardown"); rowB.Name(L"B-after-teardown"); modelA.CurrentView().Clear(); modelB.CurrentView().Clear(); Pump(100);
    for(auto const& counts:observed) Require(counts->models==1 && counts->roots==1,L"Teardown revokes all page observers");
    std::wcout<<L"PASS NewTabMenu A/B navigation: 8 real navigations, model/root events=8/8, actual ItemsSource/render replacement, old-A isolation, three repeated cycles and detached teardown"<<std::endl;
}
