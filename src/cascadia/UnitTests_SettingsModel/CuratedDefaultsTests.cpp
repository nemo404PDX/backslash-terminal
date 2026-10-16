// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
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
