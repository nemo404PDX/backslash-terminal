// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <fstream>
#include <til/io.h>

#include "../TerminalSettingsModel/ColorScheme.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/resource.h"
#include "JsonTestClass.h"
#include "TestUtils.h"
#include "../TerminalSettingsModel/AppearanceConfig.h"
#include "../TerminalSettingsModel/GlobalAppSettings.h"
#include "../TerminalSettingsModel/SavedTabGroups.h"
#include "../TerminalSettingsModel/StartupGroupPolicy.h"
#include "../TerminalSettingsModel/GroupSavePolicy.h"
#include "../TerminalSettingsModel/ApplicationState.h"

using namespace Microsoft::Console;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace WEX::Common;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Microsoft::Terminal::Control;

// Different architectures will hash the same SendInput command to a different ID
// Check for the correct ID based on the architecture
#if defined(_M_IX86)
#define SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH "56911147"
#else
#define SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH "A020D2"
#endif

#if defined(_M_IX86)
#define SEND_INPUT2_ARCH_SPECIFIC_ACTION_HASH "35488AA6"
#else
#define SEND_INPUT2_ARCH_SPECIFIC_ACTION_HASH "58D1971"
#endif

namespace SettingsModelUnitTests
{
    class SerializationTests : public JsonTestClass
    {
        TEST_CLASS(SerializationTests);

        TEST_METHOD(BackslashRetroIntensityModel);
        TEST_METHOD(BackslashScanlineDimensions);
        TEST_METHOD(BackslashPhosphorTint);
        TEST_METHOD(BackslashScanlineWhite);
        TEST_METHOD(BackslashPhosphorContrastGuard);
        TEST_METHOD(BackslashStartupGroupScope);
        TEST_METHOD(BackslashStartupGroupSelection);
        TEST_METHOD(BackslashStartupGroupSettings);
        TEST_METHOD(BackslashStartupGroupPicker);
        TEST_METHOD(BackslashSavedGroupsRoundtrip);
        TEST_METHOD(BackslashSavedGroupsRestore);
        TEST_METHOD(BackslashGroupSaveEligibility);
        TEST_METHOD(BackslashGroupSaveDraftMerge);
        TEST_METHOD(BackslashGroupSaveNoticeState);
        TEST_METHOD(BackslashProfilesSidebarState);
        TEST_METHOD(BackslashRetroIntensityValidation);
        TEST_METHOD(GlobalSettings);
        TEST_METHOD(Profile);
        TEST_METHOD(ColorScheme);
        TEST_METHOD(Actions);
        TEST_METHOD(CascadiaSettings);
        TEST_METHOD(LegacyFontSettings);

        TEST_METHOD(RoundtripReloadEnvVars);
        TEST_METHOD(DontRoundtripNoReloadEnvVars);

        TEST_METHOD(RoundtripUserModifiedColorSchemeCollision);
        TEST_METHOD(RoundtripUserModifiedColorSchemeCollisionUnusedByProfiles);
        TEST_METHOD(RoundtripUserDeletedColorSchemeCollision);

        TEST_METHOD(RoundtripGenerateActionID);
        TEST_METHOD(NoGeneratedIDsForIterableAndNestedCommands);
        TEST_METHOD(GeneratedActionIDsEqualForIdenticalCommands);
        TEST_METHOD(RoundtripLegacyToModernActions);
        TEST_METHOD(RoundtripUserActionsSameAsInBoxAreRemoved);
        TEST_METHOD(RoundtripActionsSameNameDifferentCommandsAreRetained);
        TEST_METHOD(MultipleActionsAreCollapsed);

        TEST_METHOD(ProfileWithInvalidIcon);

    private:
        // Method Description:
        // - deserializes and reserializes a json string representing a settings object model of type T
        // - verifies that the generated json string matches the provided one
        // Template Types:
        // - <T>: The type of Settings Model object to generate (must be impl type)
        // Arguments:
        // - jsonString - JSON string we're performing the test on
        // Return Value:
        // - the JsonObject representing this instance
        template<typename T>
        void RoundtripTest(const std::string_view& jsonString)
        {
            const auto json{ VerifyParseSucceeded(jsonString) };
            const auto settings{ T::FromJson(json) };
            const auto result{ settings->ToJson() };

            // Compare toString(json) instead of jsonString here.
            // The toString writes the json out alphabetically.
            // This trick allows jsonString to _not_ have to be
            // written alphabetically.
            VERIFY_ARE_EQUAL(toString(json), toString(result));
        }

        // Helper to remove the `$schema` property from a json object. We
        // populate that based off the local path to the settings file. Of
        // course, that's entirely unpredictable in tests. So cut it out before
        // we do any sort of roundtrip testing.
        static Json::Value removeSchema(Json::Value json)
        {
            if (json.isMember("$schema"))
            {
                json.removeMember("$schema");
            }
            return json;
        }
    };

