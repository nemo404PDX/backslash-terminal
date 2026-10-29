// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#include <windows.h>
#include <restartmanager.h>
#include <string>
#include <stdexcept>
#pragma comment(lib, "rstrtmgr.lib")
namespace Backslash::ModernExplorer::Management
{
    template<typename Restart> void AfterRestartConfirmation(bool confirmed, Restart&& restart) { if (confirmed) { restart(); } }
    // Called only by the separately confirmed troubleshooting action. Never by
    // status, enable, repair, removal or shell-change notification.
    inline void RestartExplorerGracefully()
    {
        const auto shell = GetShellWindow(); DWORD pid{};
        if (!shell || !GetWindowThreadProcessId(shell, &pid) || !pid) { throw std::runtime_error("The current desktop shell could not be identified. Restart Explorer manually in Task Manager if needed."); }
        const auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process) { throw std::runtime_error("Explorer cannot be inspected without additional permissions. No restart was attempted."); }
        struct Close { HANDLE h; ~Close() { CloseHandle(h); } } close{process};
        wchar_t image[32768]{}; DWORD length = 32768; wchar_t windows[32768]{};
        if (!QueryFullProcessImageNameW(process, 0, image, &length) || !GetWindowsDirectoryW(windows, 32768) || _wcsicmp(image, (std::wstring(windows) + L"\\explorer.exe").c_str()) != 0) { throw std::runtime_error("The desktop shell is not Windows Explorer. No restart was attempted."); }
        RM_UNIQUE_PROCESS application{}; application.dwProcessId = pid; FILETIME exit{}, kernel{}, user{};
        if (!GetProcessTimes(process, &application.ProcessStartTime, &exit, &kernel, &user)) { throw std::runtime_error("Explorer start time could not be verified. No restart was attempted."); }
        DWORD session{}; wchar_t key[CCH_RM_SESSION_KEY + 1]{};
        auto error = RmStartSession(&session, 0, key);
        if (error) { throw std::runtime_error("Windows Restart Manager could not start. No restart was attempted."); }
        struct End { DWORD session; ~End() { RmEndSession(session); } } end{session};
        error = RmRegisterResources(session, 0, nullptr, 1, &application, 0, nullptr);
        if (error) { throw std::runtime_error("Windows could not register this Explorer process for a safe restart."); }
        // Refuse if the process is not registered as restartable. Never force
        // shutdown, terminate a process, elevate or invoke taskkill.
        const auto shutdown = RmShutdown(session, RmShutdownOnlyRegistered, nullptr);
        const auto restart = RmRestart(session, 0, nullptr); // Required even after a partial shutdown error.
        if (shutdown || restart) { throw std::runtime_error("Windows could not complete a safe Explorer restart. Check the desktop and use Task Manager if needed. No forced termination was used."); }
    }
}
