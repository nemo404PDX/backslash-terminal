// Copyright (c) Backslash contributors. Licensed under the MIT license.
#include "ExplorerCommand.h"
#include "ManifestIdentity.h"
#include <fstream>
#include <shellapi.h>
#include <iostream>
using namespace Backslash::ModernExplorer;
using Microsoft::WRL::ComPtr;
void Require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
template<typename F> void Reject(F action) { bool failed = false; try { action(); } catch (const std::exception&) { failed = true; } Require(failed, "Expected rejection"); }
std::string Ascii(const std::wstring& text) { std::string out; for (const auto ch : text) { Require(ch <= 127, "Expected escaped ASCII JSON"); out += static_cast<char>(ch); } return out; }
std::string Snapshot(HKEY key)
{
    std::string out;
    for (DWORD i = 0;; ++i)
    {
        wchar_t name[256]; DWORD length = 256, type{}, bytes{};
        const auto code = RegEnumValueW(key, i, name, &length, nullptr, &type, nullptr, &bytes); if (code == ERROR_NO_MORE_ITEMS) { break; } Check(code);
        std::vector<BYTE> data(bytes); Check(RegQueryValueExW(key, name, nullptr, &type, data.data(), &bytes));
        out.append(reinterpret_cast<const char*>(name), length * sizeof(wchar_t)); out += ':' + std::to_string(type) + ':' + std::to_string(bytes) + ':';
        if (!data.empty()) { out.append(reinterpret_cast<const char*>(data.data()), data.size()); }
    }
    return out;
}
// Minimal site exercises the same IServiceProvider -> IFolderView -> folder
// contract as Explorer, without creating/controlling an Explorer window.
struct FolderSite final : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IServiceProvider, IFolderView>
{
    ComPtr<IShellItem> folder;
    explicit FolderSite(IShellItem* argument) : folder(argument) {}
    IFACEMETHODIMP QueryService(REFGUID service, REFIID id, void** argument) override { if (service != SID_SFolderView) { return E_NOINTERFACE; } return QueryInterface(id, argument); }
    IFACEMETHODIMP GetFolder(REFIID id, void** argument) override { return folder.CopyTo(id, argument); }
    IFACEMETHODIMP GetCurrentViewMode(UINT*) override { return E_NOTIMPL; }
    IFACEMETHODIMP SetCurrentViewMode(UINT) override { return E_NOTIMPL; }
    IFACEMETHODIMP Item(int, PITEMID_CHILD*) override { return E_NOTIMPL; }
    IFACEMETHODIMP ItemCount(UINT, int*) override { return E_NOTIMPL; }
    IFACEMETHODIMP Items(UINT, REFIID, void**) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetSelectionMarkedItem(int*) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetFocusedItem(int*) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetItemPosition(PCUITEMID_CHILD, POINT*) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetSpacing(POINT*) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetDefaultSpacing(POINT*) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetAutoArrange() override { return E_NOTIMPL; }
    IFACEMETHODIMP SelectItem(int, DWORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP SelectAndPositionItems(UINT, PCUITEMID_CHILD_ARRAY, POINT*, DWORD) override { return E_NOTIMPL; }
};
ComPtr<IShellItem> Item(const std::wstring& path) { ComPtr<IShellItem> item; CheckHR(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item)), "Create shell item"); return item; }
ComPtr<IShellItemArray> Selection(IShellItem* item) { ComPtr<IShellItemArray> items; CheckHR(SHCreateShellItemArrayFromShellItem(item, IID_PPV_ARGS(&items)), "Create shell selection"); return items; }
int wmain(int argc, wchar_t** argv)
{
    const std::wstring registryRoot = L"Software\\Backslash.ExplorerIntegration.Tests\\Modern-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    struct Cleanup { std::wstring path; ~Cleanup() { RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str()); } } cleanup{registryRoot};
    try
    {
        Require(argc == 6, "Expected work, target executable, DLL, manifest and launch sink");
        CheckHR(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "COM init");
        struct ComCleanup { ~ComCleanup() { CoUninitialize(); } } comCleanup;
        const std::filesystem::path work(argv[1]);
        const auto a = work / L"copy A & \u65e5\u672c\u8a9e;literal"; const auto b = work / L"copy B";
        std::filesystem::create_directories(a); std::filesystem::create_directories(b);
        for (const auto& copy : { a, b }) { std::filesystem::copy_file(argv[2], copy / L"Backslash.exe"); std::ofstream(copy / L"TerminalApp.dll"); }
        const auto targetA = (a / L"Backslash.exe").wstring(), targetB = (b / L"Backslash.exe").wstring();
        TargetStore store{ HKEY_CURRENT_USER, registryRoot };
        Require(!store.Read().exists, "Starts absent");
        store.Set(targetA, work.wstring(), L""); Require(EqualPath(store.Read().target, targetA), "Enable target");
        store.Set(targetA, work.wstring(), targetA); Require(EqualPath(store.Read().target, targetA), "Idempotent enable");
        Reject([&] { store.Set(targetB, work.wstring(), L""); }); Require(EqualPath(store.Read().target, targetA), "Reject stale repoint");
        store.Set(targetB, work.wstring(), targetA); Require(EqualPath(store.Read().target, targetB), "Repoint");
        Reject([&] { store.Remove(targetA); }); Require(EqualPath(store.Read().target, targetB), "Different-copy removal guarded");
        Legacy::Key key; Check(RegOpenKeyExW(HKEY_CURRENT_USER, registryRoot.c_str(), 0, KEY_ALL_ACCESS, &key.value));
        const std::wstring embedded{L'a', L'\0', L'b'};
        struct Bad { DWORD kind; std::vector<BYTE> bytes; };
        const auto bytes = [](const std::wstring& text, bool terminator) { const auto first = reinterpret_cast<const BYTE*>(text.c_str()); return std::vector<BYTE>(first, first + (text.size() + (terminator ? 1 : 0)) * sizeof(wchar_t)); };
        const std::vector<Bad> bad{ {REG_EXPAND_SZ,bytes(Owner,true)}, {REG_DWORD,{1,0,0,0}}, {REG_BINARY,{1,2}}, {REG_MULTI_SZ,{0,0,0,0}}, {REG_SZ,{}}, {REG_SZ,{1}}, {REG_SZ,bytes(L"bad",false)}, {REG_SZ,bytes(embedded,true)} };
        size_t malformed = 0;
        for (const auto name : { L"Owner", L"TargetExecutable", L"SupportRoot", L"SchemaVersion" })
        {
            for (const auto& argument : bad)
            {
                Check(RegSetValueExW(key.value, name, 0, argument.kind, argument.bytes.data(), static_cast<DWORD>(argument.bytes.size())));
                const auto before = Snapshot(key.value); Reject([&] { store.Set(targetA, work.wstring(), targetB); }); Reject([&] { store.Remove(targetB); }); Require(Snapshot(key.value) == before, "Rejected configuration preserved");
                Legacy::Write(key.value, name, std::wstring(name) == L"Owner" ? Owner : std::wstring(name) == L"TargetExecutable" ? targetB : std::wstring(name) == L"SupportRoot" ? work.wstring() : L"1"); ++malformed;
            }
        }
        Legacy::Write(key.value, L"Unknown", L"preserve"); Reject([&] { store.Remove(targetB); }); Check(RegDeleteValueW(key.value, L"Unknown"));
        Legacy::Write(key.value, L"Owner", L"foreign"); Reject([&] { store.Set(targetA, work.wstring(), targetB); }); Reject([&] { store.Remove(targetB); }); Legacy::Write(key.value, L"Owner", Owner);
        Check(RegDeleteValueW(key.value, L"Owner")); Reject([&] { store.Remove(targetB); }); Legacy::Write(key.value, L"Owner", Owner);
        Legacy::Write(key.value, L"SchemaVersion", L"2"); Reject([&] { store.Set(targetA, work.wstring(), targetB); }); Reject([&] { store.Remove(targetB); }); Legacy::Write(key.value, L"SchemaVersion", L"1");
        Reject([&] { store.Set(targetA, (work / L"missing-support").wstring(), targetB); }); Require(store.Read().target == targetB, "Invalid support cannot repoint target");
        Legacy::Key child; Check(RegCreateKeyExW(key.value, L"foreign-child", 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &child.value, nullptr)); Reject([&] { store.Remove(targetB); }); Check(RegDeleteKeyW(key.value, L"foreign-child"));
        std::wcout << L"PASS: target enable/idempotency/repoint/expected-copy guard, " << malformed << L" malformed values, foreign/schema collisions preserved\n";
        const auto folder = work / L"folder & % ! ^ (x);new-tab" / L";\u65e5\u672c\u8a9e";
        std::filesystem::create_directories(folder); std::ofstream(work / L"file.txt");
        size_t launches = 0, errors = 0; LaunchRequest captured;
        auto command = Microsoft::WRL::Make<Command>(store, [&](const LaunchRequest& request) { ++launches; captured = request; }, [&](const std::wstring&) { ++errors; });
        const auto shellFolder = Item(folder.wstring()); const auto items = Selection(shellFolder.Get());
        EXPCMDSTATE state{}; CheckHR(command->GetState(items.Get(), FALSE, &state), "Selected state"); Require(state == ECS_ENABLED, "Selected folder enabled");
        CheckHR(command->Invoke(items.Get(), nullptr), "Selected invoke"); Require(launches == 1 && errors == 0 && captured.folder == folder.wstring() && captured.executable == targetB, "Selected exact folder and target");
        int count{}; auto parsed = CommandLineToArgvW(captured.command.c_str(), &count); Require(parsed && count == 3 && std::wstring(parsed[0]) == targetB && std::wstring(parsed[1]) == Legacy::Flag && std::wstring(parsed[2]) == folder.wstring(), "Literal argv boundaries"); LocalFree(parsed);
        auto site = Microsoft::WRL::Make<FolderSite>(shellFolder.Get()); ComPtr<IServiceProvider> provider; CheckHR(site.As(&provider), "Test provider"); CheckHR(command->SetSite(provider.Get()), "Set background site");
        CheckHR(command->GetState(nullptr, FALSE, &state), "Background state"); Require(state == ECS_ENABLED, "Background enabled"); CheckHR(command->Invoke(nullptr, nullptr), "Background invoke"); Require(launches == 2 && captured.folder == folder.wstring(), "Background exact folder");
        const auto file = Item((work / L"file.txt").wstring()); const auto fileItems = Selection(file.Get()); CheckHR(command->GetState(fileItems.Get(), FALSE, &state), "File state"); Require(state == ECS_HIDDEN, "Nonfolder hidden"); Require(FAILED(command->Invoke(fileItems.Get(), nullptr)) && launches == 2 && errors == 1, "Unsupported selected file reports failure without launch");
        PIDLIST_ABSOLUTE pidl{}; CheckHR(SHParseDisplayName(folder.c_str(), nullptr, &pidl, 0, nullptr), "Parse test PIDL"); PCIDLIST_ABSOLUTE ids[]{pidl,pidl}; ComPtr<IShellItemArray> multiple; CheckHR(SHCreateShellItemArrayFromIDLists(2, ids, &multiple), "Create multi selection"); CoTaskMemFree(pidl);
        CheckHR(command->GetState(multiple.Get(), FALSE, &state), "Multi state"); Require(state == ECS_HIDDEN, "Multiple folders hidden");
        Require(FAILED(command->Invoke(multiple.Get(), nullptr)) && launches == 2, "Multiple selection cannot fall back to background");
        std::filesystem::remove(targetB); Require(FAILED(command->Invoke(items.Get(), nullptr)) && launches == 2, "Deleted target has no fallback"); std::filesystem::copy_file(argv[2], targetB);
        Reject([&] { Prepare(store, (work / L"missing").wstring()); }); Reject([&] { Prepare(store, (work / L"file.txt").wstring()); });
        for (const auto unsupported : {L"relative",L"C:relative",L"shell:Downloads",L"\\\\?\\C:\\",L"\\\\.\\pipe\\x"}) { Reject([&] { ValidateTarget(unsupported); }); }
        for (const auto folderText : {L"C:\\",L"C:\\space\\",L"\\\\server\\share\\",L"C:\\a;new-tab\\;b"})
        { const auto cmd = LaunchCommand(targetA, folderText); parsed = CommandLineToArgvW(cmd.c_str(), &count); Require(parsed && count == 3 && std::wstring(parsed[2]) == folderText, "Root/trailing/UNC argv"); LocalFree(parsed); }
        std::wcout << L"PASS: production command selection/background COM contracts, files/multiselect rejection, deleted target no fallback, error capture, literal Unicode/metacharacter/root/UNC argv\n";
        const auto marker = folder / L"modern-launch-observed.json";
        Launch({ argv[5], LaunchCommand(argv[5], folder.wstring()), folder.wstring() });
        for (unsigned retry = 0; retry < 100 && !std::filesystem::exists(marker); ++retry) { Sleep(50); }
        Require(std::filesystem::exists(marker), "Owned launch sink completed");
        std::ifstream observed(marker); const std::string text{ std::istreambuf_iterator<char>(observed), std::istreambuf_iterator<char>() };
        Require(text.find("\"identity\":\"unpackaged\"") != std::string::npos, "Real launcher child is unpackaged");
        const auto expectedArgument = L"\"argument\":" + Json(folder.wstring()), expectedCwd = L"\"cwd\":" + Json(folder.wstring());
        Require(text.find(Ascii(expectedArgument)) != std::string::npos && text.find(Ascii(expectedCwd)) != std::string::npos, "Real launcher child argument and cwd preserved");
        std::wcout << L"PASS: real CreateProcess launcher, suspended child identity guard and owned child completion\n";
        HMODULE dll = LoadLibraryW(argv[3]); Require(dll != nullptr, "Load built DLL without registration");
        using GetFactory = HRESULT(__stdcall*)(REFCLSID, REFIID, void**); using CanUnload = HRESULT(__stdcall*)();
        const auto getFactory = reinterpret_cast<GetFactory>(GetProcAddress(dll, "DllGetClassObject")); const auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(dll, "DllCanUnloadNow")); Require(getFactory && canUnload, "COM exports");
        { ComPtr<IClassFactory> factory; CheckHR(getFactory(__uuidof(Command), IID_PPV_ARGS(&factory)), "Factory"); ComPtr<IExplorerCommand> loaded; CheckHR(factory->CreateInstance(nullptr,IID_PPV_ARGS(&loaded)), "DLL instance"); LPWSTR title{}; CheckHR(loaded->GetTitle(nullptr,&title), "DLL title"); Require(std::wstring(title) == L"Open in Backslash", "Independent label"); CoTaskMemFree(title); Require(canUnload() == S_FALSE, "DLL lifetime"); }
        Require(canUnload() == S_OK, "DLL released"); FreeLibrary(dll);
        const auto identity = IdentityJson(argv[4]); Require(identity.find(PackageName) != std::wstring::npos && identity.find(Publisher) != std::wstring::npos, "Manifest identity parsed by Windows");
        std::wcout << L"PASS: actual DLL factory/lifetime/title without registration; Windows Appx manifest reader validates independent identity\n";
        store.Remove(targetB); Require(!store.Read().exists, "Remove"); store.Remove(L"");
        // An owned partial record can be repaired or removed, but is not launched.
        Legacy::Key partial; Check(RegCreateKeyExW(HKEY_CURRENT_USER, registryRoot.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &partial.value, nullptr)); Legacy::Write(partial.value, L"Owner", Owner);
        Reject([&] { Prepare(store, folder.wstring()); }); store.Set(targetA, work.wstring(), L""); Require(store.Read().target == targetA, "Partial repair"); store.Remove(targetA);
        std::wcout << L"PASS: bounded owned remove/idempotency, partial repair, no package deployment or live target/Explorer changes\n";
        return 0;
    }
    catch (const Error& error) { std::cerr << error.what() << " HRESULT=" << std::hex << static_cast<unsigned>(error.code) << '\n'; return 1; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
