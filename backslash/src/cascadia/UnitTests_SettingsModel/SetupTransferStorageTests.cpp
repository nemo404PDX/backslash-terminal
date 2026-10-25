// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/SetupTransferStorage.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/ApplicationState.h"
#include "../TerminalSettingsModel/SavedTabGroups.h"
#include <fstream>

using namespace WEX::TestExecution;
namespace P = Microsoft::Terminal::Backslash::Pins;
namespace S = Microsoft::Terminal::Backslash::SetupTransfer;
namespace SettingsModelUnitTests
{
    class SetupTransferStorageTests
    {
        TEST_CLASS(SetupTransferStorageTests);
        TEST_METHOD(BackupNoopAndOwnershipPreservation);
        TEST_METHOD(AttachedPinChangesAndStalePreviewsAreRejected);
        TEST_METHOD(FailureRollsBackBothFiles);
        TEST_METHOD(FailedRollbackFencesAndRecovers);
        TEST_METHOD(RecoveryPreservesSubsequentExternalEdit);
        TEST_METHOD(ExportSnapshotDoesNotCleanLeases);
        TEST_METHOD(OrdinaryModelSaveRejectsInterleavedImport);
        TEST_METHOD(OrdinaryModelCopyAdvancesRepeatedSaveBaseline);
        TEST_METHOD(AutomaticReloadRetriesAreBounded);
        TEST_METHOD(GroupAppendRebasePreservesDirtyDraft);
        TEST_METHOD(MalformedLoadFallbackRetainsOnlyResetBaseline);
        static std::filesystem::path Root()
        {
            const auto root = std::filesystem::temp_directory_path() / (L"Backslash-setup-storage-" + std::wstring{ winrt::to_hstring(P::NewId()) });
            std::filesystem::create_directories(root); S::Storage::DurableWrite(root / L"settings.json", "{\"profiles\":{\"defaults\":{\"useAcrylic\":false},\"list\":[]}}"); return root;
        }
        static P::Destination Pin()
        {
            return { P::NewId(), "{22222222-2222-4222-8222-222222222222}", "", P::Kind::Local, "Fixture", "fixture-machine", "C:\\fixture\\A" };
        }
        static P::Owner Owner() { return { P::NewId(), 123, 456, 1, 1 }; }
        static P::Life Alive(const P::Owner&) { return P::Life::Alive; }
        static std::string Read(const std::filesystem::path& path)
        {
            std::ifstream stream{ path, std::ios::binary }; return { std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
        }
        static S::Storage Storage(const std::filesystem::path& root)
        {
            return S::Storage{ root, S::Storage::DurableWrite, S::Storage::Remove, Alive };
        }
        static std::filesystem::path Pending(const std::filesystem::path& root) { return root / L"setup-import.pending.json"; }
        static void CreateFailedRollback(const std::filesystem::path& root, const S::DiskSnapshot& expected, const P::Catalog& proposed)
        {
            int pinWrites = 0;
            S::Storage failing{ root, [&](const auto& path, auto bytes) {
                if (path == root / L"settings.json") { throw std::runtime_error("injected settings failure"); }
                if (path == root / L"pins.json" && ++pinWrites > 1) { throw std::runtime_error("injected rollback failure"); }
                S::Storage::DurableWrite(path, bytes);
            }, [&](const auto& path) {
                if (path == root / L"pins.json") { throw std::runtime_error("injected rollback removal failure"); }
                S::Storage::Remove(path);
            }, Alive };
            VERIFY_THROWS(failing.Apply(expected, "{\"initialRows\":42}", proposed), std::runtime_error);
            VERIFY_IS_TRUE(std::filesystem::exists(Pending(root)));
        }
    };

