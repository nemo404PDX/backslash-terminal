// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include <json/json.h>
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Microsoft::Terminal::Backslash::SetupTransfer
{
    // Preview diagnostics never resolve imported resources. Test-only injected
    // probes may return true/false; nullopt means honestly unverified.
    struct ResourceProbes
    {
        std::function<std::optional<bool>(const std::filesystem::path&)> exists;
        std::function<std::optional<bool>(const std::wstring&)> font;
        std::function<std::optional<bool>(const std::wstring&)> executable;
    };

    inline std::wstring ExpandedReference(const std::string& text)
    {
        const auto input = winrt::to_hstring(text);
        const auto length = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
        if (!length || length > 32768) { return std::wstring{ input }; }
        std::wstring expanded(length, L'\0');
        if (!ExpandEnvironmentStringsW(input.c_str(), expanded.data(), length)) { return std::wstring{ input }; }
        expanded.resize(length - 1);
        return expanded;
    }

    inline ResourceProbes PreviewResourceProbes()
    {
        ResourceProbes probes;
        // Even drive paths and PATH entries may redirect through mapped drives,
        // reparse points or network shares. Do not probe any imported reference.
        probes.exists = [](const std::filesystem::path&) -> std::optional<bool> { return std::nullopt; };
        probes.font = [](const std::wstring&) -> std::optional<bool> { return std::nullopt; };
        probes.executable = [](const std::wstring&) -> std::optional<bool> { return std::nullopt; };
        return probes;
    }

    inline bool UnverifiablePath(std::wstring_view text)
    {
        if (text.empty()) { return false; }
        // Purely lexical: UNC, rooted/device paths, URI schemes, drive-relative
        // paths and alternate streams must never reach even injected probes.
        if (text.front() == L'\\' || text.front() == L'/') { return true; }
        const auto colon = text.find(L':');
        if (colon != std::wstring_view::npos &&
            (colon != 1 || text.size() < 3 || (text[2] != L'\\' && text[2] != L'/') || text.find(L':', 2) != std::wstring_view::npos)) { return true; }
        const auto file = std::filesystem::path{text}.filename().stem().native();
        const auto equal = [&](std::wstring_view name) { return CompareStringOrdinal(file.c_str(), static_cast<int>(file.size()), name.data(), static_cast<int>(name.size()), TRUE) == CSTR_EQUAL; };
        return equal(L"NUL") || equal(L"CON") || equal(L"PRN") || equal(L"AUX") || equal(L"CONIN$") || equal(L"CONOUT$") ||
            (file.size() == 4 && file[3] >= L'1' && file[3] <= L'9' &&
             (CompareStringOrdinal(file.data(), 3, L"COM", 3, TRUE) == CSTR_EQUAL || CompareStringOrdinal(file.data(), 3, L"LPT", 3, TRUE) == CSTR_EQUAL));
    }

    inline std::vector<std::string> Analyze(const Json::Value& settings, const std::filesystem::path& settingsDirectory,
                                          const ResourceProbes& probes = PreviewResourceProbes())
    {
        std::set<std::string> findings;
        const auto pathReference = [&](const Json::Value& value, const char* kind) {
            if (!value.isString() || value.asString().empty()) { return; }
            const auto text = value.asString();
            if (text == "desktopWallpaper" || text == "none") { return; }
            // Network references are deliberately not fetched, including UNC paths.
            if (UnverifiablePath(std::wstring_view{winrt::to_hstring(text)}))
            {
                findings.insert(std::string{ kind } + " reference is not bundled or verified: " + text);
                return;
            }
            const auto expanded = ExpandedReference(text);
            auto path = std::filesystem::path{ expanded };
            if (path.is_relative()) { path = settingsDirectory / path; }
            if (UnverifiablePath(expanded) || UnverifiablePath(path.native()))
            {
                findings.insert(std::string{kind} + " reference is not bundled or verified: " + text);
                return;
            }
            const auto found = probes.exists(path);
            if (!found) { findings.insert(std::string{kind} + " reference was not checked before import: " + text); }
            else if (!*found) { findings.insert(std::string{ "Missing " } + kind + ": " + text); }
        };
        const auto appearance = [&](const Json::Value& profile) {
            for (const auto* key : { "backgroundImage", "experimental.pixelShaderPath", "experimental.pixelShaderImagePath" })
            {
                pathReference(profile[key], "image/shader");
            }
            const auto fontValue = profile["font"].isObject() ? profile["font"]["face"] : profile["fontFace"];
            if (fontValue.isString())
            {
                const auto familyList = fontValue.asString();
                size_t begin = 0;
                while (begin < familyList.size())
                {
                    const auto end = familyList.find(',', begin);
                    auto family = familyList.substr(begin, end == std::string::npos ? end : end - begin);
                    const auto first = family.find_first_not_of(" \t");
                    const auto last = family.find_last_not_of(" \t");
                    if (first != std::string::npos)
                    {
                        family = family.substr(first, last - first + 1);
                        if (UnverifiablePath(ExpandedReference(family)))
                        {
                            findings.insert("Font reference was not checked before import: " + family);
                            if (end == std::string::npos) { break; }
                            begin = end + 1;
                            continue;
                        }
                        const auto found = probes.font(std::wstring{ winrt::to_hstring(family) });
                        if (!found) { findings.insert("Font was not checked before import: " + family); }
                        else if (!*found) { findings.insert("Font is not installed: " + family); }
                    }
                    if (end == std::string::npos) { break; }
                    begin = end + 1;
                }
            }
        };
        const auto profile = [&](const Json::Value& value) {
            appearance(value);
            if (value["unfocusedAppearance"].isObject()) { appearance(value["unfocusedAppearance"]); }
            if (value["icon"].isString())
            {
                const auto text = value["icon"].asString();
                // A font glyph is a valid icon, not a missing file.
                if (text.find_first_of("./\\:") != std::string::npos) { pathReference(value["icon"], "icon"); }
            }
            if (value["commandline"].isString() && !value["commandline"].asString().empty())
            {
                const auto command = ExpandedReference(value["commandline"].asString());
                int argc = 0;
                auto argv = CommandLineToArgvW(command.c_str(), &argc);
                if (!argv || argc == 0) { findings.insert("Shell command could not be checked; nothing was executed."); }
                else
                {
                    const std::wstring executable{ argv[0] };
                    if (UnverifiablePath(executable))
                    {
                        findings.insert("Network shell path was not checked; nothing was executed.");
                    }
                    else
                    {
                        const auto found = probes.executable(executable);
                        if (!found) { findings.insert("Shell executable was not checked before import; nothing was executed: " + winrt::to_string(executable)); }
                        else if (!*found) { findings.insert("Shell executable was not found: " + winrt::to_string(executable)); }
                    }
                }
                if (argv) { LocalFree(argv); }
            }
        };
        profile(settings["profiles"]["defaults"]);
        for (const auto& value : settings["profiles"]["list"]) { profile(value); }
        return { findings.begin(), findings.end() };
    }
}
