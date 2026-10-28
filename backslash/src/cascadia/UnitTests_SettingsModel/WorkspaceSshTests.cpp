// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "JsonTestClass.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/ApplicationState.h"
#include "../TerminalSettingsModel/SshHostGenerator.h"
#include "../TerminalSettingsModel/DynamicProfileUtils.h"
#include "../TerminalSettingsModel/PinnedLaunch.h"
#include "../TerminalSettingsModel/SetupTransfer.h"
#include "../TerminalSettingsModel/FileUtils.h"
#include "../inc/SettingsClosePolicy.h"

using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace WEX::TestExecution;
using namespace WEX::Logging;
namespace W = ::Microsoft::Terminal::Backslash::WorkspaceSsh;
namespace S = winrt::Microsoft::Terminal::Settings::Model::implementation::SshCommandline;
namespace P = ::Microsoft::Terminal::Backslash::Pins;
namespace SettingsModelUnitTests
{
    class WorkspaceSshTests
    {
        BEGIN_TEST_CLASS(WorkspaceSshTests)
        END_TEST_CLASS()
        TEST_METHOD(ImportResolvesAddressAndDefaultsWithoutExecutingConfig);
        TEST_METHOD(ImportedWorkspaceVisibilityRoundtrip);
        TEST_METHOD(DiscoveredHostVisibilityRoundtrip);
        TEST_METHOD(DiscoveredHostShellAppearanceRoundtrip);
        TEST_METHOD(AdvancedTransientSaveAdoptsWithoutMutationOnFailure);
        TEST_METHOD(SelectedAliasImportSaveRestartAndDeduplicate);
        TEST_METHOD(ScanAndCancelDoNotImport);
        TEST_METHOD(AliasSchemaRejectsMalformedAndUnsupportedValues);
        TEST_METHOD(PreferencesPrefillWithoutAdoption);
        TEST_METHOD(InheritedDefaultsRemainInheritedAfterAdoption);
        TEST_METHOD(DependenciesAndMalformedInputBlockImport);
        TEST_METHOD(WorkspaceLaunchPreservesShellAndPinOptions);
        TEST_METHOD(ImportDraftDeduplicatesAndRoundtripsWithoutHostDiscovery);
        TEST_METHOD(SessionProfilesDoNotPersistOrChangeSeenState);
        TEST_METHOD(LegacyProfilesAndWorkspaceSurviveDiscoveryCycles);
        TEST_METHOD(StaleImportReviewDoesNotChangeDraft);
        TEST_METHOD(SetupTransferRoundtripsWorkspaceRecordsWithoutHostConfig);
        TEST_METHOD(FreshSessionUsesPortableDefault);
        TEST_METHOD(HashTokensAreNotLossilyImported);
        TEST_METHOD(ImportCapacityIsValidatedBeforeMutation);
        TEST_METHOD(OnlySingleOptionalEqualsSeparatorIsAccepted);
        TEST_METHOD(EditedLegacyAliasCollisionRejectsWithoutDroppingEdits);
        TEST_METHOD(FullCollectionSaveFailureStillAllowsDraftDiscard);
        TEST_METHOD(ReviewedEndpointImportRetainsExplicitLegacyEdits);
    };
    void WorkspaceSshTests::ImportResolvesAddressAndDefaultsWithoutExecutingConfig()
    {
        const auto review = W::Preview("box", "User alice\nHost box other\n HostName server.example\n Port 2222\n", "Host *\n User system\n Port 22\n", "fallback");
        VERIFY_IS_TRUE(review.record.has_value()); VERIFY_ARE_EQUAL(std::string{"alice"}, review.record->user);
        VERIFY_ARE_EQUAL(std::string{"server.example"}, review.record->host); VERIFY_ARE_EQUAL(2222u, review.record->port);
        VERIFY_IS_TRUE(W::Preview("BOX", "Host box\n", "", "fallback").record.has_value());
        VERIFY_IS_FALSE(W::Preview("missing", "", "", "fallback").record.has_value());
        VERIFY_ARE_EQUAL(std::string{"fallback"}, W::Preview("box", "Host box\n", "", "fallback").record->user);
    }
    void WorkspaceSshTests::DependenciesAndMalformedInputBlockImport()
    {
        for (const auto* option : { "IdentityFile C:/not-read/private", "CertificateFile key-cert.pub", "IdentityAgent agent", "UserKnownHostsFile host-trust", "ProxyJump proxy", "ProxyCommand never-executed", "RemoteCommand never-executed", "Include never-read", "Match exec never-executed", "Compression yes", "HostName %h", "Port -1", "Port 65536", "User \"two words\"", "NoValue" })
        {
            const auto review = W::Preview("box", std::string{"Host box\n"} + option + "\n", "", "alice");
            VERIFY_IS_FALSE(review.record.has_value()); VERIFY_IS_FALSE(review.issues.empty());
        }
        VERIFY_IS_FALSE(W::Preview("box", "Host !excluded *\n", "", "alice").record.has_value());
        VERIFY_IS_FALSE(W::Preview("box", "Host box\n", "Include never-read\n", "alice").record.has_value());
        VERIFY_IS_FALSE(W::Preview("box", std::string(1024 * 1024 + 1, 'x'), "", "alice").record.has_value());
        auto record = W::Value({"box", "host", "alice", 22}); record["exec"] = "never";
        VERIFY_THROWS(W::Read(record), std::invalid_argument);
        Json::Value duplicates{Json::arrayValue}; duplicates.append(W::Value({"box", "host", "alice", 22})); duplicates.append(W::Value({"BOX", "host", "alice", 22}));
        VERIFY_THROWS(W::ReadRecords(duplicates), std::invalid_argument);
    }
    void WorkspaceSshTests::WorkspaceLaunchPreservesShellAndPinOptions()
    {
        const auto base = W::BaseCommand({"box", "server.example", "alice", 2222}, L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe");
        std::wstring exe, host, custom; VERIFY_IS_TRUE(S::Base(base, exe, host)); VERIFY_ARE_EQUAL(std::wstring{L"box"}, host);
        for (int mode = 0; mode < 5; ++mode)
        {
            const auto command = S::Build(base, mode, L"pwsh -NoLogo"); VERIFY_IS_TRUE(command.has_value());
            const auto recognized = S::Recognize(base, *command, custom);
            VERIFY_ARE_EQUAL(mode == 4 ? 3 : mode, recognized); // Identical pwsh custom is canonical mode 3.
            const auto args = S::Argv(*command); VERIFY_IS_TRUE(std::find(args.begin(), args.end(), L"HostName=server.example") != args.end());
            VERIFY_IS_TRUE(std::find(args.begin(), args.end(), L"IdentityAgent=none") != args.end());
        }
        P::Destination pin{ "{01234567-1111-2222-3333-444444444444}", "{11234567-1111-2222-3333-444444444444}", "Backslash.Terminal.SSH", P::Kind::SshPosix, "fixture", "box", "/srv/folder with quote'" };
        const auto launch = P::PrepareLaunch(pin, *S::Build(base, 3), 3, base);
        VERIFY_IS_TRUE(launch.directory.empty()); VERIFY_IS_TRUE(launch.commandline.find(L"HostName=server.example") != launch.commandline.npos);
        VERIFY_IS_TRUE(launch.commandline.find(L"UserKnownHostsFile=") != launch.commandline.npos);
        VERIFY_THROWS(P::PrepareLaunch(pin, *S::Build(base, 3) + L" extra", 3, base), std::invalid_argument);
        auto localPin = pin; localPin.kind = P::Kind::Local; localPin.path = "C:\\fixture";
        VERIFY_THROWS(P::Validate(localPin), std::invalid_argument);
        Json::Value group{Json::objectValue}, step{Json::objectValue}, tab{Json::arrayValue}, root{Json::objectValue};
        group["id"] = pin.id; group["name"] = "remote fixture"; group["folderMode"] = "captured"; group["focusedTab"] = 0u;
        step["kind"] = "newTab"; step["profile"] = pin.profile; step["source"] = pin.source; step["folder"] = "C:\\fixture";
        tab.append(step); group["tabs"].append(tab); root["version"] = 1u; root["groups"].append(group);
        VERIFY_THROWS(::Microsoft::Terminal::Backslash::SavedTabGroups::ReadValue(root), std::invalid_argument);
        root["groups"][0]["tabs"][0][0].removeMember("folder");
        VERIFY_ARE_EQUAL(size_t{1}, ::Microsoft::Terminal::Backslash::SavedTabGroups::ReadValue(root).groups.size());
    }
    namespace
    {
        struct SyntheticSsh : IDynamicProfileGenerator
        {
            std::wstring_view GetNamespace() const noexcept override { return L"Windows.Terminal.SSH"; }
            std::wstring_view GetDisplayName() const noexcept override { return L"fixture"; }
            std::wstring_view GetIcon() const noexcept override { return L""; }
            void GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& out) const override
            {
                SshHostGenerator::GenerateForHosts(L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe", {L"box"}, out);
                for (const auto& p : out) { p->SessionOnly(true); }
            }
        };
        auto ModelWithHost(std::string_view json)
        {
            implementation::SettingsLoader loader{json, "{}"}; loader.inboxSettings.profiles = loader.GenerateProfilesFor(SyntheticSsh{});
            loader.MergeInboxIntoUserSettings(); loader.FinalizeLayering();
            return winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        }
        constexpr std::string_view local = R"({"profiles":{"list":[{"name":"local","guid":"{6239a42c-1de4-49a3-80bd-e8fdd045185c}","commandline":"cmd.exe"}]}})";
    }
    namespace
    {
        auto ReloadVisibilityModel(const std::string& json, bool discover)
        {
            implementation::SettingsLoader loader{json,"{}"};
            if (discover && loader.IsProfileSourceEnabled(L"Windows.Terminal.SSH")) { loader.inboxSettings.profiles=loader.GenerateProfilesFor(SyntheticSsh{}); }
            loader.GenerateWorkspaceProfiles();loader.MergeInboxIntoUserSettings();loader.FinalizeLayering();
            return winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        }
        auto VisibilityState()
        {
            GUID id;THROW_IF_FAILED(CoCreateGuid(&id));
            const auto directory=std::filesystem::temp_directory_path()/(L"Backslash-ssh-visibility-"+std::wstring{winrt::to_hstring(id)});
            std::filesystem::create_directories(directory);
            return winrt::make_self<implementation::ApplicationState>(directory);
        }
        bool ActiveContains(const winrt::com_ptr<implementation::CascadiaSettings>& settings,const winrt::guid& id)
        {
            for (const auto& p:settings->ActiveProfiles()) { if(p.Guid()==id){return true;} }return false;
        }
    }
    void WorkspaceSshTests::ImportedWorkspaceVisibilityRoundtrip()
    {
        const auto previous=SshHostGenerator::HostDiscoveryEnabled();const auto restore=wil::scope_exit([&]{SshHostGenerator::HostDiscoveryEnabled(previous);});SshHostGenerator::HostDiscoveryEnabled(true);
        auto model=ModelWithHost(local);const auto host=CreateDynamicProfile(L"SSH - box")->Guid();
        model->PreviewSshImport(host,"Host box\nHostName server.invalid\nUser fixture\n", "", "fallback");
        const auto imported=model->ImportSshProfile(host);const auto id=imported.Guid();const auto records=model->GlobalSettings().WorkspaceSshConnectionsJson();
        VERIFY_IS_TRUE(id!=host);VERIFY_ARE_EQUAL(winrt::hstring{W::Source},imported.Source());
        auto state=VisibilityState();
        for (bool hidden:{true,false,true})
        {
            const auto draftObject=model->Copy();auto draft=winrt::get_self<implementation::CascadiaSettings>(draftObject);draft->FindProfile(id).Hidden(hidden);
            std::string saved;VERIFY_IS_TRUE(draft->WriteSettingsToDisk([&](std::string_view value){saved=value;},*state));
            model=ReloadVisibilityModel(saved,false);
            VERIFY_ARE_EQUAL(hidden,model->FindProfile(id).Hidden());VERIFY_ARE_EQUAL(!hidden,ActiveContains(model,id));
            VERIFY_ARE_EQUAL(records,model->GlobalSettings().WorkspaceSshConnectionsJson());
            model=ReloadVisibilityModel(saved,true);
            model->PreviewSshImport(host,"Host box\nHostName server.invalid\nUser fixture\n", "", "fallback");VERIFY_IS_FALSE(model->CanImportSshProfile());
            VERIFY_ARE_EQUAL(hidden,model->FindProfile(id).Hidden());
        }
    }
    void WorkspaceSshTests::DiscoveredHostVisibilityRoundtrip()
    {
        const auto previous=SshHostGenerator::HostDiscoveryEnabled();const auto restore=wil::scope_exit([&]{SshHostGenerator::HostDiscoveryEnabled(previous);});SshHostGenerator::HostDiscoveryEnabled(true);
        auto model=ModelWithHost(local);const auto id=CreateDynamicProfile(L"SSH - box")->Guid();auto state=VisibilityState();
        for (bool hidden:{true,false,true})
        {
            const auto draftObject=model->Copy();auto draft=winrt::get_self<implementation::CascadiaSettings>(draftObject);draft->FindProfile(id).Hidden(hidden);
            std::string saved;VERIFY_IS_TRUE(draft->WriteSettingsToDisk([&](std::string_view value){saved=value;},*state));
            model=ReloadVisibilityModel(saved,true);
            VERIFY_ARE_EQUAL(hidden,model->FindProfile(id).Hidden());VERIFY_ARE_EQUAL(!hidden,ActiveContains(model,id));
            VERIFY_ARE_EQUAL(2u,model->ToJson(true)["profiles"]["list"].size());
            auto off=ReloadVisibilityModel(saved,false);VERIFY_IS_NOT_NULL(off->FindProfile(id)); VERIFY_IS_FALSE(off->FindProfile(id).Orphaned());
            model=ReloadVisibilityModel(Json::writeString(Json::StreamWriterBuilder{},off->ToJson(true)),true);
            VERIFY_ARE_EQUAL(hidden,model->FindProfile(id).Hidden());
        }
    }
    void WorkspaceSshTests::ImportDraftDeduplicatesAndRoundtripsWithoutHostDiscovery()
    {
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); }); SshHostGenerator::HostDiscoveryEnabled(true);
        const auto settings = ModelWithHost(local); const auto guid = CreateDynamicProfile(L"SSH - box")->Guid();
        VERIFY_IS_TRUE(settings->FindProfile(guid).SetSshShell(3, L""));
        const auto before = settings->ExplicitSettingsJson();
        settings->PreviewSshImport(guid, "Host box\n HostName server.example\n User alice\n Port 2222\n", "", "fallback");
        VERIFY_IS_TRUE(settings->CanImportSshProfile()); VERIFY_ARE_EQUAL(before, settings->ExplicitSettingsJson());
        const auto imported = settings->ImportSshProfile(guid); VERIFY_ARE_EQUAL(winrt::hstring{W::Source}, imported.Source());
        VERIFY_ARE_EQUAL(winrt::hstring{L"SSH box"}, imported.ConnectionIdentity(imported.Commandline()));
        VERIFY_ARE_EQUAL(3, imported.SshShell()); VERIFY_IS_TRUE(imported.SetSshShell(3, L""));
        settings->PreviewSshImport(guid, "Host box\n HostName server.example\n User alice\n Port 2222\n", "", "fallback"); VERIFY_IS_FALSE(settings->CanImportSshProfile());
        settings->PreviewSshImport(guid, "Host box\n HostName different\n User alice\n", "", "fallback"); VERIFY_IS_FALSE(settings->CanImportSshProfile());
        const auto saved = settings->ExplicitSettingsJson(); SshHostGenerator::HostDiscoveryEnabled(false);
        implementation::SettingsLoader reload{ winrt::to_string(saved), "{}" };
        // Exercise the production workspace generator without host configuration.
        reload.GenerateWorkspaceProfiles();
        reload.MergeInboxIntoUserSettings(); reload.FinalizeLayering();
        const auto loaded = winrt::make_self<implementation::CascadiaSettings>(std::move(reload));
        const auto found = loaded->FindProfile(imported.Guid()); VERIFY_IS_NOT_NULL(found); VERIFY_IS_FALSE(found.Orphaned()); VERIFY_ARE_EQUAL(3, found.SshShell());
        VERIFY_IS_TRUE(loaded->ScanSshProfiles(SyntheticSsh{}).Size() == 0); // Saved aliases are filtered without reading host config.
        VERIFY_ARE_EQUAL(saved, settings->ExplicitSettingsJson());
        // Policy/source filtering is established when the loader is constructed.
        auto disabledJson = settings->ToJson(); disabledJson["disabledProfileSources"].append("Windows.Terminal.SSH");
        implementation::SettingsLoader disabled{Json::writeString(Json::StreamWriterBuilder{}, disabledJson), "{}"}; disabled.GenerateWorkspaceProfiles();
        VERIFY_IS_TRUE(disabled.inboxSettings.profiles.empty());
    }
    void WorkspaceSshTests::SessionProfilesDoNotPersistOrChangeSeenState()
    {
        implementation::SettingsLoader fixture{local, "{}"}; fixture.inboxSettings.profiles = fixture.GenerateProfilesFor(SyntheticSsh{});
        fixture.MergeInboxIntoUserSettings(); fixture.FinalizeLayering();
        GUID testGuid; THROW_IF_FAILED(CoCreateGuid(&testGuid));
        const auto directory = std::filesystem::temp_directory_path() / (L"Backslash-workspace-ssh-tests-" + std::wstring{winrt::to_hstring(testGuid)});
        std::filesystem::create_directories(directory); auto state = winrt::make_self<implementation::ApplicationState>(directory);
        VERIFY_IS_FALSE(fixture.DisableDeletedProfiles(*state)); VERIFY_IS_TRUE(state->GeneratedProfiles().empty());
        const auto model = winrt::make_self<implementation::CascadiaSettings>(std::move(fixture)); VERIFY_ARE_EQUAL(2u, model->AllProfiles().Size());
        VERIFY_ARE_EQUAL(1u, model->ToJson()["profiles"]["list"].size());
        VERIFY_IS_TRUE(model->WriteSettingsToDisk([](std::string_view) {}, *state)); VERIFY_IS_TRUE(state->GeneratedProfiles().empty());
        const auto copy = model->Copy(); VERIFY_ARE_EQUAL(1u, winrt::get_self<implementation::CascadiaSettings>(copy)->ToJson()["profiles"]["list"].size());
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); });
        SshHostGenerator::HostDiscoveryEnabled(false); std::vector<winrt::com_ptr<implementation::Profile>> none;
        SshHostGenerator{}.GenerateProfiles(none); VERIFY_IS_TRUE(none.empty());
        const auto blocked = ModelWithHost(R"({"disabledProfileSources":["Windows.Terminal.SSH"],"profiles":{"list":[{"name":"local","commandline":"cmd.exe"}]}})");
        VERIFY_ARE_EQUAL(1u, blocked->AllProfiles().Size());
    }
    void WorkspaceSshTests::LegacyProfilesAndWorkspaceSurviveDiscoveryCycles()
    {
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); });
        auto json = ModelWithHost(local)->ToJson(true);
        const auto hostGuid = CreateDynamicProfile(L"SSH - box")->Guid();
        Json::Value legacy{Json::objectValue}; legacy["guid"] = winrt::to_string(winrt::to_hstring(hostGuid)); legacy["name"] = "Previously saved SSH"; legacy["source"] = "Windows.Terminal.SSH";
        json["profiles"]["list"].append(legacy); json["backslash.sshConnections"].append(W::Value({"kept", "kept.invalid", "alice", 22}));
        for (int cycle = 0; cycle < 4; ++cycle)
        {
            const bool enabled = cycle % 2 != 0; SshHostGenerator::HostDiscoveryEnabled(enabled);
            implementation::SettingsLoader loader{Json::writeString(Json::StreamWriterBuilder{}, json), "{}"};
            if (enabled) { loader.inboxSettings.profiles = loader.GenerateProfilesFor(SyntheticSsh{}); }
            else { std::vector<winrt::com_ptr<implementation::Profile>> output; SshHostGenerator{}.GenerateProfiles(output); VERIFY_IS_TRUE(output.empty()); }
            loader.GenerateWorkspaceProfiles(); loader.MergeInboxIntoUserSettings(); loader.FinalizeLayering();
            const auto model = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
            const auto prior = model->FindProfile(hostGuid); VERIFY_IS_NOT_NULL(prior); VERIFY_ARE_EQUAL(!enabled, prior.Orphaned());
            VERIFY_ARE_EQUAL(enabled ? 3u : 2u, model->ActiveProfiles().Size());
            VERIFY_ARE_EQUAL(winrt::hstring{L"Previously saved SSH"}, prior.Name());
            const auto saved = model->ToJson(true); VERIFY_ARE_EQUAL(3u, saved["profiles"]["list"].size());
            VERIFY_ARE_EQUAL(1u, saved["backslash.sshConnections"].size());
            VERIFY_IS_FALSE(saved.isMember("hostSshDiscoveryEnabled"));
        }
    }
    void WorkspaceSshTests::StaleImportReviewDoesNotChangeDraft()
    {
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); }); SshHostGenerator::HostDiscoveryEnabled(true);
        const auto model = ModelWithHost(local); const auto guid = CreateDynamicProfile(L"SSH - box")->Guid();
        model->PreviewSshImport(guid, "Host box\n HostName server.invalid\n User alice\n", "", "fallback"); VERIFY_IS_TRUE(model->CanImportSshProfile());
        model->FindProfile(winrt::guid{L"{6239a42c-1de4-49a3-80bd-e8fdd045185c}"}).Name(L"new unsaved local display name"); const auto draft = model->ExplicitSettingsJson();
        VERIFY_THROWS(model->ImportSshProfile(guid), std::invalid_argument); VERIFY_ARE_EQUAL(draft, model->ExplicitSettingsJson());
        model->PreviewSshImport(guid, "Host box\n IdentityFile never-read\n", "", "fallback"); VERIFY_IS_FALSE(model->CanImportSshProfile());
        model->PreviewSshImport(guid, "Host box\n HostName server.invalid\n User alice\n", "", "fallback");
        SshHostGenerator::HostDiscoveryEnabled(false); VERIFY_THROWS(model->ImportSshProfile(guid), std::invalid_argument);
        model->PreviewSshImport(guid); VERIFY_IS_FALSE(model->CanImportSshProfile()); // Production entry point exits before host reads.
        VERIFY_ARE_EQUAL(draft, model->ExplicitSettingsJson());
        SshHostGenerator::HostDiscoveryEnabled(true);
        const auto localProfile = model->FindProfile(winrt::guid{L"{6239a42c-1de4-49a3-80bd-e8fdd045185c}"});
        localProfile.Name(L"workspace ssh - BOX"); const auto namedDraft = model->ExplicitSettingsJson();
        model->PreviewSshImport(guid, "Host box\nHostName server.invalid\nUser alice\n", "", "fallback"); VERIFY_IS_FALSE(model->CanImportSshProfile());
        VERIFY_ARE_EQUAL(namedDraft, model->ExplicitSettingsJson()); localProfile.Name(L"new unsaved local display name");
        VERIFY_IS_TRUE(model->FindProfile(guid).SetSshShell(4, L"echo synthetic"));
        model->PreviewSshImport(guid); VERIFY_IS_FALSE(model->CanImportSshProfile()); // Advanced command guard performs no host read.
        VERIFY_IS_TRUE(model->FindProfile(guid).SetSshShell(3, L""));
        model->PreviewSshImport(guid, "Host box\nHostName server.invalid\nUser alice\n", "", "fallback"); VERIFY_IS_TRUE(model->CanImportSshProfile());
        VERIFY_IS_TRUE(model->FindProfile(guid).SetSshShell(2, L""));
        const auto shellDraft = model->ExplicitSettingsJson();
        VERIFY_THROWS(model->ImportSshProfile(guid), std::invalid_argument); VERIFY_ARE_EQUAL(shellDraft, model->ExplicitSettingsJson());
    }
    void WorkspaceSshTests::SetupTransferRoundtripsWorkspaceRecordsWithoutHostConfig()
    {
        namespace T = ::Microsoft::Terminal::Backslash::SetupTransfer;
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); }); SshHostGenerator::HostDiscoveryEnabled(true);
        const auto model = ModelWithHost(local); const auto guid = CreateDynamicProfile(L"SSH - box")->Guid();
        model->PreviewSshImport(guid, "Host box\n HostName server.invalid\n User alice\n", "", "fallback"); const auto imported = model->ImportSshProfile(guid);
        const auto document = T::Parse(T::Export(model->ToJson(true), {}));
        SshHostGenerator::HostDiscoveryEnabled(false); implementation::SettingsLoader baseLoader{local, "{}"}; baseLoader.MergeInboxIntoUserSettings(); baseLoader.FinalizeLayering();
        const auto destination = winrt::make_self<implementation::CascadiaSettings>(std::move(baseLoader));
        const auto plan = T::Prepare(document, destination->ToJson(true), {}, [&](const auto& json) {
            const auto preview = destination->ValidateSetupImport(winrt::to_hstring(T::JsonText(json)));
            const auto found = preview.FindProfile(imported.Guid()); VERIFY_IS_NOT_NULL(found); VERIFY_IS_FALSE(found.Orphaned());
            VERIFY_IS_TRUE(std::wstring_view{found.Commandline()}.find(L"HostName=server.invalid") != std::wstring_view::npos);
        });
        VERIFY_IS_TRUE(plan.CanApply()); VERIFY_ARE_EQUAL(1u, plan.settings["backslash.sshConnections"].size());
        const auto repeated = T::Prepare(document, plan.settings, plan.pins); VERIFY_IS_TRUE(repeated.CanApply()); VERIFY_IS_FALSE(repeated.changed);
        auto conflicting = plan.settings; conflicting["backslash.sshConnections"][0]["host"] = "different.invalid";
        const auto conflict = T::Prepare(document, conflicting, {}); VERIFY_IS_FALSE(conflict.CanApply());
        VERIFY_ARE_EQUAL(std::string{"different.invalid"}, conflict.settings["backslash.sshConnections"][0]["host"].asString());
        auto extra = plan.settings; extra["backslash.sshConnections"].append(W::Value({"retained", "other.invalid", "bob", 2222}));
        const auto merged = T::Prepare(document, extra, {}); VERIFY_IS_TRUE(merged.CanApply()); VERIFY_ARE_EQUAL(2u, merged.settings["backslash.sshConnections"].size());
    }
    void WorkspaceSshTests::FreshSessionUsesPortableDefault()
    {
        VERIFY_IS_FALSE(SshHostGenerator::HostDiscoveryEnabled());
        wchar_t enable[2]{};
        if (GetEnvironmentVariableW(L"BACKSLASH_TEST_SESSION_ENABLE_ON_EXIT", enable, 2) && enable[0] == L'1') { SshHostGenerator::HostDiscoveryEnabled(true); VERIFY_IS_TRUE(SshHostGenerator::HostDiscoveryEnabled()); }
        // Run independently in a fresh copied test host with .portable to prove
        // a prior process's enabled session never persists to a subsequent one.
    }

    void WorkspaceSshTests::HashTokensAreNotLossilyImported()
    {
        for (const auto* option : { "User alice#ops", "HostName server#suffix", "Port 22#suffix", "User \"alice#ops\"" })
        {
            const auto review = W::Preview("box", std::string{"Host box\n"} + option + "\n", "", "fallback");
            VERIFY_IS_FALSE(review.record.has_value()); VERIFY_IS_FALSE(review.issues.empty());
        }
        const auto allowed = W::Preview("box", "# comment\nHost box # comment\nHostName server.example # comment\nUser alice # comment\n", "", "fallback");
        VERIFY_IS_TRUE(allowed.record.has_value()); VERIFY_ARE_EQUAL(std::string{"alice"}, allowed.record->user);
    }
    void WorkspaceSshTests::ImportCapacityIsValidatedBeforeMutation()
    {
        const auto old = SshHostGenerator::HostDiscoveryEnabled(); const auto done = wil::scope_exit([&]{ SshHostGenerator::HostDiscoveryEnabled(old); }); SshHostGenerator::HostDiscoveryEnabled(true);
        const auto model = ModelWithHost(local); const auto guid = CreateDynamicProfile(L"SSH - box")->Guid();
        Json::Value records{Json::arrayValue}; for (int i = 0; i < 511; ++i) { records.append(W::Value({"cap" + std::to_string(i), "server.invalid", "alice", 22})); }
        model->GlobalSettings().WorkspaceSshConnectionsJson(winrt::to_hstring(Json::writeString(Json::StreamWriterBuilder{}, records)));
        model->PreviewSshImport(guid, "Host box\nHostName server.invalid\nUser alice\n", "", "fallback"); VERIFY_IS_TRUE(model->CanImportSshProfile());
        model->ImportSshProfile(guid); VERIFY_ARE_EQUAL(512u, model->ToJson()["backslash.sshConnections"].size());
        const auto full = model->ExplicitSettingsJson();
        // Change the synthetic alias identity to request a distinct 513th record.
        std::vector<winrt::com_ptr<implementation::Profile>> candidates;
        SshHostGenerator::GenerateForHosts(L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe", {L"extra"}, candidates);
        candidates.front()->Origin(OriginTag::Generated); candidates.front()->Source(L"Windows.Terminal.SSH");
        model->_sshScanCandidates = candidates;
        const auto before = model->ExplicitSettingsJson();
        VERIFY_THROWS(model->RestoreSshProfile(candidates.front()->Guid()), std::invalid_argument);
        VERIFY_ARE_EQUAL(before, model->ExplicitSettingsJson());
        auto oversized = model->ToJson()["backslash.sshConnections"]; oversized.append(W::Value({"oversized", "server.invalid", "alice", 22}));
        VERIFY_THROWS(model->GlobalSettings().WorkspaceSshConnectionsJson(winrt::to_hstring(Json::writeString(Json::StreamWriterBuilder{}, oversized))), winrt::hresult_invalid_argument);
        VERIFY_ARE_EQUAL(before, model->ExplicitSettingsJson());
        (void)full;
    }

    void WorkspaceSshTests::OnlySingleOptionalEqualsSeparatorIsAccepted()
    {
        for (const auto* separator : { "==22", " = =22", " == 22", " =\t=22" })
        {
            const auto review = W::Preview("box", std::string{"Host box\nPort"} + separator + "\n", "", "alice");
            VERIFY_IS_FALSE(review.record.has_value()); VERIFY_IS_FALSE(review.issues.empty());
        }
        for (const auto* separator : { " 22", "=22", " =22", "= 22", " = 22", " \t=\t22" })
        {
            const auto review = W::Preview("box", std::string{"Host box\nPort"} + separator + "\n", "", "alice");
            VERIFY_IS_TRUE(review.record.has_value()); VERIFY_ARE_EQUAL(22u, review.record->port);
        }
    }

}