    void SetupTransferStorageTests::BackupNoopAndOwnershipPreservation()
    {
        const auto root = Root(); const auto pin = Pin(); const auto owner = Owner();
        P::Store store{ root, Alive }; const auto lease = store.Add(pin, owner);
        auto storage = Storage(root); const auto before = storage.Capture();
        VERIFY_ARE_EQUAL(size_t{1}, before.attachedPins.size());
        const auto originalSettings = Read(root / L"settings.json"), originalPins = Read(root / L"pins.json");
        VERIFY_IS_FALSE(storage.Apply(before, before.settings, before.pins).changed);
        auto proposed = before.pins; proposed.Add(Pin());
        const auto result = storage.Apply(before, "{\"initialRows\":42}", proposed);
        VERIFY_IS_TRUE(result.changed);
        VERIFY_ARE_EQUAL(originalSettings, Read(result.backupDirectory / L"settings.json"));
        VERIFY_ARE_EQUAL(originalPins, Read(result.backupDirectory / L"pins.json"));
        const auto after = store.Get();
        VERIFY_ARE_EQUAL(size_t{2}, after.catalog.pins.size());
        VERIFY_ARE_EQUAL(size_t{1}, after.leases.size());
        VERIFY_ARE_EQUAL(lease.nonce, after.leases.at(pin.id).nonce);
        VERIFY_IS_TRUE(after.leases.at(pin.id).owner == owner);
        VERIFY_IS_FALSE(storage.HasPendingRecovery());
        const auto again = storage.Capture();
        VERIFY_IS_FALSE(storage.Apply(again, again.settings, again.pins).changed);
    }
    void SetupTransferStorageTests::AttachedPinChangesAndStalePreviewsAreRejected()
    {
        const auto root = Root(); P::Store store{root, Alive}; store.Add(Pin(), Owner());
        auto storage = Storage(root); const auto before = storage.Capture(); auto proposed = before.pins;
        proposed.pins[0].label = "Changed";
        VERIFY_THROWS(storage.Apply(before, "{\"initialRows\":42}", proposed), std::invalid_argument);
        VERIFY_ARE_EQUAL(before.settings, Read(root / L"settings.json"));
        S::Storage::DurableWrite(root / L"settings.json", "{\"initialRows\":43}");
        VERIFY_THROWS(storage.Apply(before, "{\"initialRows\":42}", before.pins), std::runtime_error);
        VERIFY_ARE_EQUAL(std::string{"{\"initialRows\":43}"}, Read(root / L"settings.json"));
        const auto latest = storage.Capture(); store.Add(Pin(), Owner());
        VERIFY_THROWS(storage.Apply(latest, latest.settings, latest.pins), std::runtime_error);
        VERIFY_ARE_EQUAL(size_t{2}, store.Get().catalog.pins.size());
    }
    void SetupTransferStorageTests::FailureRollsBackBothFiles()
    {
        const auto root = Root(); auto normal = Storage(root); const auto before = normal.Capture(); auto proposed = before.pins; proposed.Add(Pin());
        bool failed = false;
        S::Storage failing{root, [&](const auto& path, auto bytes) {
            if (!failed && path == root / L"settings.json") { failed = true; throw std::runtime_error("injected apply failure"); }
            S::Storage::DurableWrite(path, bytes);
        }, S::Storage::Remove, Alive};
        VERIFY_THROWS(failing.Apply(before, "{\"initialRows\":42}", proposed), std::runtime_error);
        VERIFY_ARE_EQUAL(before.settings, Read(root / L"settings.json"));
        VERIFY_IS_FALSE(std::filesystem::exists(root / L"pins.json"));
        VERIFY_IS_FALSE(normal.HasPendingRecovery());
        VERIFY_IS_TRUE(normal.Capture().pins.pins.empty());
    }
    void SetupTransferStorageTests::FailedRollbackFencesAndRecovers()
    {
        const auto root = Root(); auto normal = Storage(root); const auto before = normal.Capture(); auto proposed = before.pins; proposed.Add(Pin());
        CreateFailedRollback(root, before, proposed);
        VERIFY_THROWS(normal.Capture(), winrt::hresult_error);
        VERIFY_THROWS((P::Store{root, Alive}.Get()), winrt::hresult_error);
        VERIFY_THROWS(P::Store::WithSetupLock(root, [] { return true; }), winrt::hresult_error);
        VERIFY_IS_TRUE(normal.Recover());
        VERIFY_IS_FALSE(normal.HasPendingRecovery());
        VERIFY_ARE_EQUAL(before.settings, Read(root / L"settings.json"));
        VERIFY_IS_FALSE(std::filesystem::exists(root / L"pins.json"));
        VERIFY_IS_FALSE(normal.Recover());
    }
    void SetupTransferStorageTests::RecoveryPreservesSubsequentExternalEdit()
    {
        const auto root = Root(); auto normal = Storage(root); const auto before = normal.Capture(); auto proposed = before.pins; proposed.Add(Pin());
        CreateFailedRollback(root, before, proposed);
        S::Storage::DurableWrite(root / L"settings.json", "{\"external\":true}");
        VERIFY_THROWS(normal.Recover(), std::runtime_error);
        VERIFY_ARE_EQUAL(std::string{"{\"external\":true}"}, Read(root / L"settings.json"));
        VERIFY_IS_TRUE(normal.HasPendingRecovery());
    }
    void SetupTransferStorageTests::ExportSnapshotDoesNotCleanLeases()
    {
        const auto root = Root(); P::Store{root, Alive}.Add(Pin(), Owner()); const auto bytes = Read(root / L"pins.json");
        S::Storage storage{root, S::Storage::DurableWrite, S::Storage::Remove, [](const auto&) { return P::Life::ProcessGone; }};
        const auto captured = storage.Capture();
        VERIFY_ARE_EQUAL(size_t{1}, captured.pins.pins.size());
        VERIFY_ARE_EQUAL(bytes, Read(root / L"pins.json"));
    }
    void SetupTransferStorageTests::OrdinaryModelSaveRejectsInterleavedImport()
    {
        namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
        const auto root = Root(); const auto path = root / L"settings.json";
        const std::string initial = R"({"profiles":{"defaults":{"useAcrylic":false},"list":[{"name":"Fixture","guid":"{22222222-2222-4222-8222-222222222222}"}]},"initialRows":30})";
        S::Storage::DurableWrite(path, initial);
        const auto baseline = S::SettingsWriteState::Capture(path);
        auto draft = winrt::make_self<M::CascadiaSettings>(std::string_view{initial}, std::string_view{}, baseline);
        draft->GlobalSettings().InitialRows(44);
        M::ApplicationState state{ root };
        auto storage = Storage(root); const auto expected = storage.Capture();
        auto importedPins = expected.pins; importedPins.Add(Pin());
        const std::string imported = R"({"profiles":{"defaults":{"useAcrylic":true},"list":[{"name":"Fixture","guid":"{22222222-2222-4222-8222-222222222222}"}]},"initialRows":73})";
        // Deterministically pause the actual model's save pipeline after its
        // source snapshot/serialization, then commit import before the CAS write.
        VERIFY_IS_FALSE(draft->WriteSettingsToDisk(path, state, [&] { storage.Apply(expected, imported, importedPins); }));
        VERIFY_ARE_EQUAL(imported, Read(path));
        VERIFY_ARE_EQUAL(size_t{1}, storage.Capture().pins.pins.size());
        VERIFY_ARE_EQUAL(44, draft->GlobalSettings().InitialRows()); // Draft preserved.
        VERIFY_IS_TRUE(std::wstring_view{draft->LastSaveErrorMessage()}.find(L"changed on disk") != std::wstring_view::npos);
        // Automatic load/fixup uses the same rejected-save outcome to discard
        // stale work and reconstruct from the new snapshot (bounded in LoadAll).
        int attempts = 0;
        const auto reloaded = S::SettingsWriteState::RetryAutomaticLoad([&]() -> std::optional<winrt::com_ptr<M::CascadiaSettings>> {
            const auto current = S::SettingsWriteState::Capture(path);
            auto candidate = winrt::make_self<M::CascadiaSettings>(std::string_view{current.bytes}, std::string_view{}, current);
            ++attempts;
            const auto saved = candidate->WriteSettingsToDisk(path, state, [&] {
                if (attempts == 1)
                {
                    const auto newer = storage.Capture();
                    std::string newest = imported;
                    newest.replace(newest.find("73"), 2, "74");
                    storage.Apply(newer, newest, newer.pins);
                }
            });
            if (!saved) { return std::nullopt; }
            return candidate;
        });
        VERIFY_ARE_EQUAL(2, attempts);
        VERIFY_ARE_EQUAL(74, reloaded->GlobalSettings().InitialRows());
        VERIFY_IS_TRUE(reloaded->ProfileDefaults().DefaultAppearance().UseAcrylic());
        VERIFY_ARE_EQUAL(size_t{1}, storage.Capture().pins.pins.size());
    }

