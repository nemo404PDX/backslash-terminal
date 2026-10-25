// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "PinnedDestinations.h"

namespace Microsoft::Terminal::Backslash::Pins
{
    using ExpandLocalDirectory = std::function<std::string(const std::string&)>;
    struct DirectoryRequest
    {
        Kind kind = Kind::Local;
        std::string path;
        bool needsConfirmation = true;
        bool invalidReport = false;
    };
    struct CapturedDirectory
    {
        Kind kind;
        std::string path;
        DirectoryOrigin origin;
    };
    inline Kind DirectoryKind(Kind kind, const std::string& path)
    {
        if (kind == Kind::SshWindows || kind == Kind::SshPosix) { return path.starts_with('/') ? Kind::SshPosix : Kind::SshWindows; }
        return kind;
    }
    inline bool UsableDirectory(Kind kind, const std::string& path)
    {
        return kind == Kind::Wsl || kind == Kind::SshPosix ? PosixPath(path) : WindowsPath(path);
    }
    inline CapturedDirectory ConfirmDirectory(Kind kind, std::string text, const ExpandLocalDirectory& expand)
    {
        if (!SafeText(text, 32760)) { throw std::invalid_argument("Enter a single folder path without line breaks."); }
        const auto begin = text.find_first_not_of(" \t");
        if (begin == text.npos) { throw std::invalid_argument("Enter the folder you want to pin."); }
        text = text.substr(begin, text.find_last_not_of(" \t") - begin + 1);
        if (kind == Kind::Local)
        {
            text = expand(text);
            if (text.find('%') != text.npos) { throw std::invalid_argument("Use a full folder path or a defined variable such as %USERPROFILE%. Unresolved variables and literal % in folder names are not supported."); }
        }
        kind = DirectoryKind(kind, text);
        if (!UsableDirectory(kind, text))
        {
            throw std::invalid_argument(kind == Kind::Local ? "Enter a full Windows folder path, such as C:\\Users\\name\\AppData." : "Enter the full folder path on that host. Local folders and local environment variables are not used for remote pins.");
        }
        return { kind, std::move(text), DirectoryOrigin::Confirmed };
    }
    inline DirectoryRequest RequestDirectory(Kind kind, const std::string& reported, const std::string& starting,
                                             const std::string& home, const ExpandLocalDirectory& expand)
    {
        const auto reportedKind = DirectoryKind(kind, reported);
        if (UsableDirectory(reportedKind, reported)) { return { reportedKind, reported, false, false }; }
        DirectoryRequest request{ kind, {}, true, !reported.empty() };
        // A startup seed is only an editable suggestion. It never becomes a
        // reported cwd. Remote requests never consult the client's local seed.
        if (kind == Kind::Local)
        {
            for (const auto& candidate : { starting, home })
            {
                try { request.path = ConfirmDirectory(kind, candidate, expand).path; break; }
                catch (const std::exception&) {}
            }
        }
        return request;
    }
    inline CapturedDirectory CurrentDirectory(const DirectoryRequest& request)
    {
        if (request.needsConfirmation) { throw std::invalid_argument("No usable current folder report is available."); }
        return { request.kind, request.path, DirectoryOrigin::Reported };
    }
    inline std::optional<CapturedDirectory> FinishDirectory(const DirectoryRequest& request,
                                                            const std::optional<std::string>& confirmed,
                                                            const ExpandLocalDirectory& expand)
    {
        if (!request.needsConfirmation) { return CapturedDirectory{ request.kind, request.path, DirectoryOrigin::Reported }; }
        if (!confirmed) { return {}; } // Cancel is not an implicit acceptance of the suggestion.
        return ConfirmDirectory(request.kind, *confirmed, expand);
    }
}
