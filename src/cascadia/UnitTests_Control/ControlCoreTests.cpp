// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "../TerminalControl/EventArgs.h"
#include "../TerminalControl/ControlCore.h"
#include "MockControlSettings.h"
#include "MockConnection.h"
#include "../../inc/TestUtils.h"
#include "../../renderer/base/renderer.hpp"
#include "../../renderer/atlas/AtlasEngine.h"
#include <d2d1_3.h>
#include "../../renderer/atlas/BackendD3D.h"
#include <fstream>

using namespace Microsoft::Console;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace WEX::Common;

using namespace winrt;
using namespace winrt::Microsoft::Terminal;

namespace ControlUnitTests
{
    class ControlCoreTests
    {
        BEGIN_TEST_CLASS(ControlCoreTests)
            TEST_CLASS_PROPERTY(L"TestTimeout", L"0:0:10") // 10s timeout
        END_TEST_CLASS()

        TEST_METHOD(ComPtrSettings);
        TEST_METHOD(InstantiateCore);
        TEST_METHOD(TestInitialize);
        TEST_METHOD(TestAdjustAcrylic);

        TEST_METHOD(TestFreeAfterClose);

        TEST_METHOD(TestFontInitializedInCtor);

        TEST_METHOD(TestClearScrollback);
        TEST_METHOD(TestClearScreen);
        TEST_METHOD(TestClearAll);
        TEST_METHOD(TestReadEntireBuffer);
        TEST_METHOD(BackslashRenderingVisibilityKeepsOutput);
        TEST_METHOD(BackslashPostProcessWARP);
        TEST_METHOD(BackslashPhosphorOpacityWARP);
        TEST_METHOD(BackslashConnectionTitleSnapshot);
        TEST_METHOD(BackslashReportedDirectory);

        TEST_METHOD(TestSelectCommandSimple);
        TEST_METHOD(TestSelectOutputSimple);
        TEST_METHOD(TestCommandContext);
        TEST_METHOD(TestCommandContextWithPwshGhostText);

        TEST_METHOD(TestSelectOutputScrolling);
        TEST_METHOD(TestSelectOutputExactWrap);

        TEST_METHOD(TestSimpleClickSelection);

        TEST_CLASS_SETUP(ModuleSetup)
        {
            winrt::init_apartment(winrt::apartment_type::single_threaded);

            return true;
        }
        TEST_CLASS_CLEANUP(ClassCleanup)
        {
            winrt::uninit_apartment();
            return true;
        }

        std::tuple<winrt::com_ptr<MockControlSettings>, winrt::com_ptr<MockConnection>> _createSettingsAndConnection()
        {
            Log::Comment(L"Create settings object");
            auto settings = winrt::make_self<MockControlSettings>();
            VERIFY_IS_NOT_NULL(settings);

            Log::Comment(L"Create connection object");
            auto conn = winrt::make_self<MockConnection>();
            VERIFY_IS_NOT_NULL(conn);

            return { settings, conn };
        }

        winrt::com_ptr<Control::implementation::ControlCore> createCore(Control::IControlSettings settings,
                                                                        TerminalConnection::ITerminalConnection conn)
        {
            Log::Comment(L"Create ControlCore object");

            auto core = winrt::make_self<Control::implementation::ControlCore>(settings, settings, conn);
            core->_inUnitTests = true;
            return core;
        }

        void _standardInit(winrt::com_ptr<Control::implementation::ControlCore> core)
        {
            // "Consolas" ends up with an actual size of 9x19 at 96DPI. So
            // let's just arbitrarily start with a 270x380px (30x20 chars) window
            core->Initialize(270, 380, 1.0);
#ifndef NDEBUG
            core->_terminal->_suppressLockChecks = true;
#endif
            VERIFY_IS_TRUE(core->_initializedTerminal);
            VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        }
    };

    void ControlCoreTests::ComPtrSettings()
    {
        Log::Comment(L"Just make sure we can instantiate a settings obj in a com_ptr");
        auto settings = winrt::make_self<MockControlSettings>();

        Log::Comment(L"Verify literally any setting, it doesn't matter");
        VERIFY_ARE_EQUAL(DEFAULT_FOREGROUND, settings->DefaultForeground());
    }

    void ControlCoreTests::InstantiateCore()
    {
        auto [settings, conn] = _createSettingsAndConnection();

        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
    }

    void ControlCoreTests::TestInitialize()
    {
        auto [settings, conn] = _createSettingsAndConnection();

        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);

