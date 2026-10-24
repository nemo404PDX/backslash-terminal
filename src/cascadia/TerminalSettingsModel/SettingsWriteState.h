// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "PinnedStore.h"

namespace Microsoft::Terminal::Backslash::SetupTransfer
{
    struct SettingsFileSnapshot
    {
        bool exists = false;
        std::string bytes;
        FILETIME lastWriteTime{};
        bool SameContents(const SettingsFileSnapshot& other) const noexcept { return exists == other.exists && bytes == other.bytes; }
        static SettingsFileSnapshot Read(const std::filesystem::path& path)
        {
            wil::unique_hfile file{ CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
            if (!file)
            {
                if (GetLastError() == ERROR_FILE_NOT_FOUND) { return {}; }
                THROW_LAST_ERROR();
            }
            LARGE_INTEGER size{}; THROW_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &size));
            if (size.QuadPart < 0 || size.QuadPart > 64 * 1024 * 1024) { throw std::invalid_argument("Settings file exceeds the read limit."); }
            SettingsFileSnapshot result{ true, std::string(static_cast<size_t>(size.QuadPart), '\0'), {} };
            DWORD count = 0;
            THROW_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), result.bytes.data(), static_cast<DWORD>(result.bytes.size()), &count, nullptr));
            if (count != result.bytes.size()) { throw std::runtime_error("Settings changed while reading; reload and retry."); }
            THROW_IF_WIN32_BOOL_FALSE(GetFileTime(file.get(), nullptr, nullptr, &result.lastWriteTime));
            return result;
        }
    };

    // A model's exact disk baseline travels with its editable Copy. Successful
    // writes advance only that model, so repeated Save works while older windows
    // and automatic load/fixup work cannot overwrite a committed import.
    class SettingsWriteState
    {
        std::optional<SettingsFileSnapshot> _baseline;
    public:
        SettingsWriteState() = default;
        explicit SettingsWriteState(SettingsFileSnapshot baseline) : _baseline{ std::move(baseline) } {}
        static SettingsFileSnapshot Capture(const std::filesystem::path& path, const std::function<void(const SettingsFileSnapshot&)>& whileLocked = {})
        {
            return Pins::Store::WithSetupLock(path.parent_path(), [&] {
                auto snapshot = SettingsFileSnapshot::Read(path);
                if (whileLocked) { whileLocked(snapshot); }
                return snapshot;
            });
        }
        template<typename F> static auto RetryAutomaticLoad(F&& attempt)
        {
            for (int count = 0; count < 3; ++count)
            {
                auto current = attempt();
                if (current) { return std::move(*current); }
            }
            throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
                L"Settings kept changing while loading. No older settings were written. Retry loading Settings.");
        }
        bool MatchesCurrent(const std::filesystem::path& path) const
        {
            return Pins::Store::WithSetupLock(path.parent_path(), [&] {
                return _baseline && _baseline->SameContents(SettingsFileSnapshot::Read(path));
            });
        }
        void AdoptIfUnchanged(const SettingsWriteState& before, const SettingsWriteState& after)
        {
            if (!_baseline || !before._baseline || !after._baseline || !_baseline->SameContents(*before._baseline))
            {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), L"The Settings draft baseline changed; reload before merging saved Groups.");
            }
            _baseline = after._baseline;
        }
        FILETIME Commit(const std::filesystem::path& path, std::string_view contents)
        {
            return Pins::Store::WithSetupLock(path.parent_path(), [&] {
                const auto actual = SettingsFileSnapshot::Read(path);
                if (!_baseline || !_baseline->SameContents(actual))
                {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
                        L"Settings changed on disk after this draft was loaded. Your draft was kept and nothing was saved. Reload or Reset the Settings page, then reapply your changes.");
                }
                FILETIME written{};
                til::io::write_utf8_string_to_file_atomic(path, contents, &written);
                _baseline = SettingsFileSnapshot{ true, std::string{ contents }, written };
                return written;
            });
        }
    };
}