namespace SettingsModelUnitTests
{
    void WorkspaceSshTests::DiscoveredHostShellAppearanceRoundtrip()
    {
        auto model = ModelWithHost(local);
        const auto id = CreateDynamicProfile(L"SSH - box")->Guid();
        for (int mode = 0; mode <= 4; ++mode)
        {
            auto profile = model->FindProfile(id);
            VERIFY_IS_TRUE(profile.SetSshShell(mode, L"fixture-shell --login"));
            Json::Value appearance{Json::objectValue};
            appearance["font"]["size"] = 19;
            appearance["experimental.retroTerminalGlowIntensity"] = 0.4;
            appearance["unfocusedAppearance"]["opacity"] = 65;
            winrt::get_self<implementation::Profile>(profile)->LayerJson(appearance);
            const auto saved = Json::writeString(Json::StreamWriterBuilder{}, model->ToJson(true));
            VERIFY_IS_TRUE(saved.find("fixture-shell") != std::string::npos || mode != 4);
            auto off = ReloadVisibilityModel(saved, false);
            VERIFY_IS_NOT_NULL(off->FindProfile(id));
            model = ReloadVisibilityModel(Json::writeString(Json::StreamWriterBuilder{}, off->ToJson(true)), true);
            VERIFY_ARE_EQUAL(mode, model->FindProfile(id).SshShell());
            const auto output = winrt::get_self<implementation::Profile>(model->FindProfile(id))->ToJson();
            VERIFY_ARE_EQUAL(19, output["font"]["size"].asInt());
            VERIFY_ARE_EQUAL(65, output["unfocusedAppearance"]["opacity"].asInt());
            VERIFY_IS_TRUE(output.isMember("experimental.retroTerminalGlowIntensity"));
        }
    }
}

