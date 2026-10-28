// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SavedTabGroups.h"

namespace Microsoft::Terminal::Backslash::HostSshVisibility
{
    inline constexpr auto Key = "backslash.hostSshProfileVisibility";
    // Explicit preferences only; generated endpoints and launch commands never enter this map.
    inline Json::Value Preferences(const Json::Value& profile)
    {
        Json::Value result{ Json::objectValue };
        for (const auto key : { "scrollbarState", "experimental.useBackgroundImageForWindow", "tabColor", "hidden", "font", "colorScheme", "foreground", "background", "selectionBackground", "cursorColor", "cursorShape", "cursorHeight", "opacity", "useAcrylic", "backgroundImage", "backgroundImageOpacity", "backgroundImageStretchMode", "backgroundImageAlignment", "padding", "antialiasingMode", "adjustIndistinguishableColors", "intenseTextStyle", "unfocusedAppearance" })
        {
            if (profile.isMember(key)) { result[key] = profile[key]; }
        }

        for (const auto key : { "adjustIndistinguishableColors", "backgroundImage", "backgroundImageAlignment", "backgroundImageOpacity", "backgroundImageStretchMode", "cursorHeight", "cursorShape", "experimental.pixelShaderImagePath", "experimental.pixelShaderPath", "experimental.retroBezelBadge", "experimental.retroBezelBadgeBacking", "experimental.retroBezelBadgeLettering", "experimental.retroBezelBadgeStyle", "experimental.retroBezelBottomSize", "experimental.retroBezelCenterFullscreen", "experimental.retroBezelColor", "experimental.retroBezelEnabled", "experimental.retroBezelFiller", "experimental.retroBezelLeftSize", "experimental.retroBezelRightSize", "experimental.retroBezelSize", "experimental.retroBezelTopSize", "experimental.retroTerminalContrastGuard", "experimental.retroTerminalEffect", "experimental.retroTerminalGlowIntensity", "experimental.retroTerminalScanlineGap", "experimental.retroTerminalScanlineIntensity", "experimental.retroTerminalScanlineThickness", "experimental.retroTerminalScanlineWhite", "experimental.retroTerminalSchemePhosphor", "experimental.retroTerminalTintColor", "experimental.retroTerminalTintIntensity", "intenseTextStyle", "useAcrylic" })
        {
            if (profile.isMember(key)) { result[key] = profile[key]; }
        }
        return result;
    }
    inline void ValidateAppearance(const Json::Value& value, bool unfocused = false)
    {
        if (!value.isObject()) { throw std::invalid_argument("SSH appearance preferences must be objects."); }
        if (Preferences(value) != value) { throw std::invalid_argument("Unknown SSH appearance preference."); }
        for (const auto& key : value.getMemberNames())
        {
            const auto& item = value[key];
            if (item.isNull()) { continue; }
            if (key == "unfocusedAppearance")
            {
                if (unfocused) { throw std::invalid_argument("Nested unfocused appearance is invalid."); }
                ValidateAppearance(item, true);
            }
            else if (key == "font")
            {
                if (unfocused || !item.isObject()) { throw std::invalid_argument("Invalid SSH font preferences."); }
                for (const auto& field : item.getMemberNames())
                {
                    const auto& setting = item[field];
                    if (setting.isNull()) { continue; }
                    bool valid = false;
                    if (field == "face" || field == "cellWidth" || field == "cellHeight") { valid = setting.isString(); }
                    else if (field == "size") { valid = setting.isNumeric(); }
                    else if (field == "weight") { valid = setting.isString() || setting.isUInt(); }
                    else if (field == "builtinGlyphs" || field == "colorGlyphs") { valid = setting.isBool(); }
                    else if (field == "axes" || field == "features")
                    {
                        valid = setting.isObject();
                        if (valid) { for (const auto& tag : setting.getMemberNames()) { if (tag.size() != 4 || !setting[tag].isNumeric()) { valid = false; } } }
                    }
                    if (!valid) { throw std::invalid_argument("Invalid or unknown SSH font preference."); }
                }
            }
            else if (key == "colorScheme" && item.isObject())
            {
                for (const auto& theme : item.getMemberNames()) { if ((theme != "light" && theme != "dark") || !item[theme].isString()) { throw std::invalid_argument("Invalid SSH color scheme preference."); } }
            }
            else
            {
                const bool boolean = key == "experimental.useBackgroundImageForWindow" || key == "hidden" || key == "useAcrylic" || key == "experimental.retroBezelEnabled" || key == "experimental.retroBezelCenterFullscreen" || key == "experimental.retroTerminalEffect" || key == "experimental.retroTerminalScanlineWhite" || key == "experimental.retroTerminalContrastGuard" || key == "experimental.retroTerminalSchemePhosphor";
                const bool number = key == "opacity" || key == "cursorHeight" || key == "backgroundImageOpacity" || key.ends_with("Intensity") || key.ends_with("Thickness") || key.ends_with("Gap");
                if ((boolean && !item.isBool()) || (number && !item.isNumeric()) || (!boolean && !number && !item.isString())) { throw std::invalid_argument("Invalid SSH appearance preference type."); }
            }
        }
    }
    inline Json::Value Read(const Json::Value& value)
    {
        if (!value.isObject() || value.size() > 4096)
        {
            throw std::invalid_argument("Host SSH visibility must be an object with at most 4096 profile preferences.");
        }
        Json::Value result{ Json::objectValue };
        for (const auto& key : value.getMemberNames())
        {
            const auto canonical = SavedTabGroups::Guid(key);
            if ((!value[key].isBool() && !value[key].isObject()) || result.isMember(canonical))
            {
                throw std::invalid_argument("Host SSH visibility needs unique profile GUIDs and boolean values.");
            }
            if (value[key].isBool()) { result[canonical]["hidden"] = value[key]; }
            else
            {
                result[canonical] = Preferences(value[key]);
                ValidateAppearance(result[canonical]);
                if (value[key].isMember("remoteShell"))
                {
                    const auto& shell = value[key]["remoteShell"];
                    if (!shell.isInt() || shell.asInt() < 0 || shell.asInt() > 4) { throw std::invalid_argument("Invalid remote SSH shell."); }
                    result[canonical]["remoteShell"] = shell;
                    if (shell.asInt() == 4)
                    {
                        const auto& command = value[key]["remoteCommand"];
                        if (!command.isString() || command.asString().find_first_not_of(" \t") == std::string::npos || command.asString().find_first_of("\r\n%") != std::string::npos)
                        { throw std::invalid_argument("Custom remote SSH command must be nonempty and single-line, without percent expansion."); }
                    }
                    if (value[key].isMember("remoteCommand"))
                    {
                        if (!value[key]["remoteCommand"].isString()) { throw std::invalid_argument("Invalid remote SSH command."); }
                        result[canonical]["remoteCommand"] = value[key]["remoteCommand"];
                    }
                }
            }
        }
        return result;
    }
}
