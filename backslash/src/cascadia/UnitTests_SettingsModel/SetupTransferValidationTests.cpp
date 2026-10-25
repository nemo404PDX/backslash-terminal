// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "JsonTestClass.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/TerminalWarnings.h"
#include "../TerminalSettingsModel/resource.h"
#include "../TerminalSettingsModel/SetupTransfer.h"

using namespace WEX::TestExecution;
namespace M = winrt::Microsoft::Terminal::Settings::Model;
namespace I = winrt::Microsoft::Terminal::Settings::Model::implementation;
namespace T = Microsoft::Terminal::Backslash::SetupTransfer;

namespace SettingsModelUnitTests
{
    class SetupTransferValidationTests : public JsonTestClass
    {
        TEST_CLASS(SetupTransferValidationTests);
        TEST_METHOD(DynamicOnlyVisibilityUsesDestinationContext);
        TEST_METHOD(ActuallyHiddenProfilesRemainRejected);
        TEST_METHOD(CandidateDisabledAndUnavailableSourcesAreRejected);
        TEST_METHOD(FragmentParentsAndBasePathsRemainDetached);
        TEST_METHOD(FragmentUpdateParentsPreservePrecedence);
        TEST_METHOD(PreviewNeverInvokesImportedMediaResolver);
        TEST_METHOD(OrdinaryModeStillInvokesInjectedMediaResolver);

        static constexpr const char* DynamicId = "{71969451-1122-4333-8444-123456789abc}";
        static constexpr const wchar_t* Source = L"Backslash.Test.Source";
        static constexpr const wchar_t* FragmentSource = L"Backslash.Test.Fragment";