namespace SettingsModelUnitTests
{
    void WorkspaceSshTests::AdvancedTransientSaveAdoptsWithoutMutationOnFailure()
    {
        auto model = ModelWithHost(local);
        const auto id = CreateDynamicProfile(L"SSH - box")->Guid();
        auto state = VisibilityState();
        const auto clean = model->DraftComparisonJson();
        auto profile = model->FindProfile(id);
        profile.Commandline(L"fixture-advanced.exe --custom");
        Json::Value fields{Json::objectValue}; fields["environment"]["FIXTURE"] = "kept";
        winrt::get_self<implementation::Profile>(profile)->LayerJson(fields);
        profile.Hidden(true);
        const auto draft = model->DraftComparisonJson();
        VERIFY_ARE_NOT_EQUAL(clean, draft);
        VERIFY_IS_FALSE(model->WriteSettingsToDisk([](std::string_view) { throw std::runtime_error("synthetic write failure"); }, *state));
        VERIFY_ARE_EQUAL(draft, model->DraftComparisonJson());
        VERIFY_IS_TRUE(state->GeneratedProfiles().empty());
        std::string saved;
        VERIFY_IS_TRUE(model->WriteSettingsToDisk([&](std::string_view text) { saved = text; }, *state));
        auto fresh = ReloadVisibilityModel(saved, false);
        const auto owned = fresh->FindProfile(id);
        VERIFY_IS_NOT_NULL(owned); VERIFY_IS_FALSE(owned.Orphaned()); VERIFY_IS_TRUE(owned.Hidden());
        VERIFY_ARE_EQUAL(winrt::hstring{L"fixture-advanced.exe --custom"}, owned.Commandline());
        VERIFY_ARE_EQUAL(std::string{"kept"}, winrt::get_self<implementation::Profile>(owned)->ToJson()["environment"]["FIXTURE"].asString());
        VERIFY_IS_FALSE(ActiveContains(fresh, id));
        VERIFY_IS_TRUE(fresh->ScanSshProfiles(SyntheticSsh{}).Size() == 0);
        VERIFY_ARE_EQUAL(draft, model->DraftComparisonJson());
    }

