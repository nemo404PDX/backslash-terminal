// Copyright (c) Backslash contributors. Licensed under the MIT license.
// Selection/site resolution follows the fork's ShellExtension/OpenTerminalHere.
#pragma once
#include "TargetStore.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl.h>
#include <atomic>
#include <appmodel.h>

namespace Backslash::ModernExplorer
{
    inline std::atomic<long> Objects{}, Locks{};
    using Launcher = std::function<void(const LaunchRequest&)>;
    using ErrorSink = std::function<void(const std::wstring&)>;
    inline std::wstring Widen(const char* text)
    {
        const auto count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        std::wstring result(static_cast<size_t>(count), L'\0');
        if (count) { MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), count); result.pop_back(); }
        return result;
    }
    inline void Launch(const LaunchRequest& request)
    {
        STARTUPINFOW startup{ sizeof(startup) }; startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_SHOWNORMAL;
        PROCESS_INFORMATION process{}; auto command = request.command;
        if (!CreateProcessW(request.executable.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED, nullptr, request.folder.c_str(), &startup, &process))
        { throw Error(HRESULT_FROM_WIN32(GetLastError()), "CreateProcess failed for the selected Backslash copy"); }
        struct Close { PROCESS_INFORMATION process; ~Close() { CloseHandle(process.hThread); CloseHandle(process.hProcess); } } close{process};
        // Never allow helper package identity to silently change portable storage.
        // Inspect only this newly created, still-suspended child before its code runs.
        UINT32 length{};
        if (GetPackageFullName(process.hProcess, &length, nullptr) != APPMODEL_ERROR_NO_PACKAGE)
        { TerminateProcess(process.hProcess, 1); throw Error(E_UNEXPECTED, "Launch refused because the terminal inherited or has package identity"); }
        if (ResumeThread(process.hThread) == static_cast<DWORD>(-1))
        { const auto code = GetLastError(); TerminateProcess(process.hProcess, 1); throw Error(HRESULT_FROM_WIN32(code), "Could not resume the new terminal process"); }
    }
    inline void ShowError(const std::wstring& error)
    {
        MessageBoxW(nullptr, (L"Open in Backslash failed.\n\n" + error + L"\n\nInspect the selected target and repair or choose another copy with Backslash.ExplorerHost.exe.").c_str(), L"Backslash Explorer integration", MB_OK | MB_ICONERROR);
    }
    template<typename F> HRESULT Guard(F&& action) noexcept
    {
        try { return action(); }
        catch (const Error& error) { return error.code; }
        catch (const std::exception&) { return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }
        catch (...) { return E_FAIL; }
    }

    struct __declspec(uuid("35af07f8-0746-4fba-9e2f-f1a1b1231dfa")) Command final :
        Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IExplorerCommand, IObjectWithSite>
    {
        TargetStore store;
        Launcher launcher;
        ErrorSink errors;
        Microsoft::WRL::ComPtr<IUnknown> site;
        Command(TargetStore selected = TargetStore{}, Launcher launch = Launch, ErrorSink sink = ShowError) : store(std::move(selected)), launcher(std::move(launch)), errors(std::move(sink)) { ++Objects; }
        ~Command() { --Objects; }

        HRESULT Location(IShellItemArray* items, IShellItem** location)
        {
            if (!location) { return E_POINTER; } *location = nullptr;
            if (items)
            {
                DWORD count{}; const auto result = items->GetCount(&count); if (FAILED(result)) { return result; }
                if (count > 1) { return S_FALSE; }
                if (count == 1) { return items->GetItemAt(0, location); }
            }
            if (!site) { return S_FALSE; }
            Microsoft::WRL::ComPtr<IServiceProvider> provider;
            auto result = site.As(&provider); if (FAILED(result)) { return result; }
            Microsoft::WRL::ComPtr<IFolderView> view;
            result = provider->QueryService(SID_SFolderView, IID_PPV_ARGS(&view)); if (FAILED(result)) { return result; }
            if (!view) { return S_FALSE; }
            return view->GetFolder(IID_PPV_ARGS(location));
        }
        static bool Folder(IShellItem* item)
        {
            SFGAOF attributes{};
            return item && SUCCEEDED(item->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM, &attributes)) &&
                (attributes & SFGAO_FILESYSTEM) && (attributes & SFGAO_FOLDER) && !(attributes & SFGAO_STREAM);
        }
        IFACEMETHODIMP GetTitle(IShellItemArray*, LPWSTR* title) noexcept override { return title ? SHStrDupW(L"Open in Backslash", title) : E_POINTER; }
        IFACEMETHODIMP GetToolTip(IShellItemArray*, LPWSTR* tip) noexcept override { return tip ? SHStrDupW(L"Open this folder in the explicitly selected Backslash copy", tip) : E_POINTER; }
        IFACEMETHODIMP GetIcon(IShellItemArray*, LPWSTR* icon) noexcept override
        {
            if (!icon) { return E_POINTER; } *icon = nullptr;
            return Guard([&]() -> HRESULT { const auto config = store.Read(); if (config.target.empty()) { return S_FALSE; } return SHStrDupW((Legacy::Quote(config.target) + L",0").c_str(), icon); });
        }
        IFACEMETHODIMP GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state) noexcept override
        {
            if (!state) { return E_POINTER; } *state = ECS_HIDDEN;
            return Guard([&]() -> HRESULT {
                Microsoft::WRL::ComPtr<IShellItem> item;
                const auto result = Location(items, &item); if (result != S_OK || !Folder(item.Get())) { return S_OK; }
                // Only a bounded registry read here. Filesystem/UNC validation and process work occur in Invoke.
                *state = ECS_DISABLED;
                const auto config = store.Read(); if (config.exists && !config.target.empty()) { *state = ECS_ENABLED; }
                return S_OK;
            });
        }
        IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx*) noexcept override
        {
            const auto result = Guard([&]() -> HRESULT {
                Microsoft::WRL::ComPtr<IShellItem> item; const auto resolved = Location(items, &item);
                if (resolved != S_OK || !Folder(item.Get())) { throw Error(E_INVALIDARG, "Select exactly one filesystem folder or a folder background"); }
                struct Name { PWSTR text{}; ~Name() { CoTaskMemFree(text); } } path;
                const auto named = item->GetDisplayName(SIGDN_FILESYSPATH, &path.text);
                if (FAILED(named)) { throw Error(named, "Selected location has no filesystem path"); }
                if (!path.text) { throw Error(E_INVALIDARG, "Selected location returned an empty filesystem path"); }
                launcher(Prepare(store, path.text)); return S_OK;
            });
            if (FAILED(result))
            {
                // No throw may cross COM, including a failing diagnostic callback.
                try { wchar_t code[16]; swprintf_s(code, L"0x%08X", static_cast<unsigned>(result)); errors(L"Folder validation, target configuration or launch failed (" + std::wstring(code) + L"). The target may be missing, incompatible or inaccessible."); } catch (...) {}
            }
            return result;
        }
        IFACEMETHODIMP GetCanonicalName(GUID* id) noexcept override { if (!id) { return E_POINTER; } *id = __uuidof(Command); return S_OK; }
        IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags) noexcept override { if (!flags) { return E_POINTER; } *flags = ECF_DEFAULT; return S_OK; }
        IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** commands) noexcept override { if (!commands) { return E_POINTER; } *commands = nullptr; return E_NOTIMPL; }
        IFACEMETHODIMP SetSite(IUnknown* object) noexcept override { site = object; return S_OK; }
        IFACEMETHODIMP GetSite(REFIID id, void** object) noexcept override { if (!object) { return E_POINTER; } *object = nullptr; return site ? site.CopyTo(id, object) : E_FAIL; }
    };
}
