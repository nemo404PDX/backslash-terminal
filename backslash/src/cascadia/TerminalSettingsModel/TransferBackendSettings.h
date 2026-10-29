// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "../inc/TransferBackends.h"
#include <stdexcept>
namespace Microsoft::Terminal::Backslash::Transfers
{
    inline constexpr auto SettingsKey = "backslash.transferBackends";
    inline constexpr std::array Keys{ "sftp", "sabnzbd", "aria2", "rclone" };
    // Versioned preferences only. No credentials, transport or installation side effects.
    inline Json::Value Defaults()
    {
        Json::Value value{Json::objectValue}; value["version"] = 1;
        for (auto key : Keys) { value[key]["enabled"] = false; }
        value["sftp"]["profile"] = "";
        return value;
    }
    inline void Validate(const Json::Value& value)
    {
        if (!value.isObject() || value.size() != 5 || !value["version"].isInt() || value["version"].asInt() != 1)
        { throw std::invalid_argument("Unsupported Transfers backend settings. Edit JSON to correct them; existing data is preserved."); }
        for (auto key : Keys)
        {
            const auto& entry = value[key];
            if (!entry.isObject() || entry.size() != (std::string_view{key} == "sftp" ? 2u : 1u) || !entry["enabled"].isBool())
            { throw std::invalid_argument("Invalid Transfers backend configuration."); }
        }
        const auto& profile = value["sftp"]["profile"];
        if (!profile.isString()) { throw std::invalid_argument("Invalid SFTP profile reference."); }
        if (!profile.asString().empty())
        {
            try { if (winrt::guid{winrt::to_hstring(profile.asString())} == winrt::guid{}) { throw std::invalid_argument("Empty identity"); } }
            catch (...) { throw std::invalid_argument("SFTP needs a saved SSH profile identity."); }
        }
    }
    inline Json::Value Parse(std::string_view bytes)
    {
        Json::CharReaderBuilder builder; builder["rejectDupKeys"] = true; builder["failIfExtra"] = true;
        const std::unique_ptr<Json::CharReader> reader{builder.newCharReader()}; Json::Value value; std::string errors;
        if (bytes.size() > 16384 || !reader->parse(bytes.data(), bytes.data()+bytes.size(), &value, &errors))
        { throw std::invalid_argument("Invalid Transfers backend settings JSON."); }
        Validate(value); return value;
    }
}