    void WorkspaceSshTests::SelectedAliasImportSaveRestartAndDeduplicate()
    {
        // Optional owned fixture transport for a producer and a genuinely fresh
        // consumer process. No host files, discovery or SSH launch are involved.
        wchar_t fixturePath[32768]{};
        const auto fixtureLength = GetEnvironmentVariableW(L"BACKSLASH_TEST_ALIAS_RESTART_FILE", fixturePath, ARRAYSIZE(fixturePath));
        const std::filesystem::path fixture = fixtureLength && fixtureLength < ARRAYSIZE(fixturePath) ? std::filesystem::path{fixturePath} : std::filesystem::path{};
        wchar_t consume[2]{};
        if (!fixture.empty() && GetEnvironmentVariableW(L"BACKSLASH_TEST_ALIAS_RESTART_READ", consume, ARRAYSIZE(consume)) && consume[0] == L'1')
        {
            VERIFY_IS_FALSE(SshHostGenerator::HostDiscoveryEnabled());
            const auto expected = CreateDynamicProfile(L"Workspace SSH - box")->Guid();
            for (const bool hidden : {false, true})
            {
                auto path = fixture; path += hidden ? L".hidden" : L".visible";
                std::ifstream file{path, std::ios::binary}; VERIFY_IS_TRUE(file.good());
                const std::string saved{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
                auto fresh = ReloadVisibilityModel(saved, false);
                const auto profile = fresh->FindProfile(expected);
                VERIFY_IS_NOT_NULL(profile); VERIFY_IS_FALSE(profile.Orphaned());
                VERIFY_ARE_EQUAL(hidden, profile.Hidden()); VERIFY_ARE_EQUAL(!hidden, ActiveContains(fresh, expected));
                VERIFY_ARE_EQUAL(2u, fresh->AllProfiles().Size());
                VERIFY_ARE_EQUAL(1u, fresh->ToJson()["backslash.sshConnections"].size());
            }
            return;
        }
        const auto checkpoint = [&](std::string_view saved, bool hidden) {
            if (!fixture.empty())
            {
                auto path = fixture; path += hidden ? L".hidden" : L".visible";
                std::ofstream file{path, std::ios::binary | std::ios::trunc};
                file.write(saved.data(), static_cast<std::streamsize>(saved.size())); VERIFY_IS_TRUE(file.good());
            }
        };
        struct Several : SyntheticSsh
        {
            void GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& out) const override
            {
                SshHostGenerator::GenerateForHosts(L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe", {L"box other BOX"}, out);
                for (const auto& p : out) { p->SessionOnly(true); }
            }
        };
        auto model = ReloadVisibilityModel(std::string{local}, false);
        const auto before = model->ExplicitSettingsJson();
        auto candidates = model->ScanSshProfiles(Several{});
        VERIFY_ARE_EQUAL(2u, candidates.Size());
        VERIFY_ARE_EQUAL(before, model->ExplicitSettingsJson());
        VERIFY_ARE_EQUAL(1u, model->ActiveProfiles().Size());
        const auto selected = candidates.GetAt(0).Guid();
        const auto imported = model->RestoreSshProfile(selected);
        VERIFY_IS_NOT_NULL(imported); const auto id = imported.Guid();
        VERIFY_IS_TRUE(id != selected); VERIFY_IS_FALSE(imported.Orphaned());
        VERIFY_ARE_EQUAL(1u, model->ToJson()["backslash.sshConnections"].size());
        VERIFY_ARE_EQUAL(std::string{"hostAlias"}, model->ToJson()["backslash.sshConnections"][0]["mode"].asString());
        VERIFY_IS_NULL(model->RestoreSshProfile(selected));
        VERIFY_ARE_EQUAL(1u, model->ScanSshProfiles(Several{}).Size());
        auto state = VisibilityState(); std::string saved;
        VERIFY_IS_FALSE(model->WriteSettingsToDisk([](std::string_view) { throw std::runtime_error("synthetic failure"); }, *state));
        VERIFY_ARE_EQUAL(1u, ReloadVisibilityModel(std::string{local}, false)->AllProfiles().Size());
        VERIFY_IS_TRUE(model->WriteSettingsToDisk([&](std::string_view text) { saved=text; }, *state));
        checkpoint(saved, false);
        auto fresh = ReloadVisibilityModel(saved, false);
        VERIFY_IS_NOT_NULL(fresh->FindProfile(id)); VERIFY_IS_FALSE(fresh->FindProfile(id).Orphaned());
        VERIFY_IS_TRUE(ActiveContains(fresh, id));
        VERIFY_ARE_EQUAL(2u, fresh->AllProfiles().Size());
        VERIFY_IS_TRUE(std::wstring_view{fresh->FindProfile(id).Commandline()}.find(L"-F none") == std::wstring_view::npos);
        fresh->FindProfile(id).Hidden(true);
        VERIFY_IS_TRUE(fresh->WriteSettingsToDisk([&](std::string_view text) { saved=text; }, *state));
        checkpoint(saved, true);
        fresh = ReloadVisibilityModel(saved, false);
        VERIFY_IS_TRUE(fresh->FindProfile(id).Hidden()); VERIFY_IS_FALSE(ActiveContains(fresh,id));
        VERIFY_ARE_EQUAL(1u, fresh->ScanSshProfiles(Several{}).Size());
        VERIFY_IS_TRUE(fresh->WriteSettingsToDisk([&](std::string_view text) { saved=text; }, *state));
        VERIFY_ARE_EQUAL(1u, fresh->ToJson()["backslash.sshConnections"].size());
    }

    void WorkspaceSshTests::ScanAndCancelDoNotImport()
    {
        auto source = ReloadVisibilityModel(std::string{local}, false);
        const auto before = source->ExplicitSettingsJson();
        const auto clone = source->Copy(); auto draft = winrt::get_self<implementation::CascadiaSettings>(clone);
        auto candidates = draft->ScanSshProfiles(SyntheticSsh{});
        VERIFY_ARE_EQUAL(1u,candidates.Size());
        VERIFY_ARE_EQUAL(before,draft->ExplicitSettingsJson()); // Empty selection: no action.
        draft->RestoreSshProfile(candidates.GetAt(0).Guid());
        VERIFY_ARE_NOT_EQUAL(before,draft->ExplicitSettingsJson());
        VERIFY_ARE_EQUAL(before,source->ExplicitSettingsJson());
        VERIFY_ARE_EQUAL(1u,ReloadVisibilityModel(winrt::to_string(before),false)->AllProfiles().Size());
        VERIFY_IS_NULL(draft->RestoreSshProfile(winrt::guid{}));
    }

    void WorkspaceSshTests::AliasSchemaRejectsMalformedAndUnsupportedValues()
    {
        const W::Record alias{"box", {}, {}, 22, true};
        VERIFY_IS_TRUE(W::Read(W::Value(alias)) == alias);
        auto value = W::Value(alias);
        for (const auto* bad : {"", "-option", "a b", "%expand%", "bad\nvalue"})
        { value["alias"] = bad; VERIFY_THROWS(W::Read(value),std::invalid_argument); }
        value = W::Value(alias); value["mode"] = "unknown"; VERIFY_THROWS(W::Read(value),std::invalid_argument);
        value = W::Value(alias); value["port"] = 22; VERIFY_THROWS(W::Read(value),std::invalid_argument);
        value = W::Value(alias); value["alias"] = 42; VERIFY_THROWS(W::Read(value),std::invalid_argument);
        value = W::Value(alias); value["profile"] = "malformed"; VERIFY_THROWS(W::Read(value),std::invalid_argument);
        value = W::Value(alias); value["profile"] = 42; VERIFY_THROWS(W::Read(value),std::invalid_argument);
        Json::Value array{Json::arrayValue}; array.append(W::Value(alias)); array.append(W::Value({"BOX","host.invalid","fixture",22}));
        VERIFY_THROWS(W::ReadRecords(array),std::invalid_argument);
        const auto command = W::BaseCommand(alias,L"ssh.exe");
        VERIFY_ARE_EQUAL(std::wstring{L"\"ssh.exe\" \"box\""},command);
    }

    void WorkspaceSshTests::PreferencesPrefillWithoutAdoption()
    {
        auto json = ReloadVisibilityModel(std::string{local},false)->ToJson();
        const auto id=CreateDynamicProfile(L"SSH - box")->Guid();
        const auto key=winrt::to_string(winrt::to_hstring(id));
        json["backslash.hostSshProfileVisibility"][key]["hidden"]=true;
        json["backslash.hostSshProfileVisibility"][key]["font"]["size"]=21;
        auto model=ReloadVisibilityModel(Json::writeString(Json::StreamWriterBuilder{},json),false);
        const auto before=model->ExplicitSettingsJson();
        auto candidates=model->ScanSshProfiles(SyntheticSsh{});
        VERIFY_ARE_EQUAL(1u,candidates.Size()); VERIFY_IS_TRUE(candidates.GetAt(0).Hidden());
        VERIFY_ARE_EQUAL(before,model->ExplicitSettingsJson());
        const auto imported=model->RestoreSshProfile(id);
        VERIFY_IS_TRUE(imported.Hidden());
        VERIFY_ARE_EQUAL(21,winrt::get_self<implementation::Profile>(imported)->ToJson()["font"]["size"].asInt());
    }

    void WorkspaceSshTests::InheritedDefaultsRemainInheritedAfterAdoption()
    {
        auto json=ReloadVisibilityModel(std::string{local},false)->ToJson();
        json["profiles"]["defaults"]["font"]["size"]=18;
        auto model=ReloadVisibilityModel(Json::writeString(Json::StreamWriterBuilder{},json),true);
        const auto id=CreateDynamicProfile(L"SSH - box")->Guid();
        model->FindProfile(id).Hidden(true);
        auto saved=model->ToJson();
        VERIFY_IS_FALSE(saved["profiles"]["list"][1].isMember("font"));
        saved["profiles"]["defaults"]["font"]["size"]=23;
        auto fresh=ReloadVisibilityModel(Json::writeString(Json::StreamWriterBuilder{},saved),false);
        VERIFY_IS_NOT_NULL(fresh->FindProfile(id)); VERIFY_IS_FALSE(fresh->FindProfile(id).Orphaned());
        VERIFY_ARE_EQUAL(23.0f, fresh->FindProfile(id).FontInfo().FontSize());
    }
}

namespace SettingsModelUnitTests
{
    void WorkspaceSshTests::EditedLegacyAliasCollisionRejectsWithoutDroppingEdits()
    {
        namespace Close = ::Microsoft::Terminal::Backslash::SettingsClosePolicy;
        for (const bool reference : {false, true})
        {
            for (int edit = 0; edit < 3; ++edit)
            {
                auto json = ModelWithHost(local)->ToJson();
                const auto record = reference ? W::Value({"BOX", {}, {}, 22, true}) : W::Value({"BOX", "saved.invalid", "fixture", 2222});
                json["backslash.sshConnections"].append(record);
                implementation::SettingsLoader loader{Json::writeString(Json::StreamWriterBuilder{}, json), "{}"};
                loader.GenerateWorkspaceProfiles();
                // Reproduce a mixed legacy session: the transient row predates
                // the durable owner. Normal new scans already suppress it.
                auto discovered = loader.GenerateProfilesFor(SyntheticSsh{});
                loader.inboxSettings.profiles.insert(loader.inboxSettings.profiles.end(), discovered.begin(), discovered.end());
                loader.MergeInboxIntoUserSettings(); loader.FinalizeLayering();
                auto model = winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
                const auto id = CreateDynamicProfile(L"SSH - box")->Guid();
                const auto ownerId = CreateDynamicProfile(L"Workspace SSH - BOX")->Guid();
                const auto owner = model->FindProfile(ownerId); VERIFY_IS_NOT_NULL(owner);
                const auto ownerBefore = winrt::get_self<implementation::Profile>(owner)->ToJson();
                const auto endpointBefore = model->GlobalSettings().WorkspaceSshConnectionsJson();
                const auto clean = model->DraftComparisonJson();
                const auto cleanSnapshot = model->Copy();
                const auto legacy = model->FindProfile(id); VERIFY_IS_TRUE(legacy.IsTransientHostSsh());
                if (edit == 0) { legacy.Hidden(true); }
                else if (edit == 1)
                {
                    Json::Value fields{Json::objectValue}; fields["font"]["size"] = 19;
                    winrt::get_self<implementation::Profile>(legacy)->LayerJson(fields);
                }
                else
                {
                    legacy.Commandline(L"synthetic-advanced.exe --edited");
                    Json::Value fields{Json::objectValue}; fields["environment"]["FIXTURE"] = "retained";
                    winrt::get_self<implementation::Profile>(legacy)->LayerJson(fields);
                }
                const auto draft = model->DraftComparisonJson(); VERIFY_ARE_NOT_EQUAL(clean, draft);
                const auto explicitBefore = winrt::get_self<implementation::Profile>(legacy)->ToJson();
                auto state = VisibilityState(); const auto stateBefore = state->GeneratedProfiles();
                bool written = false;
                VERIFY_IS_FALSE(model->WriteSettingsToDisk([&](std::string_view) { written = true; }, *state));
                VERIFY_IS_FALSE(written);
                VERIFY_IS_TRUE(std::wstring_view{model->LastSaveErrorMessage()}.find(L"conflicts") != std::wstring_view::npos);
                VERIFY_IS_TRUE(explicitBefore == winrt::get_self<implementation::Profile>(legacy)->ToJson());
                VERIFY_IS_TRUE(ownerBefore == winrt::get_self<implementation::Profile>(owner)->ToJson());
                VERIFY_ARE_EQUAL(endpointBefore, model->GlobalSettings().WorkspaceSshConnectionsJson());
                VERIFY_IS_TRUE(stateBefore == state->GeneratedProfiles());
                VERIFY_THROWS(model->ExplicitSettingsJson(), std::invalid_argument);
                VERIFY_ARE_EQUAL(draft, model->DraftComparisonJson());
                VERIFY_IS_TRUE(Close::Evaluate(false, false, false, clean != model->DraftComparisonJson()) == Close::Decision::ConfirmDiscard);
                const auto discarded = cleanSnapshot.Copy();
                VERIFY_ARE_EQUAL(clean, discarded.DraftComparisonJson());
            }
        }
    }

