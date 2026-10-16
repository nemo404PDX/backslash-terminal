// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"

#include "../TerminalSettingsModel/ColorScheme.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/resource.h"
#include "../TerminalSettingsModel/EnumMappings.h"
#include "JsonTestClass.h"
#include "../inc/RetroBezelGeometry.h"

using namespace Microsoft::Console;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace WEX::Common;

namespace SettingsModelUnitTests
{
    class ProfileTests : public JsonTestClass
    {
        TEST_CLASS(ProfileTests);

        TEST_METHOD(BackslashBezelModel);
        TEST_METHOD(BackslashBezelPaletteRoundtrip);
        TEST_METHOD(BackslashBezelGeometry);
        TEST_METHOD(ProfileGeneratesGuid);
        TEST_METHOD(LayerProfileProperties);
        TEST_METHOD(LayerProfileIcon);
        TEST_METHOD(LayerProfilesOnArray);
        TEST_METHOD(ProfileWithEnvVars);
        TEST_METHOD(ProfileWithEnvVarsSameNameDifferentCases);

        TEST_METHOD(DuplicateProfileTest);
        TEST_METHOD(TestGenGuidsForProfiles);
        TEST_METHOD(TestCorrectOldDefaultShellPaths);
        TEST_METHOD(ProfileDefaultsProhibitedSettings);
    };