        static Json::Value Candidate(const wchar_t* source = Source)
        {
            auto value = T::ReadJson(R"({"profiles":{"defaults":{},"list":[
                {"guid":"{61c54bbd-c2c6-5271-96e7-009a87ff44bf}","name":"Windows PowerShell","hidden":true},
                {"guid":"{0caa0dad-35be-5f56-a8ff-afceeeaa6101}","name":"Command Prompt","hidden":true}]}})");
            Json::Value dynamic{Json::objectValue}; dynamic["guid"] = DynamicId; dynamic["name"] = "Fixture dynamic";
            dynamic["source"] = winrt::to_string(winrt::hstring{source});
            value["profiles"]["list"].append(dynamic);
            value["defaultProfile"] = DynamicId;
            return value;
        }

        static winrt::com_ptr<I::CascadiaSettings> Destination(bool fragment = false, bool update = false)
        {
            const auto candidate = Candidate(fragment ? FragmentSource : Source);
            auto json = candidate;
            if (update) { json["profiles"]["list"][2]["source"] = ""; }
            I::SettingsLoader loader{T::JsonText(json), I::LoadStringResource(IDR_DEFAULTS)};
            loader.userSettings.baseLayerProfile->SourceBasePath = L"C:\\SetupFixtures";
            loader.userSettings.globals->SourceBasePath = L"C:\\SetupFixtures";
            if (!fragment)
            {
                auto parent = winrt::make_self<I::Profile>();
                parent->Guid(Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(DynamicId).c_str()));
                parent->Name(L"Generated fixture"); parent->Source(Source); parent->Origin(M::OriginTag::Generated);
                parent->Commandline(L"fixture-shell.exe"); parent->FontInfo().FontSize(23);
                parent->SourceBasePath = L"C:\\GeneratedFixture";
                loader.inboxSettings.profiles.push_back(parent);
            }
            loader.MergeInboxIntoUserSettings();
            if (fragment)
            {
                Json::Value row{Json::objectValue};
                row[update ? "updates" : "guid"] = DynamicId;
                if (!update) { row["name"] = "Fragment fixture"; }
                row["font"]["size"] = 23;
                row["commandline"] = "fragment-shell.exe";
                Json::Value fragmentJson{Json::objectValue}; fragmentJson["profiles"].append(row);
                loader.MergeFragmentIntoUserSettings(FragmentSource, L"C:\\FragmentFixture", T::JsonText(fragmentJson));
            }
            loader.FinalizeLayering(); loader.FixupUserSettings();
            return winrt::make_self<I::CascadiaSettings>(std::move(loader));
        }

        static M::Profile Find(const M::CascadiaSettings& model)
        {
            return model.FindProfile(Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(DynamicId).c_str()));
        }
    };

    void SetupTransferValidationTests::DynamicOnlyVisibilityUsesDestinationContext()
    {
        const auto destination = Destination(); const auto input = Candidate();
        VERIFY_THROWS_SPECIFIC(winrt::make_self<I::CascadiaSettings>(T::JsonText(input), I::LoadStringResource(IDR_DEFAULTS)), const I::SettingsException, [](const auto& ex) {
            return ex.Error() == M::SettingsLoadErrors::AllProfilesHidden;
        });
        const auto before = destination->ExplicitSettingsJson();
        const auto result = destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input)));
        VERIFY_ARE_EQUAL(1u, result.ActiveProfiles().Size());
        VERIFY_ARE_EQUAL(winrt::hstring{L"Fixture dynamic"}, result.ActiveProfiles().GetAt(0).Name());
        VERIFY_ARE_EQUAL(23.0f, Find(result).FontInfo().FontSize());
        VERIFY_IS_FALSE(Find(result).FontInfo().HasFontSize());
        VERIFY_IS_FALSE(winrt::get_self<I::Profile>(Find(result))->Orphaned());
        VERIFY_ARE_EQUAL(0u, result.Warnings().Size());
        const auto explicitResult = T::ReadJson(winrt::to_string(result.ExplicitSettingsJson()));
        const auto& profile = explicitResult["profiles"]["list"][2];
        VERIFY_IS_FALSE(profile.isMember("font")); VERIFY_IS_FALSE(profile.isMember("commandline"));
        VERIFY_ARE_EQUAL(before, destination->ExplicitSettingsJson());
    }

    void SetupTransferValidationTests::ActuallyHiddenProfilesRemainRejected()
    {
        const auto destination = Destination(); auto input = Candidate();
        input["profiles"]["list"][2]["hidden"] = true;
        VERIFY_THROWS_SPECIFIC(destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input))), const I::SettingsException, [](const auto& ex) {
            return ex.Error() == M::SettingsLoadErrors::AllProfilesHidden;
        });
        VERIFY_ARE_EQUAL(1u, destination->ActiveProfiles().Size());
    }

    void SetupTransferValidationTests::CandidateDisabledAndUnavailableSourcesAreRejected()
    {
        const auto destination = Destination(); auto input = Candidate();
        input["disabledProfileSources"].append(winrt::to_string(winrt::hstring{Source}));
        VERIFY_THROWS(destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input))), winrt::hresult_invalid_argument);
        input = Candidate(L"Backslash.Test.OtherSource");
        VERIFY_THROWS(destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input))), winrt::hresult_invalid_argument);
        input = Candidate(); input["profiles"]["list"][2]["guid"] = "{71969451-1122-4333-8444-abcdefabcdef}";
        VERIFY_THROWS(destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input))), winrt::hresult_invalid_argument);
        VERIFY_ARE_EQUAL(1u, destination->ActiveProfiles().Size());
    }

    void SetupTransferValidationTests::FragmentParentsAndBasePathsRemainDetached()
    {
        const auto destination = Destination(true); const auto before = destination->ExplicitSettingsJson();
        auto input = Candidate(FragmentSource);
        input["profiles"]["defaults"]["font"]["size"] = 19;
        auto result = destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input)));
        const auto profile = Find(result);
        VERIFY_ARE_EQUAL(19.0f, profile.FontInfo().FontSize());
        VERIFY_IS_FALSE(profile.FontInfo().HasFontSize());
        const auto candidateImpl = winrt::get_self<I::Profile>(profile);
        const auto sourceProfile = Find(*destination); const auto sourceImpl = winrt::get_self<I::Profile>(sourceProfile);
        bool checked = false;
        for (const auto& parent : candidateImpl->Parents())
        {
            if (parent->Origin() != M::OriginTag::Fragment) { continue; }
            VERIFY_ARE_EQUAL(winrt::hstring{L"C:\\FragmentFixture"}, parent->SourceBasePath);
            for (const auto& original : sourceImpl->Parents()) { VERIFY_IS_TRUE(parent.get() != original.get()); }
            parent->FontInfo().FontSize(99); checked = true;
        }
        VERIFY_IS_TRUE(checked);
        VERIFY_ARE_EQUAL(23.0f, sourceProfile.FontInfo().FontSize());
        VERIFY_ARE_EQUAL(before, destination->ExplicitSettingsJson());
        input["disabledProfileSources"].append(winrt::to_string(winrt::hstring{FragmentSource}));
        VERIFY_THROWS(destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input))), winrt::hresult_invalid_argument);
    }

    void SetupTransferValidationTests::FragmentUpdateParentsPreservePrecedence()
    {
        const auto destination = Destination(true, true);
        auto input = Candidate(); input["profiles"]["list"][2]["source"] = "";
        const auto result = destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input)));
        VERIFY_ARE_EQUAL(1u, result.ActiveProfiles().Size());
        VERIFY_ARE_EQUAL(23.0f, Find(result).FontInfo().FontSize());
        VERIFY_ARE_EQUAL(winrt::hstring{L"fragment-shell.exe"}, Find(result).Commandline());
        VERIFY_IS_TRUE(Find(result).Source().empty());
        VERIFY_IS_FALSE(Find(result).HasCommandline());
    }
    // Only the isolated test host's process environment changes; it is restored
    // before the test ends. The referenced network path is never contacted.
    static auto SetupNetworkEnvironmentFixture()
    {
        constexpr auto name = L"BACKSLASH_SETUP_PREVIEW_NETWORK";
        const auto length = GetEnvironmentVariableW(name, nullptr, 0);
        std::wstring previous(length, L'\0');
        if (length)
        {
            GetEnvironmentVariableW(name, previous.data(), length);
            previous.resize(length - 1);
        }
        VERIFY_IS_TRUE(SetEnvironmentVariableW(name, LR"(\\uncontacted.invalid\share)") != FALSE);
        return wil::scope_exit([previous = std::move(previous), existed = length != 0] {
            SetEnvironmentVariableW(L"BACKSLASH_SETUP_PREVIEW_NETWORK", existed ? previous.c_str() : nullptr);
        });
    }

    void SetupTransferValidationTests::PreviewNeverInvokesImportedMediaResolver()
    {
        const auto restoreEnvironment = SetupNetworkEnvironmentFixture();
        const auto destination = Destination();
        const auto before = destination->ExplicitSettingsJson();
        auto input = Candidate();
        auto& profile = input["profiles"]["list"][2];
        profile["backgroundImage"] = R"(\\uncontacted.invalid\share\image.png)";
        profile["icon"] = R"(\\?\UNC\uncontacted.invalid\share\icon.ico)";
        profile["experimental.pixelShaderPath"] = R"(%BACKSLASH_SETUP_PREVIEW_NETWORK%\shader.hlsl)";
        profile["unfocusedAppearance"]["backgroundImage"] = R"(\??\UNC\uncontacted.invalid\share\unfocused.png)";
        profile["bellSound"].append(R"(\\.\UNC\uncontacted.invalid\share\sound.wav)");
        size_t calls = 0;
        const M::MediaResourceResolver recording{ [&](M::OriginTag, const winrt::hstring&, const M::IMediaResource& resource) {
            ++calls;
            resource.Resolve(resource.Path()); // no IO, even if unexpectedly called
        } };
        const auto preview = destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(input)), recording);
        VERIFY_ARE_EQUAL(size_t{0}, calls);
        VERIFY_ARE_EQUAL(1u, preview.ActiveProfiles().Size());
        VERIFY_ARE_EQUAL(winrt::to_hstring(profile["backgroundImage"].asString()), Find(preview).DefaultAppearance().BackgroundImagePath().Path());
        VERIFY_ARE_EQUAL(before, destination->ExplicitSettingsJson());
    }

    void SetupTransferValidationTests::OrdinaryModeStillInvokesInjectedMediaResolver()
    {
        const auto restoreEnvironment = SetupNetworkEnvironmentFixture();
        auto input = Candidate();
        auto& profile = input["profiles"]["list"][2];
        // This fixture has an ordinary local profile: no discovery is needed.
        profile["source"] = "";
        const std::array<std::string, 4> paths{
            R"(\\uncontacted.invalid\share\image.png)",
            R"(\\?\UNC\uncontacted.invalid\share\icon.ico)",
            R"(%BACKSLASH_SETUP_PREVIEW_NETWORK%\shader.hlsl)",
            R"(\??\UNC\uncontacted.invalid\share\unfocused.png)"
        };
        profile["backgroundImage"] = paths[0];
        profile["icon"] = paths[1];
        profile["experimental.pixelShaderPath"] = paths[2];
        profile["unfocusedAppearance"]["backgroundImage"] = paths[3];
        std::set<std::string> seen;
        const M::MediaResourceResolver recording{ [&](M::OriginTag, const winrt::hstring&, const M::IMediaResource& resource) {
            seen.emplace(winrt::to_string(resource.Path()));
            resource.Resolve(resource.Path()); // replaces all filesystem resolution
        } };
        auto loader = I::SettingsLoader::Default(T::JsonText(input), I::LoadStringResource(IDR_DEFAULTS));
        const auto ordinary = winrt::make_self<I::CascadiaSettings>(std::move(loader), I::CascadiaSettings::ValidationMode::Ordinary, recording);
        VERIFY_ARE_EQUAL(1u, ordinary->ActiveProfiles().Size());
        for (const auto& path : paths) { VERIFY_IS_TRUE(seen.contains(path)); }
    }


}