    void WorkspaceSshTests::FullCollectionSaveFailureStillAllowsDraftDiscard()
    {
        namespace Close = ::Microsoft::Terminal::Backslash::SettingsClosePolicy;
        auto model = ModelWithHost(local);
        Json::Value records{Json::arrayValue};
        for (int i = 0; i < 512; ++i) { records.append(W::Value({"capacity" + std::to_string(i), "saved.invalid", "fixture", 22})); }
        model->GlobalSettings().WorkspaceSshConnectionsJson(winrt::to_hstring(Json::writeString(Json::StreamWriterBuilder{}, records)));
        const auto clean = model->DraftComparisonJson();
        const auto cleanSnapshot = model->Copy();
        const auto profile = model->FindProfile(CreateDynamicProfile(L"SSH - box")->Guid());
        profile.Hidden(true); profile.Commandline(L"synthetic-advanced.exe --retained");
        Json::Value fields{Json::objectValue}; fields["environment"]["FIXTURE"] = "retained";
        winrt::get_self<implementation::Profile>(profile)->LayerJson(fields);
        const auto draft = model->DraftComparisonJson(); VERIFY_ARE_NOT_EQUAL(clean, draft);
        auto state = VisibilityState(); const auto stateBefore = state->GeneratedProfiles();
        bool written = false;
        VERIFY_IS_FALSE(model->WriteSettingsToDisk([&](std::string_view) { written = true; }, *state));
        VERIFY_IS_FALSE(written); VERIFY_IS_TRUE(stateBefore == state->GeneratedProfiles());
        VERIFY_THROWS(model->ExplicitSettingsJson(), std::invalid_argument);
        VERIFY_ARE_EQUAL(draft, model->DraftComparisonJson());
        VERIFY_IS_TRUE(Close::Evaluate(false, false, false, clean != model->DraftComparisonJson()) == Close::Decision::ConfirmDiscard);
        const auto kept = model->Copy(); VERIFY_ARE_EQUAL(draft, kept.DraftComparisonJson());
        const auto discarded = cleanSnapshot.Copy(); VERIFY_ARE_EQUAL(clean, discarded.DraftComparisonJson());
        VERIFY_ARE_EQUAL(512u, W::ReadRecords(Json::Value{records}).size());
        VERIFY_ARE_EQUAL(winrt::hstring{L"synthetic-advanced.exe --retained"}, profile.Commandline());
    }