    void ProfileTests::BackslashBezelModel()
    {
        const auto defaults = winrt::make_self<implementation::Profile>();
        VERIFY_IS_FALSE(defaults->RetroBezelEnabled());
        VERIFY_ARE_EQUAL(RetroBezelSize::Medium, defaults->RetroBezelSize());
        VERIFY_ARE_EQUAL(RetroBezelColor::OffWhite, defaults->RetroBezelColor());
        VERIFY_ARE_EQUAL(RetroBezelBadge::None, defaults->RetroBezelBadge());
        VERIFY_IS_FALSE(defaults->RetroBezelCenterFullscreen());
        defaults->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelEnabled":true,
            "experimental.retroBezelSize":"wide", "experimental.retroBezelColor":"agedYellow",
            "experimental.retroBezelBadge":"top", "experimental.retroBezelCenterFullscreen":false})"));
        const auto profile = winrt::make_self<implementation::Profile>();
        profile->AddLeastImportantParent(defaults);
        VERIFY_IS_TRUE(profile->RetroBezelEnabled());
        VERIFY_IS_FALSE(profile->HasRetroBezelEnabled());
        VERIFY_ARE_EQUAL(*defaults, profile->RetroBezelColorOverrideSource());
        VERIFY_ARE_EQUAL(RetroBezelBadge::Top, profile->RetroBezelBadge());
        VERIFY_IS_FALSE(profile->RetroBezelCenterFullscreen());
        const auto input = VerifyParseSucceeded(R"({"experimental.retroBezelEnabled":false,
            "experimental.retroBezelSize":"slim", "experimental.retroBezelColor":"cream",
            "experimental.retroBezelBadge":"none", "experimental.retroBezelCenterFullscreen":true})");
        profile->LayerJson(input);
        VERIFY_ARE_EQUAL(input, profile->ToJson());
        VERIFY_IS_FALSE(profile->RetroBezelEnabled());
        VERIFY_IS_TRUE(profile->HasRetroBezelEnabled());
        const auto copy = profile->CopySettings();
        VERIFY_ARE_EQUAL(input, copy->ToJson());
        VERIFY_THROWS(profile->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelSize":"enormous"})")), std::exception);
        VERIFY_ARE_EQUAL(RetroBezelSize::Slim, profile->RetroBezelSize());
        VERIFY_THROWS(profile->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelColor":42})")), std::exception);
        VERIFY_THROWS(profile->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelBadge":"manufacturer"})")), std::exception);
        // Casing setters preserve independently owned text colors and CRT intensities.
        profile->DefaultAppearance().DarkColorSchemeName(L"Campbell");
        profile->DefaultAppearance().RetroTerminalGlowIntensity(0.0f);
        profile->RetroBezelEnabled(true);
        profile->RetroBezelColor(RetroBezelColor::LightYellow);
        VERIFY_ARE_EQUAL(L"Campbell", profile->DefaultAppearance().DarkColorSchemeName());
        VERIFY_ARE_EQUAL(0.0f, profile->DefaultAppearance().RetroTerminalGlowIntensity());
        profile->ClearRetroBezelEnabled();
        profile->ClearRetroBezelSize();
        profile->ClearRetroBezelColor();
        profile->ClearRetroBezelBadge();
        profile->ClearRetroBezelCenterFullscreen();
        VERIFY_IS_TRUE(profile->RetroBezelEnabled());
        VERIFY_ARE_EQUAL(RetroBezelSize::Wide, profile->RetroBezelSize());
        VERIFY_ARE_EQUAL(RetroBezelColor::AgedYellow, profile->RetroBezelColor());
        profile->LayerJson(input);
        profile->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelEnabled":null,
            "experimental.retroBezelSize":null, "experimental.retroBezelColor":null,
            "experimental.retroBezelBadge":null, "experimental.retroBezelCenterFullscreen":null})"));
        VERIFY_IS_FALSE(profile->HasRetroBezelEnabled());
        VERIFY_IS_FALSE(profile->HasRetroBezelColor());
        VERIFY_IS_TRUE(profile->RetroBezelEnabled());
        VERIFY_ARE_EQUAL(RetroBezelBadge::Top, profile->RetroBezelBadge());
    }

    void ProfileTests::BackslashBezelPaletteRoundtrip()
    {
        constexpr const char* names[] = { "offWhite", "cream", "lightYellow", "agedYellow", "graphite",
                                          "charcoal", "nearBlack", "slateBlue", "mutedOlive" };
        constexpr RetroBezelColor values[] = { RetroBezelColor::OffWhite, RetroBezelColor::Cream,
            RetroBezelColor::LightYellow, RetroBezelColor::AgedYellow, RetroBezelColor::Graphite,
            RetroBezelColor::Charcoal, RetroBezelColor::NearBlack, RetroBezelColor::SlateBlue, RetroBezelColor::MutedOlive };
        const auto mappings = implementation::EnumMappings::RetroBezelColor();
        VERIFY_ARE_EQUAL(uint32_t{ 9 }, mappings.Size());
        auto settings = winrt::make_self<implementation::CascadiaSettings>(R"({"profiles":[{
            "name":"palette","guid":"{6239a42c-0000-49a3-80bd-e8fdd045185c}"}]})");
        const auto source = settings->AllProfiles().GetAt(0);
        for (int i = 0; i < 9; ++i)
        {
            VERIFY_ARE_EQUAL(i, static_cast<int>(values[i])); // Saved values 0-3 retain their meaning.
            VERIFY_ARE_EQUAL(values[i], mappings.Lookup(winrt::to_hstring(names[i])));
            auto input = VerifyParseSucceeded("{}");
            input["experimental.retroBezelColor"] = names[i];
            const auto parent = winrt::make_self<implementation::Profile>();
            parent->LayerJson(input);
            const auto child = winrt::make_self<implementation::Profile>();
            child->AddLeastImportantParent(parent);
            VERIFY_ARE_EQUAL(values[i], child->RetroBezelColor());
            VERIFY_IS_FALSE(child->HasRetroBezelColor());
            VERIFY_ARE_EQUAL(*parent, child->RetroBezelColorOverrideSource());
            child->LayerJson(input);
            VERIFY_ARE_EQUAL(input, child->ToJson());
            const auto copied = child->CopySettings();
            VERIFY_ARE_EQUAL(input, copied->ToJson());
            const auto reloaded = winrt::make_self<implementation::Profile>();
            reloaded->LayerJson(copied->ToJson());
            VERIFY_ARE_EQUAL(values[i], reloaded->RetroBezelColor());
            child->ClearRetroBezelColor();
            VERIFY_IS_FALSE(child->HasRetroBezelColor());
            VERIFY_ARE_EQUAL(values[i], child->RetroBezelColor());
            child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelColor":null})"));
            VERIFY_IS_FALSE(child->HasRetroBezelColor());
            source.RetroBezelColor(values[i]);
            const auto duplicate = settings->DuplicateProfile(source);
            VERIFY_ARE_EQUAL(values[i], duplicate.RetroBezelColor());
        }
        const auto invalid = winrt::make_self<implementation::Profile>();
        invalid->RetroBezelColor(RetroBezelColor::NearBlack);
        VERIFY_THROWS(invalid->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelColor":"neon"})")), std::exception);
        VERIFY_ARE_EQUAL(RetroBezelColor::NearBlack, invalid->RetroBezelColor());
    }

    void ProfileTests::BackslashBezelGeometry()
    {
        using namespace ::Microsoft::Terminal::Backslash;
        // Exhaust the bounded sizes/badge modes. Insets are DIPs and do not contain text padding.
        for (int size = 0; size < 3; ++size)
        {
            for (int badge = 0; badge < 3; ++badge)
            {
                VERIFY_ARE_EQUAL(0.0f, GetBezelInsets(false, size, badge, true).Height());
                const auto plain = GetBezelInsets(true, size, 0);
                const auto frame = GetBezelInsets(true, size, badge);
                const auto centered = GetBezelInsets(true, size, badge, true);
                VERIFY_ARE_EQUAL(plain.Width(), frame.Width());
                VERIFY_ARE_EQUAL(plain.Height() + (badge ? 28.0f : 0.0f), frame.Height());
                VERIFY_ARE_EQUAL(centered.top, centered.bottom);
                VERIFY_ARE_EQUAL(frame.Height() + (badge ? 28.0f : 0.0f), centered.Height());
            }
        }
        VERIFY_ARE_EQUAL(128.0f, GetBezelInsets(true, 0, 0).Width());
        VERIFY_ARE_EQUAL(160.0f, GetBezelInsets(true, 1, 0).Width());
        VERIFY_ARE_EQUAL(192.0f, GetBezelInsets(true, 2, 0).Width());
    }

    void ProfileTests::ProfileGeneratesGuid()
    {
        // Parse some profiles without guids. We should NOT generate new guids
        // for them. If a profile doesn't have a GUID, we'll leave its _guid
        // set to nullopt. The Profile::Guid() getter will
        // ensure all profiles have a GUID that's actually set.
        // The null guid _is_ a valid guid, so we won't re-generate that
        // guid. null is _not_ a valid guid, so we'll leave that nullopt

        // See SettingsTests::ValidateProfilesGenerateGuids for a version of
        // this test that includes synthesizing GUIDS for profiles without GUIDs
        // set

        const auto parseAndVerifyProfile = [](const std::string profile, const bool hasGuid) {
            const auto profileAsJson = VerifyParseSucceeded(profile);
            const auto profileParsed = implementation::Profile::FromJson(profileAsJson);

            VERIFY_ARE_EQUAL(profileParsed->HasGuid(), hasGuid);
            return profileParsed;
        };

        // Invalid GUID Values
        const std::string profileWithoutGuid{ R"({
                                              "name" : "profile0"
                                              })" };
        const std::string secondProfileWithoutGuid{ R"({
                                              "name" : "profile1"
                                              })" };
        const std::string profileWithNullForGuid{ R"({
                                              "name" : "profile2",
                                              "guid" : null
                                              })" };
        const std::string profileWithHyphenlessGuid{ R"({
                                              "name" : "profile4",
                                              "guid" : "{6239A42C1DE449A380BDE8FDD045185C}"
                                              })" };
        const std::string profileWithRawGuid{ R"({
                                              "name" : "profile4",
                                              "guid" : "6239a42c-1de4-49a3-80bd-e8fdd045185c"
                                              })" };
        const std::string profileWithGuidFormatP{ R"({
                                              "name" : "profile4",
                                              "guid" : "(6239a42c-1de4-49a3-80bd-e8fdd045185c)\\"
                                              })" };
        // Valid GUIDs
        const std::string profileWithNullGuid{ R"({
                                              "name" : "profile3",
                                              "guid" : "{00000000-0000-0000-0000-000000000000}"
                                              })" };
        const std::string profileWithGuidFormatB{ R"({
                                              "name" : "profile4",
                                              "guid" : "{6239a42c-1de4-49a3-80bd-e8fdd045185c}"
                                              })" };
        const std::string profileWithGuidUpperCaseFormatB{ R"({
                                              "name" : "profile4",
                                              "guid" : "{6239A42C-1DE4-49A3-80BD-E8FDD045185C}"
                                              })" };

        parseAndVerifyProfile(profileWithoutGuid, false);
        parseAndVerifyProfile(secondProfileWithoutGuid, false);

        // The following crash JSON parsing
        VERIFY_THROWS(parseAndVerifyProfile(profileWithHyphenlessGuid, false), std::exception);
        VERIFY_THROWS(parseAndVerifyProfile(profileWithRawGuid, false), std::exception);
        VERIFY_THROWS(parseAndVerifyProfile(profileWithGuidFormatP, false), std::exception);

        const auto parsedNullGuidProfile = parseAndVerifyProfile(profileWithNullGuid, true);
        const auto parsedGuidProfileFormatB = parseAndVerifyProfile(profileWithGuidFormatB, true);
        const auto parsedGuidProfileUpperCaseFormatB = parseAndVerifyProfile(profileWithGuidUpperCaseFormatB, true);

        const winrt::guid nullGuid{};
        const winrt::guid cmdGuid = Utils::GuidFromString(L"{6239a42c-1de4-49a3-80bd-e8fdd045185c}");

        VERIFY_ARE_EQUAL(parsedNullGuidProfile->Guid(), nullGuid);
        VERIFY_ARE_EQUAL(parsedGuidProfileFormatB->Guid(), cmdGuid);
        VERIFY_ARE_EQUAL(parsedGuidProfileUpperCaseFormatB->Guid(), cmdGuid);
    }

    void ProfileTests::LayerProfileProperties()
    {
        static constexpr std::string_view profile0String{ R"({
            "name": "profile0",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "foreground": "#000000",
            "background": "#010101",
            "selectionBackground": "#010101"
        })" };
        static constexpr std::string_view profile1String{ R"({
            "name": "profile1",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "foreground": "#020202",
            "startingDirectory": "C:/"
        })" };
        static constexpr std::string_view profile2String{ R"({
            "name": "profile2",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "foreground": "#030303",
            "selectionBackground": "#020202"
        })" };

        const auto profile0Json = VerifyParseSucceeded(profile0String);
        const auto profile1Json = VerifyParseSucceeded(profile1String);
        const auto profile2Json = VerifyParseSucceeded(profile2String);

        auto profile0 = implementation::Profile::FromJson(profile0Json);
        VERIFY_IS_NOT_NULL(profile0->DefaultAppearance().Foreground());
        VERIFY_ARE_EQUAL(til::color(0, 0, 0), til::color{ profile0->DefaultAppearance().Foreground().Value() });

        VERIFY_IS_NOT_NULL(profile0->DefaultAppearance().Background());
        VERIFY_ARE_EQUAL(til::color(1, 1, 1), til::color{ profile0->DefaultAppearance().Background().Value() });

        VERIFY_IS_NOT_NULL(profile0->DefaultAppearance().SelectionBackground());
        VERIFY_ARE_EQUAL(til::color(1, 1, 1), til::color{ profile0->DefaultAppearance().SelectionBackground().Value() });

        VERIFY_ARE_EQUAL(L"profile0", profile0->Name());

        VERIFY_IS_TRUE(profile0->StartingDirectory().empty());

        Log::Comment(NoThrowString().Format(
            L"Layering profile1 on top of profile0"));
        auto profile1{ profile0->CreateChild() };
        profile1->LayerJson(profile1Json);

        VERIFY_IS_NOT_NULL(profile1->DefaultAppearance().Foreground());
        VERIFY_ARE_EQUAL(til::color(2, 2, 2), til::color{ profile1->DefaultAppearance().Foreground().Value() });

        VERIFY_IS_NOT_NULL(profile1->DefaultAppearance().Background());
        VERIFY_ARE_EQUAL(til::color(1, 1, 1), til::color{ profile1->DefaultAppearance().Background().Value() });

        VERIFY_IS_NOT_NULL(profile1->DefaultAppearance().Background());
        VERIFY_ARE_EQUAL(til::color(1, 1, 1), til::color{ profile1->DefaultAppearance().Background().Value() });

        VERIFY_ARE_EQUAL(L"profile1", profile1->Name());

        VERIFY_IS_FALSE(profile1->StartingDirectory().empty());
        VERIFY_ARE_EQUAL(L"C:/", profile1->StartingDirectory());

        Log::Comment(NoThrowString().Format(
            L"Layering profile2 on top of (profile0+profile1)"));
        auto profile2{ profile1->CreateChild() };
        profile2->LayerJson(profile2Json);

        VERIFY_IS_NOT_NULL(profile2->DefaultAppearance().Foreground());
        VERIFY_ARE_EQUAL(til::color(3, 3, 3), til::color{ profile2->DefaultAppearance().Foreground().Value() });

        VERIFY_IS_NOT_NULL(profile2->DefaultAppearance().Background());
        VERIFY_ARE_EQUAL(til::color(1, 1, 1), til::color{ profile2->DefaultAppearance().Background().Value() });

        VERIFY_IS_NOT_NULL(profile2->DefaultAppearance().SelectionBackground());
        VERIFY_ARE_EQUAL(til::color(2, 2, 2), til::color{ profile2->DefaultAppearance().SelectionBackground().Value() });

        VERIFY_ARE_EQUAL(L"profile2", profile2->Name());

        VERIFY_IS_FALSE(profile2->StartingDirectory().empty());
        VERIFY_ARE_EQUAL(L"C:/", profile2->StartingDirectory());
    }

    void ProfileTests::LayerProfileIcon()
    {
        static constexpr std::string_view profile0String{ R"({
            "name": "profile0",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "icon": "not-null.png"
        })" };
        static constexpr std::string_view profile1String{ R"({
            "name": "profile1",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "icon": null
        })" };
        static constexpr std::string_view profile2String{ R"({
            "name": "profile2",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}"
        })" };
        static constexpr std::string_view profile3String{ R"({
            "name": "profile3",
            "guid": "{6239a42c-1111-49a3-80bd-e8fdd045185c}",
            "icon": "another-real.png"
        })" };

        const auto profile0Json = VerifyParseSucceeded(profile0String);
        const auto profile1Json = VerifyParseSucceeded(profile1String);
        const auto profile2Json = VerifyParseSucceeded(profile2String);
        const auto profile3Json = VerifyParseSucceeded(profile3String);

        auto profile0 = implementation::Profile::FromJson(profile0Json);
        VERIFY_IS_FALSE(profile0->Icon().Path().empty());
        VERIFY_ARE_EQUAL(L"not-null.png", profile0->Icon().Path());

        Log::Comment(NoThrowString().Format(
            L"Verify that layering an object the key set to null will clear the key"));
        profile0->LayerJson(profile1Json);
        VERIFY_IS_TRUE(profile0->Icon().Path().empty());

        profile0->LayerJson(profile2Json);
        VERIFY_IS_TRUE(profile0->Icon().Path().empty());

        profile0->LayerJson(profile3Json);
        VERIFY_IS_FALSE(profile0->Icon().Path().empty());
        VERIFY_ARE_EQUAL(L"another-real.png", profile0->Icon().Path());

        Log::Comment(NoThrowString().Format(
            L"Verify that layering an object _without_ the key will not clear the key"));
        profile0->LayerJson(profile2Json);
        VERIFY_IS_FALSE(profile0->Icon().Path().empty());
        VERIFY_ARE_EQUAL(L"another-real.png", profile0->Icon().Path());

        auto profile1 = implementation::Profile::FromJson(profile1Json);
        VERIFY_IS_TRUE(profile1->Icon().Path().empty());
        profile1->LayerJson(profile3Json);
        VERIFY_IS_FALSE(profile1->Icon().Path().empty());
        VERIFY_ARE_EQUAL(L"another-real.png", profile1->Icon().Path());
    }

    void ProfileTests::LayerProfilesOnArray()
    {
        static constexpr std::string_view inboxProfiles{ R"({
            "profiles": [
                {
                    "name" : "profile0",
                    "guid" : "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                }, {
                    "name" : "profile1",
                    "guid" : "{6239a42c-1111-49a3-80bd-e8fdd045185c}"
                }, {
                    "name" : "profile2",
                    "guid" : "{6239a42c-2222-49a3-80bd-e8fdd045185c}"
                }
            ]
        })" };
        static constexpr std::string_view userProfiles{ R"({
            "profiles": [
                {
                    "name" : "profile3",
                    "guid" : "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                }, {
                    "name" : "profile4",
                    "guid" : "{6239a42c-1111-49a3-80bd-e8fdd045185c}"
                }
            ]
        })" };

        const auto settings = winrt::make_self<implementation::CascadiaSettings>(userProfiles, inboxProfiles);
        const auto allProfiles = settings->AllProfiles();
        VERIFY_ARE_EQUAL(3u, allProfiles.Size());
        VERIFY_ARE_EQUAL(L"profile3", allProfiles.GetAt(0).Name());
        VERIFY_ARE_EQUAL(L"profile4", allProfiles.GetAt(1).Name());
        VERIFY_ARE_EQUAL(L"profile2", allProfiles.GetAt(2).Name());
    }

    void ProfileTests::DuplicateProfileTest()
    {
        static constexpr std::string_view userProfiles{ R"({
            "profiles": {
                "defaults": {
                    "font": {
                        "size": 123
                    }
                },
                "list": [
                    {
                        "name": "profile0",
                        "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                        "backgroundImage": "file:///some/path",
                        "experimental.retroBezelEnabled": true,
                        "experimental.retroBezelSize": "wide",
                        "experimental.retroBezelColor": "agedYellow",
                        "experimental.retroBezelBadge": "top",
                        "experimental.retroBezelCenterFullscreen": false,
                        "experimental.retroTerminalGlowIntensity": 0,
                        "experimental.retroTerminalScanlineIntensity": 0.75,
                        "hidden": false,
                    }
                ]
            }
        })" };

        const auto settings = winrt::make_self<implementation::CascadiaSettings>(userProfiles);
        const auto profile = settings->AllProfiles().GetAt(0);
        profile.DefaultAppearance().RetroTerminalScanlineThickness(2.75f);
        profile.DefaultAppearance().RetroTerminalScanlineGap(0.0f);
        profile.DefaultAppearance().RetroTerminalTintIntensity(0.45f);
        profile.DefaultAppearance().RetroTerminalTintColor(winrt::Microsoft::Terminal::Core::Color{ 255, 176, 0, 255 });
        const auto duplicatedProfile = settings->DuplicateProfile(profile);

        // GH#11392: Ensure duplicated profiles properly inherit the base layer, even for nested objects.
        VERIFY_ARE_EQUAL(123, duplicatedProfile.FontInfo().FontSize());
        VERIFY_IS_TRUE(duplicatedProfile.RetroBezelEnabled());
        VERIFY_ARE_EQUAL(RetroBezelColor::AgedYellow, duplicatedProfile.RetroBezelColor());
        VERIFY_ARE_EQUAL(RetroBezelSize::Wide, duplicatedProfile.RetroBezelSize());
        VERIFY_ARE_EQUAL(RetroBezelBadge::Top, duplicatedProfile.RetroBezelBadge());
        VERIFY_IS_FALSE(duplicatedProfile.RetroBezelCenterFullscreen());
        VERIFY_IS_TRUE(duplicatedProfile.DefaultAppearance().HasRetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.0f, duplicatedProfile.DefaultAppearance().RetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.75f, duplicatedProfile.DefaultAppearance().RetroTerminalScanlineIntensity());
        VERIFY_ARE_EQUAL(2.75f, duplicatedProfile.DefaultAppearance().RetroTerminalScanlineThickness());
        VERIFY_IS_TRUE(duplicatedProfile.DefaultAppearance().HasRetroTerminalScanlineGap());
        VERIFY_ARE_EQUAL(0.0f, duplicatedProfile.DefaultAppearance().RetroTerminalScanlineGap());
        VERIFY_IS_TRUE(duplicatedProfile.DefaultAppearance().HasRetroTerminalTintIntensity());
        VERIFY_IS_TRUE(duplicatedProfile.DefaultAppearance().HasRetroTerminalTintColor());
        VERIFY_ARE_EQUAL(0.45f, duplicatedProfile.DefaultAppearance().RetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(255, 176, 0), til::color{ duplicatedProfile.DefaultAppearance().RetroTerminalTintColor() });

        duplicatedProfile.Guid(profile.Guid());
        duplicatedProfile.Name(profile.Name());

        const auto json = winrt::get_self<implementation::Profile>(profile)->ToJson();
        const auto duplicatedJson = winrt::get_self<implementation::Profile>(duplicatedProfile)->ToJson();
        VERIFY_ARE_EQUAL(json, duplicatedJson, til::u8u16(toString(duplicatedJson)).c_str());
    }

    void ProfileTests::TestGenGuidsForProfiles()
    {
        // We'll generate GUIDs in the Profile::Guid getter. We should make sure that
        // the GUID generated for a dynamic profile (with a source) is different
        // than that of a profile without a source.

        static constexpr std::string_view inboxSettings{ R"({
            "profiles": [
                {
                    "name" : "profile0",
                    "source": "Terminal.App.UnitTest.0"
                },
                {
                    "name" : "profile1"
                }
            ]
        })" };
        static constexpr std::string_view userSettings{ R"({
            "profiles": [
                {
                    "name": "profile0",
                    "source": "Terminal.App.UnitTest.0",
                },
                {
                    "name": "profile0"
                }
            ]
        })" };

        const auto settings = winrt::make_self<implementation::CascadiaSettings>(userSettings, inboxSettings);

        VERIFY_ARE_EQUAL(3u, settings->AllProfiles().Size());

        VERIFY_ARE_EQUAL(L"profile0", settings->AllProfiles().GetAt(0).Name());
        VERIFY_IS_TRUE(settings->AllProfiles().GetAt(0).HasGuid());
        VERIFY_IS_FALSE(settings->AllProfiles().GetAt(0).Source().empty());

        VERIFY_ARE_EQUAL(L"profile0", settings->AllProfiles().GetAt(1).Name());
        VERIFY_IS_TRUE(settings->AllProfiles().GetAt(1).HasGuid());
        VERIFY_IS_TRUE(settings->AllProfiles().GetAt(1).Source().empty());

        VERIFY_ARE_NOT_EQUAL(settings->AllProfiles().GetAt(0).Guid(), settings->AllProfiles().GetAt(1).Guid());
    }

    void ProfileTests::ProfileWithEnvVars()
    {
        const std::string profileString{ R"({
            "name": "profile0",
            "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "environment": {
                "VAR_1": "value1",
                "VAR_2": "value2",
                "VAR_3": "%VAR_3%;value3"
            }
        })" };
        const auto profile = implementation::Profile::FromJson(VerifyParseSucceeded(profileString));
        std::vector<IEnvironmentVariableMap> envVarMaps{};
        envVarMaps.emplace_back(profile->EnvironmentVariables());
        for (auto& envMap : envVarMaps)
        {
            VERIFY_ARE_EQUAL(static_cast<uint32_t>(3), envMap.Size());
            VERIFY_ARE_EQUAL(L"value1", envMap.Lookup(L"VAR_1"));
            VERIFY_ARE_EQUAL(L"value2", envMap.Lookup(L"VAR_2"));
            VERIFY_ARE_EQUAL(L"%VAR_3%;value3", envMap.Lookup(L"VAR_3"));
        }
    }

    void ProfileTests::ProfileWithEnvVarsSameNameDifferentCases()
    {
        const std::string userSettings{ R"({
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                    "environment": {
                        "FOO": "VALUE",
                        "Foo": "Value"
                    }
                }
            ]
        })" };
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(userSettings);
        const auto warnings = settings->Warnings();
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(2), warnings.Size());
        uint32_t index;
        VERIFY_IS_TRUE(warnings.IndexOf(SettingsLoadWarnings::InvalidProfileEnvironmentVariables, index));
    }

    void ProfileTests::TestCorrectOldDefaultShellPaths()
    {
        static constexpr std::string_view inboxProfiles{ R"({
            "profiles": [
                {
                    "guid": "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}",
                    "name": "Windows PowerShell",
                    "commandline": "%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",
                },
                {
                    "guid": "{0caa0dad-35be-5f56-a8ff-afceeeaa6101}",
                    "name": "Command Prompt",
                    "commandline": "%SystemRoot%\\System32\\cmd.exe",
                }
            ]
        })" };
        static constexpr std::string_view userProfiles{ R"({
            "profiles": {
                "defaults":
                {
                    "commandline": "pwsh.exe"
                },
                "list":
                [
                    {
                        "name" : "powershell 1",
                        "commandline": "powershell.exe",
                        "guid" : "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}"
                    },
                    {
                        "name" : "powershell 2",
                        "commandline": "powershell.exe",
                        "guid" : "{61c54bbd-0000-5271-96e7-009a87ff44bf}"
                    },
                    {
                        "name" : "cmd 1",
                        "commandline": "cmd.exe",
                        "guid" : "{0caa0dad-35be-5f56-a8ff-afceeeaa6101}"
                    },
                    {
                        "name" : "cmd 2",
                        "commandline": "cmd.exe",
                        "guid" : "{0caa0dad-0000-5f56-a8ff-afceeeaa6101}"
                    }
                ]
            }
        })" };

        implementation::SettingsLoader loader{ userProfiles, inboxProfiles };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        loader.FixupUserSettings();

        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto allProfiles = settings->AllProfiles();
        VERIFY_ARE_EQUAL(4u, allProfiles.Size());
        VERIFY_ARE_EQUAL(L"powershell 1", allProfiles.GetAt(0).Name());
        VERIFY_ARE_EQUAL(L"powershell 2", allProfiles.GetAt(1).Name());
        VERIFY_ARE_EQUAL(L"cmd 1", allProfiles.GetAt(2).Name());
        VERIFY_ARE_EQUAL(L"cmd 2", allProfiles.GetAt(3).Name());

        VERIFY_ARE_EQUAL(L"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", allProfiles.GetAt(0).Commandline());
        VERIFY_ARE_EQUAL(L"powershell.exe", allProfiles.GetAt(1).Commandline());
        VERIFY_ARE_EQUAL(L"%SystemRoot%\\System32\\cmd.exe", allProfiles.GetAt(2).Commandline());
        VERIFY_ARE_EQUAL(L"cmd.exe", allProfiles.GetAt(3).Commandline());
    }

    void ProfileTests::ProfileDefaultsProhibitedSettings()
    {
        static constexpr std::string_view userProfiles{ R"({
            "profiles": {
                "defaults":
                {
                    "guid": "{00000000-0000-0000-0000-000000000000}",
                    "name": "Default Profile Name",
                    "source": "Default Profile Source",
                    "commandline": "foo.exe"
                },
                "list":
                [
                    {
                        "name" : "PowerShell",
                        "commandline": "powershell.exe",
                        "guid" : "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}"
                    },
                    {
                        "name": "Profile with just a name"
                    },
                    {
                        "guid": "{a0776706-1fa6-4439-b46c-287a65c084d5}",
                    }
                ]
            }
        })" };

        const auto settings = winrt::make_self<implementation::CascadiaSettings>(userProfiles);

        // Profile Defaults should not have a GUID, name, source, or commandline.
        const auto profileDefaults = settings->ProfileDefaults();
        VERIFY_IS_FALSE(profileDefaults.HasGuid());
        VERIFY_IS_FALSE(profileDefaults.HasName());
        VERIFY_IS_FALSE(profileDefaults.HasSource());
        VERIFY_IS_FALSE(profileDefaults.HasCommandline());

        const auto allProfiles = settings->AllProfiles();
        VERIFY_ARE_EQUAL(3u, allProfiles.Size());

        // Profile settings should be set to the ones set at that layer
        VERIFY_ARE_EQUAL(L"PowerShell", allProfiles.GetAt(0).Name());
        VERIFY_ARE_EQUAL(L"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", allProfiles.GetAt(0).Commandline());
        VERIFY_ARE_EQUAL(Utils::GuidFromString(L"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}"), static_cast<GUID>(allProfiles.GetAt(0).Guid()));
        VERIFY_IS_FALSE(allProfiles.GetAt(0).HasSource());

        // Profile should not inherit the values attempted to be set on the Profiles Defaults layer
        // This profile only has a name set
        VERIFY_ARE_EQUAL(L"Profile with just a name", allProfiles.GetAt(1).Name());
        VERIFY_ARE_NOT_EQUAL(Utils::GuidFromString(L"{00000000-0000-0000-0000-000000000000}"), static_cast<GUID>(allProfiles.GetAt(1).Guid()));
        VERIFY_ARE_NOT_EQUAL(L"Default Profile Source", allProfiles.GetAt(1).Source());
        VERIFY_ARE_NOT_EQUAL(L"foo.exe", allProfiles.GetAt(1).Commandline());

        // Profile should not inherit the values attempted to be set on the Profiles Defaults layer
        // This profile only has a guid set
        VERIFY_ARE_NOT_EQUAL(L"Default Profile Name", allProfiles.GetAt(2).Name());
        VERIFY_ARE_NOT_EQUAL(Utils::GuidFromString(L"{00000000-0000-0000-0000-000000000000}"), static_cast<GUID>(allProfiles.GetAt(2).Guid()));
        VERIFY_ARE_NOT_EQUAL(L"Default Profile Source", allProfiles.GetAt(2).Source());
        VERIFY_ARE_NOT_EQUAL(L"foo.exe", allProfiles.GetAt(2).Commandline());
    }
}
