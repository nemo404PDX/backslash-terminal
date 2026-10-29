// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#include "ModernExplorerPolicy.h"
#include "../../../tools/backslash/modern-explorer/TargetStore.h"
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <bcrypt.h>
#include <appmodel.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <map>
#include <array>
#pragma comment(lib, "bcrypt.lib")
namespace Backslash::ModernExplorer::Management
{
    namespace Legacy = ::Microsoft::Terminal::Backslash::ExplorerIntegration;
    inline constexpr wchar_t PackageName[] = L"Backslash.ExplorerIntegration.Dev";
    inline constexpr wchar_t Publisher[] = L"CN=Backslash Development";
    inline constexpr wchar_t PackageFamily[] = L"Backslash.ExplorerIntegration.Dev_whmwxsrdn33ha";
    inline constexpr wchar_t PackageFull[] = L"Backslash.ExplorerIntegration.Dev_0.0.1.0_x64__whmwxsrdn33ha";
    struct PayloadFile { const wchar_t* name; const wchar_t* hash; };
    inline constexpr std::array<PayloadFile, 8> Payload{{
        { L"AppxManifest.xml", L"6D13A92325CEDB0A7271BA63A97FF64BE8C769EB657695C0A4A33C70857165D4" },
        { L"Backslash.ExplorerHost.exe", L"95FC44280DBE1F6BBB615C6529A3F7D3C86ABDE21D48B948F442AF4294E2E681" },
        { L"Backslash.ExplorerCommand.dll", L"103D4511C02D1AB6F4861EA40A99ED8126C216C4EC3148B21CF6696FA5F0CE2F" },
        { L"LICENSE", L"5D177F23ECFEB0EA8E050B6A5A16355E1AE9A0B286436CA8F83ED08B3795BE6B" },
        { L"NOTICE.md", L"E7FBAADEE6AB20C28B87730A510EE5F5815D8FB4BD88D1D54D282DC2A74C0726" },
        { L"Assets\\Logo150.png", L"996836C2BF14CBC999B982380A1A5CF7EEDE74702FD9DFA2A807B3C2DAF8ECE7" },
        { L"Assets\\Logo44.png", L"450973D4A88AC51ED633DAD9CA1E256AEB23192AAE8ED1D5431D66AB5B3F8319" },
        { L"Assets\\StoreLogo.png", L"3B9B8A26DD1A8EFFCF37F53B0D61F1F04CB26AB0D0A4B3D96C4970A502295E9F" }
    }};
    inline void NoReparse(std::filesystem::path path)
    {
        for (; !path.empty();)
        {
            const auto attributes = GetFileAttributesW(path.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) { throw Error(E_ACCESSDENIED, "Reparse paths are not supported; nothing adopted or overwritten"); }
            if (attributes == INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Cannot inspect integration path"); }
            const auto parent = path.parent_path(); if (parent == path) { break; } path = parent;
        }
    }
    inline bool Exists(const std::filesystem::path& path) { NoReparse(path); return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
    inline std::wstring Hash(const std::filesystem::path& path)
    {
        NoReparse(path);
        const auto file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Cannot read reviewed helper file"); }
        struct Close { HANDLE h; ~Close() { CloseHandle(h); } } close{file};
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 16 * 1024 * 1024) { throw Error(E_INVALIDARG, "Helper file size is invalid"); }
        std::vector<BYTE> bytes(static_cast<size_t>(size.QuadPart)); DWORD count{};
        if (!ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr) || count != bytes.size()) { throw Error(E_FAIL, "Helper file changed during read"); }
        BCRYPT_ALG_HANDLE algorithm{};
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) { throw Error(E_FAIL, "SHA256 unavailable"); }
        struct CloseAlgorithm { BCRYPT_ALG_HANDLE h; ~CloseAlgorithm() { BCryptCloseAlgorithmProvider(h, 0); } } closeAlgorithm{algorithm};
        BYTE digest[32]{};
        if (BCryptHash(algorithm, nullptr, 0, bytes.data(), static_cast<ULONG>(bytes.size()), digest, sizeof(digest)) < 0) { throw Error(E_FAIL, "SHA256 failed"); }
        std::wstring text; for (const auto value : digest) { wchar_t part[3]; swprintf_s(part, L"%02X", value); text += part; } return text;
    }
    inline void RequireHash(const std::filesystem::path& path, const wchar_t* hash) { if (Hash(path) != hash) { throw Error(E_ACCESSDENIED, "Helper file differs from the reviewed payload; preserved"); } }
    inline bool Files(const std::filesystem::path& root, bool ownedDevelopmentPackage = false)
    {
        if (!Exists(root)) { return false; }
        bool complete = true;
        for (const auto& entry : std::filesystem::directory_iterator(root))
        {
            NoReparse(entry.path());
            const auto relative = entry.path().filename().wstring();
            // Windows adds this hidden directory to a loose registration. Only
            // acknowledge it after exact package/location/development checks.
            // Never traverse, copy or remove its Windows-owned contents.
            if (ownedDevelopmentPackage && Same(relative, L"microsoft.system.package.metadata") && entry.is_directory()) { continue; }
            if (relative == L"Assets" && entry.is_directory())
            {
                for (const auto& asset : std::filesystem::directory_iterator(entry.path()))
                {
                    NoReparse(asset.path()); bool known = false;
                    for (const auto& file : Payload) { if (L"Assets\\" + asset.path().filename().wstring() == file.name && !asset.is_directory()) { known = true; } }
                    if (!known) { throw Error(E_ACCESSDENIED, "Unknown support content; preserved"); }
                }
            }
            else
            {
                bool known = false; for (const auto& file : Payload) { if (relative == file.name && !entry.is_directory()) { known = true; } }
                if (!known) { throw Error(E_ACCESSDENIED, "Unknown support content; preserved"); }
            }
        }
        for (const auto& file : Payload) { const auto path = root / file.name; if (Exists(path)) { RequireHash(path, file.hash); } else { complete = false; } }
        return complete;
    }
    inline void ValidateText(const std::wstring& text)
    {
        if (!text.empty() && !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr)) { throw Error(E_ACCESSDENIED, "Invalid UTF-16 target value; preserved"); }
    }
    inline std::map<std::wstring, std::wstring> ReadTargetValues()
    {
        const auto config = TargetStore{}.Read(); if (!config.exists) { return {}; }
        Legacy::Key key; Check(RegOpenKeyExW(HKEY_CURRENT_USER, ProductionKey, 0, KEY_READ, &key.value));
        std::map<std::wstring, std::wstring> result;
        for (DWORD i = 0;; ++i)
        {
            wchar_t name[256]; DWORD length = 256; const auto code = RegEnumValueW(key.value, i, name, &length, nullptr, nullptr, nullptr, nullptr); if (code == ERROR_NO_MORE_ITEMS) { break; } Check(code);
            const std::wstring valueName(name, length);
            if (valueName != L"Owner" && valueName != L"SchemaVersion" && valueName != L"TargetExecutable" && valueName != L"SupportRoot") { throw Error(E_ACCESSDENIED, "Unknown target value; preserved"); }
            const auto value = Legacy::Read(key.value, name); ValidateText(value); result.emplace(valueName, value);
        }
        DWORD children{}; Check(RegQueryInfoKeyW(key.value, nullptr, nullptr, nullptr, &children, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
        if (children || !result.contains(L"Owner") || result.at(L"Owner") != Owner || (result.contains(L"SchemaVersion") && !result.at(L"SchemaVersion").empty() && result.at(L"SchemaVersion") != L"1")) { throw Error(E_ACCESSDENIED, "Target ownership/schema changed; preserved"); }
        return result;
    }
    inline std::wstring FileUri(const std::filesystem::path& path)
    {
        std::wstring uri(100000, L'\0'); DWORD length = static_cast<DWORD>(uri.size());
        winrt::check_hresult(UrlCreateFromPathW(path.c_str(), uri.data(), &length, 0)); uri.resize(wcslen(uri.c_str())); return uri;
    }
    inline std::wstring DescribeError() noexcept
    {
        try { throw; }
        catch (const winrt::hresult_error& error) { wchar_t code[11]{}; swprintf_s(code, L"0x%08X", static_cast<unsigned>(error.code().value)); return std::wstring(error.message()) + L" (HRESULT " + code + L")"; }
        catch (const Error& error) { wchar_t code[11]{}; swprintf_s(code, L"0x%08X", static_cast<unsigned>(error.code)); const std::string text = error.what(); return std::wstring(text.begin(), text.end()) + L" (HRESULT " + code + L")"; }
        catch (const std::exception& error) { const std::string text = error.what(); return { text.begin(), text.end() }; }
        catch (...) { return L"Unknown integration error. Check status before trying again."; }
    }
    class NativeBackend final : public Backend
    {
        std::wstring _current;
        std::filesystem::path _support, _bundle;
        std::map<std::wstring, std::wstring> _beforeValues;
        std::vector<std::filesystem::path> _createdFiles, _createdDirectories;
        State _before;
        bool _changed{}, _registrationRequested{}, _targetAttempted{};
        std::wstring _selected;
        std::vector<winrt::Windows::ApplicationModel::Package> Packages()
        {
            using namespace winrt::Windows::Management::Deployment;
            std::vector<winrt::Windows::ApplicationModel::Package> result;
            for (const auto& package : PackageManager{}.FindPackagesForUser(L""))
            {
                const auto id = package.Id();
                if (id.Name() != PackageName) { continue; }
                if (id.Name() != PackageName || id.Publisher() != Publisher || id.FamilyName() != PackageFamily || id.FullName() != PackageFull || !package.IsDevelopmentMode() || !Same(package.InstalledLocation().Path().c_str(), _support.wstring())) { throw Error(E_ACCESSDENIED, "A different package identity, install location or deployment type exists; preserved"); }
                NoReparse(package.InstalledLocation().Path().c_str()); result.push_back(package);
            }
            if (result.size() > 1) { throw Error(E_ACCESSDENIED, "Multiple helper registrations exist; preserved"); } return result;
        }
        void Directory(const std::filesystem::path& path)
        {
            NoReparse(path); if (Exists(path)) { return; }
            if (!CreateDirectoryW(path.c_str(), nullptr)) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Support directory appeared or could not be created; stopped"); }
            _createdDirectories.push_back(path); _changed = true; NoReparse(path);
        }
        void RestoreTarget()
        {
            const auto current = ReadTargetValues();
            const std::map<std::wstring, std::wstring> selected{{L"Owner", Owner},{L"SchemaVersion", L"1"},{L"TargetExecutable", _selected},{L"SupportRoot", _support.wstring()}};
            for (const auto& [name, value] : current)
            {
                const auto old = _beforeValues.find(name);
                const auto now = selected.find(name);
                if ((old == _beforeValues.end() || old->second != value) && (now == selected.end() || now->second != value)) { throw Error(E_ACCESSDENIED, "Target changed outside this operation; preserved"); }
            }
            if (_beforeValues.empty()) { if (!current.empty()) { TargetStore{}.Remove(TargetStore{}.Read().target); } return; }
            if (current.empty()) { throw Error(E_ACCESSDENIED, "Previous target disappeared; preserved"); }
            Legacy::Key key; Check(RegOpenKeyExW(HKEY_CURRENT_USER, ProductionKey, 0, KEY_SET_VALUE, &key.value));
            for (const auto& [name, value] : _beforeValues) { Legacy::Write(key.value, name.c_str(), value); }
            for (const auto& [name, value] : current) { (void)value; if (!_beforeValues.contains(name)) { Check(RegDeleteValueW(key.value, name.c_str())); } }
        }
    public:
        explicit NativeBackend(std::wstring current) : _current(std::move(current))
        {
            PWSTR local{}; winrt::check_hresult(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local));
            _support = std::filesystem::path(local) / L"Backslash" / L"ExplorerIntegration" / L"Dev"; CoTaskMemFree(local);
            _bundle = std::filesystem::path(_current).parent_path() / L"ExplorerIntegration";
        }
        State Read() override
        {
            State state; state.current = _current; state.support = _support.wstring(); state.details = L"Support: " + _support.wstring() + L"\nThis copy: " + _current + L"\n";
            try
            {
                UINT32 length{}; const bool unpackaged = GetCurrentPackageFullName(&length, nullptr) == APPMODEL_ERROR_NO_PACKAGE;
                wchar_t build[32]{}; DWORD size = sizeof(build);
                const auto buildResult = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, build, &size);
                state.supported = unpackaged && buildResult == ERROR_SUCCESS && wcstoul(build, nullptr, 10) >= 22000 && Exists(std::filesystem::path(_current).parent_path() / L".portable");
#if !defined(_M_AMD64)
                state.supported = false;
#endif
                if (!state.supported) { state.problem = L"This development integration supports unpackaged x64 portable builds on Windows 11. Signed end-user deployment is not included."; }
                DWORD policy{}, policySize = sizeof(policy);
                state.developerMode = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", L"AllowDevelopmentWithoutDevLicense", RRF_RT_REG_DWORD, nullptr, &policy, &policySize) == ERROR_SUCCESS && policy == 1;
                const auto config = TargetStore{}.Read(); const auto values = ReadTargetValues(); state.config = config.exists; state.target = config.target;
                if (!config.support.empty() && !Same(config.support, _support.wstring())) { throw Error(E_ACCESSDENIED, "Target configuration uses a different support location; preserved"); }
                state.configComplete = values.size() == 4 && !config.target.empty() && Same(config.support, _support.wstring()) && values.at(L"SchemaVersion") == L"1";
                state.package = !Packages().empty(); state.supportExists = Exists(_support); state.filesComplete = Files(_support, state.package);
                if (state.supportExists && !state.package && !state.config && !Exists(_support / L"AppxManifest.xml")) { throw Error(E_ACCESSDENIED, "Unmarked partial support directory; preserved"); }
                try { state.bundle = Files(_bundle); } catch (...) { state.details += L"Payload: " + DescribeError() + L"\n"; }
                if (!state.target.empty()) { try { Validate(state.target); state.targetValid = true; } catch (...) { state.details += L"Selected copy: " + DescribeError() + L"\n"; } }
                Validate(_current);
                state.details += L"Development helper: " + std::wstring(PackageFull) + L"\nRegistration: " + (state.package ? L"verified current-user loose development package" : L"not registered") + L"\nSelected copy: " + (state.target.empty() ? L"none" : state.target) + L"\nPayload: " + _bundle.wstring();
                if (!state.bundle && !state.filesComplete) { state.problem = L"The reviewed helper payload is missing or incomplete. Restore this complete portable build folder before enabling or repairing integration."; }
                else if (state.package && (!state.filesComplete || !state.configComplete || !state.targetValid)) { state.problem = L"Modern integration needs repair. Use this copy to replace an unavailable selected copy."; }
                else if (!state.package && !state.developerMode) { state.problem = L"Installing this development helper requires Developer Mode already enabled. This build does not install a signed distribution or change Windows policy."; }
            }
            catch (...) { state.safe = false; state.problem = DescribeError(); }
            try
            {
                size_t owned{}, foreign{};
                for (const auto& entry : Legacy::Registration{}.Inspect(_current)) { if (entry.exists) { if (entry.owned) { ++owned; } else { ++foreign; } } }
                state.legacy = L"Classic menu entries (Show more options): " + std::to_wstring(owned) + L" owned, " + std::to_wstring(foreign) + L" foreign. Modern actions leave these unchanged.";
            }
            catch (...) { state.legacy = L"Classic entries could not be inspected. They are left unchanged."; }
            return state;
        }
        void Begin(const State& state) override
        {
            const auto now = Read();
            if (!now.safe || !Same(now.target, state.target) || now.package != state.package || now.supportExists != state.supportExists) { throw Error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "Integration changed; check status and confirm again"); }
            _before = now; _beforeValues = ReadTargetValues();
        }
        void Validate(const std::wstring& target) override { NoReparse(target); NoReparse(std::filesystem::path(target).parent_path() / L"TerminalApp.dll"); (void)ValidateTarget(target); }
        void InstallFiles() override
        {
            if (!Files(_bundle)) { throw Error(E_INVALIDARG, "Reviewed helper payload is incomplete"); }
            (void)Files(_support, !Packages().empty());
            Directory(_support.parent_path().parent_path()); Directory(_support.parent_path()); Directory(_support); Directory(_support / L"Assets");
            for (const auto& file : Payload)
            {
                const auto destination = _support / file.name; const auto source = _bundle / file.name; RequireHash(source, file.hash);
                if (!Exists(destination)) { if (!CopyFileW(source.c_str(), destination.c_str(), TRUE)) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Copy failed or destination appeared; stopped"); } _createdFiles.push_back(destination); _changed = true; }
                RequireHash(destination, file.hash);
            }
        }
        void Register() override
        {
            if (!Packages().empty()) { throw Error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "Package appeared before registration; stopped"); }
            if (!Files(_support)) { throw Error(E_INVALIDARG, "Support payload is incomplete"); }
            using namespace winrt::Windows::Management::Deployment;
            RegisterPackageOptions options; options.DeveloperMode(true); options.ExternalLocationUri(winrt::Windows::Foundation::Uri(FileUri(_support.wstring() + L"\\")));
            _registrationRequested = true;
            const auto result = PackageManager{}.RegisterPackageByUriAsync(winrt::Windows::Foundation::Uri(FileUri(_support / L"AppxManifest.xml")), options).get();
            if (result.ExtendedErrorCode() < 0) { throw winrt::hresult_error(result.ExtendedErrorCode(), result.ErrorText()); }
            if (Packages().empty()) { throw Error(E_FAIL, "Windows did not expose the expected registration"); } _changed = true;
        }
        void Select(const std::wstring& target, const std::wstring& expected) override
        {
            (void)ReadTargetValues();
            const auto current = TargetStore{}.Read();
            if (!current.support.empty() && !Same(current.support, _support.wstring())) { throw Error(E_ACCESSDENIED, "Target support location changed; preserved"); }
            _selected = target; _targetAttempted = true;
            _changed = true; TargetStore{}.Set(target, _support.wstring(), expected);
        }
        void RemovePackage() override
        {
            const auto packages = Packages(); if (packages.empty()) { return; }
            RequireHash(_support / L"AppxManifest.xml", Payload[0].hash); (void)Files(_support, true);
            const auto result = winrt::Windows::Management::Deployment::PackageManager{}.RemovePackageAsync(PackageFull).get();
            if (result.ExtendedErrorCode() < 0) { throw winrt::hresult_error(result.ExtendedErrorCode(), result.ErrorText()); }
            if (!Packages().empty()) { throw Error(E_FAIL, "Windows still reports the package; files preserved"); } _changed = true;
        }
        void RemoveTarget(const std::wstring& expected) override
        {
            (void)ReadTargetValues();
            const auto current = TargetStore{}.Read();
            if (!current.support.empty() && !Same(current.support, _support.wstring())) { throw Error(E_ACCESSDENIED, "Target support location changed; preserved"); }
            TargetStore{}.Remove(expected); _changed = true;
        }
        void RemoveFiles() override
        {
            if (!Packages().empty()) { throw Error(E_ACCESSDENIED, "Package remains registered; files preserved"); }
            (void)Files(_support, !Packages().empty());
            for (const auto& file : Payload) { const auto path = _support / file.name; if (Exists(path)) { RequireHash(path, file.hash); if (!DeleteFileW(path.c_str())) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "File removal failed; remaining files preserved"); } _changed = true; } }
            for (const auto& directory : { _support / L"Assets", _support }) { if (Exists(directory) && !RemoveDirectoryW(directory.c_str())) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Support directory is not empty or could not be removed; preserved"); } }
        }
        void Rollback() override
        {
            (void)Files(_support, !Packages().empty());
            if (_targetAttempted) { RestoreTarget(); }
            if (!_before.package && _registrationRequested) { RemovePackage(); }
            for (const auto& path : _createdFiles) { const PayloadFile* known = nullptr; for (const auto& file : Payload) { if (path == _support / file.name) { known = &file; } } if (!known) { throw Error(E_ACCESSDENIED, "Unknown rollback file"); } RequireHash(path, known->hash); if (!DeleteFileW(path.c_str())) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Rollback file removal failed"); } }
            for (auto it = _createdDirectories.rbegin(); it != _createdDirectories.rend(); ++it) { NoReparse(*it); if (!RemoveDirectoryW(it->c_str())) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Rollback directory is not empty; preserved"); } }
        }
        bool Changed() const override { return _changed; }
        void Notify() noexcept override { SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, nullptr, nullptr); }
        std::wstring FailureMessage() noexcept override { return DescribeError(); }
    };
}
