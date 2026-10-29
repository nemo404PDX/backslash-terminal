// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
// Production Settings page exercised in the owned offscreen fixture. Follow-up
// Save/reload and discovery cases use only its synthetic portable settings/config.
// No connection, installation or real user-settings write is performed here.
static void TestTransferBackendsPage(X::Hosting::DesktopWindowXamlSource const& island, HWND hwnd,
                                    winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings const& defaults)
{
    const auto settings = defaults.Copy();
    const auto profile = settings.CreateNewProfile();
    profile.Name(L"Synthetic saved SSH profile"); profile.Source(L"Backslash.Terminal.SSH"); profile.Hidden(true);
    const auto sourceJson = settings.GlobalSettings().TransferBackendsJson();
    E::MainPage page{settings}; page.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
    island.Content(page); page.Measure({1024,768}); page.Arrange({0,0,1024,768}); page.UpdateLayout(); Pump(100);
    const auto content = [&] { return page.FindName(L"contentFrame").as<X::Controls::Frame>().Content().as<X::Controls::StackPanel>(); };
    const auto toggle = [&](size_t i) { return content().Children().GetAt(static_cast<uint32_t>(i+2)).as<X::Controls::Border>().Child().as<X::Controls::StackPanel>().Children().GetAt(0).as<X::Controls::ToggleSwitch>(); };
    const auto selector = [&] { return content().Children().GetAt(2).as<X::Controls::Border>().Child().as<X::Controls::StackPanel>().Children().GetAt(1).as<X::Controls::ComboBox>(); };
    page.OpenTransferBackends(); page.UpdateLayout(); Pump(100);
    Require(page.Breadcrumbs().Size()==2,L"Settings Transfers Backends has one shared breadcrumb route");
    Require(selector().Items().Size()==2,L"Hidden owned SSH profile is selectable without scanning");
    for(size_t i=0;i<4;++i)Require(!toggle(i).IsOn(),L"Backend defaults disabled");
    selector().SelectedIndex(1); toggle(0).IsOn(true); Pump(50);
    for(int n=0;n<3;++n)
    {
        page.OpenTransferBackends(); page.UpdateLayout(); Pump(30);
        Require(toggle(0).IsOn()&&selector().SelectedIndex()==1,L"Repeated page navigation preserves live backend draft");
        Require(page.Breadcrumbs().Size()==2,L"Repeated navigation does not accumulate breadcrumbs");
    }
    Require(settings.GlobalSettings().TransferBackendsJson()==sourceJson,L"Backend edits remain isolated from source settings");
    X::Automation::Peers::ButtonAutomationPeer reset{page.FindName(L"ResetButton").as<X::Controls::Button>()};
    reset.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(50);page.UpdateLayout();
    Require(!toggle(0).IsOn()&&selector().SelectedIndex()==0,L"Discard restores backend enablement and profile reference");
    // Unsupported loaded data is retained and displayed without editable controls.
    auto malformed=winrt::Windows::Data::Json::JsonObject::Parse(sourceJson); malformed.SetNamedValue(L"version",winrt::Windows::Data::Json::JsonValue::CreateNumberValue(2));
    // The setter rejects invalid data; loaded unknown versions are covered by model tests.
    bool rejected=false;try{settings.GlobalSettings().TransferBackendsJson(malformed.Stringify());}catch(...){rejected=true;}
    Require(rejected,L"Projected settings setter rejects unsupported backend version");
    std::wcout<<L"PASS Transfers Backends: production navigation, hidden owned SSH reference, draft isolation, repeated flow, discard and invalid version"<<std::endl;
}

