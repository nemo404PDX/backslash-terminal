// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "JsonTestClass.h"
#include "../TerminalSettingsModel/SetupTransfer.h"
#include "../TerminalSettingsModel/SetupTransferResources.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"

using namespace WEX::TestExecution;
namespace S = Microsoft::Terminal::Backslash::SetupTransfer;
namespace P = Microsoft::Terminal::Backslash::Pins;
namespace B = Microsoft::Terminal::Backslash;

namespace SettingsModelUnitTests
{
    class SetupTransferTests : public JsonTestClass
    {
        TEST_CLASS(SetupTransferTests);
        TEST_METHOD(ExplicitRoundtripAndInheritance);
        TEST_METHOD(DeepBezelColorsRoundtrip);
        TEST_METHOD(WhiteScanlineRoundtrip);
        TEST_METHOD(BackslashPhosphorContrastGuard);
        TEST_METHOD(RepeatChangedAndUnrelatedProfiles);
        TEST_METHOD(DefaultsAndOverridesReplaceWithoutFlattening);
        TEST_METHOD(ProfileAndGroupConflicts);
        TEST_METHOD(GroupAndPinIdsUpdateWithoutDuplication);
        TEST_METHOD(RejectMalformedUnsupportedAndUnknown);
        TEST_METHOD(CrossReferencesAndValidationArePreflightOnly);
        TEST_METHOD(PinRuntimeFieldsNeverExport);
        TEST_METHOD(ExplicitGlobalFalseAndAbsentPreferences);
        TEST_METHOD(ResourceDefinitionsPreserveUnrelatedProfiles);
        TEST_METHOD(ResourceReviewUsesReadOnlyInjectedProbes);
        TEST_METHOD(ResourceReviewNeverProbesExternalPaths);
        TEST_METHOD(ProductionResourceReviewDoesNotResolveReferences);
        TEST_METHOD(ModelExplicitExportPreservesGlobalOffAndIsReadOnly);

        static constexpr const char* A = "{11111111-1111-4111-8111-111111111111}";
        static constexpr const char* C = "{22222222-2222-4222-8222-222222222222}";
        static constexpr const char* Group = "{33333333-3333-4333-8333-333333333333}";
        static constexpr const char* Pin = "{44444444-4444-4444-8444-444444444444}";

        static Json::Value Settings()
        {
            auto result = S::ReadJson(R"({"profiles":{"defaults":{"font":{"face":"Consolas","size":17},"experimental.retroBezelEnabled":false},"list":[]}})");
            result["profiles"]["list"].append(Profile(A, "Alpha"));
            return result;
        }
        static Json::Value Profile(const char* id, const char* name)
        {
            Json::Value result{ Json::objectValue }; result["guid"] = id; result["name"] = name;
            return result;
        }
        static P::Catalog Pins()
        {
            P::Catalog result;
            result.Add({ Pin, A, "", P::Kind::Local, "Alpha folder", "fixture-machine", "C:\\fixture" });
            return result;
        }
        static Json::Value Groups(const char* id = Group, const char* name = "Saved layout")
        {
            Json::Value step{ Json::objectValue }; step["kind"] = "newTab"; step["profile"] = A; step["source"] = "";
            Json::Value tab{ Json::arrayValue }; tab.append(step);
            B::SavedTabGroups result;
            result.groups.push_back({ id, name, false, 0, {tab} });
            return result.ToValue();
        }
        static S::Document Document(const Json::Value& settings, const P::Catalog& pins = {})
        {
            return S::Parse(S::Export(settings, pins));
        }
    };