    void WorkspaceSshTests::ReviewedEndpointImportRetainsExplicitLegacyEdits()
    {
        const auto previous = SshHostGenerator::HostDiscoveryEnabled();
        const auto restore = wil::scope_exit([&] { SshHostGenerator::HostDiscoveryEnabled(previous); });
        SshHostGenerator::HostDiscoveryEnabled(true);
        auto model = ModelWithHost(local);
        const auto id = CreateDynamicProfile(L"SSH - box")->Guid();
        const auto legacy = model->FindProfile(id);
        legacy.Hidden(true); legacy.Name(L"edited connection");
        Json::Value fields{Json::objectValue}; fields["font"]["size"] = 20; fields["environment"]["FIXTURE"] = "retained";
        winrt::get_self<implementation::Profile>(legacy)->LayerJson(fields);
        VERIFY_IS_TRUE(legacy.SetSshShell(3, L""));
        model->PreviewSshImport(id, "Host box\nHostName saved.invalid\nUser fixture\nPort 2222\n", "", "fallback");
        VERIFY_IS_TRUE(model->CanImportSshProfile());
        const auto imported = model->ImportSshProfile(id);
        auto state = VisibilityState(); std::string saved;
        VERIFY_IS_TRUE(model->WriteSettingsToDisk([&](std::string_view value) { saved = value; }, *state));
        auto fresh = ReloadVisibilityModel(saved, false);
        const auto owner = fresh->FindProfile(imported.Guid());
        VERIFY_IS_NOT_NULL(owner); VERIFY_IS_TRUE(owner.Hidden()); VERIFY_IS_FALSE(ActiveContains(fresh, owner.Guid()));
        VERIFY_ARE_EQUAL(winrt::hstring{L"edited connection"}, owner.Name()); VERIFY_ARE_EQUAL(3, owner.SshShell());
        const auto explicitFields = winrt::get_self<implementation::Profile>(owner)->ToJson();
        VERIFY_ARE_EQUAL(20, explicitFields["font"]["size"].asInt());
        VERIFY_ARE_EQUAL(std::string{"retained"}, explicitFields["environment"]["FIXTURE"].asString());
        VERIFY_IS_TRUE(std::wstring_view{owner.Commandline()}.find(L"HostName=saved.invalid") != std::wstring_view::npos);
    }
}
