// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "../../types/inc/utils.hpp"

#include <json/json.h>
#include "../../types/inc/utils.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Microsoft::Terminal::Backslash::Pins
{
    // Remembered destinations contain no live tab/window ownership, split layout,
    // command line, environment, credentials, terminal text or mutable cwd state.
    enum class Kind { Local, Wsl, SshWindows, SshPosix };
    enum class DirectoryOrigin { Unknown, ProfileSeed, Reported, Confirmed };

    struct Destination
    {
        std::string id;
        std::string profile;
        std::string source;
        Kind kind = Kind::Local;
        std::string label;
        std::string host;
        std::string path;
        bool operator==(const Destination&) const = default;
    };

    inline bool SafeText(const std::string& value, size_t limit)
    {
        return value.size() <= limit && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
    }

    inline std::string Guid(const std::string& text)
    {
        if (text.size() != 38 || text.front() != '{' || text.back() != '}') { throw std::invalid_argument("Invalid pin/profile GUID."); }
        const winrt::guid guid = ::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(text).c_str());
        if (guid == winrt::guid{}) { throw std::invalid_argument("Empty pin/profile GUID."); }
        return winrt::to_string(::Microsoft::Console::Utils::GuidToString(guid));
    }

    inline bool WindowsPath(const std::string& value)
    {
        if (!SafeText(value, 32760) || value.find_first_of("%\"<>|?*") != value.npos) { return false; }
        if (value.size() >= 3 && ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z')) &&
            value[1] == ':' && (value[2] == '\\' || value[2] == '/')) { return true; }
        if (!value.starts_with("\\\\") || value.starts_with("\\\\.\\")) { return false; }
        const auto serverEnd = value.find('\\', 2);
        return serverEnd > 2 && serverEnd != value.npos && serverEnd + 1 < value.size() && value[serverEnd + 1] != '\\';
    }

    inline bool PosixPath(const std::string& value)
    {
        return !value.empty() && value.front() == '/' && SafeText(value, 32760) && value.find('%') == value.npos;
    }

    inline const char* KindName(Kind kind)
    {
        switch (kind)
        {
        case Kind::Local: return "local";
        case Kind::Wsl: return "wsl";
        case Kind::SshWindows: return "sshWindows";
        case Kind::SshPosix: return "sshPosix";
        default: throw std::invalid_argument("Unsupported pin destination kind.");
        }
    }

    inline Kind ParseKind(const std::string& value)
    {
        for (auto kind : { Kind::Local, Kind::Wsl, Kind::SshWindows, Kind::SshPosix })
        {
            if (value == KindName(kind)) { return kind; }
        }
        throw std::invalid_argument("Unsupported pin destination kind.");
    }

    inline Destination Validate(Destination value)
    {
        value.id = Guid(value.id);
        value.profile = Guid(value.profile);
        KindName(value.kind);
        if (!SafeText(value.source, 1024) || !SafeText(value.label, 1024) || value.host.empty() || !SafeText(value.host, 1024))
        {
            throw std::invalid_argument("Invalid fixed pin identity.");
        }
        const bool wsl = value.kind == Kind::Wsl;
        const bool ssh = value.kind == Kind::SshWindows || value.kind == Kind::SshPosix;
        if ((wsl && value.source != "Windows.Terminal.Wsl") || (ssh && value.source != "Windows.Terminal.SSH" && value.source != "Backslash.Terminal.SSH") ||
            (value.kind == Kind::Local && (value.source == "Windows.Terminal.Wsl" || value.source == "Windows.Terminal.SSH" || value.source == "Backslash.Terminal.SSH")))
        {
            throw std::invalid_argument("Pin path context does not match its profile source.");
        }
        if (!(wsl || value.kind == Kind::SshPosix ? PosixPath(value.path) : WindowsPath(value.path)))
        {
            throw std::invalid_argument("A pin needs a bounded absolute path in its own host context, without environment expansion.");
        }
        return value;
    }

    inline Destination Capture(Destination candidate, DirectoryOrigin origin)
    {
        if (origin != DirectoryOrigin::Reported && origin != DirectoryOrigin::Confirmed)
        {
            throw std::invalid_argument("The current directory is unavailable. A profile starting directory is not a reported destination.");
        }
        return Validate(std::move(candidate)); // Copy; later navigation cannot mutate it.
    }

    inline std::string Identity(const Destination& value)
    {
        return value.host + " - " + (value.label.empty() ? value.profile : value.label) + "\n" + value.path;
    }

    struct Catalog
    {
        static constexpr size_t MaxPins = 128;
        static constexpr size_t MaxBytes = 1024 * 1024;
        std::vector<Destination> pins;

        static void Keys(const Json::Value& value, const std::set<std::string>& allowed)
        {
            if (!value.isObject()) { throw std::invalid_argument("Invalid pin catalog object."); }
            for (const auto& name : value.getMemberNames())
            {
                if (!allowed.contains(name)) { throw std::invalid_argument("Unsupported pin data; keep the original catalog unchanged."); }
            }
        }

        static std::string Text(const Json::Value& value, const char* key)
        {
            if (!value[key].isString()) { throw std::invalid_argument("Missing or invalid pin text."); }
            return value[key].asString();
        }

        static Catalog Read(const std::string& content)
        {
            if (content.size() > MaxBytes) { throw std::invalid_argument("Pin catalog is too large."); }
            Json::CharReaderBuilder builder;
            builder["rejectDupKeys"] = true;
            builder["failIfExtra"] = true;
            builder["allowComments"] = false;
            builder["stackLimit"] = 64;
            std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
            Json::Value root;
            std::string errors;
            if (!reader->parse(content.data(), content.data() + content.size(), &root, &errors)) { throw std::invalid_argument("Invalid pin catalog JSON."); }
            Keys(root, { "version", "pins" });
            if (!root["version"].isUInt() || root["version"].asUInt() != 1) { throw std::invalid_argument("Unsupported pin catalog version; preserve the original data."); }
            if (!root["pins"].isArray() || root["pins"].size() > MaxPins) { throw std::invalid_argument("Invalid pin list or pin limit exceeded."); }
            Catalog result;
            for (const auto& item : root["pins"])
            {
                Keys(item, { "id", "profile", "source", "kind", "label", "host", "path" });
                result.Add({ Text(item, "id"), Text(item, "profile"), Text(item, "source"), ParseKind(Text(item, "kind")), Text(item, "label"), Text(item, "host"), Text(item, "path") });
            }
            return result;
        }

        void Add(Destination value)
        {
            value = Validate(std::move(value));
            if (pins.size() >= MaxPins) { throw std::invalid_argument("Pin limit exceeded."); }
            if (std::any_of(pins.begin(), pins.end(), [&](const auto& pin) { return pin.id == value.id; })) { throw std::invalid_argument("Duplicate pin ID."); }
            pins.emplace_back(std::move(value));
        }

        bool Remove(const std::string& id)
        {
            const auto normalized = Guid(id);
            const auto found = std::find_if(pins.begin(), pins.end(), [&](const auto& pin) { return pin.id == normalized; });
            if (found == pins.end()) { return false; }
            pins.erase(found);
            return true;
        }

        std::string Write() const
        {
            if (pins.size() > MaxPins) { throw std::invalid_argument("Pin limit exceeded."); }
            Json::Value root{ Json::objectValue };
            root["version"] = 1;
            root["pins"] = Json::Value{ Json::arrayValue };
            std::set<std::string> ids;
            for (const auto& candidate : pins)
            {
                const auto pin = Validate(candidate);
                if (!ids.emplace(pin.id).second) { throw std::invalid_argument("Duplicate pin ID."); }
                Json::Value row{ Json::objectValue };
                row["id"] = pin.id; row["profile"] = pin.profile; row["source"] = pin.source;
                row["kind"] = KindName(pin.kind); row["label"] = pin.label; row["host"] = pin.host; row["path"] = pin.path;
                root["pins"].append(std::move(row));
            }
            auto result = Json::writeString(Json::StreamWriterBuilder{}, root);
            if (result.size() > MaxBytes) { throw std::invalid_argument("Pin catalog is too large."); }
            return result;
        }
    };

    enum class Intent { Unpin, IndividualClose, WindowClose, QuitAll, Transfer, NaturalExit, Recovery, BulkTabClose, PaneOnlyClose };
    struct Action
    {
        bool removeRemembered = false;
        bool closeSession = false;
        bool moveToNormalArea = false;
        bool operator==(const Action&) const = default;
    };

    inline Action Plan(Intent intent, bool confirmed, bool live = true)
    {
        if (!confirmed) { return {}; }
        switch (intent)
        {
        case Intent::Unpin:
            if (!live) { throw std::invalid_argument("An unopened pin has no session to unpin. Close its entry to remove it."); }
            return { true, false, true };
        case Intent::IndividualClose: return { true, live, false };
        case Intent::WindowClose:
        case Intent::QuitAll:
        case Intent::BulkTabClose:
        case Intent::PaneOnlyClose: return { false, live, false };
        case Intent::Transfer:
        case Intent::NaturalExit:
        case Intent::Recovery: return {};
        default: throw std::invalid_argument("This pin close intent needs an explicit product policy.");
        }
    }

    // No state-file location/ownership policy is selected here. The UI must perform
    // any session close or demotion only after this writer boundary succeeds.
    inline bool RemoveAfterWrite(Catalog& current, const std::string& id, Intent intent, bool confirmed,
                                 const std::function<void(const std::string&)>& writer, bool live = true)
    {
        if (!Plan(intent, confirmed, live).removeRemembered) { return false; }
        auto next = current;
        if (!next.Remove(id)) { return false; }
        writer(next.Write());
        current = std::move(next);
        return true;
    }

    struct ProfileTarget
    {
        std::string profile;
        std::string source;
        std::string host;
        Kind kind = Kind::Local;
        bool active = false;
        bool elevationCompatible = false;
        bool supportsFixedPath = false;
    };

    inline Destination Resolve(const Destination& candidate, const std::optional<ProfileTarget>& target,
                               const std::function<bool(const std::string&)>& localPathExists)
    {
        const auto pin = Validate(candidate);
        if (!target || !target->active || Guid(target->profile) != pin.profile || target->source != pin.source)
        {
            throw std::invalid_argument("The pinned profile is unavailable or replaced. No default profile is substituted.");
        }
        if (target->host != pin.host || target->kind != pin.kind || !target->elevationCompatible || !target->supportsFixedPath)
        {
            throw std::invalid_argument("The pinned destination is incompatible with the current profile/launcher.");
        }
        if (pin.kind == Kind::Local && !localPathExists(pin.path)) { throw std::invalid_argument("The pinned folder is unavailable; the destination was not changed."); }
        return pin; // Remote paths never reach a local filesystem probe.
    }
}
