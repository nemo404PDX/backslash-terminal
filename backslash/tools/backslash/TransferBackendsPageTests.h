// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
// Production Settings page exercised in the owned offscreen fixture. Follow-up
// Save/reload and discovery cases use only its synthetic portable settings/config.
// No connection, installation or real user-settings write is performed here.
static void TestTransferBackendsPage(X::Hosting::DesktopWindowXamlSource const& island, HWND hwnd,
                                    winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings const& defaults, const std::filesystem::path& work)
{
    const auto settings = defaults.Copy();
    const auto profile = settings.CreateNewProfile();
    profile.Name(L"Synthetic saved SSH profile"); profile.Source(L"Backslash.Terminal.SSH"); profile.Hidden(true);
    const auto sourceJson = settings.GlobalSettings().TransferBackendsJson();
    E::MainPage page{settings}; page.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
    island.Content(page); page.Measure({1024,768}); page.Arrange({0,0,1024,768}); page.UpdateLayout(); Pump(100);
    const auto menu=page.FindName(L"SettingsNav").as<winrt::Microsoft::UI::Xaml::Controls::NavigationView>();
    const auto findToggle=[](auto&& self,const X::DependencyObject& parent)->X::Controls::Button {
        if(const auto element=parent.try_as<X::FrameworkElement>();element&&element.Name()==L"TogglePaneButton")return element.as<X::Controls::Button>();
        for(int i=0;i<X::Media::VisualTreeHelper::GetChildrenCount(parent);++i)if(const auto found=self(self,X::Media::VisualTreeHelper::GetChild(parent,i)))return found;
        return nullptr;
    };
    const auto menuButton=findToggle(findToggle,menu);
    Require(static_cast<bool>(menuButton)&&menu.IsPaneToggleButtonVisible(),L"Settings uses the native hamburger inside its pane");
    Require(menuButton.UseSystemFocusVisuals(),L"Neutral Settings menu keeps system keyboard focus cues");
    for(int n=0;n<4;++n){
        const bool wasOpen=menu.IsPaneOpen();
        X::Automation::Peers::ButtonAutomationPeer peer{menuButton};
        peer.GetPattern(X::Automation::Peers::PatternInterface::Invoke).as<X::Automation::Provider::IInvokeProvider>().Invoke();Pump(100);
        Require(menu.IsPaneOpen()!=wasOpen,L"Neutral Settings hamburger toggles actual navigation pane repeatedly");
    }
    for(const auto theme:{X::ElementTheme::Dark,X::ElementTheme::Light}){
        page.RequestedTheme(theme);Pump(50);
        for(const auto state:{L"Normal",L"PointerOver",L"Pressed",L"Disabled"})
            Require(X::VisualStateManager::GoToState(menuButton,state,false),L"Neutral menu retains standard interaction states");
        X::VisualStateManager::GoToState(menuButton,L"Normal",false);
        menu.IsPaneOpen(false);Pump(300);CheckNavigationPane(menu,theme==X::ElementTheme::Dark);CaptureSettings(page,work/(theme==X::ElementTheme::Dark?L"settings-dark-collapsed.png":L"settings-light-collapsed.png"));
        menu.IsPaneOpen(true);Pump(300);CheckNavigationPane(menu,theme==X::ElementTheme::Dark);CaptureSettings(page,work/(theme==X::ElementTheme::Dark?L"settings-dark-expanded.png":L"settings-light-expanded.png"));
    }
    // Theme changes must use the production settings-update path. Merely
    // overriding RequestedTheme does not update the explicit Mica/body color.
    for(const auto name:{L"dark",L"light"}) {
        const auto themed=settings.Copy();
        themed.GlobalSettings().Theme(winrt::Microsoft::Terminal::Settings::Model::ThemePair{name});
        E::MainPage themedPage{themed};themedPage.SetHostingWindow(reinterpret_cast<uint64_t>(hwnd));
        island.Content(themedPage);themedPage.Width(680);themedPage.Height(500);themedPage.UpdateLayout();Pump(250);
        const auto themedMenu=themedPage.FindName(L"SettingsNav").as<Mux::NavigationView>();
        const bool dark=std::wstring_view{name}==L"dark";
        for(const bool open:{false,true}) {
            themedMenu.IsPaneOpen(open);themedPage.UpdateLayout();Pump(200);CheckNavigationPane(themedMenu,dark);
            const auto surface=FindNavigationSplit(themedMenu).Content().as<X::Controls::Grid>().Background().as<X::Media::SolidColorBrush>().Color();
            Require(dark?surface.R<80:surface.R>180,L"Configured Settings theme updates the actual content surface color");
            CaptureSettings(themedPage,work/(dark?(open?L"settings-configured-dark-expanded.png":L"settings-configured-dark-collapsed.png"):(open?L"settings-configured-light-expanded.png":L"settings-configured-light-collapsed.png")));
        }
        island.Content(page);Pump(100);
    }
    page.RequestedTheme(X::ElementTheme::Default);
    const auto startup=page.FindName(L"contentFrame").as<X::Controls::Frame>().Content().try_as<E::Launch>();
    Require(static_cast<bool>(startup),L"Settings initially displays actual Startup page");
    const auto explorerActions=startup.FindName(L"ExplorerActions").as<X::Controls::Grid>();
    startup.FindName(L"ExplorerEnable").as<X::Controls::Button>().Content(winrt::box_value(L"Use this copy"));
    for(const double textSize:{14.0,21.0})for(const double available:{280.0,360.0,500.0}){
        for(const auto& child:explorerActions.Children())child.as<X::Controls::Button>().FontSize(textSize);
        explorerActions.Width(available);page.UpdateLayout();Pump(100);
        double required=16;
        for(const auto& child:explorerActions.Children())required+=child.as<X::FrameworkElement>().DesiredSize().Width;
        for(const auto& child:explorerActions.Children()){
            const auto control=child.as<X::FrameworkElement>();const auto point=control.TransformToVisual(explorerActions).TransformPoint({0,0});
            std::wcout<<L"EXPLORER_ACTION size="<<textSize<<L" available="<<available<<L" actual="<<explorerActions.ActualWidth()<<L" columns="<<explorerActions.ColumnDefinitions().Size()<<L" x="<<point.X<<L" width="<<control.ActualWidth()<<L" desired="<<control.DesiredSize().Width<<L" required="<<required<<std::endl;
            Require(point.X>=0&&point.X+control.ActualWidth()<=available+1,L"Explorer actions fit narrow and enlarged-text rows");
            if(required<=available)Require(X::Controls::Grid::GetRow(control)==0,L"Explorer actions use one row whenever their content actually fits");
        }
        if(explorerActions.ColumnDefinitions().Size()>1){
            const auto first=explorerActions.Children().GetAt(0).as<X::FrameworkElement>();const auto second=explorerActions.Children().GetAt(1).as<X::FrameworkElement>();
            const auto gap=second.TransformToVisual(explorerActions).TransformPoint({0,0}).X-first.ActualWidth();
            Require(std::abs(gap-8)<1,L"Explorer buttons have consistent content-sized eight-DIP gaps");
        }
    }
    explorerActions.ClearValue(X::FrameworkElement::WidthProperty());
    page.Width(480);page.Height(900);page.Measure({480,900});page.Arrange({0,0,480,900});page.UpdateLayout();Pump(350);
    const auto startupFrame=page.FindName(L"contentFrame").as<X::Controls::Frame>();
    for(const auto& child:explorerActions.Children()){
        const auto button=child.as<X::FrameworkElement>();const auto point=button.TransformToVisual(startupFrame).TransformPoint({0,0});
        Require(point.X>=16&&point.X+button.ActualWidth()<=startupFrame.ActualWidth()-15,L"Unforced enlarged-text Explorer actions fit the real constrained Settings frame");
    }
    const auto startupBody=startup.Content().as<X::Controls::StackPanel>().Children().GetAt(0).as<X::FrameworkElement>();
    for(const float width:{1800.0f,480.0f}) {
        page.Width(width);page.Measure({width,900});page.Arrange({0,0,width,900});page.UpdateLayout();Pump(350);
        for(const bool open:{false,true}){
            menu.IsPaneOpen(open);page.UpdateLayout();Pump(350);
            const auto heading=page.FindName(L"SettingsBreadcrumbHost").as<X::FrameworkElement>();
            const auto headingX=heading.TransformToVisual(page).TransformPoint({0,0}).X;
            const auto bodyX=startupBody.TransformToVisual(page).TransformPoint({0,0}).X;
            std::wcout<<L"STARTUP_HEADER width="<<width<<L" heading="<<headingX<<L" body="<<bodyX<<std::endl;
            Require(std::abs(headingX-bodyX)<2,L"Startup heading aligns with centered Settings cards in wide and narrow menu states");
        }
    }
    page.ClearValue(X::FrameworkElement::WidthProperty());page.ClearValue(X::FrameworkElement::HeightProperty());page.UpdateLayout();Pump(100);
    const auto content = [&] { return page.FindName(L"contentFrame").as<X::Controls::Frame>().Content().as<X::Controls::StackPanel>(); };
    const auto toggle = [&](size_t i) { return content().Children().GetAt(static_cast<uint32_t>(i+2)).as<X::Controls::Border>().Child().as<X::Controls::StackPanel>().Children().GetAt(0).as<X::Controls::ToggleSwitch>(); };
    const auto selector = [&] { return content().Children().GetAt(2).as<X::Controls::Border>().Child().as<X::Controls::StackPanel>().Children().GetAt(1).as<X::Controls::ComboBox>(); };
    page.OpenTransferBackends(); page.UpdateLayout(); Pump(100);
    // Backend cards follow the same body width as ordinary Settings pages.
    // Exercise a viewport wider than that limit, then a constrained viewport.
    const auto bodyLimit=winrt::unbox_value<double>(page.Resources().Lookup(winrt::box_value(L"StandardControlMaxWidth")));
    for(const float width : {1800.0f, 480.0f, 1800.0f})
    {
        page.Width(width); page.Height(900);
        page.Measure({width,900}); page.Arrange({0,0,width,900}); page.UpdateLayout(); Pump(350);
        for(const bool open:{false,true}) { menu.IsPaneOpen(open);page.UpdateLayout();Pump(350);CheckNavigationPane(menu,page.ActualTheme()==X::ElementTheme::Dark);
        const auto body=content();
        const auto frame=page.FindName(L"contentFrame").as<X::Controls::Frame>();
        const auto offset=body.TransformToVisual(frame).TransformPoint({0,0}).X;
        std::wcout<<L"BACKENDS_BODY viewport="<<width<<L" actual="<<body.ActualWidth()<<L" limit="<<bodyLimit<<L" frame="<<frame.ActualWidth()<<L" offset="<<offset<<std::endl;
        const auto heading=page.FindName(L"SettingsBreadcrumbHost").as<X::FrameworkElement>();
        const auto headingX=heading.TransformToVisual(page).TransformPoint({0,0}).X;
        const auto bodyX=body.TransformToVisual(page).TransformPoint({0,0}).X;
        std::wcout<<L"SETTINGS_HEADER heading="<<headingX<<L" body="<<bodyX<<L" width="<<heading.ActualWidth()<<std::endl;
        Require(std::abs(headingX-bodyX)<2,L"Shared Settings breadcrumb aligns with centered body at wide and narrow sizes");
        Require(body.ActualWidth()>0&&body.ActualWidth()<=bodyLimit+0.5,L"Backend body respects shared Settings maximum width");
        Require(body.ActualWidth()<width,L"Backend body fits constrained Settings viewport");
        Require(std::abs(offset+body.ActualWidth()/2-frame.ActualWidth()/2)<1,L"Backend body remains centered in Settings content frame");
        if(width>bodyLimit+600)Require(std::abs(body.ActualWidth()-bodyLimit)<0.5,L"Wide backend body reaches shared Settings width without stretching beyond it");
        }
    }
    page.ClearValue(X::FrameworkElement::WidthProperty()); page.ClearValue(X::FrameworkElement::HeightProperty());
    page.Measure({1024,768}); page.Arrange({0,0,1024,768}); page.UpdateLayout(); Pump(50);
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
    Require(std::wstring_view{BackendSftpCard(reopened).Children().GetAt(2).as<X::Controls::TextBlock>().Text()}.starts_with(L"Unavailable"),L"Configured reference is not marked ready before a verified connection");
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
