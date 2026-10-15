// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/ApplicationState.h"
#include "../TerminalSettingsModel/IDynamicProfileGenerator.h"
#include "../TerminalSettingsModel/DynamicProfileUtils.h"
#include "../TerminalSettingsModel/SshHostGenerator.h"
#include "../TerminalSettingsModel/PinnedIdentity.h"
#include "../TerminalSettingsModel/PinnedStore.h"
#include "../TerminalSettingsModel/PinnedLaunch.h"
#include "../inc/PowerShellDirectoryIntegration.h"
#include "../TerminalSettingsAppAdapterLib/TerminalSettings.h"
#include <thread>
#include <bit>
#include <map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <winrt/Microsoft.Terminal.TerminalConnection.h>
using namespace WEX::TestExecution;
using namespace winrt;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Microsoft::Terminal::Control;
namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
namespace P = ::Microsoft::Terminal::Backslash::Pins;
namespace C = ::Microsoft::Terminal::Backslash::ConnectionIdentityTitle;
namespace T = winrt::Microsoft::Terminal::TerminalConnection;

namespace SettingsModelUnitTests
{
    // The actual adapter and runtime ControlInteractivity/Core are used below.
    // Only the connection transport is a no-process fixture; it cannot execute
    // pwsh/ssh/wsl or touch the user's shell/profile/configuration files.
    class IdentityConnection : public implements<IdentityConnection, T::ITerminalConnection>
    {
    public:
        uint32_t starts = 0;
        void Initialize(const winrt::Windows::Foundation::Collections::ValueSet&) {}
        void Start() { ++starts; }
        void WriteInput(const array_view<const char16_t> data) { TerminalOutput.raise(data); }
        void Resize(uint32_t, uint32_t) {}
        void Close() {}
        guid SessionId() const { return {}; }
        T::ConnectionState State() const { return T::ConnectionState::Connected; }
        til::event<T::TerminalOutputHandler> TerminalOutput;
        til::typed_event<T::ITerminalConnection, winrt::Windows::Foundation::IInspectable> StateChanged;
    };
    class IdentityGenerator : public IDynamicProfileGenerator
    {
    public:
        std::wstring source = L"Windows.Terminal.PowershellCore", name = L"PowerShell", commandline = L"pwsh.exe -NoLogo";
        std::wstring_view GetNamespace() const noexcept override { return source; }
        std::wstring_view GetDisplayName() const noexcept override { return L"Identity fixture"; }
        std::wstring_view GetIcon() const noexcept override { return {}; }
        void GenerateProfiles(std::vector<com_ptr<M::Profile>>& profiles) const override
        {
            if (source == L"Windows.Terminal.SSH") { SshHostGenerator::GenerateForHosts(L"C:\\fixture\\ssh.exe", { L"fixtureAlias" }, profiles); }
            else { auto profile = CreateDynamicProfile(name); profile->Commandline(hstring{ commandline }); profiles.emplace_back(std::move(profile)); }
        }
    };
    class PinnedIdentityTests
    {
        TEST_CLASS(PinnedIdentityTests);
        TEST_METHOD(ActualLocalPowerShellTransport);
        TEST_METHOD(PowerShellIntegrationConstruction);
        TEST_METHOD(PowerShellPromptAndStatusPreserved);
        TEST_METHOD(FreshGeneratedDefaultPwshThroughActualCore);
        TEST_METHOD(FreshInheritedWindowsPowerShellThroughActualCore);
        TEST_METHOD(StartupOverridesRenameReloadAndHostCase);
        TEST_METHOD(ForeignStartupLaunchersAreRejected);
        TEST_METHOD(GeneratedRemoteIdentitiesKeepExactBoundaries);
        bool _uninitApartment = false;
        TEST_CLASS_SETUP(InitializeApartment)
        {
            try { init_apartment(apartment_type::multi_threaded); }
            catch (const hresult_error& e) { if (e.code() != RPC_E_CHANGED_MODE) { throw; } init_apartment(apartment_type::single_threaded); }
            _uninitApartment = true; return true;
        }
        TEST_CLASS_CLEANUP(CleanupApartment)
        {
            if (_uninitApartment) { uninit_apartment(); } return true;
        }
        template<typename F> static void worker(F&& action)
        {
            try { action(); }
            catch (const hresult_error& e) { throw std::runtime_error("WinRT fixture error " + std::to_string(static_cast<int32_t>(e.code())) + ": " + to_string(e.message())); }
        }
        static winrt::Windows::Foundation::IActivationFactory localFactory(const wchar_t* dll, const wchar_t* runtimeClass)
        {
            // Activate only this test build's DLLs. No package registration and
            // no installed Terminal activation/factory/version may be selected.
            static std::mutex lock; static std::map<std::wstring, wil::unique_hmodule> modules;
            std::scoped_lock guard{ lock }; auto& module = modules[dll];
            if (!module)
            {
                const auto path = std::filesystem::path{ wil::GetModuleFileNameW<std::wstring>(nullptr) }.parent_path() / dll;
                module.reset(LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)); THROW_LAST_ERROR_IF(!module);
            }
            using GetFactory = HRESULT(WINAPI*)(void*, void**);
            const auto function = std::bit_cast<GetFactory>(GetProcAddress(module.get(), "DllGetActivationFactory")); THROW_LAST_ERROR_IF(!function);
            const hstring name{ runtimeClass }; void* factory = nullptr; check_hresult(function(get_abi(name), &factory));
            return { factory, take_ownership_from_abi };
        }
        static ControlInteractivity realCore(const IControlSettings& control, const T::ITerminalConnection& connection)
        {
            return localFactory(L"Microsoft.Terminal.Control.dll", L"Microsoft.Terminal.Control.ControlInteractivity").as<IControlInteractivityFactory>().CreateInstance(control, control.as<IControlAppearance>(), connection);
        }
        static com_ptr<M::CascadiaSettings> generated(const IdentityGenerator& generator)
        {
            Json::Value json; json["defaultProfile"] = to_string(generator.source == L"Windows.Terminal.SSH" ? hstring{ L"SSH - fixtureAlias" } : hstring{ generator.name });
            json["profiles"]["defaults"]["startingDirectory"] = "%USERPROFILE%";
            json["profiles"]["list"] = Json::Value{ Json::arrayValue };
            const auto userJson = Json::writeString(Json::StreamWriterBuilder{}, json);
            WEX::Logging::Log::Comment(L"Identity fixture: parse and layer generated default profile");
            M::SettingsLoader loader{ userJson, "{}" };
            loader.inboxSettings.profiles = loader.GenerateProfilesFor(generator); loader.MergeInboxIntoUserSettings(); loader.FinalizeLayering();
            const auto root = std::filesystem::temp_directory_path() / (L"Backslash-identity-" + std::wstring{ to_hstring(P::NewId()) });
            std::filesystem::create_directories(root);
            auto state = make_self<M::ApplicationState>(root); loader.DisableDeletedProfiles(*state); loader.FixupUserSettings();
            auto settings = make_self<M::CascadiaSettings>(std::move(loader));
            WEX::Logging::Log::Comment(L"Identity fixture: profile loader complete"); return settings;
        }
        static ControlInteractivity coreFor(const com_ptr<M::CascadiaSettings>& settings, const NewTerminalArgs& args, com_ptr<IdentityConnection>& connection)
        {
            const auto adapted = TerminalSettings::CreateWithNewTerminalArgs(*settings, args);
            connection = make_self<IdentityConnection>();
            const auto control = adapted.DefaultSettings().as<IControlSettings>();
            WEX::Logging::Log::Comment(L"Identity fixture: create actual local ControlInteractivity/Core");
            auto interactivity = realCore(control, *connection);
            VERIFY_IS_TRUE(interactivity.Core().Initialize(320, 240, 1.0f)); connection->Start();
            return interactivity;
        }
        static std::string expand(const std::string& text)
        {
            const auto wide = to_hstring(text); return to_string(wil::ExpandEnvironmentStringsW<std::wstring>(wide.c_str()));
        }
        static void captureAndConfirm(const Profile& profile, const ControlCore& core)
        {
            const auto target = P::TargetForProfile(profile, false);
            const auto launch = P::InitialIdentity(core);
            const auto request = P::PrepareCapture(target, launch, to_string(core.ReportedWorkingDirectory()), to_string(core.Settings().StartingDirectory()), "%USERPROFILE%", expand);
            VERIFY_IS_TRUE(request.needsConfirmation); VERIFY_IS_TRUE(core.ReportedWorkingDirectory().empty());
            VERIFY_IS_FALSE(P::FinishDirectory(request, {}, expand).has_value());
            const auto chosen = P::FinishDirectory(request, std::string{ "%USERPROFILE%\\AppData" }, expand);
            VERIFY_IS_TRUE(chosen->origin == P::DirectoryOrigin::Confirmed);
            P::Destination pin{ P::NewId(), target.profile, target.source, chosen->kind, to_string(profile.Name()), target.host, chosen->path };
            pin = P::Capture(pin, chosen->origin); (void)P::Resolve(pin, target, [](const auto&) { return true; });
            const auto launchArgs = P::PrepareLaunch(pin, profile.Commandline(), profile.SshShell());
            VERIFY_IS_TRUE(launchArgs.directory == std::wstring_view{ to_hstring(chosen->path) }); VERIFY_IS_TRUE(launchArgs.commandline.empty());
            VERIFY_IS_TRUE(chosen->path.find('%') == chosen->path.npos);
        }
    };
    void PinnedIdentityTests::ActualLocalPowerShellTransport()
    {
        worker([&] {
            wchar_t enabled[2]{};
            if (!GetEnvironmentVariableW(L"BACKSLASH_TEST_PIN_REAL_SHELL", enabled, 2) || enabled[0] != L'1')
            {
                WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Opt in to the owned local PowerShell ConPTY regression."); return;
            }
            const auto root = std::filesystem::temp_directory_path() / (L"Backslash-live-cwd-" + std::wstring{ to_hstring(P::NewId()) });
            const auto third = root / L"third sp ace;\u03a9's"; std::filesystem::create_directories(third);
            const auto second = std::filesystem::path{ L"D:\\codex-workspace\\backslash-terminal\\artifacts\\live-cwd-fixtures" } / to_hstring(P::NewId()).c_str() / L"second sp ace;\u03a9's";
            std::filesystem::create_directories(second);
            IdentityGenerator generator; generator.commandline = L"\"C:\\Program Files\\PowerShell\\7\\pwsh.exe\" -NoLogo -NoProfile";
            const auto settings = generated(generator); const auto profile = settings->GetProfileForArgs(nullptr);
            const auto home = wil::ExpandEnvironmentStringsW<std::wstring>(L"%USERPROFILE%"); profile.StartingDirectory(home);
            const auto adapted = TerminalSettings::CreateWithNewTerminalArgs(*settings, nullptr); const auto control = adapted.DefaultSettings().as<IControlSettings>();
            const auto factory = localFactory(L"TerminalConnection.dll", L"Microsoft.Terminal.TerminalConnection.ConptyConnection");
            auto connection = factory.ActivateInstance<T::ConptyConnection>();
            const auto environment = single_threaded_map<hstring, hstring>(); environment.Insert(L"APPDATA", hstring{ root.wstring() });
            connection.Initialize(factory.as<T::IConptyConnectionStatics>().CreateSettings(control.Commandline(), home, L"Owned live cwd fixture", false, {}, environment.GetView(), 24, 100, {}, profile.Guid()));
            auto instance = realCore(control, connection); auto close = wil::scope_exit([&] { instance.Close(); });
            const auto core = instance.Core(); VERIFY_IS_TRUE(core.Initialize(800, 480, 1.0f)); connection.Start();
            const auto awaitPath = [&](const std::wstring& expected) {
                const auto deadline = GetTickCount64() + 15000;
                while (std::wstring_view{ core.ReportedWorkingDirectory() } != expected && GetTickCount64() < deadline) { Sleep(20); }
                VERIFY_ARE_EQUAL(hstring{ expected }, core.ReportedWorkingDirectory());
            };
            const auto send = [&](const std::wstring& command) { connection.WriteInput(winrt_wstring_to_array_view(command + L"\r")); };
            const auto quote = [](std::wstring value) { for (size_t i = 0; (i = value.find(L'\'', i)) != value.npos; i += 2) { value.insert(i, 1, L'\''); } return L"'" + value + L"'"; };
            const auto capture = [&] {
                const auto target = P::TargetForProfile(profile, false);
                const auto request = P::PrepareCapture(target, P::InitialIdentity(core), to_string(core.ReportedWorkingDirectory()), to_string(control.StartingDirectory()), "%USERPROFILE%", expand);
                const auto directory = P::CurrentDirectory(request);
                return P::Capture({ P::NewId(), target.profile, target.source, directory.kind, to_string(profile.Name()), target.host, directory.path }, directory.origin);
            };
            awaitPath(home); const auto first = capture();
            VERIFY_ARE_EQUAL(control.Commandline(), connection.Commandline()); VERIFY_ARE_EQUAL(control.Commandline(), core.InitialLaunchCommandline());
            send(L"Set-Location -LiteralPath " + quote(second.wstring())); awaitPath(second.wstring()); const auto secondPin = capture();
            send(L"Set-Location -LiteralPath " + quote(third.wstring())); awaitPath(third.wstring()); const auto thirdPin = capture();
            VERIFY_ARE_EQUAL(to_string(home), first.path); VERIFY_ARE_EQUAL(to_string(second.wstring()), secondPin.path); VERIFY_ARE_EQUAL(to_string(third.wstring()), thirdPin.path);
            P::Catalog catalog; catalog.Add(first); catalog.Add(secondPin); catalog.Add(thirdPin); const auto roundtrip = P::Catalog::Read(catalog.Write());
            VERIFY_ARE_EQUAL(size_t{ 3 }, roundtrip.pins.size()); VERIFY_ARE_EQUAL(first.path, roundtrip.pins[0].path);
            send(L"New-PSDrive -Name BsOwned -PSProvider FileSystem -Root " + quote(second.wstring()) + L" | Out-Null; Set-Location BsOwned:\\"); awaitPath(second.wstring() + L"\\");
            send(L"& 'C:\\Program Files\\PowerShell\\7\\pwsh.exe' -NoLogo -NoProfile -NonInteractive -Command 'Start-Sleep -Seconds 2'"); awaitPath(L""); awaitPath(second.wstring() + L"\\");
            const auto marker = root / L"provider-ready.txt";
            send(L"Set-Location HKCU:\\; [IO.File]::WriteAllText(" + quote(marker.wstring()) + L",'ready')");
            const auto deadline = GetTickCount64() + 15000; while (!std::filesystem::exists(marker) && GetTickCount64() < deadline) { Sleep(20); }
            VERIFY_IS_TRUE(std::filesystem::exists(marker)); awaitPath(L"");
            const auto noPath = P::RequestDirectory(P::Kind::Local, to_string(core.ReportedWorkingDirectory()), to_string(home), "%USERPROFILE%", expand);
            VERIFY_THROWS(P::CurrentDirectory(noPath), std::exception);
            send(L"Set-Location -LiteralPath " + quote(third.wstring())); awaitPath(third.wstring());
            send(L"exit"); const auto exitDeadline = GetTickCount64() + 15000;
            while (connection.State() < T::ConnectionState::Closed && GetTickCount64() < exitDeadline) { Sleep(20); }
            VERIFY_IS_TRUE(core.ReportedWorkingDirectory().empty());
            WEX::Logging::Log::Comment((L"Owned live PS7 regression: " + root.wstring() + L"; second drive: " + second.wstring()).c_str());
        });
    }

    void PinnedIdentityTests::PowerShellIntegrationConstruction()
    {
        namespace I = ::Microsoft::Terminal::Backslash::PowerShellDirectoryIntegration;
        for (const auto* command : { L"pwsh.exe", L"pwsh -NoLogo -NoProfile", L"\"C:\\Program Files\\PowerShell\\7\\PWSH.EXE\" -NoExit -STA" })
        {
            const auto physical = I::Prepare(command); VERIFY_IS_TRUE(physical.starts_with(command)); VERIFY_IS_TRUE(physical.find(L" -EncodedCommand ") != physical.npos);
            VERIFY_ARE_EQUAL(physical, I::Prepare(physical));
        }
        for (const auto* command : { L"powershell.exe", L"cmd.exe", L"ssh.exe alias pwsh", L"wsl.exe pwsh", L"pwsh -NonInteractive", L"pwsh -Command foo", L"pwsh -File developer.ps1", L"pwsh -EncodedCommand QQ==", L"pwsh -WorkingDirectory C:\\", L"pwsh -HostName alias" })
        {
            VERIFY_ARE_EQUAL(std::wstring{ command }, I::Prepare(command));
        }
        std::wstring longLauncher = L"pwsh.exe"; for (int i = 0; i < 3800; ++i) { longLauncher += L" -NoLogo"; }
        VERIFY_ARE_EQUAL(longLauncher, I::Prepare(longLauncher));
        VERIFY_ARE_EQUAL(std::wstring{ L"QQA=" }, I::Encode(L"A")); VERIFY_ARE_EQUAL(std::wstring{ L"QQBCAA==" }, I::Encode(L"AB"));
    }

    void PinnedIdentityTests::PowerShellPromptAndStatusPreserved()
    {
        worker([&] {
            wchar_t enabled[2]{}; if (!GetEnvironmentVariableW(L"BACKSLASH_TEST_PIN_REAL_SHELL", enabled, 2) || enabled[0] != L'1') { WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Owned local real shell opt-in."); return; }
            namespace I = ::Microsoft::Terminal::Backslash::PowerShellDirectoryIntegration;
            const auto root = std::filesystem::temp_directory_path() / (L"Backslash-prompt-" + std::wstring{ to_hstring(P::NewId()) }); std::filesystem::create_directories(root);
            const auto marker = root / L"prompt-status.txt"; const auto installed = root / L"installed.txt";
            const auto quote = [](std::wstring value) { for (size_t i = 0; (i = value.find(L'\'', i)) != value.npos; i += 2) { value.insert(i, 1, L'\''); } return L"'" + value + L"'"; };
            std::wstring script = L"$global:priorCalls=0; $ExecutionContext.SessionState.InvokeCommand.LocationChangedAction={param($s,$e) $global:priorCalls++}; function global:prompt { $ok=$?; $code=$global:LASTEXITCODE; [IO.File]::AppendAllText(" + quote(marker.wstring()) + L",\"$ok|$code|$global:priorCalls`n\"); 'CUSTOM OWNED> ' }; $original=$function:prompt.ToString();\n";
            script += I::Bootstrap; script += L"\n$first=$global:__BackslashCwdV1;\n"; script += I::Bootstrap;
            script += L"\nif ($function:prompt.ToString() -ne $original -or ![object]::ReferenceEquals($first,$global:__BackslashCwdV1)) { throw 'Changed prompt or installed twice' }; [IO.File]::WriteAllText(" + quote(installed.wstring()) + L",'unchanged');";
            IdentityGenerator generator; generator.commandline = L"\"C:\\Program Files\\PowerShell\\7\\pwsh.exe\" -NoLogo -NoProfile -NoExit -EncodedCommand " + I::Encode(script);
            const auto settings = generated(generator); const auto profile = settings->GetProfileForArgs(nullptr); const auto adapted = TerminalSettings::CreateWithNewTerminalArgs(*settings, nullptr); const auto control = adapted.DefaultSettings().as<IControlSettings>();
            const auto factory = localFactory(L"TerminalConnection.dll", L"Microsoft.Terminal.TerminalConnection.ConptyConnection"); auto connection = factory.ActivateInstance<T::ConptyConnection>();
            const auto environment = single_threaded_map<hstring, hstring>(); environment.Insert(L"APPDATA", hstring{ root.wstring() });
            connection.Initialize(factory.as<T::IConptyConnectionStatics>().CreateSettings(control.Commandline(), root.wstring(), L"Owned prompt fixture", false, {}, environment.GetView(), 24, 100, {}, profile.Guid()));
            auto instance = realCore(control, connection); auto close = wil::scope_exit([&] { instance.Close(); }); const auto core = instance.Core(); VERIFY_IS_TRUE(core.Initialize(800, 480, 1.0f)); connection.Start();
            const auto lines = [&] { std::vector<std::string> result; std::ifstream input{ marker }; std::string line; while (std::getline(input, line)) { result.push_back(line); } return result; };
            const auto awaitPrompt = [&](size_t minimum) { const auto end = GetTickCount64() + 15000; while ((lines().size() < minimum || core.ReportedWorkingDirectory().empty()) && GetTickCount64() < end) { Sleep(20); } VERIFY_IS_TRUE(lines().size() >= minimum); VERIFY_IS_FALSE(core.ReportedWorkingDirectory().empty()); };
            awaitPrompt(1); VERIFY_IS_TRUE(std::filesystem::exists(installed));
            const auto run = [&](const std::wstring& command) { const auto count = lines().size(); connection.WriteInput(winrt_wstring_to_array_view(command + L"\r")); awaitPrompt(count + 1); return lines().back(); };
            VERIFY_IS_TRUE(run(L"& $env:ComSpec /d /c exit 7").starts_with("False|7|"));
            VERIFY_IS_TRUE(run(L"& $env:ComSpec /d /c exit 0").starts_with("True|0|"));
            VERIFY_IS_TRUE(run(L"Write-Error 'owned failure' -ErrorAction Continue").starts_with("False|0|"));
            VERIFY_IS_TRUE(run(L"Set-Location $env:TEMP").ends_with("|1"));
            VERIFY_ARE_EQUAL(control.Commandline(), connection.Commandline());
            connection.WriteInput(winrt_wstring_to_array_view(L"exit\r"));
            WEX::Logging::Log::Comment((L"Original prompt/status/idempotence fixture: " + marker.wstring()).c_str());
        });
    }


    void PinnedIdentityTests::FreshGeneratedDefaultPwshThroughActualCore()
    {
        worker([&] {
            for (const auto* command : { L"pwsh.exe -NoLogo", L"\"C:\\Program Files\\PowerShell\\7\\pwsh.exe\" -NoLogo" })
            {
                IdentityGenerator generator; generator.commandline = command; const auto settings = generated(generator);
                const auto profile = settings->GetProfileForArgs(nullptr); com_ptr<IdentityConnection> connection;
                auto instance = coreFor(settings, nullptr, connection); auto close = wil::scope_exit([&] { instance.Close(); }); const auto core = instance.Core();
                VERIFY_ARE_EQUAL(profile.Guid(), core.InitialLaunchProfileId()); VERIFY_ARE_EQUAL(profile.Source(), core.InitialLaunchProfileSource());
                VERIFY_ARE_EQUAL(hstring{ C::LocalMachineName() }, core.InitialLaunchConnectionIdentity()); VERIFY_ARE_EQUAL(hstring{ command }, core.InitialLaunchCommandline());
                VERIFY_IS_TRUE(std::wstring_view{ core.InitialConnectionTitlePrefix() }.ends_with(L" \u00b7 PowerShell"));
                // This real producer output failed the old, mojibake boundary.
                VERIFY_IS_FALSE(std::wstring{ core.InitialConnectionTitlePrefix() }.starts_with(C::LocalMachineName() + L" \u00c2\u00b7 "));
                connection->WriteInput(winrt_wstring_to_array_view(L"PS C:\\Users\\nemo> ")); captureAndConfirm(profile, core); VERIFY_ARE_EQUAL(uint32_t{ 1 }, connection->starts);
            }
        });
    }
    void PinnedIdentityTests::FreshInheritedWindowsPowerShellThroughActualCore()
    {
        worker([&] {
            const auto settings = make_self<M::CascadiaSettings>(R"({"defaultProfile":"{22222222-2222-4222-8222-222222222222}","profiles":{"defaults":{"commandline":"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe -NoLogo","startingDirectory":"%USERPROFILE%"},"list":[{"guid":"{22222222-2222-4222-8222-222222222222}","name":"Windows PowerShell"}]}})", "{}");
            const auto profile = settings->GetProfileForArgs(nullptr); com_ptr<IdentityConnection> connection; auto instance = coreFor(settings, nullptr, connection); auto close = wil::scope_exit([&] { instance.Close(); });
            VERIFY_IS_TRUE(profile.Source().empty()); captureAndConfirm(profile, instance.Core());
        });
    }
    void PinnedIdentityTests::StartupOverridesRenameReloadAndHostCase()
    {
        worker([&] {
            IdentityGenerator generator; const auto settings = generated(generator); const auto profile = settings->GetProfileForArgs(nullptr);
            NewTerminalArgs args; args.Profile(to_hstring(profile.Guid())); args.Commandline(L"\"C:\\Program Files\\PowerShell\\7\\PWSH.EXE\" -NoProfile"); args.TabTitle(L"Decorated \u00c2\u00b7 title"); args.StartingDirectory(L"%USERPROFILE%\\AppData");
            com_ptr<IdentityConnection> connection; auto instance = coreFor(settings, args, connection); auto close = wil::scope_exit([&] { instance.Close(); }); const auto core = instance.Core();
            captureAndConfirm(profile, core); const auto original = P::InitialIdentity(core);
            profile.Name(L"Renamed \u00b7 PowerShell"); settings->GlobalSettings().ShowConnectionIdentity(false);
            const auto updated = TerminalSettings::CreateWithProfile(*settings, profile); const auto fresh = updated.DefaultSettings().as<IControlSettings>(); core.UpdateSettings(fresh, fresh.as<IControlAppearance>());
            VERIFY_IS_TRUE(P::InitialIdentity(core).commandline == original.commandline); VERIFY_ARE_EQUAL(profile.Guid(), core.InitialLaunchProfileId());
            auto target = P::TargetForProfile(profile, false); for (auto& c : target.host) { c = static_cast<char>(tolower(static_cast<unsigned char>(c))); }
            P::PreflightCapture(target, P::InitialIdentity(core)); captureAndConfirm(profile, core);
            auto wrong = target; wrong.host += "-other"; VERIFY_THROWS(P::PreflightCapture(wrong, original), std::exception);
            wrong = target; wrong.profile = P::NewId(); VERIFY_THROWS(P::PreflightCapture(wrong, original), std::exception);
            wrong = target; wrong.source = "another-source"; VERIFY_THROWS(P::PreflightCapture(wrong, original), std::exception);
        });
    }
    void PinnedIdentityTests::ForeignStartupLaunchersAreRejected()
    {
        worker([&] {
            IdentityGenerator generator; const auto settings = generated(generator); const auto profile = settings->GetProfileForArgs(nullptr);
            for (const auto* command : { L"ssh.exe otherHost", L"wsl.exe -d OtherDistro", L"pwsh.exe -HostName remote", L"pwsh.exe -Command ssh.exe remote" })
            {
                NewTerminalArgs args; args.Profile(to_hstring(profile.Guid())); args.Commandline(command);
                com_ptr<IdentityConnection> connection; auto instance = coreFor(settings, args, connection); auto close = wil::scope_exit([&] { instance.Close(); });
                VERIFY_IS_TRUE(instance.Core().InitialLaunchConnectionIdentity().empty()); VERIFY_THROWS(P::PreflightCapture(P::TargetForProfile(profile, false), P::InitialIdentity(instance.Core())), std::exception);
            }
            NewTerminalArgs append; append.Commandline(L"-HostName remote"); append.AppendCommandLine(true);
            com_ptr<IdentityConnection> connection; auto instance = coreFor(settings, append, connection); auto close = wil::scope_exit([&] { instance.Close(); });
            VERIFY_THROWS(P::PreflightCapture(P::TargetForProfile(profile, false), P::InitialIdentity(instance.Core())), std::exception);
        });
    }
    void PinnedIdentityTests::GeneratedRemoteIdentitiesKeepExactBoundaries()
    {
        worker([&] {
            for (bool ssh : { true, false })
            {
                IdentityGenerator generator;
                if (ssh) { generator.source = L"Windows.Terminal.SSH"; generator.name = L"SSH - fixtureAlias"; }
                else { generator.source = L"Windows.Terminal.Wsl"; generator.name = L"Ubuntu-24.04"; std::wstring system; THROW_IF_FAILED(wil::GetSystemDirectoryW<std::wstring>(system)); generator.commandline = L"\"" + system + L"\\wsl.exe\" -d Ubuntu-24.04"; }
                const auto settings = generated(generator); const auto profile = settings->GetProfileForArgs(nullptr); if (ssh) { VERIFY_IS_TRUE(profile.SetSshShell(3, {})); }
                com_ptr<IdentityConnection> connection; auto instance = coreFor(settings, nullptr, connection); auto close = wil::scope_exit([&] { instance.Close(); });
                const auto target = P::TargetForProfile(profile, false); const auto initial = P::InitialIdentity(instance.Core()); P::PreflightCapture(target, initial);
                const auto never = [](const std::string&) -> std::string { throw std::runtime_error("No local suggestion for remote capture"); };
                const auto request = P::PrepareCapture(target, initial, "", "C:\\local-client", "%USERPROFILE%", never); VERIFY_IS_TRUE(request.needsConfirmation); VERIFY_IS_TRUE(request.path.empty());
                auto other = initial; other.identity += "-other"; VERIFY_THROWS(P::PreflightCapture(target, other), std::exception);
                auto renamed = target; renamed.host += "-other"; VERIFY_THROWS(P::PreflightCapture(renamed, initial), std::exception);
            }
        });
    }
}