static X::Controls::StackPanel BackendPanel(E::MainPage const& page)
{
    return page.FindName(L"contentFrame").as<X::Controls::Frame>().Content().as<X::Controls::StackPanel>();
}
static X::Controls::StackPanel BackendSftpCard(E::MainPage const& page)
{
    return BackendPanel(page).Children().GetAt(2).as<X::Controls::Border>().Child().as<X::Controls::StackPanel>();
}
static X::Controls::ComboBox BackendSelector(E::MainPage const& page)
{
    return BackendSftpCard(page).Children().GetAt(1).as<X::Controls::ComboBox>();
}
static void HostBackendPage(X::Hosting::DesktopWindowXamlSource const& island, E::MainPage const& page)
{
    island.Content(page);page.Measure({1024,768});page.Arrange({0,0,1024,768});page.UpdateLayout();Pump(75);
    page.OpenTransferBackends();page.UpdateLayout();Pump(50);
}
static void InvokeBackendSave(E::MainPage const& page)
{
    X::Automation::Peers::ButtonAutomationPeer save{page.FindName(L"SaveButton").as<X::Controls::Button>()};
    save.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();
    Pump(150);page.UpdateLayout();
    Require(!page.CanMoveSettingsEditor(),L"Native Save completed and awaits the normal settings reload");
}
static void WriteBackendFixture(std::filesystem::path const& fixture, winrt::Windows::Data::Json::JsonObject const& value)
{
    using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
    Require(std::filesystem::path{Settings::SettingsPath().c_str()}==fixture/L"settings"/L"settings.json",L"Save target remains the owned fixture settings file");
    std::filesystem::create_directories(fixture/L"settings");
    std::ofstream stream{fixture/L"settings"/L"settings.json",std::ios::binary|std::ios::trunc};
    const auto bytes=winrt::to_string(value.Stringify());stream.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));stream.close();
    Require(static_cast<bool>(stream),L"Synthetic settings fixture written");
}
static winrt::Windows::Data::Json::JsonObject ReadBackendFixture(std::filesystem::path const& fixture)
{
    std::ifstream stream{fixture/L"settings"/L"settings.json",std::ios::binary};
    const std::string bytes{std::istreambuf_iterator<char>{stream},std::istreambuf_iterator<char>{}};
    return winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(bytes));
}
static void TestTransferBackendAcceptance(X::Hosting::DesktopWindowXamlSource const& island, HWND hwnd,
                                        winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings const& defaults,
                                        std::filesystem::path const& fixture)
{
    using Settings=winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings;
    namespace J=winrt::Windows::Data::Json;
    const auto missing=L"{22222222-3333-4444-aaaa-bbbbbbbbbbbb}";
    auto seed=J::JsonObject::Parse(defaults.ExplicitSettingsJson());
    auto profiles=J::JsonObject::Parse(LR"({"list":[{"guid":"{77777777-8888-4999-abcd-123456789012}","name":"Synthetic fixture profile","commandline":"cmd.exe /d","hidden":false}]})");
    seed.SetNamedValue(L"profiles",profiles);
    seed.SetNamedValue(L"defaultProfile",J::JsonValue::CreateStringValue(L"{77777777-8888-4999-abcd-123456789012}"));
    auto disabled=J::JsonArray{};
    for(auto source:{L"Windows.Terminal.Azure",L"Windows.Terminal.PowershellCore",L"Windows.Terminal.Wsl",L"Windows.Terminal.VisualStudio"})disabled.Append(J::JsonValue::CreateStringValue(source));
    seed.SetNamedValue(L"disabledProfileSources",disabled);
    auto backends=J::JsonObject::Parse(defaults.GlobalSettings().TransferBackendsJson());
    backends.GetNamedObject(L"sftp").SetNamedValue(L"enabled",J::JsonValue::CreateBooleanValue(true));
    backends.GetNamedObject(L"sftp").SetNamedValue(L"profile",J::JsonValue::CreateStringValue(missing));
    seed.SetNamedValue(L"backslash.transferBackends",backends);
    Settings::HostSshDiscoveryEnabled(false);
    WriteBackendFixture(fixture,seed);(void)Settings::LoadAll();
    // Omitting an already-seen inbox profile exercises the real Deleted state.
    WriteBackendFixture(fixture,seed);const auto settings=Settings::LoadAll();
    winrt::Microsoft::Terminal::Settings::Model::Profile deleted{nullptr};
    for(auto const& profile:settings.AllProfiles())if(profile.Deleted()){deleted=profile;break;}
    Require(static_cast<bool>(deleted),L"Loader produced a genuinely deleted fixture profile");
    const auto deletedSource=deleted.Source();
    deleted.Source(L"Windows.Terminal.SSH");deleted.Name(L"Deleted SSH fixture");
    Require(deleted.Deleted()&&!deleted.IsTransientHostSsh(),L"Deleted SSH case is independent of transient filtering");
    // Explicit discovery reads only the harness's synthetic config and inert client placeholder.
    const auto candidates=settings.ScanSshProfiles();Require(candidates.Size()>0,L"Synthetic transient discovery fixture exists");
    const auto transient=candidates.GetAt(0);Require(transient.IsTransientHostSsh(),L"Discovery fixture has a genuine transient marker");
    const auto replacement=settings.RestoreSshProfile(transient.Guid());Require(static_cast<bool>(replacement),L"Owned replacement profile imported in fixture draft");
    replacement.Hidden(true);settings.AllProfiles().Append(transient);
    Require(!replacement.Deleted()&&!replacement.IsTransientHostSsh(),L"Replacement is owned and hidden, not transient/deleted");
    const auto original=settings.GlobalSettings().TransferBackendsJson();
    E::MainPage page{settings};page.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));HostBackendPage(island,page);
    Require(BackendSelector(page).Items().Size()==3,L"Selector contains placeholder, hidden owned replacement and missing reference only; deleted/transient profiles excluded");
    Require(BackendSelector(page).SelectedIndex()==2,L"Missing saved GUID remains visibly selected");
    const auto missingText=winrt::unbox_value<winrt::hstring>(BackendSelector(page).Items().GetAt(2).as<X::Controls::ComboBoxItem>().Content());
    Require(std::wstring_view{missingText}.starts_with(L"Saved profile missing"),L"Missing reference has an explanatory label");
    Require(std::wstring_view{BackendSftpCard(page).Children().GetAt(2).as<X::Controls::TextBlock>().Text()}.starts_with(L"Unconfigured"),L"Missing profile never produces a ready backend");
    BackendSelector(page).SelectedIndex(0);page.OpenTransferBackends();page.UpdateLayout();Pump(30);
    Require(BackendSelector(page).Items().Size()==2&&BackendSelector(page).SelectedIndex()==0,L"Clearing missing reference removes stale missing row on repeated navigation");
    BackendSelector(page).SelectedIndex(1);page.OpenTransferBackends();page.UpdateLayout();Pump(30);
    Require(BackendSelector(page).SelectedIndex()==1,L"Missing reference can be replaced with an existing owned profile");
    Require(settings.GlobalSettings().TransferBackendsJson()==original,L"Clear/replace remains a draft until native Save");
    // Filter probes above deliberately injected a detached transient candidate.
    // Restore the ordinary owned model before testing Save; that candidate is not
    // a profile installed by the real import workflow and must not be adopted.
    for(uint32_t i=0;i<settings.AllProfiles().Size();++i)if(settings.AllProfiles().GetAt(i).Guid()==transient.Guid()){settings.AllProfiles().RemoveAt(i);break;}
    deleted.Source(deletedSource);
    E::MainPage savePage{settings};savePage.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));HostBackendPage(island,savePage);
    Require(BackendSelector(savePage).SelectedIndex()==2,L"Save case starts with a preserved missing reference");
    BackendSelector(savePage).SelectedIndex(1);InvokeBackendSave(savePage);
    const auto reloaded=Settings::LoadAll();const auto persisted=J::JsonObject::Parse(reloaded.GlobalSettings().TransferBackendsJson()).GetNamedObject(L"sftp");
    Require(persisted.GetNamedBoolean(L"enabled")&&winrt::guid{persisted.GetNamedString(L"profile")}==replacement.Guid(),L"Native Save then disk reload preserves enablement and replacement GUID");
    Require(!reloaded.FindProfile(transient.Guid()),L"Transient discovery was not persisted by native Save");
    E::MainPage reopened{reloaded};reopened.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));HostBackendPage(island,reopened);
    Require(BackendSelector(reopened).SelectedIndex()==1,L"New native editor resolves the saved owned profile after disk reload");
    Require(std::wstring_view{BackendSftpCard(reopened).Children().GetAt(2).as<X::Controls::TextBlock>().Text()}.starts_with(L"Unavailable"),L"Configured reference remains unavailable without a real engine");
    std::wcout<<L"PASS Transfers acceptance: missing GUID; genuine deleted/transient filtering; clear/replace; native Save and disk reload"<<std::endl;
    for(int variant=0;variant<2;++variant)
    {
        auto document=ReadBackendFixture(fixture);auto malformed=J::JsonObject::Parse(reloaded.GlobalSettings().TransferBackendsJson());
        malformed.SetNamedValue(L"futureField",J::JsonValue::CreateStringValue(L"retain exact unknown configuration"));
        if(variant==0)malformed.SetNamedValue(L"version",J::JsonValue::CreateNumberValue(99));
        else malformed.GetNamedObject(L"sftp").SetNamedValue(L"enabled",J::JsonValue::CreateStringValue(L"invalid boolean"));
        document.SetNamedValue(L"backslash.transferBackends",malformed);WriteBackendFixture(fixture,document);
        const auto loaded=Settings::LoadAll();const auto preserved=loaded.GlobalSettings().TransferBackendsJson();
        loaded.GlobalSettings().InitialCols(121+variant); // Unrelated preference; Save still uses the production button.
        E::MainPage errorPage{loaded};errorPage.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));HostBackendPage(island,errorPage);
        const auto panel=BackendPanel(errorPage);Require(panel.Children().Size()==3,L"Malformed configuration renders heading, notice and error only");
        for(auto const& child:panel.Children())Require(static_cast<bool>(child.try_as<X::Controls::TextBlock>()),L"Error-only backend panel has no editable controls");
        Require(std::wstring_view{panel.Children().GetAt(2).as<X::Controls::TextBlock>().Text()}.starts_with(L"Backend settings are invalid"),L"Preserved malformed data has a visible error explanation");
        InvokeBackendSave(errorPage);const auto after=Settings::LoadAll();
        Require(after.GlobalSettings().InitialCols()==121+variant,L"Native unrelated Save reached disk");
        Require(after.GlobalSettings().TransferBackendsJson()==preserved,L"Malformed backend configuration survives unrelated native Save and disk reload unchanged");
        Require(ReadBackendFixture(fixture).GetNamedObject(L"backslash.transferBackends").GetNamedString(L"futureField")==L"retain exact unknown configuration",L"Unknown fields remain on disk");
    }
    std::wcout<<L"PASS Transfers acceptance: loaded malformed/unsupported configurations; error-only rendering; preservation during unrelated native Save/reload"<<std::endl;
}
