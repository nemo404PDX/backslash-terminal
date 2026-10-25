// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/ColorScheme.h"
#include "../TerminalSettingsModel/resource.h"
#include "../TerminalSettingsAppAdapterLib/TerminalSettings.h"
#include "../inc/LaunchPositionPolicy.h"
using namespace WEX::TestExecution;
using namespace winrt;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::Microsoft::Terminal::Settings::Model;
namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
namespace C = winrt::Microsoft::Terminal::Control;
namespace Core = winrt::Microsoft::Terminal::Core;
namespace SettingsModelUnitTests
{
    class CuratedDefaultsTests
    {
        TEST_CLASS(CuratedDefaultsTests);
        TEST_METHOD(PackagedDefaultsAndAdapter);
        TEST_METHOD(ExplicitSettingsRoundtrip);
        TEST_METHOD(CenteringPrecedence);
        TEST_METHOD(BackslashColorSchemeFreshOnly);
        TEST_METHOD(CheesePuffOptionalColorScheme);
    };
    void CuratedDefaultsTests::PackagedDefaultsAndAdapter()
    {
        const auto settings=M::CascadiaSettings::LoadDefaults();
        VERIFY_ARE_EQUAL(80,settings.GlobalSettings().InitialCols());
        VERIFY_ARE_EQUAL(28,settings.GlobalSettings().InitialRows());
        VERIFY_IS_TRUE(settings.GlobalSettings().CenterOnLaunch());
        VERIFY_IS_TRUE(settings.ActiveProfiles().Size()>=2);
        for(const auto profile:settings.ActiveProfiles())
        {
            const auto appearance=profile.DefaultAppearance();
            VERIFY_ARE_EQUAL(16.0f,profile.FontInfo().FontSize());
            VERIFY_ARE_EQUAL(hstring{L"Cascadia Mono"},profile.FontInfo().FontFace());
            VERIFY_ARE_EQUAL(Core::CursorStyle::FilledBox,appearance.CursorShape());
            VERIFY_ARE_EQUAL(C::ScrollbarState::Hidden,profile.ScrollState());
            VERIFY_IS_FALSE(appearance.RetroTerminalEffect());
            VERIFY_ARE_EQUAL(0.15f,appearance.RetroTerminalGlowIntensity());
            VERIFY_ARE_EQUAL(0.10f,appearance.RetroTerminalScanlineIntensity());
            VERIFY_ARE_EQUAL(4.0f,appearance.RetroTerminalScanlineThickness());
            VERIFY_ARE_EQUAL(4.0f,appearance.RetroTerminalScanlineGap());
            VERIFY_ARE_EQUAL(0.0f,appearance.RetroTerminalTintIntensity());
            VERIFY_ARE_EQUAL(til::color(0,255,65),til::color{appearance.RetroTerminalTintColor()});
            VERIFY_IS_FALSE(profile.RetroBezelEnabled());
            VERIFY_ARE_EQUAL(RetroBezelBadge::None,profile.RetroBezelBadge());
            VERIFY_IS_FALSE(profile.RetroBezelCenterFullscreen());
            const auto adapted=TerminalSettings::CreateWithProfile(settings,profile);
            const auto control=adapted.DefaultSettings();
            VERIFY_ARE_EQUAL(16.0f,control->FontSize());
            VERIFY_ARE_EQUAL(Core::CursorStyle::FilledBox,control->CursorShape());
            VERIFY_ARE_EQUAL(C::ScrollbarState::Hidden,control->ScrollState());
            VERIFY_ARE_EQUAL(28,control->InitialRows());VERIFY_ARE_EQUAL(80,control->InitialCols());
            VERIFY_IS_FALSE(control->RetroTerminalEffect());VERIFY_IS_FALSE(control->RetroBezelEnabled());
            VERIFY_ARE_EQUAL(0,control->RetroBezelBadge());
            VERIFY_ARE_EQUAL(0.15f,control->RetroTerminalGlowIntensity());VERIFY_ARE_EQUAL(0.10f,control->RetroTerminalScanlineIntensity());
            VERIFY_ARE_EQUAL(4.0f,control->RetroTerminalScanlineThickness());VERIFY_ARE_EQUAL(4.0f,control->RetroTerminalScanlineGap());
        }
        const auto raw=make_self<M::Profile>();
        VERIFY_ARE_EQUAL(16.0f,raw->FontInfo().FontSize());
        VERIFY_ARE_EQUAL(Core::CursorStyle::FilledBox,raw->DefaultAppearance().CursorShape());
        VERIFY_ARE_EQUAL(C::ScrollbarState::Hidden,raw->ScrollState());
    }
    void CuratedDefaultsTests::ExplicitSettingsRoundtrip()
    {
        const std::string user=R"({"initialCols":101,"initialRows":37,"centerOnLaunch":false,"initialPosition":"19,27",
            "profiles":{"defaults":{"font":{"size":11},"cursorShape":"bar","scrollbarState":"visible",
                "experimental.retroTerminalEffect":true,"experimental.retroTerminalGlowIntensity":0.25,
                "experimental.retroTerminalScanlineIntensity":0.85,"experimental.retroTerminalScanlineThickness":1,
                "experimental.retroTerminalScanlineGap":2,"experimental.retroBezelEnabled":true,
                "experimental.retroBezelBadge":"top","experimental.retroBezelCenterFullscreen":true},
                "list":[{"guid":"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}","font":{"size":18},
                    "experimental.retroTerminalGlowIntensity":0.7,"experimental.retroTerminalTintColor":"#B450FF"}]}})";
        const auto inbox=M::LoadStringResource(IDR_DEFAULTS);
        const auto settings=make_self<M::CascadiaSettings>(std::string_view{user},inbox);
        const auto verify=[](const com_ptr<M::CascadiaSettings>& value){
            VERIFY_ARE_EQUAL(101,value->GlobalSettings().InitialCols());VERIFY_ARE_EQUAL(37,value->GlobalSettings().InitialRows());
            VERIFY_IS_FALSE(value->GlobalSettings().CenterOnLaunch());
            const auto profile=value->FindProfile(guid{L"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}"});
            VERIFY_ARE_EQUAL(18.0f,profile.FontInfo().FontSize());
            VERIFY_ARE_EQUAL(Core::CursorStyle::Bar,profile.DefaultAppearance().CursorShape());
            VERIFY_ARE_EQUAL(C::ScrollbarState::Visible,profile.ScrollState());
            VERIFY_IS_TRUE(profile.DefaultAppearance().RetroTerminalEffect());VERIFY_IS_TRUE(profile.RetroBezelEnabled());
            VERIFY_ARE_EQUAL(0.7f,profile.DefaultAppearance().RetroTerminalGlowIntensity());
            VERIFY_ARE_EQUAL(0.85f,profile.DefaultAppearance().RetroTerminalScanlineIntensity());
            VERIFY_ARE_EQUAL(1.0f,profile.DefaultAppearance().RetroTerminalScanlineThickness());
            VERIFY_ARE_EQUAL(2.0f,profile.DefaultAppearance().RetroTerminalScanlineGap());
            VERIFY_ARE_EQUAL(RetroBezelBadge::Top,profile.RetroBezelBadge());VERIFY_IS_TRUE(profile.RetroBezelCenterFullscreen());
            VERIFY_ARE_EQUAL(til::color(180,80,255),til::color{profile.DefaultAppearance().RetroTerminalTintColor()});
        };
        verify(settings);
        const auto saved=Json::writeString(Json::StreamWriterBuilder{},settings->ToJson());
        const auto reloaded=make_self<M::CascadiaSettings>(std::string_view{saved},inbox);verify(reloaded);
        const auto position=reloaded->GlobalSettings().InitialPosition();
        VERIFY_ARE_EQUAL(19,position.X.Value());VERIFY_ARE_EQUAL(27,position.Y.Value());
    }
    void CuratedDefaultsTests::BackslashColorSchemeFreshOnly()
    {
        const auto inbox = M::LoadStringResource(IDR_DEFAULTS);
        const auto fresh = make_self<M::CascadiaSettings>(M::LoadStringResource(IDR_USER_DEFAULTS), inbox);
        VERIFY_IS_TRUE(fresh->GlobalSettings().ColorSchemes().HasKey(L"Backslash"));
        for (const auto name : { L"Cyberpunk Neon", L"Hermes", L"Hermes Aurora", L"Ubuntu", L"Ubuntu-24.04-ColorScheme" })
            VERIFY_IS_TRUE(fresh->GlobalSettings().ColorSchemes().HasKey(name));
        VERIFY_ARE_EQUAL(hstring{ L"Backslash" }, fresh->ProfileDefaults().DefaultAppearance().DarkColorSchemeName());
        for (const auto profile : fresh->ActiveProfiles())
        {
            VERIFY_ARE_EQUAL(hstring{ L"Backslash" }, profile.DefaultAppearance().DarkColorSchemeName());
            VERIFY_ARE_EQUAL(hstring{ L"Backslash" }, profile.DefaultAppearance().LightColorSchemeName());
            VERIFY_IS_FALSE(profile.DefaultAppearance().HasDarkColorSchemeName());
        }
        const auto scheme = fresh->GlobalSettings().ColorSchemes().Lookup(L"Backslash");
        VERIFY_ARE_EQUAL(til::color(0x16,0x18,0x18), til::color{ scheme.Background() });
        VERIFY_ARE_EQUAL(til::color(0x14,0xff,0xca), til::color{ scheme.CursorColor() });
        VERIFY_ARE_EQUAL(size_t{ 16 }, scheme.Table().size());
        const auto saved = Json::writeString(Json::StreamWriterBuilder{}, fresh->ToJson());
        const auto restored = make_self<M::CascadiaSettings>(std::string_view{ saved }, inbox);
        VERIFY_ARE_EQUAL(hstring{ L"Backslash" }, restored->ProfileDefaults().DefaultAppearance().DarkColorSchemeName());

        // An existing file with no color choice retains the historical fallback.
        const auto existing = make_self<M::CascadiaSettings>(std::string_view{ R"({"profiles":{"list":[]}})" }, inbox);
        for (const auto profile : existing->ActiveProfiles())
            VERIFY_ARE_EQUAL(hstring{ L"Campbell" }, profile.DefaultAppearance().DarkColorSchemeName());

        // ColorScheme::FromJson requires all 16 ANSI colors. A name/background-only
        // object is rejected, so use a complete valid custom scheme for this check.
        auto customScheme = get_self<M::ColorScheme>(scheme)->ToJson();
        customScheme["background"] = "#101112";
        const auto overriddenJson = std::string{ R"({"profiles":{"defaults":{"colorScheme":"Backslash"},"list":[{"guid":"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}","colorScheme":{"dark":"Campbell","light":"Campbell Powershell"},"foreground":"#ABCDEF"}]},"schemes":[)" } + Json::writeString(Json::StreamWriterBuilder{}, customScheme) + "]}";
        const auto overridden = make_self<M::CascadiaSettings>(std::string_view{ overriddenJson }, inbox);
        const auto profile = overridden->FindProfile(guid{ L"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}" });
        VERIFY_ARE_EQUAL(hstring{ L"Campbell" }, profile.DefaultAppearance().DarkColorSchemeName());
        VERIFY_ARE_EQUAL(hstring{ L"Campbell Powershell" }, profile.DefaultAppearance().LightColorSchemeName());
        VERIFY_ARE_EQUAL(til::color(0xab,0xcd,0xef), til::color{ profile.DefaultAppearance().Foreground().Value() });
        VERIFY_ARE_EQUAL(til::color(0x10,0x11,0x12), til::color{ overridden->GlobalSettings().ColorSchemes().Lookup(overridden->ProfileDefaults().DefaultAppearance().DarkColorSchemeName()).Background() });
    }

    void CuratedDefaultsTests::CheesePuffOptionalColorScheme()
    {
        const auto inbox = M::LoadStringResource(IDR_DEFAULTS);
        const auto fresh = make_self<M::CascadiaSettings>(M::LoadStringResource(IDR_USER_DEFAULTS), inbox);
        VERIFY_ARE_EQUAL(hstring{ L"Backslash" }, fresh->ProfileDefaults().DefaultAppearance().DarkColorSchemeName());
        const auto scheme = fresh->GlobalSettings().ColorSchemes().Lookup(L"Cheese Puff");
        VERIFY_ARE_EQUAL(til::color(0x21,0x17,0x0f), til::color{ scheme.Background() });
        VERIFY_ARE_EQUAL(til::color(0xf6,0xe7,0xcd), til::color{ scheme.Foreground() });
        VERIFY_ARE_EQUAL(til::color(0xff,0xc4,0x5c), til::color{ scheme.CursorColor() });
        VERIFY_ARE_EQUAL(til::color(0x6b,0x49,0x26), til::color{ scheme.SelectionBackground() });
        const auto table = scheme.Table();
        VERIFY_ARE_EQUAL(size_t{ 16 }, table.size());
        for (uint32_t i = 0; i < 8; ++i)
            VERIFY_ARE_NOT_EQUAL(til::color{ table[i] }, til::color{ table[i + 8] });
        for (uint32_t i = 1; i < 7; ++i)
            for (uint32_t j = i + 1; j < 7; ++j)
                VERIFY_ARE_NOT_EQUAL(til::color{ table[i] }, til::color{ table[j] });

        const auto selected = make_self<M::CascadiaSettings>(std::string_view{ R"({"profiles":{"defaults":{"colorScheme":"Cheese Puff"},"list":[{"guid":"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}","colorScheme":"Campbell"}]}})" }, inbox);
        VERIFY_ARE_EQUAL(hstring{ L"Cheese Puff" }, selected->ProfileDefaults().DefaultAppearance().DarkColorSchemeName());
        const auto profile = selected->FindProfile(guid{ L"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}" });
        VERIFY_ARE_EQUAL(hstring{ L"Campbell" }, profile.DefaultAppearance().DarkColorSchemeName());
        const auto saved = Json::writeString(Json::StreamWriterBuilder{}, selected->ToJson());
        const auto restored = make_self<M::CascadiaSettings>(std::string_view{ saved }, inbox);
        VERIFY_ARE_EQUAL(hstring{ L"Cheese Puff" }, restored->ProfileDefaults().DefaultAppearance().DarkColorSchemeName());
        VERIFY_ARE_EQUAL(hstring{ L"Campbell" }, restored->FindProfile(profile.Guid()).DefaultAppearance().DarkColorSchemeName());
    }

    void CuratedDefaultsTests::CenteringPrecedence()
    {
        namespace P=::Microsoft::Terminal::Backslash::LaunchPositionPolicy;
        VERIFY_IS_FALSE(P::ShouldCenter({}));
        VERIFY_IS_TRUE(P::ShouldCenter({.enabled=true})); // Includes ordinary launches without AppArgs.
        VERIFY_IS_FALSE(P::ShouldCenter({.enabled=true,.settingPosition=true}));
        VERIFY_IS_FALSE(P::ShouldCenter({.enabled=true,.commandlinePosition=true}));
        VERIFY_IS_FALSE(P::ShouldCenter({.enabled=true,.persistedPosition=true}));
        VERIFY_IS_FALSE(P::ShouldCenter({.enabled=true,.contentBounds=true}));
        VERIFY_IS_FALSE(P::ShouldCenter({.enabled=true,.settingPosition=true,.commandlinePosition=true,.persistedPosition=true,.contentBounds=true}));
    }
}