        VERIFY_IS_FALSE(core->_initializedTerminal);
        // "Consolas" ends up with an actual size of 9x19 at 96DPI. So
        // let's just arbitrarily start with a 270x380px (30x20 chars) window
        core->Initialize(270, 380, 1.0);
#ifndef NDEBUG
        core->_terminal->_suppressLockChecks = true;
#endif
        VERIFY_IS_TRUE(core->_initializedTerminal);
        VERIFY_ARE_EQUAL(30, core->_terminal->GetViewport().Width());
    }

    void ControlCoreTests::TestAdjustAcrylic()
    {
        auto [settings, conn] = _createSettingsAndConnection();

        settings->UseAcrylic(true);
        settings->Opacity(0.5f);

        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);

        // A callback to make sure that we're raising TransparencyChanged events
        auto expectedOpacity = 0.5f;
        auto opacityCallback = [&](auto&&, Control::TransparencyChangedEventArgs args) mutable {
            VERIFY_ARE_EQUAL(expectedOpacity, args.Opacity());
            VERIFY_ARE_EQUAL(expectedOpacity, core->Opacity());
            // The Settings object's opacity shouldn't be changed
            VERIFY_ARE_EQUAL(0.5f, settings->Opacity());

            if (expectedOpacity < 1.0f)
            {
                VERIFY_IS_TRUE(settings->UseAcrylic());
                VERIFY_IS_TRUE(core->_settings.UseAcrylic());
            }

            // GH#603: Adjusting opacity shouldn't change whether or not we
            // requested acrylic.

            auto expectedUseAcrylic = expectedOpacity < 1.0f;
            VERIFY_IS_TRUE(core->_settings.UseAcrylic());
            VERIFY_ARE_EQUAL(expectedUseAcrylic, core->UseAcrylic());
        };
        core->TransparencyChanged(opacityCallback);

        VERIFY_IS_FALSE(core->_initializedTerminal);
        // "Cascadia Mono" ends up with an actual size of 9x19 at 96DPI. So
        // let's just arbitrarily start with a 270x380px (30x20 chars) window
        core->Initialize(270, 380, 1.0);
        VERIFY_IS_TRUE(core->_initializedTerminal);

        Log::Comment(L"Increasing opacity till fully opaque");
        expectedOpacity += 0.1f; // = 0.6;
        core->AdjustOpacity(0.1f);
        expectedOpacity += 0.1f; // = 0.7;
        core->AdjustOpacity(0.1f);
        expectedOpacity += 0.1f; // = 0.8;
        core->AdjustOpacity(0.1f);
        expectedOpacity += 0.1f; // = 0.9;
        core->AdjustOpacity(0.1f);
        expectedOpacity += 0.1f; // = 1.0;
        // cast to float because floating point numbers are mean
        VERIFY_ARE_EQUAL(1.0f, expectedOpacity);
        core->AdjustOpacity(0.1f);

        Log::Comment(L"Increasing opacity more doesn't actually change it to be >1.0");

        expectedOpacity = 1.0f;
        core->AdjustOpacity(0.1f);

        Log::Comment(L"Decrease opacity");
        expectedOpacity -= 0.25f; // = 0.75;
        core->AdjustOpacity(-0.25f);
        expectedOpacity -= 0.25f; // = 0.5;
        core->AdjustOpacity(-0.25f);
        expectedOpacity -= 0.25f; // = 0.25;
        core->AdjustOpacity(-0.25f);
        expectedOpacity -= 0.25f; // = 0.05;
        // cast to float because floating point numbers are mean
        VERIFY_ARE_EQUAL(0.0f, expectedOpacity);
        core->AdjustOpacity(-0.25f);

        Log::Comment(L"Decreasing opacity more doesn't actually change it to be < 0");
        expectedOpacity = 0.0f;
        core->AdjustOpacity(-0.25f);
    }

    void ControlCoreTests::TestFreeAfterClose()
    {
        {
            auto [settings, conn] = _createSettingsAndConnection();

            auto core = createCore(*settings, *conn);
            VERIFY_IS_NOT_NULL(core);

            Log::Comment(L"Close the Core, like a TermControl would");
            core->Close();
        }

        VERIFY_IS_TRUE(true, L"Make sure that the test didn't crash when the core when out of scope");
    }

    void ControlCoreTests::TestFontInitializedInCtor()
    {
        // This is to catch a dumb programming mistake I made while working on
        // the core/control split. We want the font initialized in the ctor,
        // before we even get to Core::Initialize.

        auto [settings, conn] = _createSettingsAndConnection();

        // Make sure to use something dumb like "Impact" as a font name here so
        // that you don't default to Cascadia*
        settings->FontFace(L"Impact");

        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);

        VERIFY_ARE_EQUAL(L"Impact", std::wstring_view{ core->_actualFont.GetFaceName() });
    }

    void ControlCoreTests::TestClearScrollback()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print 40 rows of 'Foo', and a single row of 'Bar' "
                     L"(leaving the cursor after 'Bar')");
        for (auto i = 0; i < 40; ++i)
        {
            conn->WriteInput(winrt_wstring_to_array_view(L"Foo\r\n"));
        }
        conn->WriteInput(winrt_wstring_to_array_view(L"Bar"));

        // We printed that 40 times, but the final \r\n bumped the view down one MORE row.
        Log::Comment(L"Check the buffer viewport before the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(21, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(41, core->BufferHeight());

        Log::Comment(L"Clear the buffer");
        core->ClearBuffer(Control::ClearBufferType::Scrollback);

        Log::Comment(L"Check the buffer after the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(0, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(20, core->BufferHeight());

        // In this test, we can't actually check if we cleared the buffer
        // contents. ConPTY will handle the actual clearing of the buffer
        // contents. We can only ensure that the viewport moved when we did a
        // clear scrollback.
    }
    void ControlCoreTests::TestClearScreen()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print 40 rows of 'Foo', and a single row of 'Bar' "
                     L"(leaving the cursor after 'Bar')");
        for (auto i = 0; i < 40; ++i)
        {
            conn->WriteInput(winrt_wstring_to_array_view(L"Foo\r\n"));
        }
        conn->WriteInput(winrt_wstring_to_array_view(L"Bar"));

        // We printed that 40 times, but the final \r\n bumped the view down one MORE row.
        Log::Comment(L"Check the buffer viewport before the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(21, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(41, core->BufferHeight());

        Log::Comment(L"Clear the buffer");
        core->ClearBuffer(Control::ClearBufferType::Screen);

        Log::Comment(L"Check the buffer after the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(21, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(41, core->BufferHeight());

        // In this test, we can't actually check if we cleared the buffer
        // contents. ConPTY will handle the actual clearing of the buffer
        // contents. We can only ensure that the viewport moved when we did a
        // clear scrollback.
    }
    void ControlCoreTests::TestClearAll()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print 40 rows of 'Foo', and a single row of 'Bar' "
                     L"(leaving the cursor after 'Bar')");
        for (auto i = 0; i < 40; ++i)
        {
            conn->WriteInput(winrt_wstring_to_array_view(L"Foo\r\n"));
        }
        conn->WriteInput(winrt_wstring_to_array_view(L"Bar"));

        // We printed that 40 times, but the final \r\n bumped the view down one MORE row.
        Log::Comment(L"Check the buffer viewport before the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(21, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(41, core->BufferHeight());

        Log::Comment(L"Clear the buffer");
        core->ClearBuffer(Control::ClearBufferType::All);

        Log::Comment(L"Check the buffer after the clear");
        VERIFY_ARE_EQUAL(20, core->_terminal->GetViewport().Height());
        VERIFY_ARE_EQUAL(0, core->ScrollOffset());
        VERIFY_ARE_EQUAL(20, core->ViewHeight());
        VERIFY_ARE_EQUAL(20, core->BufferHeight());

        // In this test, we can't actually check if we cleared the buffer
        // contents. ConPTY will handle the actual clearing of the buffer
        // contents. We can only ensure that the viewport moved when we did a
        // clear scrollback.
    }

    void ControlCoreTests::TestReadEntireBuffer()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n"));

        Log::Comment(L"Check the buffer contents");
        VERIFY_ARE_EQUAL(L"This is some text\r\nwith varying amounts\r\nof whitespace\r\n",
                         core->ReadEntireBuffer());
    }

    void ControlCoreTests::BackslashRenderingVisibilityKeepsOutput()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        auto core = createCore(*settings, *conn);
        _standardInit(core);

        core->SetRenderingVisible(false);
        VERIFY_IS_FALSE(core->_renderer->IsRenderingVisible());
        conn->WriteInput(winrt_wstring_to_array_view(L"hidden output one\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"hidden output two\r\n"));
        VERIFY_ARE_EQUAL(L"hidden output one\r\nhidden output two\r\n", core->ReadEntireBuffer());

        core->SetRenderingVisible(true);
        VERIFY_IS_TRUE(core->_renderer->IsRenderingVisible());
        core->_focusChanged(false);
        VERIFY_IS_TRUE(core->_renderer->IsRenderingVisible());

        core->Close();
        core->SetRenderingVisible(true);
        VERIFY_IS_FALSE(core->_renderer->IsRenderingVisible());
    }

    void ControlCoreTests::BackslashConnectionTitleSnapshot()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        settings->ConnectionTitlePrefix(L"SSH originalAlias");settings->LaunchProfileId(winrt::guid{L"{22222222-2222-4222-8222-222222222222}"});settings->LaunchProfileSource(L"Windows.Terminal.SSH");settings->LaunchConnectionIdentity(L"SSH originalAlias");settings->Commandline(L"ssh.exe originalAlias");settings->StartingTitle(L"Original profile");settings->StartingTitleIsExplicit(true);
        const auto core = createCore(*settings, *conn);_standardInit(core);
        VERIFY_ARE_EQUAL(winrt::hstring{ L"SSH originalAlias" }, core->InitialConnectionTitlePrefix());
        VERIFY_ARE_EQUAL(winrt::hstring{ L"Original profile" }, core->InitialStartingTitle());
        VERIFY_IS_TRUE(core->InitialTitleIsExplicit());
        auto other = winrt::make_self<MockControlSettings>();other->ConnectionTitlePrefix(L"SSH changedAlias");other->LaunchProfileId(winrt::guid{L"{33333333-3333-4333-8333-333333333333}"});other->LaunchProfileSource(L"Windows.Terminal.Wsl");other->LaunchConnectionIdentity(L"WSL other");other->Commandline(L"wsl.exe -d other");other->StartingTitle(L"Changed profile");other->StartingTitleIsExplicit(false);
        core->UpdateSettings(*other, nullptr);
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]0;remote dynamic title\x7"));
        VERIFY_ARE_EQUAL(winrt::hstring{ L"remote dynamic title" }, core->Title());
        VERIFY_ARE_EQUAL(winrt::hstring{ L"SSH originalAlias" }, core->InitialConnectionTitlePrefix());
        VERIFY_ARE_EQUAL(winrt::hstring{ L"Original profile" }, core->InitialStartingTitle());
        VERIFY_IS_TRUE(core->InitialTitleIsExplicit());
        VERIFY_ARE_EQUAL(winrt::guid{L"{22222222-2222-4222-8222-222222222222}"}, core->InitialLaunchProfileId());
        VERIFY_ARE_EQUAL(winrt::hstring{L"Windows.Terminal.SSH"}, core->InitialLaunchProfileSource());
        VERIFY_ARE_EQUAL(winrt::hstring{L"SSH originalAlias"}, core->InitialLaunchConnectionIdentity());
        VERIFY_ARE_EQUAL(winrt::hstring{L"ssh.exe originalAlias"}, core->InitialLaunchCommandline());core->Close();
    }

    void ControlCoreTests::BackslashReportedDirectory()
    {
        for (const auto* initial : { L"%USERPROFILE%", L"C:\\Users\\nemo", L"C:\\Users\\nemo\\AppData" })
        {
            auto [settings, conn] = _createSettingsAndConnection(); settings->StartingDirectory(initial);
            const auto core = createCore(*settings, *conn); _standardInit(core);
            VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty()); VERIFY_ARE_EQUAL(winrt::hstring{ initial }, core->WorkingDirectory());
            // A normal PowerShell prompt is not an authenticated cwd report.
            conn->WriteInput(winrt_wstring_to_array_view(L"PS C:\\Users\\nemo\\AppData> "));
            VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;\"C:\\Users\\nemo\\AppData\"\x7"));
            VERIFY_ARE_EQUAL(winrt::hstring{ L"C:\\Users\\nemo\\AppData" }, core->ReportedWorkingDirectory());
            for (const auto* path : { L"C:\\sp ace;\u03a9's", L"C:\\semicolon;another;folder" })
            {
                const auto report = L"\x1b]9;9;\"" + std::wstring{ path } + L"\"\x7";
                conn->WriteInput(winrt_wstring_to_array_view(report)); VERIFY_ARE_EQUAL(winrt::hstring{ path }, core->ReportedWorkingDirectory());
            }
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;\x7")); VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\valid\x7")); VERIFY_IS_FALSE(core->ReportedWorkingDirectory().empty());
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;\"\"\x7")); VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            for (const auto* bad : { L"C:\\bad|folder", L"C:\\bad\u0085folder", L"C:\\bad\tfolder", L"C:\\bad\rfolder", L"C:\\bad\nfolder" })
            {
                conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\valid\x7"));
                const auto report = L"\x1b]9;9;" + std::wstring{ bad } + L"\x7";
                conn->WriteInput(winrt_wstring_to_array_view(report)); VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            }
            for (const auto control : { wchar_t{ 0 }, wchar_t{ 0x18 }, wchar_t{ 0x1a }, wchar_t{ 0x1b } })
            {
                conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\valid\x7"));
                auto report = std::wstring{ L"\x1b]9;9;C:\\bad" }; report.push_back(control); report += L"folder\x7";
                conn->WriteInput(winrt_wstring_to_array_view(report)); VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            }
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\old-generation\x7"));
            auto next = winrt::make_self<MockConnection>(); core->Connection(*next);
            VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\stale-transport\x7")); VERIFY_IS_TRUE(core->ReportedWorkingDirectory().empty());
            next->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;C:\\new-generation\x7"));
            VERIFY_ARE_EQUAL(winrt::hstring{ L"C:\\new-generation" }, core->ReportedWorkingDirectory());
            core->Close();
        }
    }

    void ControlCoreTests::BackslashPhosphorOpacityWARP()
    {
        // Production WIC loading runs on a render worker (MTA), not the STA test host.
        std::exception_ptr workerFailure;
        std::thread worker([&]() {
            try
            {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                auto uninitialize = wil::scope_exit([] { winrt::uninit_apartment(); });
                using namespace ::Microsoft::Console::Render::Atlas;
                auto [settings, conn] = _createSettingsAndConnection();
                settings->DefaultForeground(til::color{255,255,255});settings->DefaultBackground(til::color{0,0,0});
                settings->RetroTerminalEffect(true);settings->RetroTerminalGlowIntensity(0);settings->RetroTerminalScanlineIntensity(0);settings->RetroTerminalTintIntensity(1);settings->RetroTerminalTintColor(til::color{0,255,65});
                auto core = createCore(*settings, *conn);
                _standardInit(core);
                auto& engine = *core->_renderEngine;
                engine.SetCallback([](HANDLE) {});
                engine.EnableTransparentBackground(false);
                engine.SetGraphicsAPI(GraphicsAPI::Direct3D11);
                VERIFY_SUCCEEDED(engine.UpdateViewport({ 0, 0, 29, 19 }));
                auto diagnostics = std::make_shared<::Microsoft::Console::Render::RenderDiagnostics>();
                engine.SetDiagnostics(diagnostics);
                auto& payload = engine._p;
                payload.warningCallback = [](HRESULT hr, wil::zwstring_view message) {
                    Log::Comment(String().Format(L"POSTPROCESS_WARNING 0x%08x %s", static_cast<unsigned int>(hr), message.c_str()));
                };

                wil::com_ptr<ID3D11Device> device;
                wil::com_ptr<ID3D11DeviceContext> context;
                D3D_FEATURE_LEVEL featureLevel{};
                VERIFY_SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                    D3D11_SDK_VERSION, device.put(), &featureLevel, context.put()));
                payload.device = device.query<ID3D11Device2>();
                payload.deviceContext = context.query<ID3D11DeviceContext2>();
                const auto dxgiDevice = device.query<IDXGIDevice>();
                wil::com_ptr<IDXGIAdapter> adapter;
                VERIFY_SUCCEEDED(dxgiDevice->GetAdapter(adapter.put()));
                wil::com_ptr<IDXGIFactory2> factory;
                VERIFY_SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(factory.put())));
                const DXGI_SWAP_CHAIN_DESC1 desc{
                    .Width = 270, .Height = 380, .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                    .SampleDesc = { 1, 0 }, .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                    .BufferCount = 2, .Scaling = DXGI_SCALING_STRETCH,
                    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
                    .AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED,
                };
                wil::com_ptr<IDXGISwapChain1> swapChain;
                VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(device.get(), &desc, nullptr, swapChain.put()));
                payload.swapChain.swapChain = swapChain.query<IDXGISwapChain2>();
                BackendD3D backend{ payload };

                // Draw into an unattached composition buffer and read it back. Never Present,
                // attach to a XAML surface, create a window, or depend on compositor pacing.
                auto alphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
                auto opacity=[&](float value) {
                    core->_setOpacity(value);engine._resolveTransparencySettings();
                    const auto desired=value < 1 ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
                    if(desired!=alphaMode) {
                        backend.ReleaseResources();context->ClearState();payload.swapChain.swapChain.reset();swapChain.reset();context->Flush();
                        auto next=desc;next.AlphaMode=desired;VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(device.get(),&next,nullptr,swapChain.put()));payload.swapChain.swapChain=swapChain.query<IDXGISwapChain2>();alphaMode=desired;
                    }
                    VERIFY_ARE_EQUAL(value,core->Opacity());
                    VERIFY_ARE_EQUAL(value<1,engine._api.s->target->useAlpha);
                    VERIFY_ARE_EQUAL(1.0f,engine._api.s->retroTerminalTintIntensity);
                };
                auto draw = [&](uint32_t background) {
                    VERIFY_SUCCEEDED(engine.UpdateDrawingBrushes({}, core->_terminal->GetRenderSettings(), gsl::make_not_null(core->_terminal.get()), false, true));
                    VERIFY_SUCCEEDED(engine.UpdateDrawingBrushes({}, core->_terminal->GetRenderSettings(), gsl::make_not_null(core->_terminal.get()), false, false));
                    engine._api.s.write()->misc.write()->backgroundColor = background;
                    VERIFY_SUCCEEDED(engine.InvalidateAll());
                    VERIFY_SUCCEEDED(engine.StartPaint());
                    // Fill only the unpainted default cells, before real paint calls.
                    // Explicit cell and selection backgrounds must survive EndPaint.
                    std::fill(payload.backgroundBitmap.begin(), payload.backgroundBitmap.end(), background);
                    payload.colorBitmapGenerations[0].bump();
                    const std::array<til::point_span, 1> selection{ til::point_span{ { 0, 3 }, { 1, 3 } } };
                    ::Microsoft::Console::Render::RenderFrameInfo info{};
                    info.selectionSpans = selection;
                    info.selectionBackground = til::color{ 255, 0, 0 };
                    VERIFY_SUCCEEDED(engine.PrepareRenderInfo(info));
                    const std::array<::Microsoft::Console::Render::Cluster, 2> text{
                        ::Microsoft::Console::Render::Cluster{ L"A", 1 },
                        ::Microsoft::Console::Render::Cluster{ L"B", 1 },
                    };
                    VERIFY_SUCCEEDED(engine.PaintBufferLine(text, { 0, 0 }, false));
                    engine._flushBufferLine();
                    TextAttribute red;red.SetForeground(RGB(255,0,0));
                    VERIFY_SUCCEEDED(engine.UpdateDrawingBrushes(red,core->_terminal->GetRenderSettings(),gsl::make_not_null(core->_terminal.get()),false,false));
                    VERIFY_SUCCEEDED(engine.PaintBufferLine(text,{0,1},false));
                    engine._flushBufferLine();
                    const std::array<::Microsoft::Console::Render::Cluster, 2> spaces{
                        ::Microsoft::Console::Render::Cluster{ L" ", 1 },
                        ::Microsoft::Console::Render::Cluster{ L" ", 1 },
                    };
                    auto paintCells = [&](COLORREF color, til::point position) {
                        TextAttribute attributes;
                        attributes.SetBackground(color);
                        VERIFY_SUCCEEDED(engine.UpdateDrawingBrushes(attributes, core->_terminal->GetRenderSettings(), gsl::make_not_null(core->_terminal.get()), false, false));
                        VERIFY_SUCCEEDED(engine.PaintBufferLine(spaces, position, false));
                        engine._flushBufferLine();
                    };
                    paintCells(RGB(0, 0, 255), { 0, 2 });
                    paintCells(RGB(0, 0, 0), { 0, 3 }); // Selection replaces the painted background.
                    paintCells(RGB(0, 0, 0), { 0, 4 });
                    paintCells(RGB(255, 255, 255), { 2, 4 });
                    ::Microsoft::Console::Render::CursorOptions cursor{};
                    cursor.coordCursor = { 4, 2 };
                    cursor.cursorType = CursorType::FullBox;
                    cursor.fUseColor = true;
                    cursor.cursorColor = RGB(255, 0, 0);
                    cursor.isVisible = cursor.isOn = cursor.inViewport = true;
                    VERIFY_SUCCEEDED(engine.PaintCursor(cursor));
                    VERIFY_SUCCEEDED(engine.EndPaint());
                    backend.Render(payload);

                    wil::com_ptr<ID3D11Texture2D> target;
                    VERIFY_SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(target.put())));
                    D3D11_TEXTURE2D_DESC readDesc{};
                    target->GetDesc(&readDesc);
                    readDesc.Usage = D3D11_USAGE_STAGING;
                    readDesc.BindFlags = 0;
                    readDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    readDesc.MiscFlags = 0;
                    wil::com_ptr<ID3D11Texture2D> staging;
                    VERIFY_SUCCEEDED(device->CreateTexture2D(&readDesc, nullptr, staging.put()));
                    context->CopyResource(staging.get(), target.get());
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    VERIFY_SUCCEEDED(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
                    std::vector<uint32_t> pixels(static_cast<size_t>(readDesc.Width) * readDesc.Height);
                    for (UINT y = 0; y < readDesc.Height; ++y)
                    {
                        memcpy(pixels.data() + static_cast<size_t>(y) * readDesc.Width,
                            static_cast<const char*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch,
                            static_cast<size_t>(readDesc.Width) * sizeof(uint32_t));
                    }
                    context->Unmap(staging.get(), 0);
                    return pixels;
                };

                const std::array<float,5> values{1.0f,0.99f,0.5f,0.0f,1.0f};
                std::vector<std::vector<uint32_t>> tinted,plain;
                for(auto value:values){opacity(value);tinted.push_back(draw(value<1?0u:0xff000000u));}
                const auto beforeBypass=diagnostics->postprocessDraws.load();VERIFY_IS_TRUE(beforeBypass>=5);
                engine.SetRetroTerminalEffect(false);
                for(auto value:values){opacity(value);plain.push_back(draw(value<1?0u:0xff000000u));}
                // Full intensity must color white, including transparent AA coverage.
                // Relative luminance drives a black-to-selected-phosphor ramp; white
                // never escapes to a neutral endpoint for this colored phosphor.
                auto linear = [](double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
                auto encoded = [](double c) { return c <= 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055; };
                auto expected = [&](uint32_t pixel, uint32_t endpoint = 0xff00ff41) {
                    const auto alpha = (pixel >> 24) / 255.0;
                    if (alpha == 0) { return uint32_t{ 0 }; }
                    const std::array<double, 3> original{
                        linear(std::min(1.0, ((pixel >> 16) & 255) / (255.0 * alpha))),
                        linear(std::min(1.0, ((pixel >> 8) & 255) / (255.0 * alpha))),
                        linear(std::min(1.0, (pixel & 255) / (255.0 * alpha))),
                    };
                    const std::array<double, 3> phosphor{ linear(((endpoint>>16)&255)/255.0), linear(((endpoint>>8)&255)/255.0), linear((endpoint&255)/255.0) };
                    const auto luminance = original[0] * .2126 + original[1] * .7152 + original[2] * .0722;
                    uint32_t result = pixel & 0xff000000;
                    for (size_t channel = 0; channel < 3; ++channel)
                    {
                        const auto mapped = phosphor[channel] * luminance;
                        const auto value = static_cast<uint32_t>(std::lround(encoded(std::clamp(mapped, 0.0, 1.0)) * alpha * 255));
                        result |= value << (16 - 8 * channel);
                    }
                    return result;
                };
                auto difference = [](uint32_t a, uint32_t b) {
                    int worst = 0;
                    for (const auto shift : { 0, 8, 16, 24 })
                    {
                        worst = std::max(worst, std::abs(static_cast<int>((a >> shift) & 255) - static_cast<int>((b >> shift) & 255)));
                    }
                    return worst;
                };
                const auto cell=payload.s->font->cellSize;
                for(size_t stage=0;stage<values.size();++stage)
                {
                    size_t whitePixels=0,whiteTinted=0,redPixels=0,redTinted=0,transparentPixels=0,alphaMismatch=0,holeMismatch=0,whiteEndpoints=0,endpointMismatch=0;
                    int oracleError = 0;
                    for(int y=0;y<2*cell.y;++y)for(int x=0;x<2*cell.x;++x)
                    {
                        const auto index=static_cast<size_t>(y)*desc.Width+x;const auto raw=plain[stage][index],out=tinted[stage][index];
                        const auto r=(raw>>16)&255,g=(raw>>8)&255,b=raw&255,a=raw>>24;
                        const auto rr=(out>>16)&255,gg=(out>>8)&255,bb=out&255;
                        oracleError = std::max(oracleError, difference(expected(raw), out));
                        if(a>8 && r==a && g==a && b==a){
                            ++whiteEndpoints;
                            const auto chosenPremultiplied = (raw & 0xff000000) | (a << 8) | static_cast<uint32_t>(std::lround(a * 65.0 / 255));
                            if(difference(chosenPremultiplied,out)>1){++endpointMismatch;}
                        }
                        if(a!=(out>>24)){++alphaMismatch;}
                        if(y<cell.y && r>8 && g>8 && b>8){++whitePixels;if(gg>rr && gg>bb){++whiteTinted;}}
                        if(y>=cell.y && r>8){++redPixels;if(gg>rr && gg>bb){++redTinted;}}
                    }
                    for(size_t i=0;i<tinted[stage].size();++i){if((plain[stage][i]>>24)==0){++transparentPixels;if(tinted[stage][i]!=0){++holeMismatch;}}}
                    Log::Comment(String().Format(L"PHOSPHOR_OPACITY %.2f white=%zu whiteTinted=%zu red=%zu redTinted=%zu transparent=%zu postprocess=%llu",values[stage],whitePixels,whiteTinted,redPixels,redTinted,transparentPixels,static_cast<unsigned long long>(beforeBypass)));
                    VERIFY_IS_TRUE(alphaMismatch==0);VERIFY_IS_TRUE(holeMismatch==0);VERIFY_IS_TRUE(whitePixels>0);VERIFY_IS_TRUE(redPixels>0);VERIFY_IS_TRUE(redTinted>0);
                    if(values[stage]<1){VERIFY_IS_TRUE(transparentPixels>0);}
                    VERIFY_ARE_EQUAL(whitePixels,whiteTinted,L"Every visible white glyph sample must retain the phosphor hue at all opacities");
                    VERIFY_IS_TRUE(whiteEndpoints > 0);VERIFY_IS_TRUE(endpointMismatch==0,L"White maps to the selected phosphor without changing coverage alpha");
                    auto sample = [&](const auto& frame, int x, int y) { return frame[(y * cell.y + cell.y / 2) * desc.Width + x * cell.x + cell.x / 2]; };
                    const std::array<const wchar_t*, 5> roles{ L"blue-cell", L"red-cursor", L"red-selection", L"black-endpoint", L"white-endpoint" };
                    const std::array<til::point, 5> positions{ til::point{0,2}, til::point{4,2}, til::point{0,3}, til::point{0,4}, til::point{2,4} };
                    const std::array<uint32_t, 5> originals{ 0xff0000ff, 0xffff0000, 0xffff0000, 0xff000000, 0xffffffff };
                    for(size_t role=0;role<roles.size();++role)
                    {
                        const auto raw=sample(plain[stage],positions[role].x,positions[role].y);
                        const auto out=sample(tinted[stage],positions[role].x,positions[role].y);
                        VERIFY_ARE_EQUAL(originals[role],raw);
                        oracleError=std::max(oracleError,difference(expected(raw),out));
                        VERIFY_ARE_EQUAL(raw>>24,out>>24);
                        if(role==3){VERIFY_IS_TRUE(difference(raw,out)<=1);}else{VERIFY_IS_TRUE(difference(raw,out)>8);}
                        if(role==4){VERIFY_IS_TRUE(difference(uint32_t{0xff00ff41},out)<=1,L"Opaque white cell maps to the selected phosphor endpoint");}
                        Log::Comment(String().Format(L"PHOSPHOR_ROLE %.2f %s raw=%08x tinted=%08x",values[stage],roles[role],raw,out));
                    }
                    VERIFY_IS_TRUE(oracleError<=2,L"Actual glyph/cell/cursor/selection pixels follow the hue-retaining ramp at every opacity");
                    Log::Comment(String().Format(L"PHOSPHOR_ORACLE %.2f maxChannelError=%d whiteEndpoints=%zu",values[stage],oracleError,whiteEndpoints));
                }
                // Stable colored output at 99/50/0 proves background opacity does not
                // scale terminal-content tint. Returning to opaque restores the AA samples.
                VERIFY_IS_TRUE(tinted[1] == tinted[2] && tinted[2] == tinted[3]);
                VERIFY_IS_TRUE(tinted[0] == tinted[4]);
                // Representative warm/cool phosphors must also color white glyphs,
                // with the same coverage and live opacity behavior as green.
                engine.SetRetroTerminalEffect(true);
                for(const auto endpoint : { uint32_t{0xffffb000}, uint32_t{0xff0000ff}, uint32_t{0xffb450ff} })
                {
                    engine.SetRetroTerminalTintColor(RGB((endpoint>>16)&255,(endpoint>>8)&255,endpoint&255));
                    std::vector<std::vector<uint32_t>> frames;
                    for(size_t stage=0;stage<values.size();++stage)
                    {
                        opacity(values[stage]);
                        frames.push_back(draw(values[stage]<1?0u:0xff000000u));
                        size_t whiteCount=0,coloredWhite=0,alphaErrors=0;
                        int oracleError=0;
                        for(int y=0;y<cell.y;++y)for(int x=0;x<2*cell.x;++x)
                        {
                            const auto i=static_cast<size_t>(y)*desc.Width+x;
                            const auto raw=plain[stage][i],out=frames.back()[i];
                            const auto r=(raw>>16)&255,g=(raw>>8)&255,b=raw&255;
                            if(r>8 && g>8 && b>8){++whiteCount;if(((out>>16)&255)!=((out>>8)&255) || ((out>>8)&255)!=(out&255)){++coloredWhite;}}
                            oracleError=std::max(oracleError,difference(expected(raw,endpoint),out));
                            alphaErrors+=(raw>>24)!=(out>>24);
                        }
                        const auto whiteIndex=(4*cell.y+cell.y/2)*desc.Width+2*cell.x+cell.x/2;
                        VERIFY_IS_TRUE(difference(endpoint,frames.back()[whiteIndex])<=1);
                        VERIFY_IS_TRUE(whiteCount>0);VERIFY_ARE_EQUAL(whiteCount,coloredWhite);
                        VERIFY_IS_TRUE(alphaErrors==0);VERIFY_IS_TRUE(oracleError<=2);
                        Log::Comment(String().Format(L"PHOSPHOR_PALETTE %08x %.2f white=%zu colored=%zu alphaErrors=%zu maxChannelError=%d",endpoint,values[stage],whiteCount,coloredWhite,alphaErrors,oracleError));
                    }
                    VERIFY_IS_TRUE(frames[1]==frames[2] && frames[2]==frames[3]);
                    VERIFY_IS_TRUE(frames[0]==frames[4]);
                }
                // Zero intensity is an exact terminal-content identity and needs no pass.
                opacity(.5f);engine.SetRetroTerminalTintIntensity(0);
                const auto drawsBeforeZero=diagnostics->postprocessDraws.load();
                VERIFY_IS_TRUE(draw(0)==plain[2]);
                VERIFY_ARE_EQUAL(drawsBeforeZero,diagnostics->postprocessDraws.load());
                core->Close();
            }
            catch (...) { workerFailure=std::current_exception(); }
        });
        worker.join();if(workerFailure){std::rethrow_exception(workerFailure);}
    }

    void ControlCoreTests::BackslashPostProcessWARP()
    {
        // Production WIC loading runs on a render worker (MTA), not the STA test host.
        std::exception_ptr workerFailure;
        std::thread worker([&]() {
            try
            {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                auto uninitialize = wil::scope_exit([] { winrt::uninit_apartment(); });
                using namespace ::Microsoft::Console::Render::Atlas;
                auto [settings, conn] = _createSettingsAndConnection();
                auto core = createCore(*settings, *conn);
                _standardInit(core);
                auto& engine = *core->_renderEngine;
                engine.SetCallback([](HANDLE) {});
                engine.EnableTransparentBackground(true);
                engine.SetGraphicsAPI(GraphicsAPI::Direct3D11);
                VERIFY_SUCCEEDED(engine.UpdateViewport({ 0, 0, 29, 19 }));
                auto diagnostics = std::make_shared<::Microsoft::Console::Render::RenderDiagnostics>();
                engine.SetDiagnostics(diagnostics);
                auto& payload = engine._p;
                payload.warningCallback = [](HRESULT hr, wil::zwstring_view message) {
                    Log::Comment(String().Format(L"POSTPROCESS_WARNING 0x%08x %s", static_cast<unsigned int>(hr), message.c_str()));
                };

                wil::com_ptr<ID3D11Device> device;
                wil::com_ptr<ID3D11DeviceContext> context;
                D3D_FEATURE_LEVEL featureLevel{};
                VERIFY_SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                    D3D11_SDK_VERSION, device.put(), &featureLevel, context.put()));
                payload.device = device.query<ID3D11Device2>();
                payload.deviceContext = context.query<ID3D11DeviceContext2>();
                const auto dxgiDevice = device.query<IDXGIDevice>();
                wil::com_ptr<IDXGIAdapter> adapter;
                VERIFY_SUCCEEDED(dxgiDevice->GetAdapter(adapter.put()));
                wil::com_ptr<IDXGIFactory2> factory;
                VERIFY_SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(factory.put())));
                const DXGI_SWAP_CHAIN_DESC1 desc{
                    .Width = 270, .Height = 380, .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                    .SampleDesc = { 1, 0 }, .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                    .BufferCount = 2, .Scaling = DXGI_SCALING_STRETCH,
                    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
                    .AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED,
                };
                wil::com_ptr<IDXGISwapChain1> swapChain;
                VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(device.get(), &desc, nullptr, swapChain.put()));
                payload.swapChain.swapChain = swapChain.query<IDXGISwapChain2>();
                BackendD3D backend{ payload };

                // Draw into an unattached composition buffer and read it back. Never Present,
                // attach to a XAML surface, create a window, or depend on compositor pacing.
                auto draw = [&](uint32_t background) {
                    VERIFY_SUCCEEDED(engine.UpdateDrawingBrushes({}, core->_terminal->GetRenderSettings(), gsl::make_not_null(core->_terminal.get()), false, true));
                    engine._api.s.write()->misc.write()->backgroundColor = background;
                    VERIFY_SUCCEEDED(engine.InvalidateAll());
                    VERIFY_SUCCEEDED(engine.StartPaint());
                    const std::array<::Microsoft::Console::Render::Cluster, 2> text{
                        ::Microsoft::Console::Render::Cluster{ L"A", 1 },
                        ::Microsoft::Console::Render::Cluster{ L"B", 1 },
                    };
                    VERIFY_SUCCEEDED(engine.PaintBufferLine(text, { 0, 0 }, false));
                    VERIFY_SUCCEEDED(engine.EndPaint());
                    std::fill(payload.backgroundBitmap.begin(), payload.backgroundBitmap.end(), background);
                    payload.colorBitmapGenerations[0].bump();
                    backend.Render(payload);

                    wil::com_ptr<ID3D11Texture2D> target;
                    VERIFY_SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(target.put())));
                    D3D11_TEXTURE2D_DESC readDesc{};
                    target->GetDesc(&readDesc);
                    readDesc.Usage = D3D11_USAGE_STAGING;
                    readDesc.BindFlags = 0;
                    readDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    readDesc.MiscFlags = 0;
                    wil::com_ptr<ID3D11Texture2D> staging;
                    VERIFY_SUCCEEDED(device->CreateTexture2D(&readDesc, nullptr, staging.put()));
                    context->CopyResource(staging.get(), target.get());
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    VERIFY_SUCCEEDED(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
                    std::vector<uint32_t> pixels(static_cast<size_t>(readDesc.Width) * readDesc.Height);
                    for (UINT y = 0; y < readDesc.Height; ++y)
                    {
                        memcpy(pixels.data() + static_cast<size_t>(y) * readDesc.Width,
                            static_cast<const char*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch,
                            static_cast<size_t>(readDesc.Width) * sizeof(uint32_t));
                    }
                    context->Unmap(staging.get(), 0);
                    return pixels;
                };

                engine.SetRetroTerminalEffect(false);
                engine.SetRetroTerminalGlowIntensity(0.0f);
                engine.SetRetroTerminalScanlineIntensity(0.0f);
                const auto opaqueOff = draw(0xff402010);
                engine.SetRetroTerminalTintColor(RGB(255, 176, 0));
                engine.SetRetroTerminalTintIntensity(0.0f);
                engine.SetRetroTerminalScanlineThickness(8.0f);
                engine.SetRetroTerminalScanlineGap(0.0f);
                engine.SetRetroTerminalEffect(true);
                VERIFY_IS_TRUE(opaqueOff == draw(0xff402010));
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->postprocessDraws.load());
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->shaderRecreates.load());

                engine.SetRetroTerminalEffect(false);
                const auto transparentOff = draw(0x80402010);
                engine.SetRetroTerminalEffect(true);
                VERIFY_IS_TRUE(transparentOff == draw(0x80402010));
                VERIFY_IS_TRUE(std::any_of(transparentOff.begin(), transparentOff.end(), [](uint32_t p) { return (p >> 24) > 0 && (p >> 24) < 255; }));

                engine.SetRetroTerminalScanlineThickness(1.0f);
                engine.SetRetroTerminalScanlineGap(1.0f);
                engine.SetRetroTerminalScanlineIntensity(0.5f);
                draw(0x80402010);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderRecreates.load());
                const auto activeDraws = diagnostics->postprocessDraws.load();
                engine.SetRetroTerminalScanlineThickness(0.5f);
                engine.SetRetroTerminalScanlineGap(4.0f);
                engine.SetRetroTerminalScanlineIntensity(0.7f);
                draw(0x80603020);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderRecreates.load());
                VERIFY_ARE_EQUAL(activeDraws + 1, diagnostics->postprocessDraws.load());
                const auto untinted = draw(0x80603020);
                engine.SetRetroTerminalTintIntensity(1.0f);
                engine.SetRetroTerminalTintColor(RGB(255, 176, 0));
                const auto tinted = draw(0x80603020);
                VERIFY_IS_FALSE(untinted == tinted);
                VERIFY_IS_TRUE(std::equal(untinted.begin(), untinted.end(), tinted.begin(), [](uint32_t a, uint32_t b) { return (a >> 24) == (b >> 24); }));
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderRecreates.load());
                VERIFY_IS_FALSE(backend.RequiresContinuousRedraw());
                engine.SetRetroTerminalTintIntensity(0.4f);
                engine.SetRetroTerminalTintColor(RGB(64, 128, 255));
                engine.SetRetroTerminalScanlineIntensity(0.0f);
                draw(0x80603020);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderRecreates.load());
                VERIFY_IS_GREATER_THAN(diagnostics->offscreenBytes.load(), uint64_t{ 0 });
                engine.SetRetroTerminalTintIntensity(0.0f);
                engine.SetRetroTerminalScanlineIntensity(0.7f);

                backend.ReleaseResources();
                context->ClearState();
                draw(0x80603020);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->textureCreates.load());

                const auto beforeResize = diagnostics->textureCreates.load();
                backend.ReleaseResources();
                context->ClearState();
                VERIFY_SUCCEEDED(swapChain->ResizeBuffers(0, 360, 380, DXGI_FORMAT_UNKNOWN, 0));
                VERIFY_SUCCEEDED(engine.SetWindowSize({ 360, 380 }));
                VERIFY_SUCCEEDED(engine.UpdateViewport({ 0, 0, 39, 19 }));
                draw(0x80603020);
                VERIFY_ARE_EQUAL(beforeResize + 1, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderRecreates.load());
                VERIFY_ARE_EQUAL(uint64_t{ 360 * 380 * 4 }, diagnostics->offscreenBytes.load());

                engine.SetRetroTerminalScanlineIntensity(0.0f);
                const auto beforeNeutralDraws = diagnostics->postprocessDraws.load();
                draw(0x80603020);
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->offscreenBytes.load());
                VERIFY_ARE_EQUAL(beforeNeutralDraws, diagnostics->postprocessDraws.load());
                VERIFY_ARE_EQUAL(diagnostics->textureCreates.load(), diagnostics->textureReleases.load());

                const auto directory = wil::GetEnvironmentVariableW<std::wstring>(L"BACKSLASH_TEST_SHADER_DIRECTORY");
                VERIFY_IS_FALSE(directory.empty());
                const auto shader = std::filesystem::path{ directory } / L"custom.hlsl";
                const auto image = std::filesystem::path{ directory } / L"image.png";
                engine.SetRetroTerminalTintIntensity(1.0f);
                engine.SetPixelShaderPath(shader.native());
                engine.SetPixelShaderImagePath(image.native());
                draw(0x80603020);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderCompiles.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderImageLoads.load());
                VERIFY_IS_TRUE(backend.RequiresContinuousRedraw());
                const auto customTextures = diagnostics->textureCreates.load();
                const auto customRecreates = diagnostics->shaderRecreates.load();
                draw(0x80401010);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderCompiles.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderImageLoads.load());
                VERIFY_ARE_EQUAL(customTextures, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(customRecreates, diagnostics->shaderRecreates.load());

                engine.SetPixelShaderImagePath((std::filesystem::path{ directory } / L"missing.png").native());
                const auto beforeBadImageDraws = diagnostics->postprocessDraws.load();
                draw(0x80401010);
                VERIFY_IS_FALSE(backend.RequiresContinuousRedraw());
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->offscreenBytes.load());
                VERIFY_ARE_EQUAL(beforeBadImageDraws, diagnostics->postprocessDraws.load());
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderCompiles.load());
                engine.SetPixelShaderImagePath(image.native());
                draw(0x80401010);
                VERIFY_ARE_EQUAL(uint64_t{ 1 }, diagnostics->shaderCompiles.load());
                VERIFY_IS_TRUE(backend.RequiresContinuousRedraw());

                const auto missingShader = std::filesystem::path{ directory } / (L"missing-" + std::to_wstring(GetCurrentProcessId()) + L".hlsl");
                engine.SetPixelShaderPath(missingShader.native());
                engine.SetRetroTerminalGlowIntensity(0.3f);
                draw(0x80401010);
                VERIFY_IS_FALSE(backend.RequiresContinuousRedraw());
                VERIFY_ARE_EQUAL(uint64_t{ 0 }, diagnostics->offscreenBytes.load());
                VERIFY_ARE_EQUAL(uint64_t{ 2 }, diagnostics->shaderCompiles.load());
                draw(0x80601010);
                VERIFY_ARE_EQUAL(uint64_t{ 2 }, diagnostics->shaderCompiles.load());
                {
                    std::ofstream file{ missingShader };
                    file << "Texture2D tex:register(t0);SamplerState sam:register(s0);"
                        "float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD):SV_TARGET{return tex.Sample(sam,uv);}";
                }
                draw(0x80601010);
                VERIFY_ARE_EQUAL(uint64_t{ 3 }, diagnostics->shaderCompiles.load());
                VERIFY_IS_FALSE(backend.RequiresContinuousRedraw());
                const auto beforeSourceEditTextures = diagnostics->textureCreates.load();
                const auto beforeSourceEditImages = diagnostics->shaderImageLoads.load();
                {
                    std::ifstream original{ shader };
                    std::ofstream changed{ missingShader };
                    changed << original.rdbuf();
                }
                draw(0x80401010);
                VERIFY_ARE_EQUAL(uint64_t{ 4 }, diagnostics->shaderCompiles.load());
                VERIFY_IS_TRUE(backend.RequiresContinuousRedraw());
                VERIFY_ARE_EQUAL(beforeSourceEditTextures, diagnostics->textureCreates.load());
                VERIFY_ARE_EQUAL(beforeSourceEditImages, diagnostics->shaderImageLoads.load());
                engine.SetPixelShaderPath(L"");
                draw(0x80601010);
                VERIFY_IS_FALSE(backend.RequiresContinuousRedraw());
                VERIFY_IS_GREATER_THAN(diagnostics->offscreenBytes.load(), uint64_t{ 0 });
                Log::Comment(String().Format(L"POSTPROCESS_COUNTERS {\"shaderCompiles\":%llu,\"shaderRecreates\":%llu,\"textureCreates\":%llu,\"textureReleases\":%llu,\"offscreenBytes\":%llu,\"shaderImageLoads\":%llu,\"shaderImageReleases\":%llu,\"postprocessDraws\":%llu}",
                    diagnostics->shaderCompiles.load(), diagnostics->shaderRecreates.load(), diagnostics->textureCreates.load(), diagnostics->textureReleases.load(), diagnostics->offscreenBytes.load(), diagnostics->shaderImageLoads.load(), diagnostics->shaderImageReleases.load(), diagnostics->postprocessDraws.load()));
            }
            catch (...) { workerFailure = std::current_exception(); }
        });
        worker.join();
        if (workerFailure) { std::rethrow_exception(workerFailure); }
    }

    static void _writePrompt(const winrt::com_ptr<MockConnection>& conn, const std::wstring_view& path)
    {
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;D\x7"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;A\x7"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]9;9;"));
        conn->WriteInput(winrt_wstring_to_array_view(path));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x7"));
        conn->WriteInput(winrt_wstring_to_array_view(L"PWSH "));
        conn->WriteInput(winrt_wstring_to_array_view(path));
        conn->WriteInput(winrt_wstring_to_array_view(L"> "));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;B\x7"));
    }

    void ControlCoreTests::TestSelectCommandSimple()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows");
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n"));

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the buffer contents");
        const auto& buffer = core->_terminal->GetTextBuffer();
        const auto& cursor = buffer.GetCursor();

        {
            const til::point expectedCursor{ 17, 4 };
            VERIFY_ARE_EQUAL(expectedCursor, cursor.GetPosition());
        }

        VERIFY_IS_FALSE(core->HasSelection());
        core->SelectCommand(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 17, 0 };
            const til::point expectedEnd{ 24, 0 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }

        core->_terminal->ClearSelection();
        conn->WriteInput(winrt_wstring_to_array_view(L"Boo-far"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        VERIFY_IS_FALSE(core->HasSelection());
        {
            const til::point expectedCursor{ 24, 4 };
            VERIFY_ARE_EQUAL(expectedCursor, cursor.GetPosition());
        }
        VERIFY_IS_FALSE(core->HasSelection());
        core->SelectCommand(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 17, 4 };
            const til::point expectedEnd{ 24, 4 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        core->SelectCommand(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 17, 0 };
            const til::point expectedEnd{ 24, 0 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        core->SelectCommand(false);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 17, 4 };
            const til::point expectedEnd{ 24, 4 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
    }
    void ControlCoreTests::TestSelectOutputSimple()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows");
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n"));

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the buffer contents");
        const auto& buffer = core->_terminal->GetTextBuffer();
        const auto& cursor = buffer.GetCursor();

        {
            const til::point expectedCursor{ 17, 4 };
            VERIFY_ARE_EQUAL(expectedCursor, cursor.GetPosition());
        }

        VERIFY_IS_FALSE(core->HasSelection());
        core->SelectOutput(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 24, 0 }; // The character after the prompt
            const til::point expectedEnd{ 22, 3 }; // x = the end of the text + 1 (exclusive end)
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
    }
    void ControlCoreTests::TestCommandContext()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows");
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n"));

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the command context");

        const WEX::TestExecution::DisableVerifyExceptions disableExceptionsScope;
        {
            auto historyContext{ core->CommandHistory() };
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            VERIFY_ARE_EQUAL(L"", historyContext.CurrentCommandline());
        }

        Log::Comment(L"Write 'Bar' to the command...");
        conn->WriteInput(winrt_wstring_to_array_view(L"Bar"));
        {
            auto historyContext{ core->CommandHistory() };
            // Bar shouldn't be in the history, it should be the current command
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            VERIFY_ARE_EQUAL(L"Bar", historyContext.CurrentCommandline());
        }

        Log::Comment(L"then delete it");
        conn->WriteInput(winrt_wstring_to_array_view(L"\b \b"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\b \b"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\b \b"));
        {
            auto historyContext{ core->CommandHistory() };
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            // The current commandline is now empty
            VERIFY_ARE_EQUAL(L"", historyContext.CurrentCommandline());
        }
    }

    void ControlCoreTests::TestCommandContextWithPwshGhostText()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows");
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n"));

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the command context");

        const WEX::TestExecution::DisableVerifyExceptions disableExceptionsScope;
        {
            auto historyContext{ core->CommandHistory() };
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            VERIFY_ARE_EQUAL(L"", historyContext.CurrentCommandline());
        }

        Log::Comment(L"Write 'BarBar' to the command...");
        conn->WriteInput(winrt_wstring_to_array_view(L"BarBar"));
        {
            auto historyContext{ core->CommandHistory() };
            // BarBar shouldn't be in the history, it should be the current command
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            VERIFY_ARE_EQUAL(L"BarBar", historyContext.CurrentCommandline());
        }

        Log::Comment(L"then move the cursor to the left");
        // This emulates the state the buffer is in when pwsh does it's "ghost
        // text" thing. We don't want to include all that ghost text in the
        // current commandline.
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b[D"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b[D"));
        {
            auto historyContext{ core->CommandHistory() };
            VERIFY_ARE_EQUAL(1u, historyContext.History().Size());
            // The current commandline is only the text to the left of the cursor
            auto curr{ historyContext.CurrentCommandline() };
            VERIFY_ARE_EQUAL(4u, curr.size());
            VERIFY_ARE_EQUAL(L"BarB", curr);
        }
    }

    void ControlCoreTests::TestSelectOutputScrolling()
    {
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows"); // row 0
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar")); // row 0
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n")); // row 1
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n")); // row 2
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n")); // row 3

        _writePrompt(conn, L"C:\\Windows"); // row 4
        conn->WriteInput(winrt_wstring_to_array_view(L"gci"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));

        // enough to scroll
        for (auto i = 0; i < 30; i++) // row 5-34
        {
            conn->WriteInput(winrt_wstring_to_array_view(L"-a--- 2/8/2024  9:47 README\r\n"));
        }

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the buffer contents");
        const auto& buffer = core->_terminal->GetTextBuffer();
        const auto& cursor = buffer.GetCursor();

        {
            const til::point expectedCursor{ 17, 35 };
            VERIFY_ARE_EQUAL(expectedCursor, cursor.GetPosition());
        }

        VERIFY_IS_FALSE(core->HasSelection());

        // The second mark is the first one we'll see
        core->SelectOutput(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 20, 4 }; // The character after the prompt
            const til::point expectedEnd{ 27, 34 }; // x = the end of the text + 1 (exclusive end)
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        core->SelectOutput(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 24, 0 }; // The character after the prompt
            const til::point expectedEnd{ 22, 3 }; // x = the end of the text + 1 (exclusive end)
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
    }

    void ControlCoreTests::TestSelectOutputExactWrap()
    {
        // Just like the TestSelectOutputScrolling test, but these lines will
        // exactly wrap to the right edge of the buffer, to catch a edge case
        // present in `ControlCore::_selectSpan`
        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        Log::Comment(L"Print some text");

        _writePrompt(conn, L"C:\\Windows"); // row 0
        conn->WriteInput(winrt_wstring_to_array_view(L"Foo-bar")); // row 0
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));

        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));
        conn->WriteInput(winrt_wstring_to_array_view(L"This is some text     \r\n")); // row 1
        conn->WriteInput(winrt_wstring_to_array_view(L"with varying amounts  \r\n")); // row 2
        conn->WriteInput(winrt_wstring_to_array_view(L"of whitespace         \r\n")); // row 3

        _writePrompt(conn, L"C:\\Windows"); // row 4
        conn->WriteInput(winrt_wstring_to_array_view(L"gci"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\x1b]133;C\x7"));
        conn->WriteInput(winrt_wstring_to_array_view(L"\r\n"));

        // enough to scroll
        for (auto i = 0; i < 30; i++) // row 5-35
        {
            conn->WriteInput(winrt_wstring_to_array_view(L"-a--- 2/8/2024  9:47 README.md\r\n"));
        }

        _writePrompt(conn, L"C:\\Windows");

        Log::Comment(L"Check the buffer contents");
        const auto& buffer = core->_terminal->GetTextBuffer();
        const auto& cursor = buffer.GetCursor();

        {
            const til::point expectedCursor{ 17, 35 };
            VERIFY_ARE_EQUAL(expectedCursor, cursor.GetPosition());
        }

        VERIFY_IS_FALSE(core->HasSelection());
        // The second mark is the first one we'll see
        core->SelectOutput(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 20, 4 }; // The character after the prompt
            const til::point expectedEnd{ 30, 34 }; // x = the end of the text + 1 (exclusive end)
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        core->SelectOutput(true);
        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 24, 0 }; // The character after the prompt
            const til::point expectedEnd{ 22, 3 }; // x = the end of the text + 1 (exclusive end)
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
    }

    void ControlCoreTests::TestSimpleClickSelection()
    {
        // Create a simple selection with the mouse, then click somewhere else,
        // and confirm the selection got updated.

        auto [settings, conn] = _createSettingsAndConnection();
        Log::Comment(L"Create ControlCore object");
        auto core = createCore(*settings, *conn);
        VERIFY_IS_NOT_NULL(core);
        _standardInit(core);

        // Here, we're using the UpdateSelectionMarkers as a stand-in to check
        // if the selection got updated with the renderer. Standing up a whole
        // dummy renderer for this test would be not very ergonomic. Instead, we
        // are relying on ControlCore::_updateSelectionUI both
        // TriggerSelection()'ing and also rasing this event
        bool expectedSelectionUpdate = false;
        bool gotSelectionUpdate = false;
        core->UpdateSelectionMarkers([&](auto&& /*sender*/, auto&& /*args*/) {
            VERIFY_IS_TRUE(expectedSelectionUpdate);
            expectedSelectionUpdate = false;
            gotSelectionUpdate = true;
        });

        auto needToCopy = false;
        expectedSelectionUpdate = true;
        core->LeftClickOnTerminal(til::point{ 1, 1 },
                                  1,
                                  false,
                                  true,
                                  false,
                                  needToCopy);

        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 1, 1 };
            const til::point expectedEnd{ 1, 1 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        VERIFY_IS_TRUE(gotSelectionUpdate);

        expectedSelectionUpdate = true;
        core->LeftClickOnTerminal(til::point{ 1, 2 },
                                  1,
                                  false,
                                  true,
                                  false,
                                  needToCopy);

        VERIFY_IS_TRUE(core->HasSelection());
        {
            const auto& start = core->_terminal->GetSelectionAnchor();
            const auto& end = core->_terminal->GetSelectionEnd();
            const til::point expectedStart{ 1, 1 };
            const til::point expectedEnd{ 2, 2 };
            VERIFY_ARE_EQUAL(expectedStart, start);
            VERIFY_ARE_EQUAL(expectedEnd, end);
        }
        VERIFY_IS_TRUE(gotSelectionUpdate);
    }
}
