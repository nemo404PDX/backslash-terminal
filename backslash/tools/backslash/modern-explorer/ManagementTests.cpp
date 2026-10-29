// Copyright (c) Backslash contributors. Licensed under the MIT license.
#include "../../../src/cascadia/inc/ModernExplorerIntegration.h"
#include "../../../src/cascadia/inc/ExplorerRestart.h"
#include <iostream>
#include <fstream>
using namespace Backslash::ModernExplorer::Management;
struct Fake final : Backend
{
    State state; std::wstring fail, selected; std::vector<std::wstring> calls; bool changed{}; int notices{}, rollbacks{};
    State Read() override { return state; }
    void Step(const wchar_t* name) { calls.emplace_back(name); if (fail == name) { throw std::runtime_error("fixture failure"); } changed = true; }
    void Begin(const State&) override { calls.emplace_back(L"begin"); }
    void Validate(const std::wstring&) override { calls.emplace_back(L"validate"); }
    void InstallFiles() override { Step(L"files"); }
    void Register() override { Step(L"register"); }
    void Select(const std::wstring& target, const std::wstring&) override { selected = target; Step(L"select"); }
    void RemovePackage() override { Step(L"remove-package"); }
    void RemoveTarget(const std::wstring&) override { Step(L"remove-target"); }
    void RemoveFiles() override { Step(L"remove-files"); }
    void Rollback() override { ++rollbacks; calls.emplace_back(L"rollback"); }
    bool Changed() const override { return changed; }
    void Notify() noexcept override { ++notices; calls.emplace_back(L"notify"); }
    std::wstring FailureMessage() noexcept override { return L"fixture error"; }
};
void Require(bool value, const char* text) { if (!value) { throw std::runtime_error(text); } }
Fake Fresh() { Fake fake; fake.state.supported = true; fake.state.safe = true; fake.state.developerMode = true; fake.state.bundle = true; fake.state.current = L"D:\\new copy\\Backslash.exe"; return fake; }
int wmain(int argc, wchar_t** argv)
{
    try
    {
        Require(argc == 3, "Owned fixture and reviewed payload arguments required");
        ValidateText(L"Unicode 文件 😀"); bool invalidTextRejected = false;
        try { ValidateText(std::wstring(1, static_cast<wchar_t>(0xD800))); } catch (...) { invalidTextRejected = true; }
        Require(invalidTextRejected, "Malformed UTF-16 not preserved");
        const auto fixture = std::filesystem::absolute(argv[1]); const std::filesystem::path source{argv[2]};
        wchar_t module[32768]{}; Require(GetModuleFileNameW(nullptr, module, 32768) != 0, "Read fixture module");
        Require(fixture.parent_path() == std::filesystem::path(module).parent_path() && !Exists(fixture), "Fixture must be a fresh sibling of the test binary");
        std::filesystem::create_directories(fixture / L"Assets");
        for (const auto& file : Payload) { RequireHash(source / file.name, file.hash); Require(CopyFileW((source / file.name).c_str(), (fixture / file.name).c_str(), TRUE), "Owned payload fixture copy"); }
        Require(Files(fixture), "Complete reviewed payload rejected");
        std::filesystem::create_directory(fixture / L"microsoft.system.package.metadata");
        bool unownedMetadataRejected = false; try { (void)Files(fixture); } catch (...) { unownedMetadataRejected = true; }
        Require(unownedMetadataRejected && Files(fixture, true), "Windows metadata requires proven development-package ownership");
        Require(Exists(fixture / L"microsoft.system.package.metadata"), "Windows metadata must not be removed");
        Require(RemoveDirectoryW((fixture / L"microsoft.system.package.metadata").c_str()), "Remove only empty owned test metadata fixture");
        Require(DeleteFileW((fixture / L"Assets" / L"Logo44.png").c_str()), "Remove owned fixture asset"); Require(!Files(fixture), "Missing file not repairable");
        Require(CopyFileW((source / L"Assets" / L"Logo44.png").c_str(), (fixture / L"Assets" / L"Logo44.png").c_str(), TRUE), "Restore owned fixture");
        const auto rejectFiles = [&] { bool rejected = false; try { (void)Files(fixture); } catch (...) { rejected = true; } Require(rejected, "Changed/unknown fixture accepted"); };
        { std::ofstream out(fixture / L"unknown.txt"); out << "owned fixture"; } rejectFiles(); Require(Exists(fixture / L"unknown.txt"), "Foreign content was deleted"); Require(DeleteFileW((fixture / L"unknown.txt").c_str()), "Remove only owned unknown fixture");
        { std::ofstream out(fixture / L"NOTICE.md", std::ios::trunc); out << "owned changed fixture"; } const auto changedHash = Hash(fixture / L"NOTICE.md"); rejectFiles(); Require(Hash(fixture / L"NOTICE.md") == changedHash, "Changed content not preserved");
        int restartCalls{}; AfterRestartConfirmation(false, [&] { ++restartCalls; }); Require(restartCalls == 0, "cancelled restart executed");
        AfterRestartConfirmation(true, [&] { ++restartCalls; }); Require(restartCalls == 1, "confirmed restart callback gate");
        const auto uri = FileUri(L"C:\\path with spaces\\#literal%\\文件");
        Require(uri.starts_with(L"file:///") && uri.find(L"%20") != std::wstring::npos && uri.find(L"%23") != std::wstring::npos, "literal file URI encoding");
        auto enable = Fresh(); auto result = Execute(enable, Action::Enable, L"");
        Require(result.success && enable.notices == 1 && enable.selected == enable.state.current, "enable/notify failed");
        Require(enable.calls == std::vector<std::wstring>{L"validate",L"begin",L"files",L"register",L"select",L"notify"}, "mutation/notification ordering");
        auto current = Fresh(); current.state.package = current.state.config = current.state.supportExists = current.state.filesComplete = current.state.configComplete = current.state.targetValid = true; current.state.target = current.state.current;
        result = Execute(current, Action::Enable, current.state.target); Require(result.success && !result.changed && current.calls.empty() && !current.notices, "idempotency notified or wrote");
        auto other = current; other.state.target = L"D:\\old copy\\Backslash.exe";
        result = Execute(other, Action::Repair, other.state.target); Require(result.success && other.calls.empty(), "repair took another copy");
        result = Execute(other, Action::Enable, other.state.target); Require(result.success && other.selected == other.state.current && other.notices == 1, "explicit repoint failed");
        auto missing = current; missing.state.filesComplete = false;
        result = Execute(missing, Action::Repair, missing.state.target); Require(result.success && missing.calls == std::vector<std::wstring>{L"validate",L"begin",L"files",L"notify"}, "missing-file repair changed registration/target");
        for (int scenario = 0; scenario != 6; ++scenario)
        {
            auto denied = Fresh(); std::wstring expected;
            if (scenario == 0) { denied.state.safe = false; }
            if (scenario == 1) { denied.state.supported = false; }
            if (scenario == 2) { denied.state.developerMode = false; }
            if (scenario == 3) { denied.state.bundle = false; }
            if (scenario == 4) { expected = L"stale expected copy"; }
            if (scenario == 5) { denied.state.target = L"D:\\deleted\\Backslash.exe"; expected = denied.state.target; }
            result = Execute(denied, scenario == 5 ? Action::Repair : Action::Enable, expected);
            Require(!result.success && denied.calls.empty() && !denied.notices, "rejection mutated state");
        }
        auto offlineRemove = current; offlineRemove.state.developerMode = offlineRemove.state.bundle = false;
        result = Execute(offlineRemove, Action::Remove, offlineRemove.state.target);
        Require(result.success && offlineRemove.calls == std::vector<std::wstring>{L"begin",L"remove-package",L"remove-target",L"remove-files",L"notify"}, "remove ownership/order or offline prerequisites");
        auto failed = Fresh(); failed.fail = L"register";
        result = Execute(failed, Action::Enable, L""); Require(!result.success && failed.rollbacks == 1 && failed.notices == 1 && failed.calls.back() == L"notify", "partial enable rollback/notification");
        auto failedRemove = current; failedRemove.fail = L"remove-target";
        result = Execute(failedRemove, Action::Remove, failedRemove.state.target); Require(!result.success && !failedRemove.rollbacks && failedRemove.notices == 1, "partial removal was hidden or automatically reinstalled");
        auto absent = Fresh(); result = Execute(absent, Action::Remove, L""); Require(result.success && absent.calls.empty() && !absent.notices, "absent removal mutated");
        std::wcout << L"PASS: owned complete/missing/changed/unknown file guards; enable, idempotency, explicit repoint, repair preserves selected copy, stale/foreign/unsupported/prerequisite rejection, offline removal, partial rollback, notification ordering and restart confirmation gate. No installed state changed.\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