    void SetupTransferStorageTests::OrdinaryModelCopyAdvancesRepeatedSaveBaseline()
    {
        namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
        const auto root = Root(); const auto path = root / L"settings.json";
        const std::string initial = R"({"profiles":{"defaults":{},"list":[{"name":"Fixture","guid":"{22222222-2222-4222-8222-222222222222}"}]},"initialRows":30})";
        S::Storage::DurableWrite(path, initial);
        const auto baseline = S::SettingsWriteState::Capture(path);
        auto original = winrt::make_self<M::CascadiaSettings>(std::string_view{initial}, std::string_view{}, baseline);
        const auto clone = original->Copy(); const auto draft = winrt::get_self<M::CascadiaSettings>(clone);
        M::ApplicationState state{ root };
        draft->GlobalSettings().InitialRows(41);
        VERIFY_IS_TRUE(draft->WriteSettingsToDisk(path, state));
        const auto first = Read(path); const auto firstHash = draft->Hash();
        draft->GlobalSettings().InitialRows(42);
        VERIFY_IS_TRUE(draft->WriteSettingsToDisk(path, state));
        VERIFY_IS_TRUE(draft->LastSaveErrorMessage().empty());
        VERIFY_IS_TRUE(first != Read(path));
        VERIFY_IS_TRUE(firstHash != draft->Hash());
        const auto second = Read(path);
        original->GlobalSettings().InitialRows(99);
        VERIFY_IS_FALSE(original->WriteSettingsToDisk(path, state));
        VERIFY_ARE_EQUAL(second, Read(path));
        // A synthetic/default model with no captured disk baseline cannot bypass CAS.
        auto unbound = winrt::make_self<M::CascadiaSettings>(std::string_view{initial}, std::string_view{});
        VERIFY_IS_FALSE(unbound->WriteSettingsToDisk(path, state));
        VERIFY_ARE_EQUAL(second, Read(path));
    }