    void SerializationTests::BackslashRetroIntensityModel()
    {
        const auto parent = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        VERIFY_ARE_EQUAL(0.15f, parent->RetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.10f, parent->RetroTerminalScanlineIntensity());
        VERIFY_IS_FALSE(parent->HasRetroTerminalGlowIntensity());
        VERIFY_IS_FALSE(parent->ToJson().isMember("experimental.retroTerminalGlowIntensity"));
        parent->RetroTerminalGlowIntensity(0.8f);
        parent->RetroTerminalScanlineIntensity(0.7f);
        const auto child = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        child->AddLeastImportantParent(parent);
        VERIFY_ARE_EQUAL(0.8f, child->RetroTerminalGlowIntensity());
        VERIFY_IS_FALSE(child->HasRetroTerminalGlowIntensity());
        VERIFY_IS_NOT_NULL(child->RetroTerminalGlowIntensityOverrideSource());
        child->LayerJson(VerifyParseSucceeded(R"({
            "experimental.retroTerminalEffect": false,
            "experimental.retroTerminalGlowIntensity": 0,
            "experimental.retroTerminalScanlineIntensity": 0,
            "experimental.pixelShaderPath": "custom.hlsl"
        })"));
        VERIFY_IS_TRUE(child->HasRetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.0f, child->ToJson()["experimental.retroTerminalGlowIntensity"].asFloat());
        const auto copy = implementation::AppearanceConfig::CopyAppearance(child.get(), {});
        VERIFY_ARE_EQUAL(toString(child->ToJson()), toString(copy->ToJson()));
        const auto reloaded = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        reloaded->LayerJson(copy->ToJson());
        VERIFY_ARE_EQUAL(toString(copy->ToJson()), toString(reloaded->ToJson()));
        child->RetroTerminalEffect(true);
        child->RetroTerminalEffect(false);
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalGlowIntensity());
        child->ClearRetroTerminalGlowIntensity();
        VERIFY_IS_FALSE(child->HasRetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.8f, child->RetroTerminalGlowIntensity());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineIntensity":null})"));
        VERIFY_IS_FALSE(child->HasRetroTerminalScanlineIntensity());
        VERIFY_ARE_EQUAL(0.7f, child->RetroTerminalScanlineIntensity());
        std::set<std::string> changes;
        parent->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalGlowIntensity":0.4,"experimental.retroTerminalScanlineIntensity":0.2})"));
        parent->LogSettingChanges(changes, "profile");
        VERIFY_IS_TRUE(changes.empty());
    }

    void SerializationTests::BackslashRetroIntensityValidation()
    {
        const auto appearance = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        appearance->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalGlowIntensity":-2,"experimental.retroTerminalScanlineIntensity":2})"));
        VERIFY_ARE_EQUAL(0.0f, appearance->RetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(1.0f, appearance->RetroTerminalScanlineIntensity());
        appearance->RetroTerminalGlowIntensity(2.0f);
        appearance->RetroTerminalScanlineIntensity(-1.0f);
        VERIFY_ARE_EQUAL(1.0f, appearance->RetroTerminalGlowIntensity());
        VERIFY_ARE_EQUAL(0.0f, appearance->RetroTerminalScanlineIntensity());
        VERIFY_THROWS(appearance->RetroTerminalGlowIntensity(std::numeric_limits<float>::quiet_NaN()), winrt::hresult_invalid_argument);
        VERIFY_THROWS(appearance->RetroTerminalScanlineIntensity(std::numeric_limits<float>::infinity()), winrt::hresult_invalid_argument);
        VERIFY_ARE_EQUAL(1.0f, appearance->RetroTerminalGlowIntensity());
        VERIFY_THROWS(appearance->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalGlowIntensity":1e100})")), winrt::hresult_invalid_argument);
        VERIFY_ARE_EQUAL(1.0f, appearance->RetroTerminalGlowIntensity());
        VERIFY_THROWS(appearance->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalGlowIntensity":"bad"})")), std::exception);
    }

    void SerializationTests::BackslashStartupGroupScope()
    {
        namespace P = ::Microsoft::Terminal::Backslash::StartupGroupPolicy;
        // Initial ordinary launch only; every explicit/restore/transfer source excludes it.
        for (unsigned mask = 0; mask < 64; ++mask)
        {
            const P::Context c{ (mask & 1u) != 0, (mask & 2u) != 0, (mask & 4u) != 0, (mask & 8u) != 0, (mask & 16u) != 0, (mask & 32u) != 0 };
            VERIFY_ARE_EQUAL(mask == 1u, P::Eligible(c));
        }
        VERIFY_IS_TRUE(P::Matches(L"Development group", L"velop"));
        VERIFY_IS_TRUE(P::Matches(L"Development group", L"GROUP"));
        VERIFY_IS_TRUE(P::Matches(L"作業 Ω", L"作業"));
        VERIFY_IS_FALSE(P::Matches(L"Development", L"unmatched"));
    }

    void SerializationTests::BackslashStartupGroupSelection()
    {
        namespace P = ::Microsoft::Terminal::Backslash::StartupGroupPolicy;
        using Store = ::Microsoft::Terminal::Backslash::SavedTabGroups;
        auto value = VerifyParseSucceeded(R"({"version":1,"groups":[{"id":"{11111111-1111-4111-8111-111111111111}","name":"Original","folderMode":"profileDefault","focusedTab":1,"tabs":[[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture"},{"kind":"split","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture","direction":"down","size":0.35}],[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture"}]]}]})");
        const auto id = winrt::hstring{ L"{11111111-1111-4111-8111-111111111111}" };
        const auto available = [](const winrt::guid&, const winrt::hstring& source) { return source == L"fixture"; };
        const auto folders = [](const winrt::hstring&) { return true; };
        const auto catalog = [&]() { return Store::ReadValue(value).Json(); };
        auto p = P::Prepare(catalog(), id, available, folders);
        VERIFY_IS_TRUE(p.group.has_value());VERIFY_ARE_EQUAL(size_t{2},p.restore.tabs.size());VERIFY_ARE_EQUAL(uint32_t{1},p.group->focusedTab);
        VERIFY_ARE_EQUAL(size_t{2},p.restore.tabs[0].size());VERIFY_ARE_EQUAL(size_t{1},p.restore.tabs[1].size());
        value["groups"][0]["name"] = "Renamed";p=P::Prepare(catalog(),id,available,folders);
        VERIFY_IS_TRUE(p.group.has_value());VERIFY_ARE_EQUAL(std::string{"Renamed"},p.group->name);
        value["groups"][0]["id"] = "{33333333-3333-4333-8333-333333333333}";
        p=P::Prepare(catalog(),id,available,folders);VERIFY_IS_FALSE(p.group.has_value());VERIFY_IS_FALSE(p.notice.empty());
        value["groups"][0]["id"] = winrt::to_string(id);
        p=P::Prepare(catalog(),id,[](const winrt::guid&,const winrt::hstring&){return false;},folders);
        VERIFY_IS_FALSE(p.group.has_value());VERIFY_IS_TRUE(p.restore.tabs.empty());
        value["groups"][0]["folderMode"]="captured";p=P::Prepare(catalog(),id,available,folders);
        VERIFY_ARE_EQUAL(uint32_t{3},p.restore.folderFallbacks);P::FolderChoice(p,true);VERIFY_IS_TRUE(p.group.has_value());
        P::FolderChoice(p,false);VERIFY_IS_FALSE(p.group.has_value());VERIFY_IS_TRUE(p.restore.tabs.empty());VERIFY_IS_FALSE(p.notice.empty());
        p=P::Prepare(L"{\"version\":2,\"groups\":[]}",id,available,folders);VERIFY_IS_FALSE(p.group.has_value());VERIFY_IS_FALSE(p.notice.empty());
        p=P::Prepare(catalog(),L"invalid ID",available,folders);VERIFY_IS_FALSE(p.group.has_value());
    }

    void SerializationTests::BackslashStartupGroupSettings()
    {
        const auto settings = implementation::GlobalAppSettings::FromJson(VerifyParseSucceeded(R"({"defaultProfile":"{22222222-2222-4222-8222-222222222222}"})"),OriginTag::User);
        VERIFY_IS_FALSE(settings->StartupWithSavedGroup());VERIFY_IS_TRUE(settings->StartupGroup().empty());
        const auto defaultProfile = settings->DefaultProfile();
        const auto draft = settings->Copy();draft->StartupWithSavedGroup(true);draft->StartupGroup(L"{11111111-1111-4111-8111-111111111111}");
        VERIFY_IS_FALSE(settings->StartupWithSavedGroup());VERIFY_IS_TRUE(settings->StartupGroup().empty());
        VERIFY_ARE_EQUAL(defaultProfile,draft->DefaultProfile());
        const auto saved=implementation::GlobalAppSettings::FromJson(draft->ToJson(),OriginTag::User);
        VERIFY_IS_TRUE(saved->StartupWithSavedGroup());VERIFY_ARE_EQUAL(draft->StartupGroup(),saved->StartupGroup());
        saved->StartupWithSavedGroup(false);VERIFY_ARE_EQUAL(draft->StartupGroup(),saved->StartupGroup());
        VERIFY_ARE_EQUAL(defaultProfile,saved->DefaultProfile());
    }

    void SerializationTests::BackslashStartupGroupPicker()
    {
        namespace P = ::Microsoft::Terminal::Backslash::StartupGroupPolicy;
        using Store = ::Microsoft::Terminal::Backslash::SavedTabGroups;
        auto catalog = VerifyParseSucceeded(R"({"version":1,"groups":[{"id":"{11111111-1111-4111-8111-111111111111}","name":"First group","folderMode":"profileDefault","focusedTab":0,"tabs":[[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture"}]]},{"id":"{33333333-3333-4333-8333-333333333333}","name":"Second group","folderMode":"profileDefault","focusedTab":0,"tabs":[[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture"}]]}]})");
        auto saved = Store::ReadValue(catalog).Json();
        const winrt::hstring first{L"{11111111-1111-4111-8111-111111111111}"}, second{L"{33333333-3333-4333-8333-333333333333}"};
        auto state = P::Picker(saved,L"");VERIFY_ARE_EQUAL(size_t{2},state.choices.size());VERIFY_IS_FALSE(state.selected.has_value());
        auto choice = P::Choose(saved,first);VERIFY_IS_TRUE(choice.has_value());VERIFY_ARE_EQUAL(first,choice->id);VERIFY_ARE_EQUAL(winrt::hstring{L"First group"},choice->name);
        auto settings = implementation::GlobalAppSettings::FromJson(VerifyParseSucceeded(R"({})"),OriginTag::User);
        settings->SavedTabGroupsJson(saved);settings->StartupWithSavedGroup(true);
        const auto draft = settings->Copy();draft->StartupGroup(choice->id);
        VERIFY_IS_TRUE(settings->StartupGroup().empty()); // selection edits only the draft
        // Fresh process equivalent: serialize, reload an already-created group,
        // select it, then change the selection again without rebuilding a UI list.
        auto reopened = implementation::GlobalAppSettings::FromJson(draft->ToJson(),OriginTag::User);
        state = P::Picker(reopened->SavedTabGroupsJson(),reopened->StartupGroup());
        VERIFY_IS_TRUE(state.selected.has_value());VERIFY_ARE_EQUAL(first,state.selected->id);
        choice = P::Choose(reopened->SavedTabGroupsJson(),first);VERIFY_IS_TRUE(choice.has_value());
        choice = P::Choose(reopened->SavedTabGroupsJson(),second);VERIFY_IS_TRUE(choice.has_value());reopened->StartupGroup(choice->id);
        VERIFY_ARE_EQUAL(second,reopened->StartupGroup());
        VERIFY_IS_FALSE(P::Choose(saved,L"").has_value());VERIFY_IS_FALSE(P::Choose(saved,L"First group").has_value());
        state=P::Picker(saved,L"{zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz}");VERIFY_ARE_EQUAL(size_t{2},state.choices.size());VERIFY_IS_FALSE(state.notice.empty());
        state=P::Picker(saved,second,L"FIRST");VERIFY_ARE_EQUAL(size_t{1},state.choices.size());VERIFY_ARE_EQUAL(second,state.selected->id);
        catalog["groups"][0]["name"]="Renamed group";saved=Store::ReadValue(catalog).Json();choice=P::Choose(saved,first);
        VERIFY_ARE_EQUAL(winrt::hstring{L"Renamed group"},choice->name);VERIFY_ARE_EQUAL(first,choice->id);
        catalog["groups"][0]["id"]="{44444444-4444-4444-8444-444444444444}";saved=Store::ReadValue(catalog).Json();
        VERIFY_IS_FALSE(P::Choose(saved,first).has_value());state=P::Picker(saved,first);VERIFY_IS_FALSE(state.notice.empty());VERIFY_ARE_EQUAL(size_t{2},state.choices.size());
        state=P::Picker(LR"({"version":1,"groups":[]})",first);VERIFY_IS_TRUE(state.choices.empty());VERIFY_IS_FALSE(state.selected.has_value());
        state=P::Picker(LR"({"version":2,"groups":[]})",first);VERIFY_IS_TRUE(state.choices.empty());VERIFY_IS_FALSE(state.notice.empty());
        // Actual user's already-persisted catalog is injected as a read-only
        // fixture. Never open its profiles or rewrite its source settings.
        wchar_t fixturePath[32768];
        const auto pathLength = GetEnvironmentVariableW(L"BACKSLASH_TEST_SAVED_GROUPS_JSON",fixturePath,static_cast<DWORD>(std::size(fixturePath)));
        if (pathLength > 0)
        {
            VERIFY_IS_TRUE(pathLength < std::size(fixturePath));
            std::ifstream stream{std::filesystem::path{fixturePath},std::ios::binary};VERIFY_IS_TRUE(stream.good());
            const std::string raw{std::istreambuf_iterator<char>{stream},std::istreambuf_iterator<char>{}};
            const auto real = winrt::to_hstring(raw);const auto all=P::Picker(real,L"");VERIFY_IS_FALSE(all.choices.empty());
            for (const auto& item : all.choices)
            {
                const auto initial=P::Choose(real,item.id);VERIFY_IS_TRUE(initial.has_value());VERIFY_ARE_EQUAL(item.name,initial->name);
                const auto loaded=P::Picker(real,initial->id);VERIFY_IS_TRUE(loaded.selected.has_value());VERIFY_ARE_EQUAL(item.id,loaded.selected->id);
                const auto again=P::Choose(real,loaded.selected->id);VERIFY_IS_TRUE(again.has_value());VERIFY_ARE_EQUAL(item.id,again->id);
            }
            const auto realDraft = settings->Copy();realDraft->SavedTabGroupsJson(real);realDraft->StartupGroup(all.choices.front().id);
            const auto realReloaded = implementation::GlobalAppSettings::FromJson(realDraft->ToJson(),OriginTag::User);
            VERIFY_ARE_EQUAL(all.choices.front().id,realReloaded->StartupGroup());
            VERIFY_ARE_EQUAL(Store::Read(real).Json(),Store::Read(realReloaded->SavedTabGroupsJson()).Json());
        }
    }

    void SerializationTests::BackslashGroupSaveEligibility()
    {
        namespace P = ::Microsoft::Terminal::Backslash::GroupSavePolicy;
        using K=P::PaneKind;
        auto selection=P::Select({{K::Terminal}},0);VERIFY_ARE_EQUAL(size_t{1},selection.indices.size());VERIFY_ARE_EQUAL(0u,selection.focusedTab);VERIFY_IS_FALSE(selection.skippedSettings);
        selection=P::Select({{K::Terminal,K::Terminal,K::Terminal}},0);VERIFY_ARE_EQUAL(size_t{1},selection.indices.size());VERIFY_ARE_EQUAL(0u,selection.focusedTab);
        const std::vector<std::vector<K>> tabs{{K::Terminal},{K::Settings},{K::Terminal,K::Terminal},{K::Settings},{K::Terminal}};
        selection=P::Select(tabs,4);VERIFY_ARE_EQUAL(size_t{3},selection.indices.size());VERIFY_ARE_EQUAL(0u,selection.indices[0]);VERIFY_ARE_EQUAL(2u,selection.indices[1]);VERIFY_ARE_EQUAL(4u,selection.indices[2]);VERIFY_ARE_EQUAL(2u,selection.focusedTab);VERIFY_IS_TRUE(selection.skippedSettings);
        selection=P::Select(tabs,2);VERIFY_ARE_EQUAL(1u,selection.focusedTab);
        selection=P::Select(tabs,3);VERIFY_ARE_EQUAL(0u,selection.focusedTab);
        selection=P::Select(tabs,99);VERIFY_ARE_EQUAL(0u,selection.focusedTab);
        VERIFY_THROWS(P::Select({},0),std::invalid_argument);
        try { P::Select({{K::Settings}},0);VERIFY_FAIL(L"Settings-only must not save an empty group"); }
        catch(const std::invalid_argument& e){VERIFY_ARE_EQUAL(std::string{"No terminal tabs to save."},std::string{e.what()});}
        VERIFY_THROWS(P::Select({{K::Terminal,K::Settings}},0),std::invalid_argument);
        VERIFY_THROWS(P::Select({{K::Terminal},{K::Unsupported}},0),std::invalid_argument);
        VERIFY_THROWS(P::Select({{K::Settings,K::Settings}},0),std::invalid_argument);
        VERIFY_THROWS(P::Select({{}},0),std::invalid_argument);
        VERIFY_IS_TRUE(P::ShowSettingsNotice(selection,false,false));VERIFY_IS_FALSE(P::ShowSettingsNotice(selection,true,false));VERIFY_IS_FALSE(P::ShowSettingsNotice(selection,false,true));
        VERIFY_IS_FALSE(P::ShowSettingsNotice(P::Select({{K::Terminal}},0),false,false));
        for(unsigned flags=0;flags<8;++flags){VERIFY_ARE_EQUAL(flags==7u,P::PersistNoticeChoice((flags&1)!=0,(flags&2)!=0,(flags&4)!=0));}
    }

    void SerializationTests::BackslashGroupSaveDraftMerge()
    {
        namespace P = ::Microsoft::Terminal::Backslash::GroupSavePolicy;using Store=::Microsoft::Terminal::Backslash::SavedTabGroups;
        auto value=VerifyParseSucceeded(R"({"version":1,"groups":[{"id":"{11111111-1111-4111-8111-111111111111}","name":"Existing","folderMode":"profileDefault","focusedTab":0,"tabs":[[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"fixture"}]]}]})");
        const auto before=Store::ReadValue(value).Json();auto incoming=value;auto addition=value["groups"][0];addition["id"]="{33333333-3333-4333-8333-333333333333}";addition["name"]="Captured terminals";incoming["groups"].append(addition);const auto after=Store::ReadValue(incoming).Json();
        auto globals=implementation::GlobalAppSettings::FromJson(VerifyParseSucceeded(R"({"defaultProfile":"{22222222-2222-4222-8222-222222222222}"})"),OriginTag::User);globals->SavedTabGroupsJson(before);
        const auto draft=globals->Copy();draft->InitialRows(77);draft->StartupWithSavedGroup(true);draft->StartupGroup(L"{11111111-1111-4111-8111-111111111111}");
        auto edited=value;edited["groups"][0]["name"]="Unsaved rename";draft->SavedTabGroupsJson(Store::ReadValue(edited).Json());
        const auto merged=P::MergeAddedGroups(before,after,draft->SavedTabGroupsJson());draft->SavedTabGroupsJson(merged);
        VERIFY_ARE_EQUAL(77,draft->InitialRows());VERIFY_IS_TRUE(draft->StartupWithSavedGroup());VERIFY_ARE_EQUAL(globals->DefaultProfile(),draft->DefaultProfile());VERIFY_ARE_EQUAL(before,globals->SavedTabGroupsJson());
        const auto combined=Store::Read(merged);VERIFY_ARE_EQUAL(size_t{2},combined.groups.size());VERIFY_ARE_EQUAL(std::string{"Unsaved rename"},combined.groups[0].name);VERIFY_ARE_EQUAL(std::string{"Captured terminals"},combined.groups[1].name);
        VERIFY_ARE_EQUAL(merged,P::MergeAddedGroups(before,after,merged)); // watcher replay, exactly one new GUID
        const auto loaded=implementation::GlobalAppSettings::FromJson(draft->ToJson(),OriginTag::User);VERIFY_ARE_EQUAL(merged,loaded->SavedTabGroupsJson());VERIFY_ARE_EQUAL(77,loaded->InitialRows());
        auto deleted=edited;deleted["groups"].clear();const auto keptDeletion=Store::Read(P::MergeAddedGroups(before,after,Store::ReadValue(deleted).Json()));VERIFY_ARE_EQUAL(size_t{1},keptDeletion.groups.size());VERIFY_ARE_EQUAL(std::string{"Captured terminals"},keptDeletion.groups[0].name);
        edited["groups"][0]["name"]="CAPTURED TERMINALS";const auto conflicting=Store::ReadValue(edited).Json();VERIFY_THROWS(P::MergeAddedGroups(before,after,conflicting),std::invalid_argument);VERIFY_ARE_EQUAL(std::string{"Unsaved rename"},Store::Read(draft->SavedTabGroupsJson()).groups[0].name);
        auto changed=incoming;changed["groups"][0]["name"]="External rename";VERIFY_IS_FALSE(P::AppendOnly(Store::Read(before),Store::ReadValue(changed)));
        Json::Value oldSettings,newSettings;oldSettings["defaultProfile"]="same";newSettings=oldSettings;oldSettings["backslash.savedTabGroups"]=value;newSettings["backslash.savedTabGroups"]=incoming;
        VERIFY_IS_TRUE(P::OnlyGroupsDiffer(oldSettings,newSettings));newSettings["initialRows"]=50;VERIFY_IS_FALSE(P::OnlyGroupsDiffer(oldSettings,newSettings));
        // Canceled/failed writer never assigns the pure merge to source or draft.
        VERIFY_ARE_EQUAL(before,globals->SavedTabGroupsJson());VERIFY_ARE_EQUAL(77,draft->InitialRows());
        implementation::SettingsLoader loader{R"({"defaultProfile":"{22222222-2222-4222-8222-222222222222}","profiles":{"list":[{"guid":"{22222222-2222-4222-8222-222222222222}","name":"Local","commandline":"cmd.exe"}]}})","{}"};
        loader.MergeInboxIntoUserSettings();loader.FinalizeLayering();loader.FixupUserSettings();
        const auto live = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        live->GlobalSettings().SavedTabGroupsJson(before);const auto liveProjection=live.as<winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings>();
        const auto updated=liveProjection.Copy();updated.GlobalSettings().SavedTabGroupsJson(after);
        VERIFY_IS_FALSE(liveProjection.EqualsForGroupSave(updated,false));VERIFY_IS_TRUE(liveProjection.EqualsForGroupSave(updated,true));
        const auto fullDraft=liveProjection.Copy();fullDraft.GlobalSettings().InitialRows(77);fullDraft.FindProfile(Utils::GuidFromString(L"{22222222-2222-4222-8222-222222222222}")).Name(L"Unsaved profile name");
        fullDraft.GlobalSettings().SavedTabGroupsJson(P::MergeAddedGroups(before,after,fullDraft.GlobalSettings().SavedTabGroupsJson()));
        VERIFY_ARE_EQUAL(winrt::hstring{L"Unsaved profile name"},fullDraft.FindProfile(Utils::GuidFromString(L"{22222222-2222-4222-8222-222222222222}")).Name());
        VERIFY_ARE_EQUAL(winrt::hstring{L"Local"},liveProjection.FindProfile(Utils::GuidFromString(L"{22222222-2222-4222-8222-222222222222}")).Name());
        VERIFY_ARE_EQUAL(77,fullDraft.GlobalSettings().InitialRows());VERIFY_IS_FALSE(liveProjection.EqualsForGroupSave(fullDraft,true));
        GUID writeId;THROW_IF_FAILED(CoCreateGuid(&writeId));const auto writeDirectory=std::filesystem::temp_directory_path()/(L"Backslash-group-write-tests-"+std::wstring{winrt::to_hstring(writeId)});std::filesystem::create_directory(writeDirectory);
        const auto writeState=winrt::make_self<implementation::ApplicationState>(writeDirectory);
        const auto liveJson=live->ToJson();const auto draftJson=winrt::get_self<implementation::CascadiaSettings>(fullDraft)->ToJson();
        auto updatedImpl=winrt::get_self<implementation::CascadiaSettings>(updated);
        const auto failed=updatedImpl->WriteSettingsToDisk([](std::string_view){throw std::runtime_error("injected group write failure");},*writeState);
        VERIFY_IS_FALSE(failed);VERIFY_ARE_EQUAL(toString(liveJson),toString(live->ToJson()));VERIFY_ARE_EQUAL(toString(draftJson),toString(winrt::get_self<implementation::CascadiaSettings>(fullDraft)->ToJson()));VERIFY_IS_FALSE(writeState->SavedGroupSettingsNoticeDismissed());
        std::string persisted;VERIFY_IS_TRUE(updatedImpl->WriteSettingsToDisk([&](std::string_view text){persisted=text;},*writeState));
        const auto persistedRoot=VerifyParseSucceeded(persisted);VERIFY_ARE_EQUAL(size_t{2},Store::ReadValue(persistedRoot["backslash.savedTabGroups"]).groups.size());
        VERIFY_ARE_EQUAL(toString(liveJson),toString(live->ToJson()));VERIFY_ARE_EQUAL(toString(draftJson),toString(winrt::get_self<implementation::CascadiaSettings>(fullDraft)->ToJson()));
        // Normal Reset copies the newly saved source, retaining the addition and discarding draft edits.
        const auto reset=updated.Copy();VERIFY_ARE_EQUAL(winrt::hstring{L"Local"},reset.FindProfile(Utils::GuidFromString(L"{22222222-2222-4222-8222-222222222222}")).Name());VERIFY_ARE_EQUAL(size_t{2},Store::Read(reset.GlobalSettings().SavedTabGroupsJson()).groups.size());
    }

    void SerializationTests::BackslashProfilesSidebarState()
    {
        GUID id;
        THROW_IF_FAILED(CoCreateGuid(&id));
        const auto directory = std::filesystem::temp_directory_path() / (L"Backslash-sidebar-state-tests-" + std::wstring{ winrt::to_hstring(id) });
        VERIFY_IS_TRUE(std::filesystem::create_directory(directory));
        Log::Comment(directory.c_str());
        // Explicit implementation construction: every read/write is rooted in
        // this unique temporary directory. Never resolve SharedInstance here.
        auto state = winrt::make_self<implementation::ApplicationState>(directory);
        VERIFY_IS_FALSE(state->SettingsProfilesCollapsed()); // First run is expanded.
        for (const bool collapsed : { true, false, true })
        {
            state->SettingsProfilesCollapsed(collapsed);
            state->Flush();
            state = nullptr;
            state = winrt::make_self<implementation::ApplicationState>(directory);
            VERIFY_ARE_EQUAL(collapsed, state->SettingsProfilesCollapsed());
            const auto persisted = til::io::read_file_as_utf8_string_if_exists(directory / L"state.json");
            const auto root = VerifyParseSucceeded(persisted);
            VERIFY_IS_TRUE(root["backslash.settingsProfilesCollapsed"].isBool());
            VERIFY_ARE_EQUAL(collapsed, root["backslash.settingsProfilesCollapsed"].asBool());
        }
        state = nullptr;
    }

    void SerializationTests::BackslashGroupSaveNoticeState()
    {
        GUID id;THROW_IF_FAILED(CoCreateGuid(&id));const auto directory=std::filesystem::temp_directory_path()/(L"Backslash-group-notice-tests-"+std::wstring{winrt::to_hstring(id)});std::filesystem::create_directory(directory);Log::Comment(directory.c_str());
        auto state=winrt::make_self<implementation::ApplicationState>(directory);VERIFY_IS_FALSE(state->SavedGroupSettingsNoticeDismissed());
        state->DismissedMessages(winrt::single_threaded_vector<InfoBarMessage>({InfoBarMessage::CloseOnExitInfo}));
        const auto seen = Utils::GuidFromString(L"{22222222-2222-4222-8222-222222222222}");state->GeneratedProfiles({seen});
        // Cancel, unchecked Continue and failed save keep the default preference.
        namespace P=::Microsoft::Terminal::Backslash::GroupSavePolicy;
        VERIFY_IS_FALSE(P::PersistNoticeChoice(false,true,true));VERIFY_IS_FALSE(P::PersistNoticeChoice(true,false,true));VERIFY_IS_FALSE(P::PersistNoticeChoice(true,true,false));
        state->SavedGroupSettingsNoticeDismissed(true);state->Flush();state=nullptr;
        const auto loaded=winrt::make_self<implementation::ApplicationState>(directory);VERIFY_IS_TRUE(loaded->SavedGroupSettingsNoticeDismissed());
        VERIFY_ARE_EQUAL(uint32_t{1},loaded->DismissedMessages().Size());VERIFY_ARE_EQUAL(InfoBarMessage::CloseOnExitInfo,loaded->DismissedMessages().GetAt(0));VERIFY_IS_TRUE(loaded->GeneratedProfiles().contains(seen));
        const auto selection=P::Select({{P::PaneKind::Terminal},{P::PaneKind::Settings}},1);VERIFY_IS_FALSE(P::ShowSettingsNotice(selection,loaded->SavedGroupSettingsNoticeDismissed(),false));
        VERIFY_THROWS(P::Select({{P::PaneKind::Settings}},0),std::invalid_argument); // unsuppressible empty-group error
    }

    void SerializationTests::BackslashSavedGroupsRoundtrip()
    {
        using Store = ::Microsoft::Terminal::Backslash::SavedTabGroups;
        const auto value = VerifyParseSucceeded(R"({"version":1,"groups":[{"id":"{11111111-1111-4111-8111-111111111111}","name":"Work","folderMode":"captured","focusedTab":0,"tabs":[[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":"","folder":"C:\\work"},{"kind":"split","profile":"{33333333-3333-4333-8333-333333333333}","source":"Windows.Terminal.Wsl","direction":"right","size":0.3},{"kind":"focus","id":0}],[{"kind":"newTab","profile":"{22222222-2222-4222-8222-222222222222}","source":""}]]}]})");
        const auto store = Store::ReadValue(value);
        VERIFY_ARE_EQUAL(toString(value), toString(store.ToValue()));
        auto globals = winrt::make_self<implementation::GlobalAppSettings>();
        globals->SavedTabGroupsJson(store.Json());
        const auto original = globals->SavedTabGroupsJson();
        auto clone = globals->Copy();
        auto edit = Store::Read(clone->SavedTabGroupsJson());
        const auto id = edit.groups[0].id;edit.groups[0].name = "Renamed";edit.groups[0].capturedFolders = false;
        clone->SavedTabGroupsJson(edit.Json());
        VERIFY_ARE_EQUAL(original, globals->SavedTabGroupsJson());
        VERIFY_ARE_EQUAL(id, Store::Read(clone->SavedTabGroupsJson()).groups[0].id);
        auto reloaded = implementation::GlobalAppSettings::FromJson(clone->ToJson(), OriginTag::User);
        VERIFY_ARE_EQUAL(clone->SavedTabGroupsJson(), reloaded->SavedTabGroupsJson());
        edit.groups.clear();clone->SavedTabGroupsJson(edit.Json());
        VERIFY_ARE_EQUAL(size_t{ 1 }, Store::Read(globals->SavedTabGroupsJson()).groups.size());
        edit = store;edit.groups[0].id = "{44444444-4444-4444-8444-444444444444}";edit.groups[0].name = "Work";
        clone->SavedTabGroupsJson(edit.Json());
        VERIFY_IS_TRUE(Store::Read(clone->SavedTabGroupsJson()).groups[0].id != id);
        auto future = value;future["version"] = 2;future["futureMetadata"] = "preserve";
        Json::Value root;root["backslash.savedTabGroups"] = future;
        const auto unknown = implementation::GlobalAppSettings::FromJson(root, OriginTag::User);
        VERIFY_ARE_EQUAL(toString(future), toString(unknown->Copy()->ToJson()["backslash.savedTabGroups"]));
        VERIFY_THROWS(Store::Read(unknown->SavedTabGroupsJson()), std::exception);
    }

    void SerializationTests::BackslashSavedGroupsRestore()
    {
        using Store = ::Microsoft::Terminal::Backslash::SavedTabGroups;
        NewTerminalArgs terminal;terminal.Profile(L"{22222222-2222-4222-8222-222222222222}");terminal.StartingDirectory(L"C:\\work");terminal.Commandline(L"arbitrary-in-session-command-and-secret");terminal.ContentId(42);
        ActionAndArgs first;first.Action(ShortcutAction::NewTab);first.Args(NewTabArgs{ terminal });
        ActionAndArgs split;split.Action(ShortcutAction::SplitPane);split.Args(SplitPaneArgs{ SplitType::Manual, SplitDirection::Down, 0.35f, terminal });
        ActionAndArgs focus;focus.Action(ShortcutAction::FocusPane);focus.Args(FocusPaneArgs{ uint32_t{ 0 } });
        ::Microsoft::Terminal::Backslash::SavedTabGroup group{ "{11111111-1111-4111-8111-111111111111}", "Work", true, 0, {} };
        group.tabs.push_back(Store::CaptureTab({ first, split, focus }, [](const winrt::guid&) { return winrt::hstring{ L"Windows.Terminal.PowershellCore" }; }));
        Store store;store.groups.push_back(group);const auto serialized = winrt::to_string(store.Json());
        VERIFY_IS_TRUE(serialized.find("arbitrary") == std::string::npos);VERIFY_IS_TRUE(serialized.find("contentId") == std::string::npos);VERIFY_IS_TRUE(serialized.find("commandline") == std::string::npos);
        const auto available = [](const winrt::guid&, const winrt::hstring& source) { return source == L"Windows.Terminal.PowershellCore"; };
        const auto restored = Store::Build(group, available, [](const winrt::hstring&) { return true; });
        VERIFY_ARE_EQUAL(size_t{ 1 }, restored.tabs.size());VERIFY_ARE_EQUAL(size_t{ 3 }, restored.tabs[0].size());VERIFY_ARE_EQUAL(uint32_t{ 0 }, restored.folderFallbacks);
        const auto args = restored.tabs[0][0].Args().as<NewTabArgs>().ContentArgs().as<NewTerminalArgs>();
        VERIFY_ARE_EQUAL(terminal.Profile(), args.Profile());VERIFY_ARE_EQUAL(winrt::hstring{ L"C:\\work" }, args.StartingDirectory());VERIFY_IS_TRUE(args.Commandline().empty());VERIFY_ARE_EQUAL(uint64_t{ 0 }, args.ContentId());
        const auto splitArgs = restored.tabs[0][1].Args().as<SplitPaneArgs>();VERIFY_ARE_EQUAL(SplitDirection::Down, splitArgs.SplitDirection());VERIFY_ARE_EQUAL(0.35f, splitArgs.SplitSize());
        VERIFY_ARE_EQUAL(uint32_t{ 2 }, Store::Build(group, available, [](const winrt::hstring&) { return false; }).folderFallbacks);
        group.capturedFolders = false;
        const auto defaults = Store::Build(group, available, [](const winrt::hstring&) { throw std::runtime_error("Defaults must not probe captured paths");return false; });
        VERIFY_ARE_EQUAL(uint32_t{ 0 }, defaults.folderFallbacks);VERIFY_IS_TRUE(defaults.tabs[0][0].Args().as<NewTabArgs>().ContentArgs().as<NewTerminalArgs>().StartingDirectory().empty());
        VERIFY_THROWS(Store::Build(group, [](const winrt::guid&, const winrt::hstring&) { return false; }, [](const winrt::hstring&) { return true; }), std::exception);
        auto bad = store.ToValue();bad["groups"][0]["tabs"][0][0]["commandline"] = "must-never-run";
        VERIFY_THROWS(Store::ReadValue(bad), std::exception);
        bad = store.ToValue();bad["groups"][0]["tabs"][0][1]["size"] = 1;
        VERIFY_THROWS(Store::ReadValue(bad), std::exception);
        bad = store.ToValue();bad["groups"].append(bad["groups"][0]);bad["groups"][1]["id"] = "{55555555-5555-4555-8555-555555555555}";
        VERIFY_THROWS(Store::ReadValue(bad), std::exception);
        VERIFY_IS_FALSE(Store::LocalFolder("/home/user"));VERIFY_IS_FALSE(Store::LocalFolder("\\\\server\\share"));VERIFY_IS_FALSE(Store::LocalFolder("C:relative"));VERIFY_IS_FALSE(Store::LocalFolder("C:\\%USERPROFILE%"));
        VERIFY_IS_TRUE(Store::LocalFolder("C:\\space and unicode-folder"));
    }

    void SerializationTests::BackslashScanlineWhite()
    {
        const auto parent = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        VERIFY_IS_FALSE(parent->RetroTerminalScanlineWhite());
        VERIFY_IS_FALSE(parent->HasRetroTerminalScanlineWhite());
        parent->RetroTerminalScanlineWhite(true);
        const auto child = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        child->AddLeastImportantParent(parent);
        VERIFY_IS_TRUE(child->RetroTerminalScanlineWhite());
        VERIFY_IS_FALSE(child->HasRetroTerminalScanlineWhite());
        VERIFY_IS_NOT_NULL(child->RetroTerminalScanlineWhiteOverrideSource());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineWhite":false})"));
        VERIFY_IS_FALSE(child->RetroTerminalScanlineWhite());
        VERIFY_IS_TRUE(child->HasRetroTerminalScanlineWhite());
        VERIFY_IS_FALSE(child->ToJson()["experimental.retroTerminalScanlineWhite"].asBool());
        const auto copy = implementation::AppearanceConfig::CopyAppearance(child.get(), {});
        VERIFY_ARE_EQUAL(toString(child->ToJson()), toString(copy->ToJson()));
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineWhite":null})"));
        VERIFY_IS_TRUE(child->RetroTerminalScanlineWhite());
        VERIFY_IS_FALSE(child->HasRetroTerminalScanlineWhite());
        VERIFY_IS_FALSE(child->ToJson().isMember("experimental.retroTerminalScanlineWhite"));
        VERIFY_THROWS(child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineWhite":"white"})")), std::exception);
    }
    void SerializationTests::BackslashPhosphorContrastGuard()
    {
        const auto parent = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        VERIFY_IS_TRUE(parent->RetroTerminalContrastGuard());
        VERIFY_IS_FALSE(parent->HasRetroTerminalContrastGuard());
        parent->RetroTerminalContrastGuard(true);
        const auto child = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        child->AddLeastImportantParent(parent);
        VERIFY_IS_TRUE(child->RetroTerminalContrastGuard());
        VERIFY_IS_FALSE(child->HasRetroTerminalContrastGuard());
        VERIFY_IS_NOT_NULL(child->RetroTerminalContrastGuardOverrideSource());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalContrastGuard":false})"));
        VERIFY_IS_FALSE(child->RetroTerminalContrastGuard());
        VERIFY_IS_TRUE(child->HasRetroTerminalContrastGuard());
        VERIFY_IS_FALSE(child->ToJson()["experimental.retroTerminalContrastGuard"].asBool());
        const auto copy = implementation::AppearanceConfig::CopyAppearance(child.get(), {});
        VERIFY_ARE_EQUAL(toString(child->ToJson()), toString(copy->ToJson()));
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalContrastGuard":null})"));
        VERIFY_IS_TRUE(child->RetroTerminalContrastGuard());
        VERIFY_IS_FALSE(child->HasRetroTerminalContrastGuard());
        VERIFY_IS_FALSE(child->ToJson().isMember("experimental.retroTerminalContrastGuard"));
        VERIFY_THROWS(child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalContrastGuard":"white"})")), std::exception);
    }


    void SerializationTests::BackslashPhosphorTint()
    {
        const auto parent = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        VERIFY_ARE_EQUAL(0.0f, parent->RetroTerminalTintIntensity());
        VERIFY_IS_FALSE(parent->HasRetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(0, 255, 65), til::color{ parent->RetroTerminalTintColor() });
        VERIFY_IS_FALSE(parent->HasRetroTerminalTintColor());
        parent->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintIntensity":0.6,"experimental.retroTerminalTintColor":"#FFB000"})"));
        const auto child = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        child->AddLeastImportantParent(parent);
        VERIFY_ARE_EQUAL(0.6f, child->RetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(255, 176, 0), til::color{ child->RetroTerminalTintColor() });
        VERIFY_IS_NOT_NULL(child->RetroTerminalTintColorOverrideSource());
        VERIFY_IS_FALSE(child->HasRetroTerminalTintColor());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintIntensity":0,"experimental.retroTerminalTintColor":"#234"})"));
        VERIFY_IS_TRUE(child->HasRetroTerminalTintIntensity());
        VERIFY_IS_TRUE(child->HasRetroTerminalTintColor());
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(34, 51, 68), til::color{ child->RetroTerminalTintColor() });
        const auto copy = implementation::AppearanceConfig::CopyAppearance(child.get(), {});
        VERIFY_ARE_EQUAL(toString(child->ToJson()), toString(copy->ToJson()));
        const auto reloaded = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        reloaded->LayerJson(copy->ToJson());
        VERIFY_ARE_EQUAL(toString(copy->ToJson()), toString(reloaded->ToJson()));
        child->RetroTerminalTintColor(winrt::Microsoft::Terminal::Core::Color{ 10, 20, 30, 40 });
        VERIFY_ARE_EQUAL(uint8_t{ 255 }, child->RetroTerminalTintColor().A);
        child->RetroTerminalEffect(true);
        child->RetroTerminalEffect(false);
        VERIFY_ARE_EQUAL(til::color(10, 20, 30), til::color{ child->RetroTerminalTintColor() });
        child->ClearRetroTerminalTintColor();
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintIntensity":null})"));
        VERIFY_ARE_EQUAL(0.6f, child->RetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(255, 176, 0), til::color{ child->RetroTerminalTintColor() });
        VERIFY_IS_FALSE(child->HasRetroTerminalTintIntensity());
        VERIFY_IS_FALSE(child->HasRetroTerminalTintColor());
        child->RetroTerminalTintIntensity(-10);
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalTintIntensity());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintIntensity":20})"));
        VERIFY_ARE_EQUAL(1.0f, child->RetroTerminalTintIntensity());
        VERIFY_THROWS(child->RetroTerminalTintIntensity(std::numeric_limits<float>::quiet_NaN()), winrt::hresult_invalid_argument);
        VERIFY_THROWS(child->RetroTerminalTintIntensity(std::numeric_limits<float>::infinity()), winrt::hresult_invalid_argument);
        VERIFY_THROWS(child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintIntensity":"bad"})")), std::exception);
        VERIFY_THROWS(child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalTintColor":"bad"})")), std::exception);
        VERIFY_ARE_EQUAL(1.0f, child->RetroTerminalTintIntensity());
        VERIFY_ARE_EQUAL(til::color(255, 176, 0), til::color{ child->RetroTerminalTintColor() });
        std::set<std::string> changes;
        parent->LogSettingChanges(changes, "profile");
        VERIFY_IS_TRUE(changes.empty());
    }

    void SerializationTests::BackslashScanlineDimensions()
    {
        const auto parent = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        VERIFY_ARE_EQUAL(4.0f, parent->RetroTerminalScanlineThickness());
        VERIFY_ARE_EQUAL(4.0f, parent->RetroTerminalScanlineGap());
        VERIFY_IS_FALSE(parent->HasRetroTerminalScanlineGap());
        parent->RetroTerminalScanlineThickness(2.5f);
        parent->RetroTerminalScanlineGap(3.0f);
        const auto child = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        child->AddLeastImportantParent(parent);
        VERIFY_ARE_EQUAL(2.5f, child->RetroTerminalScanlineThickness());
        VERIFY_IS_FALSE(child->HasRetroTerminalScanlineThickness());
        VERIFY_IS_NOT_NULL(child->RetroTerminalScanlineThicknessOverrideSource());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineThickness":0.75,"experimental.retroTerminalScanlineGap":0})"));
        VERIFY_IS_TRUE(child->HasRetroTerminalScanlineGap());
        VERIFY_ARE_EQUAL(0.0f, child->ToJson()["experimental.retroTerminalScanlineGap"].asFloat());
        const auto copy = implementation::AppearanceConfig::CopyAppearance(child.get(), {});
        VERIFY_ARE_EQUAL(toString(child->ToJson()), toString(copy->ToJson()));
        const auto reloaded = winrt::make_self<implementation::AppearanceConfig>(winrt::weak_ref<winrt::Microsoft::Terminal::Settings::Model::Profile>{});
        reloaded->LayerJson(copy->ToJson());
        VERIFY_ARE_EQUAL(toString(copy->ToJson()), toString(reloaded->ToJson()));
        child->RetroTerminalEffect(true);
        child->RetroTerminalEffect(false);
        VERIFY_ARE_EQUAL(0.75f, child->RetroTerminalScanlineThickness());
        child->ClearRetroTerminalScanlineThickness();
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineGap":null})"));
        VERIFY_ARE_EQUAL(2.5f, child->RetroTerminalScanlineThickness());
        VERIFY_ARE_EQUAL(3.0f, child->RetroTerminalScanlineGap());
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineThickness":-20,"experimental.retroTerminalScanlineGap":50})"));
        VERIFY_ARE_EQUAL(0.5f, child->RetroTerminalScanlineThickness());
        VERIFY_ARE_EQUAL(16.0f, child->RetroTerminalScanlineGap());
        child->RetroTerminalScanlineThickness(20);
        child->RetroTerminalScanlineGap(-1);
        VERIFY_ARE_EQUAL(8.0f, child->RetroTerminalScanlineThickness());
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalScanlineGap());
        VERIFY_THROWS(child->RetroTerminalScanlineThickness(std::numeric_limits<float>::quiet_NaN()), winrt::hresult_invalid_argument);
        VERIFY_THROWS(child->RetroTerminalScanlineGap(std::numeric_limits<float>::infinity()), winrt::hresult_invalid_argument);
        VERIFY_THROWS(child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineThickness":"bad"})")), std::exception);
        VERIFY_ARE_EQUAL(8.0f, child->RetroTerminalScanlineThickness());
        VERIFY_ARE_EQUAL(0.0f, child->RetroTerminalScanlineGap());
        std::set<std::string> changes;
        parent->LayerJson(VerifyParseSucceeded(R"({"experimental.retroTerminalScanlineThickness":4,"experimental.retroTerminalScanlineGap":5})"));
        parent->LogSettingChanges(changes, "profile");
        VERIFY_IS_TRUE(changes.empty());
    }

    void SerializationTests::GlobalSettings()
    {
        static constexpr std::string_view globalsString{ R"(
            {
                "defaultProfile": "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}",

                "initialRows": 30,
                "initialCols": 120,
                "initialPosition": ",",
                "launchMode": "default",
                "alwaysOnTop": false,
                "copyOnSelect": false,
                "copyFormatting": "all",
                "wordDelimiters": " /\\()\"'-.,:;<>~!@#$%^&*|+=[]{}~?\u2502",

                "alwaysShowTabs": true,
                "showTabsInTitlebar": true,
                "showTerminalTitleInTitlebar": true,
                "tabWidthMode": "equal",
                "tabSwitcherMode": "mru",

                "theme": "system",
                "snapToGridOnResize": true,
                "disableAnimations": false,

                "trimPaste": true,

                "warning.confirmCloseAllTabs" : true,
                "warning.largePaste" : true,
                "warning.multiLinePaste" : "automatic",

                "actions": [],
                "keybindings": []
            })" };

        static constexpr std::string_view smallGlobalsString{ R"(
            {
                "defaultProfile": "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}",
                "actions": [],
                "keybindings": []
            })" };

        RoundtripTest<implementation::GlobalAppSettings>(globalsString);
        RoundtripTest<implementation::GlobalAppSettings>(smallGlobalsString);
    }

    void SerializationTests::Profile()
    {
        static constexpr std::string_view profileString{ R"(
            {
                "name": "Windows PowerShell",
                "guid": "{61c54bbd-c2c6-5271-96e7-009a87ff44bf}",

                "commandline": "%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",
                "startingDirectory": "%USERPROFILE%",

                "icon": "ms-appx:///ProfileIcons/{61c54bbd-c2c6-5271-96e7-009a87ff44bf}.png",
                "hidden": false,

                "tabTitle": "Cool Tab",
                "suppressApplicationTitle": false,

                "font": {
                    "face": "Cascadia Mono",
                    "size": 12,
                    "weight": "normal"
                },
                "padding": "8, 8, 8, 8",
                "antialiasingMode": "grayscale",

                "cursorShape": "bar",
                "cursorColor": "#CCBBAA",
                "cursorHeight": 10,

                "altGrAliasing": true,

                "colorScheme": "Campbell",
                "tabColor": "#0C0C0C",
                "foreground": "#AABBCC",
                "background": "#BBCCAA",
                "selectionBackground": "#CCAABB",

                "useAcrylic": false,
                "opacity": 50,

                "backgroundImage": "made_you_look.jpeg",
                "backgroundImageStretchMode": "uniformToFill",
                "backgroundImageAlignment": "center",
                "backgroundImageOpacity": 1,

                "scrollbarState": "visible",
                "snapOnInput": true,
                "historySize": 9001,

                "closeOnExit": "graceful",
                "experimental.retroTerminalEffect": false,
                "environment":
                {
                    "KEY_1": "VALUE_1",
                    "KEY_2": "%KEY_1%",
                    "KEY_3": "%PATH%"
                }
            })" };

        static constexpr std::string_view smallProfileString{ R"(
            {
                "name": "Custom Profile"
            })" };

        // Setting "tabColor" to null tests two things:
        // - null should count as an explicit user-set value, not falling back to the parent's value
        // - null should be acceptable even though we're working with colors
        static constexpr std::string_view weirdProfileString{ R"(
            {
                "guid" : "{8b039d4d-77ca-5a83-88e1-dfc8e895a127}",
                "name": "Weird Profile",
                "hidden": false,
                "tabColor": null,
                "foreground": null,
                "source": "local"
            })" };

        static constexpr std::string_view profileWithIcon{ R"(
            {
                "guid" : "{8b039d4d-77ca-5a83-88e1-dfc8e895a127}",
                "name": "profileWithIcon",
                "hidden": false,
                "icon": "foo.png"
            })" };
        static constexpr std::string_view profileWithNullIcon{ R"(
            {
                "guid" : "{8b039d4d-77ca-5a83-88e1-dfc8e895a127}",
                "name": "profileWithNullIcon",
                "hidden": false,
                "icon": null
            })" };
        static constexpr std::string_view profileWithNoIcon{ R"(
            {
                "guid" : "{8b039d4d-77ca-5a83-88e1-dfc8e895a127}",
                "name": "profileWithNoIcon",
                "hidden": false,
                "icon": "none"
            })" };

        RoundtripTest<implementation::Profile>(profileString);
        RoundtripTest<implementation::Profile>(smallProfileString);
        RoundtripTest<implementation::Profile>(weirdProfileString);
        RoundtripTest<implementation::Profile>(profileWithIcon);
        RoundtripTest<implementation::Profile>(profileWithNullIcon);
        RoundtripTest<implementation::Profile>(profileWithNoIcon);
    }

    void SerializationTests::ColorScheme()
    {
        static constexpr std::string_view schemeString{ R"({
                                            "name": "Campbell",

                                            "cursorColor": "#FFFFFF",
                                            "selectionBackground": "#131313",

                                            "background": "#0C0C0C",
                                            "foreground": "#F2F2F2",

                                            "black": "#0C0C0C",
                                            "blue": "#0037DA",
                                            "cyan": "#3A96DD",
                                            "green": "#13A10E",
                                            "purple": "#881798",
                                            "red": "#C50F1F",
                                            "white": "#CCCCCC",
                                            "yellow": "#C19C00",
                                            "brightBlack": "#767676",
                                            "brightBlue": "#3B78FF",
                                            "brightCyan": "#61D6D6",
                                            "brightGreen": "#16C60C",
                                            "brightPurple": "#B4009E",
                                            "brightRed": "#E74856",
                                            "brightWhite": "#F2F2F2",
                                            "brightYellow": "#F9F1A5"
                                        })" };

        RoundtripTest<implementation::ColorScheme>(schemeString);
    }

    void SerializationTests::Actions()
    {
        // simple command
        static constexpr std::string_view actionsString1{ R"([
                                                { "command": "paste", "id": "Test.Paste" }
                                            ])" };

        // complex command
        static constexpr std::string_view actionsString2A{ R"([
                                                { "command": { "action": "setTabColor" }, "id": "Test.SetTabColor" }
                                            ])" };
        static constexpr std::string_view actionsString2B{ R"([
                                                { "command": { "action": "setTabColor", "color": "#112233" }, "id": "Test.SetTabColor112233" }
                                            ])" };
        static constexpr std::string_view actionsString2C{ R"([
                                                { "command": { "action": "copy" }, "id": "Test.Copy" },
                                                { "command": { "action": "copy", "singleLine": true, "copyFormatting": "html" }, "id": "Test.CopyWithArgs" }
                                            ])" };

        // simple command with key chords
        static constexpr std::string_view actionsString3{ R"({ "actions": [
                                                { "command": "toggleAlwaysOnTop", "id": "Test.ToggleAlwaysOnTop" } ],
                                            "keybindings": [
                                                { "keys": "ctrl+a", "id": "Test.ToggleAlwaysOnTop" },
                                                { "keys": "ctrl+b", "id": "Test.ToggleAlwaysOnTop" } ]})" };

        // complex command with key chords
        static constexpr std::string_view actionsString4A{ R"({ "actions":[
                                                { "command": { "action": "adjustFontSize", "delta": 1 }, "id": "Test.EnlargeFont" } ],
                                            "keybindings": [
                                                { "keys": "ctrl+c", "id": "Test.EnlargeFont" },
                                                { "keys": "ctrl+d", "id": "Test.EnlargeFont" } ]})" };

        // command with name and icon and multiple key chords
        static constexpr std::string_view actionsString5{ R"({ "actions":[
                                                { "icon": "image.png", "name": "Scroll To Top Name", "command": "scrollToTop", "id": "Test.ScrollToTop" } ],
                                            "keybindings": [
                                                { "id": "Test.ScrollToTop", "keys": "ctrl+f" },
                                                { "id": "Test.ScrollToTop", "keys": "ctrl+e" } ]})" };

        // complex command with new terminal args
        static constexpr std::string_view actionsString6{ R"([
                                                { "command": { "action": "newTab", "index": 0 }, "id": "Test.NewTerminal" },
                                            ])" };

        // complex command with meaningful null arg
        static constexpr std::string_view actionsString7{ R"([
                                                { "command": { "action": "renameWindow", "name": null }, "id": "Test.MeaningfulNull" }
                                            ])" };

        // nested command
        static constexpr std::string_view actionsString8{ R"([
                                                {
                                                    "name": "Change font size...",
                                                    "commands": [
                                                        { "command": { "action": "adjustFontSize", "delta": 1 } },
                                                        { "command": { "action": "adjustFontSize", "delta": -1 } },
                                                        { "command": "resetFontSize" },
                                                    ]
                                                }
                                            ])" };

        // iterable command
        static constexpr std::string_view actionsString9A{ R"([
                                                {
                                                    "name": "New tab",
                                                    "commands": [
                                                        {
                                                            "iterateOn": "profiles",
                                                            "icon": "${profile.icon}",
                                                            "name": "${profile.name}",
                                                            "command": { "action": "newTab", "profile": "${profile.name}" }
                                                        }
                                                    ]
                                                }
                                            ])" };
        static constexpr std::string_view actionsString9B{ R"([
                                                {
                                                    "commands":
                                                    [
                                                        {
                                                            "command":
                                                            {
                                                                "action": "sendInput",
                                                                "input": "${profile.name}"
                                                            },
                                                            "iterateOn": "profiles"
                                                        }
                                                    ],
                                                    "name": "Send Input ..."
                                                }
                                        ])" };
        static constexpr std::string_view actionsString9C{ R""([
                                                {
                                                    "commands":
                                                    [
                                                        {
                                                            "commands":
                                                            [
                                                                {
                                                                    "command":
                                                                    {
                                                                        "action": "sendInput",
                                                                        "input": "${profile.name} ${scheme.name}"
                                                                    },
                                                                    "iterateOn": "schemes"
                                                                }
                                                            ],
                                                            "iterateOn": "profiles",
                                                            "name": "nest level (${profile.name})"
                                                        }
                                                    ],
                                                    "name": "Send Input (Evil) ..."
                                                }
                                            ])"" };
        static constexpr std::string_view actionsString9D{ R""([
                                                {
                                                    "command":
                                                    {
                                                        "action": "newTab",
                                                        "profile": "${profile.name}"
                                                    },
                                                    "icon": "${profile.icon}",
                                                    "iterateOn": "profiles",
                                                    "name": "${profile.name}: New tab"
                                                }
                                            ])"" };

        // unbound command
        static constexpr std::string_view actionsString10{ R"({ "actions": [],
                                            "keybindings": [
                                                { "id": null, "keys": "ctrl+c" } ]})" };

        Log::Comment(L"simple command");
        RoundtripTest<implementation::ActionMap>(actionsString1);

        Log::Comment(L"complex commands");
        RoundtripTest<implementation::ActionMap>(actionsString2A);
        RoundtripTest<implementation::ActionMap>(actionsString2B);
        RoundtripTest<implementation::ActionMap>(actionsString2C);

        // ActionMap has effectively 2 "to json" calls we need to make, one for the actions and one for the keybindings
        // So we cannot use RoundtripTest<ActionMap> for actions + keychords, just use RoundTripTest<GlobalAppSettings>
        Log::Comment(L"simple command with key chords");
        RoundtripTest<implementation::GlobalAppSettings>(actionsString3);

        Log::Comment(L"complex commands with key chords");
        RoundtripTest<implementation::GlobalAppSettings>(actionsString4A);

        Log::Comment(L"command with name and icon and multiple key chords");
        RoundtripTest<implementation::GlobalAppSettings>(actionsString5);

        Log::Comment(L"complex command with new terminal args");
        RoundtripTest<implementation::ActionMap>(actionsString6);

        Log::Comment(L"complex command with meaningful null arg");
        RoundtripTest<implementation::ActionMap>(actionsString7);

        Log::Comment(L"nested command");
        RoundtripTest<implementation::ActionMap>(actionsString8);

        Log::Comment(L"iterable command");
        RoundtripTest<implementation::ActionMap>(actionsString9A);
        RoundtripTest<implementation::ActionMap>(actionsString9B);
        RoundtripTest<implementation::ActionMap>(actionsString9C);
        RoundtripTest<implementation::ActionMap>(actionsString9D);

        Log::Comment(L"unbound command");
        RoundtripTest<implementation::GlobalAppSettings>(actionsString10);
    }

    void SerializationTests::CascadiaSettings()
    {
        static constexpr std::string_view settingsString{ R"({
            "$help" : "https://aka.ms/terminal-documentation",
            "$schema" : "https://aka.ms/terminal-profiles-schema",
            "defaultProfile": "{61c54bbd-1111-5271-96e7-009a87ff44bf}",
            "disabledProfileSources": [ "Windows.Terminal.Wsl" ],
            "safeUriSchemes": [ "vscode" ],
            "newTabMenu":
            [
                {
                    "type": "remainingProfiles"
                }
            ],
            "profiles": {
                "defaults": {
                    "font": {
                        "face": "Zamora Code"
                    }
                },
                "list": [
                    {
                        "font": { "face": "Cascadia Code" },
                        "guid": "{61c54bbd-1111-5271-96e7-009a87ff44bf}",
                        "name": "HowettShell"
                    },
                    {
                        "hidden": true,
                        "guid": "{c08b0496-e71c-5503-b84e-3af7a7a6d2a7}",
                        "name": "BhojwaniShell"
                    },
                    {
                        "antialiasingMode": "aliased",
                        "guid": "{fe9df758-ac22-5c20-922d-c7766cdd13af}",
                        "name": "NiksaShell"
                    }
                ]
            },
            "schemes": [
                {
                    "name": "Cinnamon Roll",

                    "cursorColor": "#FFFFFD",
                    "selectionBackground": "#FFFFFF",

                    "background": "#3C0315",
                    "foreground": "#FFFFFD",

                    "black": "#282A2E",
                    "blue": "#0170C5",
                    "cyan": "#3F8D83",
                    "green": "#76AB23",
                    "purple": "#7D498F",
                    "red": "#BD0940",
                    "white": "#FFFFFD",
                    "yellow": "#E0DE48",
                    "brightBlack": "#676E7A",
                    "brightBlue": "#5C98C5",
                    "brightCyan": "#8ABEB7",
                    "brightGreen": "#B5D680",
                    "brightPurple": "#AC79BB",
                    "brightRed": "#BD6D85",
                    "brightWhite": "#FFFFFD",
                    "brightYellow": "#FFFD76"
                }
            ],
            "actions": [
                { "command": { "action": "sendInput", "input": "VT Griese Mode" }, "id": "Test.SendInput" }
            ],
            "keybindings": [
                { "id": "Test.SendInput", "keys": "ctrl+k" }
            ],
            "theme": "system",
            "themes": []
        })" };

        const auto settings{ winrt::make_self<implementation::CascadiaSettings>(settingsString) };

        const auto result{ settings->ToJson() };
        VERIFY_ARE_EQUAL(toString(removeSchema(VerifyParseSucceeded(settingsString))),
                         toString(removeSchema(result)));
    }

    void SerializationTests::LegacyFontSettings()
    {
        static constexpr std::string_view profileString{ R"(
            {
                "name": "Profile with legacy font settings",

                "fontFace": "Cascadia Mono",
                "fontSize": 12,
                "fontWeight": "normal"
            })" };

        static constexpr std::string_view expectedOutput{ R"(
            {
                "name": "Profile with legacy font settings",

                "font": {
                    "face": "Cascadia Mono",
                    "size": 12,
                    "weight": "normal"
                }
            })" };

        const auto json{ VerifyParseSucceeded(profileString) };
        const auto settings{ implementation::Profile::FromJson(json) };
        const auto result{ settings->ToJson() };

        const auto jsonOutput{ VerifyParseSucceeded(expectedOutput) };

        VERIFY_ARE_EQUAL(toString(jsonOutput), toString(result));
    }

    void SerializationTests::RoundtripReloadEnvVars()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "compatibility.reloadEnvironmentVariables": false,
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                    "historySize": 1,
                    "commandline": "cmd.exe"
                }
            ],
            "actions": [
                {
                    "name": "foo",
                    "command": "closePane",
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        static constexpr std::string_view newSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles":
            {
                "defaults":
                {
                    "compatibility.reloadEnvironmentVariables": false
                },
                "list":
                [
                    {
                        "name": "profile0",
                        "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                        "historySize": 1,
                        "commandline": "cmd.exe"
                    }
                ]
            },
            "actions": [
                {
                    "name": "foo",
                    "command": "closePane",
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        VERIFY_IS_TRUE(oldLoader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto oldSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(oldLoader));
        const auto oldResult{ oldSettings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        newLoader.FixupUserSettings();
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::DontRoundtripNoReloadEnvVars()
    {
        // Kinda like the above test, but confirming that _nothing_ happens if
        // we don't have a setting to migrate.

        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                    "historySize": 1,
                    "commandline": "cmd.exe"
                }
            ],
            "actions": [
                {
                    "name": "foo",
                    "command": "closePane",
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        oldLoader.FixupUserSettings();
        const auto oldSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(oldLoader));
        const auto oldResult{ oldSettings->ToJson() };

        Log::Comment(L"Now, create a _new_ settings object from the re-serialization of the first");
        implementation::SettingsLoader newLoader{ toString(oldResult), implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        newLoader.FixupUserSettings();
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        VERIFY_IS_FALSE(newSettings->ProfileDefaults().HasReloadEnvironmentVariables(),
                        L"Ensure that the new settings object didn't find a reloadEnvironmentVariables");
    }

    void SerializationTests::RoundtripUserModifiedColorSchemeCollision()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                },
                {
                    "name": "profile1",
                    "colorScheme": "Tango Dark",
                    "guid": "{d0a65a9d-8665-4128-97a4-a581aa747aa7}"
                }
            ],
            "schemes": [
                {
                    "background": "#121314",
                    "black": "#121314",
                    "blue": "#121314",
                    "brightBlack": "#121314",
                    "brightBlue": "#121314",
                    "brightCyan": "#121314",
                    "brightGreen": "#121314",
                    "brightPurple": "#121314",
                    "brightRed": "#121314",
                    "brightWhite": "#121314",
                    "brightYellow": "#121314",
                    "cursorColor": "#121314",
                    "cyan": "#121314",
                    "foreground": "#121314",
                    "green": "#121314",
                    "name": "Campbell",
                    "purple": "#121314",
                    "red": "#121314",
                    "selectionBackground": "#121314",
                    "white": "#121314",
                    "yellow": "#121314"
                },
                {
                    "background": "#000000",
                    "black": "#000000",
                    "blue": "#3465A4",
                    "brightBlack": "#555753",
                    "brightBlue": "#729FCF",
                    "brightCyan": "#34E2E2",
                    "brightGreen": "#8AE234",
                    "brightPurple": "#AD7FA8",
                    "brightRed": "#EF2929",
                    "brightWhite": "#EEEEEC",
                    "brightYellow": "#FCE94F",
                    "cursorColor": "#FFFFFF",
                    "cyan": "#06989A",
                    "foreground": "#D3D7CF",
                    "green": "#4E9A06",
                    "name": "Tango Dark",
                    "purple": "#75507B",
                    "red": "#CC0000",
                    "selectionBackground": "#FFFFFF",
                    "white": "#D3D7CF",
                    "yellow": "#C4A000"
                },
            ]
        })" };

        // Key differences: one fewer color scheme (Tango Dark has been deleted) and defaults.colorScheme is set.
        static constexpr std::string_view newSettingsJson{ R"-(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles":
            {
                "defaults": {
                    "colorScheme": "Campbell (modified)"
                },
                "list":
                [
                    {
                        "name": "profile0",
                        "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                    },
                    {
                        "name": "profile1",
                        "colorScheme": "Tango Dark",
                        "guid": "{d0a65a9d-8665-4128-97a4-a581aa747aa7}"
                    }
                ]
            },
            "actions": [ ],
            "schemes": [
                {
                    "background": "#121314",
                    "black": "#121314",
                    "blue": "#121314",
                    "brightBlack": "#121314",
                    "brightBlue": "#121314",
                    "brightCyan": "#121314",
                    "brightGreen": "#121314",
                    "brightPurple": "#121314",
                    "brightRed": "#121314",
                    "brightWhite": "#121314",
                    "brightYellow": "#121314",
                    "cursorColor": "#121314",
                    "cyan": "#121314",
                    "foreground": "#121314",
                    "green": "#121314",
                    "name": "Campbell",
                    "purple": "#121314",
                    "red": "#121314",
                    "selectionBackground": "#121314",
                    "white": "#121314",
                    "yellow": "#121314"
                }
            ]
        })-" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        VERIFY_IS_TRUE(oldLoader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto oldSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(oldLoader));
        const auto oldResult{ oldSettings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        newLoader.FixupUserSettings();
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::RoundtripUserModifiedColorSchemeCollisionUnusedByProfiles()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                }
            ],
            "schemes": [
                {
                    "background": "#111111",
                    "black": "#111111",
                    "blue": "#111111",
                    "brightBlack": "#111111",
                    "brightBlue": "#111111",
                    "brightCyan": "#111111",
                    "brightGreen": "#111111",
                    "brightPurple": "#111111",
                    "brightRed": "#111111",
                    "brightWhite": "#111111",
                    "brightYellow": "#111111",
                    "cursorColor": "#111111",
                    "cyan": "#111111",
                    "foreground": "#111111",
                    "green": "#111111",
                    "name": "Tango Dark",
                    "purple": "#111111",
                    "red": "#111111",
                    "selectionBackground": "#111111",
                    "white": "#111111",
                    "yellow": "#111111"
                },
            ]
        })" };

        // Key differences: Tango Dark has been renamed; nothing else has changed
        static constexpr std::string_view newSettingsJson{ R"-(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles":
            {
                "list":
                [
                    {
                        "name": "profile0",
                        "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                    }
                ]
            },
            "actions": [ ],
            "schemes": [
                {
                    "background": "#111111",
                    "black": "#111111",
                    "blue": "#111111",
                    "brightBlack": "#111111",
                    "brightBlue": "#111111",
                    "brightCyan": "#111111",
                    "brightGreen": "#111111",
                    "brightPurple": "#111111",
                    "brightRed": "#111111",
                    "brightWhite": "#111111",
                    "brightYellow": "#111111",
                    "cursorColor": "#111111",
                    "cyan": "#111111",
                    "foreground": "#111111",
                    "green": "#111111",
                    "name": "Tango Dark (modified)",
                    "purple": "#111111",
                    "red": "#111111",
                    "selectionBackground": "#111111",
                    "white": "#111111",
                    "yellow": "#111111"
                },
            ]
        })-" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        VERIFY_IS_TRUE(oldLoader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto oldSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(oldLoader));
        const auto oldResult{ oldSettings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        newLoader.FixupUserSettings();
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::RoundtripUserDeletedColorSchemeCollision()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                }
            ],
            "schemes": [
                {
                    "name": "Tango Dark",
                    "foreground": "#D3D7CF",
                    "background": "#000000",
                    "cursorColor": "#FFFFFF",
                    "black": "#000000",
                    "red": "#CC0000",
                    "green": "#4E9A06",
                    "yellow": "#C4A000",
                    "blue": "#3465A4",
                    "purple": "#75507B",
                    "cyan": "#06989A",
                    "white": "#D3D7CF",
                    "brightBlack": "#555753",
                    "brightRed": "#EF2929",
                    "brightGreen": "#8AE234",
                    "brightYellow": "#FCE94F",
                    "brightBlue": "#729FCF",
                    "brightPurple": "#AD7FA8",
                    "brightCyan": "#34E2E2",
                    "brightWhite": "#EEEEEC"
                }
            ]
        })" };

        // Key differences: Tango Dark has been deleted, as it was identical to the inbox one.
        static constexpr std::string_view newSettingsJson{ R"-(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles":
            {
                "list":
                [
                    {
                        "name": "profile0",
                        "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}"
                    }
                ]
            },
            "actions": [ ],
            "schemes": [ ]
        })-" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        VERIFY_IS_TRUE(oldLoader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto oldSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(oldLoader));
        const auto oldResult{ oldSettings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        newLoader.FixupUserSettings();
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::RoundtripGenerateActionID()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        implementation::SettingsLoader loader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        loader.FixupUserSettings();
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto oldResult{ settings->ToJson() };
        const auto sendInputCmd = settings->ActionMap().GetActionByKeyChord(KeyChord{ true, false, true, false, 87, 0 });

        std::string_view expectedID{ R"(User.sendInput.)" SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH };

        VERIFY_ARE_EQUAL(sendInputCmd.ID(), winrt::to_hstring(expectedID));
    }

    void SerializationTests::NoGeneratedIDsForIterableAndNestedCommands()
    {
        // for iterable commands, nested commands, and user-defined actions that already have
        // an ID, we do not need to generate an ID
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "command": "closePane",
                    "id": "thisIsMyClosePane"
                },
                {
                    "iterateOn": "profiles",
                    "icon": "${profile.icon}",
                    "name": "${profile.name}",
                    "command": { "action": "newTab", "profile": "${profile.name}" }
                },
                {
                    "name": "Change font size...",
                    "commands": [
                        { "command": { "action": "adjustFontSize", "delta": 1 } },
                        { "command": { "action": "adjustFontSize", "delta": -1 } },
                        { "command": "resetFontSize" },
                    ]
                }
            ]
        })" };

        implementation::SettingsLoader oldLoader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        oldLoader.MergeInboxIntoUserSettings();
        oldLoader.FinalizeLayering();
        VERIFY_IS_FALSE(oldLoader.FixupUserSettings(), L"Validate that there is no need to write back to disk");
    }

    void SerializationTests::GeneratedActionIDsEqualForIdenticalCommands()
    {
        static constexpr std::string_view settingsJson1{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "command": { "action": "sendInput", "input": "this is some other input string" },
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        // Both settings files define the same action, so the generated ID should be the same for both
        static constexpr std::string_view settingsJson2{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "command": { "action": "sendInput", "input": "this is some other input string" },
                    "keys": "ctrl+shift+w"
                }
            ]
        })" };

        implementation::SettingsLoader loader1{ settingsJson1, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader1.MergeInboxIntoUserSettings();
        loader1.FinalizeLayering();
        loader1.FixupUserSettings();
        const auto settings1 = winrt::make_self<implementation::CascadiaSettings>(std::move(loader1));
        const auto result1{ settings1->ToJson() };

        implementation::SettingsLoader loader2{ settingsJson2, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader2.MergeInboxIntoUserSettings();
        loader2.FinalizeLayering();
        loader2.FixupUserSettings();
        const auto settings2 = winrt::make_self<implementation::CascadiaSettings>(std::move(loader2));
        const auto result2{ settings2->ToJson() };

        const auto sendInputCmd1 = settings1->ActionMap().GetActionByKeyChord(KeyChord{ true, false, true, false, 87, 0 });
        const auto sendInputCmd2 = settings2->ActionMap().GetActionByKeyChord(KeyChord{ true, false, true, false, 87, 0 });

        VERIFY_ARE_EQUAL(sendInputCmd1.ID(), sendInputCmd1.ID());
    }

    void SerializationTests::RoundtripLegacyToModernActions()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "id": "Test.SendInput",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "keys": "ctrl+shift+w"
                },
                {
                    "command": "unbound",
                    "keys": "ctrl+shift+x"
                }
            ]
        })" };

        // modern style:
        // - no "unbound" actions, these are just keybindings that have no id
        // - no keys in actions, these are keybindings with an id
        static constexpr std::string_view newSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "id": "Test.SendInput"
                }
            ],
            "keybindings": [
                {
                    "id": "Test.SendInput",
                    "keys": "ctrl+shift+w"
                },
                {
                    "id": null,
                    "keys": "ctrl+shift+x"
                }
            ]
        })" };

        implementation::SettingsLoader loader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        VERIFY_IS_TRUE(loader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto oldResult{ settings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        VERIFY_IS_FALSE(newLoader.FixupUserSettings(), L"Validate that there is no need to write back to disk");
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::RoundtripUserActionsSameAsInBoxAreRemoved()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "command": "paste",
                    "keys": "ctrl+shift+x"
                }
            ]
        })" };

        // this action is the same as in inbox one,
        // so we will delete this action from the user's file but retain the keybinding
        static constexpr std::string_view newSettingsJson{ R"(
        {
            "actions": [
            ],
            "keybindings": [
                {
                    "id": "Terminal.PasteFromClipboard",
                    "keys": "ctrl+shift+x"
                }
            ]
        })" };

        implementation::SettingsLoader loader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        VERIFY_IS_TRUE(loader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto oldResult{ settings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        VERIFY_IS_FALSE(newLoader.FixupUserSettings(), L"Validate that there is no need to write back to disk");
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::RoundtripActionsSameNameDifferentCommandsAreRetained()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "command": { "action": "sendInput", "input": "just some input" },
                    "name": "mySendInput"
                },
                {
                    "command": { "action": "sendInput", "input": "just some input 2" },
                    "name": "mySendInput"
                }
            ]
        })" };

        // There are two different actions with the same name,
        // ensure that both are kept but have different IDs generated for them
        static constexpr std::string_view newSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "mySendInput",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "id": "User.sendInput.)" SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH R"("
                },
                {
                    "name": "mySendInput",
                    "command": { "action": "sendInput", "input": "just some input 2" },
                    "id": "User.sendInput.)" SEND_INPUT2_ARCH_SPECIFIC_ACTION_HASH R"("
                }
            ]
        })" };

        implementation::SettingsLoader loader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        VERIFY_IS_TRUE(loader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto oldResult{ settings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        VERIFY_IS_FALSE(newLoader.FixupUserSettings(), L"Validate that there is no need to write back to disk");
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::MultipleActionsAreCollapsed()
    {
        static constexpr std::string_view oldSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "icon": "myCoolIconPath.png",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "keys": "ctrl+shift+w"
                },
                {
                    "command": { "action": "sendInput", "input": "just some input" },
                    "keys": "ctrl+shift+x"
                }
            ]
        })" };

        // modern style:
        // - multiple action blocks whose purpose is simply to define more keybindings for the same action
        //   get collapsed into one action block, with the name and icon path preserved and have multiple keybindings instead
        static constexpr std::string_view newSettingsJson{ R"(
        {
            "actions": [
                {
                    "name": "foo",
                    "icon": "myCoolIconPath.png",
                    "command": { "action": "sendInput", "input": "just some input" },
                    "id": "User.sendInput.)" SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH R"("
                }
            ],
            "keybindings": [
                {
                    "keys": "ctrl+shift+w",
                    "id": "User.sendInput.)" SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH R"("
                },
                {
                    "keys": "ctrl+shift+x",
                    "id": "User.sendInput.)" SEND_INPUT_ARCH_SPECIFIC_ACTION_HASH R"("
                }
            ]
        })" };

        implementation::SettingsLoader loader{ oldSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        VERIFY_IS_TRUE(loader.FixupUserSettings(), L"Validate that this will indicate we need to write them back to disk");
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto oldResult{ settings->ToJson() };

        implementation::SettingsLoader newLoader{ newSettingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        newLoader.MergeInboxIntoUserSettings();
        newLoader.FinalizeLayering();
        VERIFY_IS_FALSE(newLoader.FixupUserSettings(), L"Validate that there is no need to write back to disk");
        const auto newSettings = winrt::make_self<implementation::CascadiaSettings>(std::move(newLoader));
        const auto newResult{ newSettings->ToJson() };

        VERIFY_ARE_EQUAL(toString(newResult), toString(oldResult));
    }

    void SerializationTests::ProfileWithInvalidIcon()
    {
        static constexpr std::string_view settingsJson{ R"(
        {
            "defaultProfile": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
            "profiles": [
                {
                    "name": "profile0",
                    "guid": "{6239a42c-0000-49a3-80bd-e8fdd045185c}",
                    "historySize": 1,
                    "commandline": "cmd.exe",
                    "icon": "c:\\this_icon_had_better_not_exist.tiff"
                }
            ]
        })" };

        implementation::SettingsLoader loader{ settingsJson, implementation::LoadStringResource(IDR_DEFAULTS) };
        loader.MergeInboxIntoUserSettings();
        loader.FinalizeLayering();
        const auto settings = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        const auto newResult{ settings->ToJson() };

        // A profile that specifies an invalid icon will fall back to the commandline on load, but that should
        // not be reflected back in settings.json as null *or* as the commandline. The value should be exactly
        // what was written in the settings file.
        VERIFY_ARE_EQUAL(R"(c:\this_icon_had_better_not_exist.tiff)", newResult["profiles"]["list"][0]["icon"].asString());
    }
}
