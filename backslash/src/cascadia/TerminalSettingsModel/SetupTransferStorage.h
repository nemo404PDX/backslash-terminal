// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "PinnedStore.h"

namespace Microsoft::Terminal::Backslash::SetupTransfer
{
    struct DiskSnapshot
    {
        std::string settings;
        bool settingsExists = false;
        Pins::Catalog pins;
        std::vector<Pins::Destination> attachedPins;
    };
    struct ApplyResult
    {
        std::filesystem::path backupDirectory;
        bool changed = false;
    };

    // Settings and Pins writers cooperate through pins.lock. The journal fences
    // readers after a crash; recovery never guesses over externally edited files.
    class Storage
    {
        struct File
        {
            bool exists = false;
            std::string bytes;
            bool operator==(const File&) const = default;
        };
        std::filesystem::path _root;
        std::function<void(const std::filesystem::path&, std::string_view)> _writer;
        std::function<void(const std::filesystem::path&)> _remove;
        std::function<Pins::Life(const Pins::Owner&)> _probe;

        static File Read(const std::filesystem::path& path)
        {
            wil::unique_hfile file{ CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
            if (!file)
            {
                if (GetLastError() == ERROR_FILE_NOT_FOUND) { return {}; }
                THROW_LAST_ERROR();
            }
            LARGE_INTEGER size{}; THROW_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &size));
            if (size.QuadPart < 0 || size.QuadPart > 64 * 1024 * 1024) { throw std::invalid_argument("Setup file exceeds the size limit."); }
            File result{ true, std::string(static_cast<size_t>(size.QuadPart), '\0') };
            DWORD count = 0;
            THROW_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), result.bytes.data(), static_cast<DWORD>(result.bytes.size()), &count, nullptr));
            if (count != result.bytes.size()) { throw std::runtime_error("Setup file changed while reading; retry."); }
            return result;
        }
        static Json::Value Parse(std::string_view text)
        {
            Json::CharReaderBuilder builder; builder["rejectDupKeys"] = true; builder["failIfExtra"] = true; builder["stackLimit"] = 128;
            std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
            Json::Value value; std::string errors;
            if (!reader->parse(text.data(), text.data() + text.size(), &value, &errors)) { throw std::invalid_argument("Invalid setup recovery journal."); }
            return value;
        }
        static Json::Value Encode(const File& file)
        {
            Json::Value value{ Json::objectValue }; value["exists"] = file.exists; value["bytes"] = file.bytes; return value;
        }
        static File Decode(const Json::Value& value)
        {
            Pins::Catalog::Keys(value, { "exists", "bytes" });
            if (!value["exists"].isBool() || !value["bytes"].isString()) { throw std::invalid_argument("Invalid setup recovery journal file entry."); }
            return { value["exists"].asBool(), value["bytes"].asString() };
        }
        void Restore(const std::filesystem::path& path, const File& file) const
        {
            if (file.exists) { _writer(path, file.bytes); } else { _remove(path); }
        }
        static DiskSnapshot Snapshot(const File& settings, const Pins::Snapshot& pins)
        {
            DiskSnapshot result{ settings.bytes, settings.exists, pins.catalog, {} };
            for (const auto& pin : pins.catalog.pins) { if (pins.leases.contains(pin.id)) { result.attachedPins.emplace_back(pin); } }
            return result;
        }
    public:
        // Flush bytes before replacing the destination and request write-through
        // rename, so a journal acknowledgement precedes either configuration write.
        static void DurableWrite(const std::filesystem::path& path, std::string_view bytes)
        {
            std::filesystem::create_directories(path.parent_path());
            const auto temporary = path.parent_path() / (L".setup-" + std::wstring{ winrt::to_hstring(Pins::NewId()) } + L".tmp");
            auto cleanup = wil::scope_exit([&] { DeleteFileW(temporary.c_str()); });
            {
                wil::unique_hfile file{ CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr) };
                THROW_LAST_ERROR_IF(!file);
                if (bytes.size() > MAXDWORD) { throw std::invalid_argument("Setup file exceeds the size limit."); }
                DWORD count = 0;
                THROW_IF_WIN32_BOOL_FALSE(WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr));
                if (count != bytes.size()) { throw std::runtime_error("Incomplete setup file write."); }
                THROW_IF_WIN32_BOOL_FALSE(FlushFileBuffers(file.get()));
            }
            THROW_IF_WIN32_BOOL_FALSE(MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH));
        }
        static void Remove(const std::filesystem::path& path)
        {
            if (!DeleteFileW(path.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) { THROW_LAST_ERROR(); }
        }
        explicit Storage(std::filesystem::path root,
                         std::function<void(const std::filesystem::path&, std::string_view)> writer = DurableWrite,
                         std::function<void(const std::filesystem::path&)> remove = Remove,
                         std::function<Pins::Life(const Pins::Owner&)> probe = Pins::Probe) :
            _root{ std::move(root) }, _writer{ std::move(writer) }, _remove{ std::move(remove) }, _probe{ std::move(probe) } {}

        DiskSnapshot Capture() const
        {
            return Pins::Store{ _root, _probe }.WithLockedSnapshot([&](const auto& pins) {
                return Snapshot(Read(_root / L"settings.json"), pins);
            });
        }
        static void ValidatePinnedChanges(const DiskSnapshot& snapshot, const Pins::Catalog& proposed)
        {
            proposed.Write(); // Full descriptor/catalog validation before any write.
            for (const auto& attached : snapshot.attachedPins)
            {
                const auto found = std::find_if(proposed.pins.begin(), proposed.pins.end(), [&](const auto& pin) { return pin.id == attached.id; });
                if (found == proposed.pins.end() || *found != attached)
                {
                    throw std::invalid_argument("Import changes an attached Pin. Close that Pin's session normally, then preview the import again. No session was moved or closed.");
                }
            }
        }
        ApplyResult Apply(const DiskSnapshot& expected, const std::string& settings, const Pins::Catalog& catalog) const
        {
            if (settings.empty() || settings.size() > 8 * 1024 * 1024 || !Parse(settings).isObject()) { throw std::invalid_argument("Invalid or oversized proposed settings."); }
            const auto validatedCatalog = Pins::Catalog::Read(catalog.Write());
            return Pins::Store{ _root, _probe }.WithLockedSnapshot([&](const auto& current) {
                const auto settingsPath = _root / L"settings.json", pinsPath = _root / L"pins.json", pending = _root / L"setup-import.pending.json";
                const auto beforeSettings = Read(settingsPath), beforePins = Read(pinsPath);
                if ((beforePins.exists && Parse(beforePins.bytes) != Parse(Pins::Store::SerializeSnapshot(current))) ||
                    (!beforePins.exists && (!current.catalog.pins.empty() || !current.leases.empty())))
                {
                    throw std::runtime_error("Pins changed while preparing the transaction. Nothing was imported; preview again.");
                }
                if (beforeSettings.bytes.size() > 8 * 1024 * 1024) { throw std::invalid_argument("Existing settings exceed the setup-transfer size limit."); }
                if (beforeSettings != File{ expected.settingsExists, expected.settings } || current.catalog.Write() != expected.pins.Write())
                {
                    throw std::runtime_error("Settings or Pins changed after preview. Nothing was imported; preview again.");
                }
                ValidatePinnedChanges(Snapshot(beforeSettings, current), validatedCatalog);
                auto next = current; next.catalog = validatedCatalog; // Never import or reassign a lease.
                const File afterSettings{ true, settings };
                // Do not create an empty pins file for a settings-only operation.
                const File afterPins = validatedCatalog.Write() == current.catalog.Write() ? beforePins : File{ true, Pins::Store::SerializeSnapshot(next) };
                if (beforeSettings == afterSettings && beforePins == afterPins) { return ApplyResult{}; }
                const auto backup = _root / (L"setup-backup-" + std::wstring{ winrt::to_hstring(Pins::NewId()) });
                std::filesystem::create_directory(backup);
                if (beforeSettings.exists) { _writer(backup / L"settings.json", beforeSettings.bytes); }
                if (beforePins.exists) { _writer(backup / L"pins.json", beforePins.bytes); }
                Json::Value journal{ Json::objectValue }; journal["version"] = 1;
                journal["beforeSettings"] = Encode(beforeSettings); journal["beforePins"] = Encode(beforePins);
                journal["afterSettings"] = Encode(afterSettings); journal["afterPins"] = Encode(afterPins);
                _writer(pending, Json::writeString(Json::StreamWriterBuilder{}, journal));
                bool writesStarted = false;
                try
                {
                    // A noncooperating external editor is detected immediately before commit.
                    if (Read(settingsPath) != beforeSettings || Read(pinsPath) != beforePins) { throw std::runtime_error("Configuration changed before commit."); }
                    writesStarted = true;
                    if (beforePins != afterPins) { Restore(pinsPath, afterPins); }
                    if (beforeSettings != afterSettings) { Restore(settingsPath, afterSettings); }
                    _remove(pending);
                }
                catch (...)
                {
                    const auto error = std::current_exception();
                    if (!writesStarted) { _remove(pending); std::rethrow_exception(error); }
                    try
                    {
                        const auto actualSettings = Read(settingsPath), actualPins = Read(pinsPath);
                        if ((actualSettings != beforeSettings && actualSettings != afterSettings) || (actualPins != beforePins && actualPins != afterPins))
                        {
                            throw std::runtime_error("A noncooperating writer changed configuration during import.");
                        }
                        if (actualSettings != beforeSettings) { Restore(settingsPath, beforeSettings); }
                        if (actualPins != beforePins) { Restore(pinsPath, beforePins); }
                        _remove(pending);
                    }
                    catch (...)
                    {
                        throw std::runtime_error("Setup import did not complete and rollback needs recovery. Settings/Pins access is blocked; use Recover interrupted setup import. Original bytes remain in the adjacent setup-backup directory and setup-import.pending.json.");
                    }
                    std::rethrow_exception(error);
                }
                return ApplyResult{ backup, true };
            });
        }
        bool HasPendingRecovery() const { return std::filesystem::exists(_root / L"setup-import.pending.json"); }
        bool Recover() const
        {
            return Pins::Store::WithSetupLock(_root, [&] {
                const auto pending = _root / L"setup-import.pending.json";
                const auto raw = Read(pending); if (!raw.exists) { return false; }
                const auto journal = Parse(raw.bytes);
                Pins::Catalog::Keys(journal, { "version", "beforeSettings", "beforePins", "afterSettings", "afterPins" });
                if (!journal["version"].isUInt() || journal["version"].asUInt() != 1) { throw std::invalid_argument("Unsupported setup recovery journal version; preserve original files."); }
                const auto beforeSettings = Decode(journal["beforeSettings"]), beforePins = Decode(journal["beforePins"]);
                const auto afterSettings = Decode(journal["afterSettings"]), afterPins = Decode(journal["afterPins"]);
                const auto settingsPath = _root / L"settings.json", pinsPath = _root / L"pins.json";
                const auto actualSettings = Read(settingsPath), actualPins = Read(pinsPath);
                if ((actualSettings != beforeSettings && actualSettings != afterSettings) || (actualPins != beforePins && actualPins != afterPins))
                {
                    throw std::runtime_error("Recovery found a subsequent external edit. Nothing was overwritten; compare the backup and current files manually.");
                }
                Restore(settingsPath, beforeSettings); Restore(pinsPath, beforePins); _remove(pending);
                return true;
            }, true);
        }
    };
}