    void SetupTransferTests::WhiteScanlineRoundtrip()
    {
        auto input = Settings();
        input["profiles"]["defaults"]["experimental.retroTerminalScanlineWhite"] = true;
        input["profiles"]["list"][0]["experimental.retroTerminalScanlineWhite"] = false;
        const auto plan = S::Prepare(Document(input), Settings(), {});
        VERIFY_IS_TRUE(plan.CanApply());
        using namespace winrt::Microsoft::Terminal::Settings::Model;
        const auto model = winrt::make_self<implementation::CascadiaSettings>(S::JsonText(plan.settings));
        VERIFY_IS_TRUE(model->ProfileDefaults().DefaultAppearance().RetroTerminalScanlineWhite());
        const auto profile = model->FindProfile(::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(A).c_str()));
        VERIFY_IS_FALSE(profile.DefaultAppearance().RetroTerminalScanlineWhite());
        VERIFY_IS_TRUE(profile.DefaultAppearance().HasRetroTerminalScanlineWhite());
        const auto roundtrip = Document(model->ToJson());
        VERIFY_IS_TRUE(roundtrip.settings["profiles"]["defaults"]["experimental.retroTerminalScanlineWhite"].asBool());
        bool found = false;
        for (const auto& saved : roundtrip.settings["profiles"]["list"])
        {
            if (saved["guid"].asString() == A)
            {
                VERIFY_IS_TRUE(saved.isMember("experimental.retroTerminalScanlineWhite"));
                VERIFY_IS_FALSE(saved["experimental.retroTerminalScanlineWhite"].asBool());
                found = true;
            }
        }
        VERIFY_IS_TRUE(found);
    }
    void SetupTransferTests::BackslashPhosphorContrastGuard()
    {
        auto input = Settings();
        input["profiles"]["defaults"]["experimental.retroTerminalSchemePhosphor"] = true;
        input["profiles"]["list"][0]["experimental.retroTerminalSchemePhosphor"] = false;
        input["profiles"]["defaults"]["experimental.retroTerminalContrastGuard"] = true;
        input["profiles"]["list"][0]["experimental.retroTerminalContrastGuard"] = false;
        input["profiles"]["list"][0]["unfocusedAppearance"]["experimental.retroTerminalContrastGuard"] = true;
        const auto plan = S::Prepare(Document(input), Settings(), {});
        VERIFY_IS_TRUE(plan.CanApply());
        using namespace winrt::Microsoft::Terminal::Settings::Model;
        const auto model = winrt::make_self<implementation::CascadiaSettings>(S::JsonText(plan.settings));
        VERIFY_IS_TRUE(model->ProfileDefaults().DefaultAppearance().RetroTerminalContrastGuard());
        VERIFY_IS_TRUE(model->ProfileDefaults().DefaultAppearance().RetroTerminalSchemePhosphor());
        const auto profile = model->FindProfile(::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(A).c_str()));
        VERIFY_IS_FALSE(profile.DefaultAppearance().RetroTerminalContrastGuard());
        VERIFY_IS_FALSE(profile.DefaultAppearance().RetroTerminalSchemePhosphor());
        VERIFY_IS_TRUE(profile.DefaultAppearance().HasRetroTerminalSchemePhosphor());
        VERIFY_IS_TRUE(profile.DefaultAppearance().HasRetroTerminalContrastGuard());
        VERIFY_IS_TRUE(profile.UnfocusedAppearance().RetroTerminalContrastGuard());
        VERIFY_IS_TRUE(profile.UnfocusedAppearance().HasRetroTerminalContrastGuard());
        const auto roundtrip = Document(model->ToJson());
        VERIFY_IS_TRUE(roundtrip.settings["profiles"]["defaults"]["experimental.retroTerminalContrastGuard"].asBool());
        bool found = false;
        for (const auto& saved : roundtrip.settings["profiles"]["list"])
        {
            if (saved["guid"].asString() == A)
            {
                VERIFY_IS_TRUE(saved.isMember("experimental.retroTerminalContrastGuard"));
                VERIFY_IS_FALSE(saved["experimental.retroTerminalContrastGuard"].asBool());
                VERIFY_IS_TRUE(saved["unfocusedAppearance"]["experimental.retroTerminalContrastGuard"].asBool());
                found = true;
            }
        }
        VERIFY_IS_TRUE(found);
    }


    void SetupTransferTests::DeepBezelColorsRoundtrip()
    {
        using namespace winrt::Microsoft::Terminal::Settings::Model;
        for (const auto name : { "deepPurple", "deepTeal" })
        {
            auto input = Settings();
            input["profiles"]["defaults"]["experimental.retroBezelColor"] = name;
            input["profiles"]["list"][0]["experimental.retroBezelColor"] = "nearBlack";
            const auto plan = S::Prepare(Document(input), Settings(), {});
            VERIFY_IS_TRUE(plan.CanApply());
            const auto model = winrt::make_self<implementation::CascadiaSettings>(S::JsonText(plan.settings));
            const auto expected = std::string_view{ name } == "deepPurple" ? RetroBezelColor::DeepPurple : RetroBezelColor::DeepTeal;
            VERIFY_ARE_EQUAL(expected, model->ProfileDefaults().RetroBezelColor());
            const auto profile = model->FindProfile(::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(A).c_str()));
            VERIFY_ARE_EQUAL(RetroBezelColor::NearBlack, profile.RetroBezelColor());
            VERIFY_IS_TRUE(profile.HasRetroBezelColor());
            const auto serialized = model->ToJson();
            const auto exported = Document(serialized);
            VERIFY_ARE_EQUAL(std::string{ name }, exported.settings["profiles"]["defaults"]["experimental.retroBezelColor"].asString());
            const auto repeat = S::Prepare(Document(plan.settings), plan.settings, plan.pins);
            VERIFY_IS_TRUE(repeat.CanApply()); VERIFY_IS_FALSE(repeat.changed);
            profile.ClearRetroBezelColor();
            VERIFY_IS_FALSE(profile.HasRetroBezelColor());
            VERIFY_ARE_EQUAL(expected, profile.RetroBezelColor());
        }
    }

    void SetupTransferTests::ExplicitRoundtripAndInheritance()
    {
        auto input = Settings();
        input["profiles"]["list"][0]["experimental.retroTerminalEffect"] = false;
        input["profiles"]["list"][0]["experimental.retroBezelEnabled"] = false;
        auto doc = Document(input, Pins());
        VERIFY_IS_TRUE(doc.settings == input);
        VERIFY_IS_FALSE(doc.settings["profiles"]["list"][0].isMember("font"));
        VERIFY_IS_FALSE(doc.settings["profiles"]["list"][0]["experimental.retroBezelEnabled"].asBool());
        VERIFY_ARE_EQUAL(size_t{1}, doc.pins.pins.size());
        auto model = winrt::make_self<winrt::Microsoft::Terminal::Settings::Model::implementation::CascadiaSettings>(S::JsonText(doc.settings));
        const auto profile = model->FindProfile(::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(A).c_str()));
        VERIFY_IS_FALSE(profile.FontInfo().HasFontFace());
        VERIFY_ARE_EQUAL(winrt::hstring{L"Consolas"}, profile.FontInfo().FontFace());
        VERIFY_IS_TRUE(profile.HasRetroBezelEnabled());
        VERIFY_IS_FALSE(profile.RetroBezelEnabled());
    }

    void SetupTransferTests::RepeatChangedAndUnrelatedProfiles()
    {
        auto input = Settings();
        auto destination = Settings(); destination["profiles"]["list"].append(Profile(C, "Keep me"));
        auto plan = S::Prepare(Document(input), destination, {});
        VERIFY_IS_TRUE(plan.CanApply());
        VERIFY_IS_FALSE(plan.changed);
        VERIFY_ARE_EQUAL(2u, plan.settings["profiles"]["list"].size());
        input["profiles"]["list"][0]["tabTitle"] = "Changed";
        plan = S::Prepare(Document(input), destination, {});
        VERIFY_IS_TRUE(plan.CanApply()); VERIFY_IS_TRUE(plan.changed);
        VERIFY_ARE_EQUAL(size_t{1}, plan.profilesUpdated);
        VERIFY_ARE_EQUAL(2u, plan.settings["profiles"]["list"].size());
        const auto again = S::Prepare(Document(input), plan.settings, plan.pins);
        VERIFY_IS_FALSE(again.changed); VERIFY_ARE_EQUAL(size_t{0}, again.profilesAdded);
        VERIFY_ARE_EQUAL(std::string{"Keep me"}, again.settings["profiles"]["list"][1]["name"].asString());
    }

    void SetupTransferTests::DefaultsAndOverridesReplaceWithoutFlattening()
    {
        auto destination = Settings();
        destination["profiles"]["list"][0]["font"]["size"] = 24;
        destination["profiles"]["list"][0]["experimental.retroBezelLeftSize"] = "wide";
        destination["profiles"]["defaults"]["experimental.retroTerminalEffect"] = true;
        auto input = Settings();
        const auto plan = S::Prepare(Document(input), destination, {});
        VERIFY_IS_TRUE(plan.CanApply());
        VERIFY_IS_FALSE(plan.settings["profiles"]["list"][0].isMember("font"));
        VERIFY_IS_FALSE(plan.settings["profiles"]["list"][0].isMember("experimental.retroBezelLeftSize"));
        VERIFY_IS_FALSE(plan.settings["profiles"]["defaults"].isMember("experimental.retroTerminalEffect"));
        VERIFY_ARE_EQUAL(17, plan.settings["profiles"]["defaults"]["font"]["size"].asInt());
    }

    void SetupTransferTests::ProfileAndGroupConflicts()
    {
        auto input = Settings(), current = Settings();
        current["profiles"]["list"].append(Profile(C, "Beta"));
        input["profiles"]["list"][0]["name"] = "beta";
        auto plan = S::Prepare(Document(input), current, {});
        VERIFY_IS_FALSE(plan.CanApply()); VERIFY_IS_TRUE(!plan.conflicts.empty());
        VERIFY_ARE_EQUAL(std::string{"Alpha"}, current["profiles"]["list"][0]["name"].asString());
        input = Settings(); current = Settings();
        input["backslash.savedTabGroups"] = Groups();
        current["backslash.savedTabGroups"] = Groups(C, "Saved layout");
        plan = S::Prepare(Document(input), current, {});
        VERIFY_IS_FALSE(plan.CanApply());
    }

    void SetupTransferTests::GroupAndPinIdsUpdateWithoutDuplication()
    {
        auto input = Settings(); input["backslash.savedTabGroups"] = Groups();
        input["backslash.startupWithSavedGroup"] = true; input["backslash.startupGroup"] = Group;
        auto plan = S::Prepare(Document(input, Pins()), Settings(), {});
        VERIFY_IS_TRUE(plan.CanApply()); VERIFY_ARE_EQUAL(size_t{1}, plan.groupsAdded); VERIFY_ARE_EQUAL(size_t{1}, plan.pinsAdded);
        auto changedPins = Pins(); changedPins.pins[0].path = "C:\\changed";
        input["backslash.savedTabGroups"]["groups"][0]["name"] = "Renamed group";
        auto changed = S::Prepare(Document(input, changedPins), plan.settings, plan.pins);
        VERIFY_IS_TRUE(changed.CanApply()); VERIFY_ARE_EQUAL(size_t{1}, changed.groupsUpdated); VERIFY_ARE_EQUAL(size_t{1}, changed.pinsUpdated);
        VERIFY_ARE_EQUAL(1u, changed.settings["backslash.savedTabGroups"]["groups"].size()); VERIFY_ARE_EQUAL(size_t{1}, changed.pins.pins.size());
        auto again = S::Prepare(Document(input, changedPins), changed.settings, changed.pins);
        VERIFY_IS_TRUE(again.CanApply()); VERIFY_IS_FALSE(again.changed);
        auto differentId = changedPins; differentId.pins[0].id = C;
        VERIFY_IS_FALSE(S::Prepare(Document(input, differentId), changed.settings, changed.pins).CanApply());
    }

    void SetupTransferTests::RejectMalformedUnsupportedAndUnknown()
    {
        VERIFY_THROWS(S::Parse("{"), std::exception);
        const auto valid = S::Export(Settings(), {});
        auto bad = S::ReadJson(valid); bad["version"] = 2;
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["leases"] = Json::Value{Json::arrayValue};
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["settings"]["future.setting"] = true;
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["settings"]["profiles"]["list"][0]["font"]["futureSetting"] = true;
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["settings"]["profiles"]["list"].append(bad["settings"]["profiles"]["list"][0]);
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["settings"]["schemes"][0]["name"] = "Duplicate";
        bad["settings"]["schemes"][1]["name"] = "duplicate";
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = S::ReadJson(valid); bad["settings"]["profiles"] = Json::Value{Json::arrayValue};
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        VERIFY_THROWS(S::Parse("{\"format\":\"backslash.setup\",\"version\":1,\"version\":2}"), std::exception);
    }

    void SetupTransferTests::CrossReferencesAndValidationArePreflightOnly()
    {
        auto current = Settings(); const auto original = current;
        auto input = Settings(); input["backslash.savedTabGroups"] = Groups();
        input["backslash.savedTabGroups"]["groups"][0]["tabs"][0][0]["profile"] = C;
        VERIFY_THROWS(S::Export(input, {}), std::exception);
        VERIFY_IS_FALSE(S::Prepare(S::Document{input, {}}, current, {}).CanApply());
        input = Settings(); auto pins = Pins(); pins.pins[0].source = "Other.Source";
        VERIFY_IS_FALSE(S::Prepare(S::Document{input, pins}, current, {}).CanApply());
        input["backslash.startupWithSavedGroup"] = true; input["backslash.startupGroup"] = Group;
        VERIFY_IS_FALSE(S::Prepare(S::Document{input, {}}, current, {}).CanApply());
        VERIFY_THROWS(S::Prepare(Document(Settings()), current, {}, [](const auto&) { throw std::invalid_argument("synthetic typed validation failure"); }), std::exception);
        VERIFY_IS_TRUE(current == original);
    }

    void SetupTransferTests::PinRuntimeFieldsNeverExport()
    {
        const auto doc = S::ReadJson(S::Export(Settings(), Pins()));
        VERIFY_IS_FALSE(doc["pins"].isMember("leases"));
        VERIFY_IS_FALSE(doc.isMember("state"));
        VERIFY_IS_FALSE(doc["settings"].isMember("recentCommands"));
        auto bad = doc; bad["pins"]["leases"] = Json::Value{Json::arrayValue};
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
        bad = doc; bad["settings"]["actions"][0]["command"]["action"] = "newTab";
        bad["settings"]["actions"][0]["command"]["__backslashPin"] = Pin;
        VERIFY_THROWS(S::Parse(S::JsonText(bad)), std::exception);
    }

    void SetupTransferTests::ExplicitGlobalFalseAndAbsentPreferences()
    {
        auto destination = Settings(); destination["alwaysOnTop"] = true; destination["copyOnSelect"] = true;
        auto input = Settings(); input["alwaysOnTop"] = false;
        input["rendering.software"] = false;
        const auto plan = S::Prepare(Document(input), destination, {});
        VERIFY_IS_TRUE(plan.CanApply()); VERIFY_IS_FALSE(plan.settings["alwaysOnTop"].asBool());
        VERIFY_IS_TRUE(plan.settings.isMember("rendering.software"));
        VERIFY_IS_FALSE(plan.settings["rendering.software"].asBool());
        VERIFY_IS_TRUE(plan.settings["copyOnSelect"].asBool());
    }

    void SetupTransferTests::ResourceDefinitionsPreserveUnrelatedProfiles()
    {
        auto current = Settings();
        auto unrelated = Profile(C, "Unrelated"); unrelated["colorScheme"] = "Keep scheme";
        current["profiles"]["list"].append(unrelated);
        current["schemes"][0]["name"] = "Keep scheme";
        current["schemes"][1]["name"] = "Shared scheme";
        current["schemes"][1]["foreground"] = "#111111";
        auto incoming = Settings(); incoming["schemes"][0]["name"] = "Shared scheme";
        incoming["schemes"][0]["foreground"] = "#222222";
        const auto plan = S::Prepare(Document(incoming), current, {});
        VERIFY_IS_TRUE(plan.CanApply());
        VERIFY_ARE_EQUAL(2u, plan.settings["schemes"].size());
        VERIFY_ARE_EQUAL(std::string{"Keep scheme"}, plan.settings["schemes"][0]["name"].asString());
        VERIFY_ARE_EQUAL(std::string{"#222222"}, plan.settings["schemes"][1]["foreground"].asString());
        VERIFY_IS_FALSE(S::Prepare(Document(incoming), plan.settings, plan.pins).changed);
    }

    void SetupTransferTests::ResourceReviewUsesReadOnlyInjectedProbes()
    {
        auto settings = Settings();
        settings["profiles"]["defaults"]["font"]["face"] = "Fixture Font, Missing Font";
        settings["profiles"]["defaults"]["backgroundImage"] = "missing.png";
        auto& profile = settings["profiles"]["list"][0];
        profile["font"]["face"] = "Fixture Font";
        profile["commandline"] = "\"C:\\Fixtures\\shell.exe\" --not-executed";
        profile["icon"] = "https://example.invalid/icon.png";
        profile["unfocusedAppearance"]["experimental.pixelShaderPath"] = "shader.hlsl";
        bool commandChecked = false;
        S::ResourceProbes probes{
            [](const auto& path) { return path.filename() == L"shader.hlsl"; },
            [](const auto& family) { return family == L"Fixture Font"; },
            [&](const auto& executable) { commandChecked = executable == L"C:\\Fixtures\\shell.exe"; return false; }
        };
        VERIFY_ARE_EQUAL(size_t{4}, S::Analyze(settings, L"C:\\Fixtures", probes).size());
        VERIFY_IS_TRUE(commandChecked);
        settings["profiles"]["list"].clear(); settings["profiles"]["defaults"].clear();
        VERIFY_IS_TRUE(S::Analyze(settings, L"C:\\Fixtures", probes).empty());
    }

    void SetupTransferTests::ResourceReviewNeverProbesExternalPaths()
    {
        constexpr auto variable = L"BACKSLASH_SETUP_NETWORK_REFERENCE_TEST";
        std::wstring previous(32768, L'\0');
        const auto length = GetEnvironmentVariableW(variable, previous.data(), static_cast<DWORD>(previous.size()));
        previous.resize(length < previous.size() ? length : 0);
        const auto restore = wil::scope_exit([&] { SetEnvironmentVariableW(variable, previous.empty() ? nullptr : previous.c_str()); });
        VERIFY_IS_TRUE(SetEnvironmentVariableW(variable, L"\\\\server.invalid\\share"));
        size_t probesCalled = 0;
        S::ResourceProbes recording{
            [&](const auto&) { ++probesCalled; return false; },
            [&](const auto&) { ++probesCalled; return false; },
            [&](const auto&) { ++probesCalled; return false; }
        };
        for (const auto* reference : { R"(\\server.invalid\share\image.png)", R"(//server.invalid/share/image.png)",
            R"(\\?\UNC\server.invalid\share\image.png)", R"(\\.\pipe\setup-test)", R"(\??\UNC\server.invalid\share\image.png)",
            R"(\Device\Mup\server.invalid\share\image.png)", "https://server.invalid/image.png", "file://server.invalid/share/image.png",
            "ms-appx://server.invalid/image.png", "NUL", R"(C:relative.png)", R"(C:\assets\image.png:stream)",
            R"(%BACKSLASH_SETUP_NETWORK_REFERENCE_TEST%\image.png)" })
        {
            auto settings = S::ReadJson(R"({"profiles":{"defaults":{},"list":[{}]}})");
            auto& defaults = settings["profiles"]["defaults"];
            defaults["backgroundImage"] = reference;
            defaults["experimental.pixelShaderPath"] = reference;
            defaults["unfocusedAppearance"]["backgroundImage"] = reference;
            auto& profile = settings["profiles"]["list"][0];
            profile["icon"] = reference;
            profile["commandline"] = std::string{"\""} + reference + "\" --not-executed";
            // Font family matching is never attempted for path-shaped names.
            if (std::string_view{reference} != "NUL") { profile["font"]["face"] = reference; }
            VERIFY_IS_FALSE(S::Analyze(settings, L"C:\\Fixtures", recording).empty());
        }
        auto relative = S::ReadJson(R"({"profiles":{"defaults":{"backgroundImage":"image.png"},"list":[]}})");
        VERIFY_IS_FALSE(S::Analyze(relative, L"\\\\server.invalid\\share", recording).empty());
        VERIFY_ARE_EQUAL(size_t{0}, probesCalled);
    }

    void SetupTransferTests::ProductionResourceReviewDoesNotResolveReferences()
    {
        const auto settings = S::ReadJson(R"({"profiles":{"defaults":{"font":{"face":"Unverified fixture font"},
            "backgroundImage":"Z:\\mapped-or-reparse\\image.png"},"list":[{"commandline":"fixture-shell.exe"}]}})");
        const auto safe = S::PreviewResourceProbes();
        VERIFY_IS_FALSE(safe.exists(L"Z:\\mapped-or-reparse\\image.png").has_value());
        VERIFY_IS_FALSE(safe.font(L"Unverified fixture font").has_value());
        VERIFY_IS_FALSE(safe.executable(L"fixture-shell.exe").has_value());
        const auto warnings = S::Analyze(settings, L"C:\\Fixtures");
        VERIFY_ARE_EQUAL(size_t{3}, warnings.size());
        for (const auto& warning : warnings) { VERIFY_IS_TRUE(warning.find("not checked before import") != std::string::npos); }
    }

    void SetupTransferTests::ModelExplicitExportPreservesGlobalOffAndIsReadOnly()
    {
        auto input = Settings();
        input["rendering.software"] = false;
        input["rendering.disablePartialInvalidation"] = false;
        input["rendering.graphicsAPI"] = "automatic";
        input["profiles"]["list"][0]["experimental.retroTerminalEffect"] = false;
        auto model = winrt::make_self<winrt::Microsoft::Terminal::Settings::Model::implementation::CascadiaSettings>(S::JsonText(input));
        const auto globals = model->GlobalSettings();
        VERIFY_IS_TRUE(globals.HasSoftwareRendering());
        VERIFY_IS_TRUE(globals.HasDisablePartialInvalidation());
        VERIFY_IS_TRUE(globals.HasGraphicsAPI());
        const auto projected = model.as<winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings>();
        const auto first = S::ReadJson(winrt::to_string(projected.ExplicitSettingsJson()));
        VERIFY_IS_TRUE(first.isMember("rendering.software"));
        VERIFY_IS_FALSE(first["rendering.software"].asBool());
        VERIFY_IS_TRUE(first.isMember("rendering.disablePartialInvalidation"));
        VERIFY_IS_FALSE(first["rendering.disablePartialInvalidation"].asBool());
        VERIFY_ARE_EQUAL(std::string{"automatic"}, first["rendering.graphicsAPI"].asString());
        VERIFY_IS_FALSE(first["profiles"]["defaults"]["experimental.retroBezelEnabled"].asBool());
        VERIFY_IS_TRUE(first["profiles"]["defaults"].isMember("experimental.retroBezelEnabled"));
        VERIFY_IS_FALSE(first["profiles"]["list"][0]["experimental.retroTerminalEffect"].asBool());
        VERIFY_IS_TRUE(first["profiles"]["list"][0].isMember("experimental.retroTerminalEffect"));
        const auto second = S::ReadJson(winrt::to_string(projected.ExplicitSettingsJson()));
        VERIFY_IS_TRUE(first == second);
        VERIFY_IS_TRUE(globals.HasSoftwareRendering());
        VERIFY_IS_TRUE(globals.HasDisablePartialInvalidation());
        VERIFY_IS_TRUE(globals.HasGraphicsAPI());
    }
}