    void SetupTransferStorageTests::AutomaticReloadRetriesAreBounded()
    {
        int attempts = 0;
        VERIFY_THROWS(S::SettingsWriteState::RetryAutomaticLoad([&]() -> std::optional<int> {
            ++attempts;
            return std::nullopt;
        }), winrt::hresult_error);
        VERIFY_ARE_EQUAL(3, attempts);
    }

    void SetupTransferStorageTests::GroupAppendRebasePreservesDirtyDraft()
    {
        namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
        namespace Model = winrt::Microsoft::Terminal::Settings::Model;
        namespace B = Microsoft::Terminal::Backslash;
        const auto root = Root(); const auto path = root / L"settings.json";
        const std::string initial = R"({"profiles":{"defaults":{},"list":[{"name":"Fixture","guid":"{22222222-2222-4222-8222-222222222222}"}]},"initialRows":30})";
        S::Storage::DurableWrite(path, initial);
        auto source = winrt::make_self<M::CascadiaSettings>(std::string_view{initial}, std::string_view{}, S::SettingsWriteState::Capture(path));
        const Model::CascadiaSettings before = *source;
        const auto copied = source->Copy(); const auto draft = winrt::get_self<M::CascadiaSettings>(copied);
        draft->GlobalSettings().InitialRows(45);
        Json::Value step{Json::objectValue}; step["kind"]="newTab"; step["profile"]="{22222222-2222-4222-8222-222222222222}"; step["source"]="";
        Json::Value tab{Json::arrayValue};tab.append(step);
        B::SavedTabGroups groups; groups.groups.push_back({"{33333333-3333-4333-8333-333333333333}","Fixture group",false,0,{tab}});
        auto json = source->ToJson(true); json["backslash.savedTabGroups"] = groups.ToValue();
        const auto changed = Json::writeString(Json::StreamWriterBuilder{}, json);
        S::Storage::DurableWrite(path, changed);
        auto fresh = winrt::make_self<M::CascadiaSettings>(std::string_view{changed}, std::string_view{}, S::SettingsWriteState::Capture(path));
        const Model::CascadiaSettings after = *fresh;
        draft->GlobalSettings().SavedTabGroupsJson(after.GlobalSettings().SavedTabGroupsJson());
        draft->RebaseAfterGroupAppend(before, after);
        VERIFY_ARE_EQUAL(45, draft->GlobalSettings().InitialRows());
        M::ApplicationState state{root};
        VERIFY_IS_TRUE(draft->WriteSettingsToDisk(path, state));
        VERIFY_THROWS(draft->RebaseAfterGroupAppend(before, after), winrt::hresult_error); // Original baseline no longer matches.
        auto nonGroup = fresh->Copy(); nonGroup.GlobalSettings().InitialRows(99);
        VERIFY_THROWS(draft->RebaseAfterGroupAppend(before, nonGroup), winrt::hresult_invalid_argument);
    }

    void SetupTransferStorageTests::MalformedLoadFallbackRetainsOnlyResetBaseline()
    {
        namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;
        const auto root = Root(); const auto path = root / L"settings.json";
        S::Storage::DurableWrite(path, "{malformed fixture");
        const auto captured = S::SettingsWriteState::Capture(path);
        auto error = winrt::make_self<M::CascadiaSettings>(captured, winrt::hstring{L"Fixture JSON syntax failure"});
        auto fallback = error->DefaultsForLoadFailure();
        const auto safe = winrt::get_self<M::CascadiaSettings>(fallback);
        VERIFY_IS_TRUE(fallback.GlobalSettings().StartupActions().empty());
        VERIFY_IS_TRUE(fallback.GetSerializationErrorMessage().empty());
        M::ApplicationState state{root};
        VERIFY_IS_TRUE(safe->WriteSettingsToDisk(path, state)); // Reset-like write, isolated path only.
        VERIFY_IS_TRUE(Read(path) != captured.bytes);
        VERIFY_THROWS(safe->DefaultsForLoadFailure(), winrt::hresult_invalid_argument);

        S::Storage::DurableWrite(path, "{malformed fixture");
        auto secondError = winrt::make_self<M::CascadiaSettings>(S::SettingsWriteState::Capture(path), winrt::hstring{L"Fixture JSON syntax failure"});
        auto staleFallback = secondError->DefaultsForLoadFailure();
        auto storage = Storage(root); const auto beforeImport = storage.Capture();
        const std::string imported = R"({"profiles":{"defaults":{},"list":[{"name":"Imported","guid":"{22222222-2222-4222-8222-222222222222}"}]},"initialRows":73})";
        storage.Apply(beforeImport, imported, beforeImport.pins);
        VERIFY_IS_FALSE(winrt::get_self<M::CascadiaSettings>(staleFallback)->WriteSettingsToDisk(path, state));
        VERIFY_ARE_EQUAL(imported, Read(path));
    }
}
