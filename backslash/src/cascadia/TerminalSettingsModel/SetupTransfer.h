// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include "MTSMSettings.h"
#include "PinnedDestinations.h"
#include "SavedTabGroups.h"
#include "WorkspaceSsh.h"
#include "HostSshVisibility.h"
#include <functional>
#include <map>
#include <string_view>

namespace Microsoft::Terminal::Backslash::SetupTransfer
{
    // v1 is a merge of explicit settings, never a flattened effective profile.
    // Defaults and matching profiles replace their explicit objects. Missing
    // fields there restore inheritance; missing top-level preferences/entities
    // leave the destination alone. No deletion or runtime ownership is implied.
    constexpr size_t MaxBytes = 8 * 1024 * 1024;
    constexpr size_t MaxProfiles = 512;

    inline std::string JsonText(const Json::Value& value)
    {
        return Json::writeString(Json::StreamWriterBuilder{}, value);
    }

    inline Json::Value ReadJson(std::string_view content)
    {
        if (content.empty() || content.size() > MaxBytes) { throw std::invalid_argument("Setup file is empty or exceeds 8 MiB."); }
        Json::CharReaderBuilder builder;
        builder["rejectDupKeys"] = true;
        builder["failIfExtra"] = true;
        builder["allowComments"] = false;
        builder["stackLimit"] = 64;
        std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
        Json::Value value;
        std::string errors;
        if (!reader->parse(content.data(), content.data() + content.size(), &value, &errors)) { throw std::invalid_argument("Setup file is not valid JSON (duplicate keys and trailing data are not allowed)."); }
        return value;
    }

    inline void Keys(const Json::Value& value, const std::set<std::string>& allowed, const char* context)
    {
        if (!value.isObject()) { throw std::invalid_argument(std::string{ context } + " must be an object."); }
        for (const auto& key : value.getMemberNames())
        {
            if (!allowed.contains(key)) { throw std::invalid_argument(std::string{ "Unsupported " } + context + " field: " + key); }
        }
    }

    inline std::set<std::string> AppearanceKeys()
    {
        std::set<std::string> keys{ "foreground", "background", "selectionBackground", "cursorColor", "opacity", "colorScheme" };
#define SETUP_KEY(type, name, key, ...) keys.emplace(key);
        MTSM_APPEARANCE_SETTINGS(SETUP_KEY)
        MTSM_RETRO_INTENSITY_SETTINGS(SETUP_KEY)
        MTSM_RETRO_COLOR_SETTINGS(SETUP_KEY)
        MTSM_RETRO_DIMENSION_SETTINGS(SETUP_KEY)
#undef SETUP_KEY
        return keys;
    }

    inline void ValidateAppearance(const Json::Value& value)
    {
        Keys(value, AppearanceKeys(), "appearance");
        if (value["colorScheme"].isObject()) { Keys(value["colorScheme"], { "dark", "light" }, "color scheme selection"); }
    }

    inline void ValidateProfile(const Json::Value& profile, bool defaults)
    {
        auto keys = AppearanceKeys();
        for (const auto* key : { "name", "guid", "source", "hidden", "updates", "padding", "tabColor", "font", "unfocusedAppearance" }) { keys.emplace(key); }
#define SETUP_KEY(type, name, key, ...) keys.emplace(key);
        MTSM_PROFILE_SETTINGS(SETUP_KEY)
#undef SETUP_KEY
        Keys(profile, keys, defaults ? "Defaults" : "profile");
        if (defaults && (profile.isMember("guid") || profile.isMember("name") || profile.isMember("source") || profile.isMember("commandline")))
        {
            throw std::invalid_argument("Defaults cannot contain profile identity or a command line.");
        }
        if (!defaults)
        {
            if (!profile["guid"].isString() || !profile["name"].isString() || profile["name"].asString().empty() || !SavedTabGroups::SafeText(profile["name"].asString(), 1024)) { throw std::invalid_argument("Each setup profile needs a stable GUID and a nonempty name."); }
            SavedTabGroups::Guid(profile["guid"].asString());
            if (profile.isMember("source") && !profile["source"].isString()) { throw std::invalid_argument("Profile source must be text."); }
        }
        if (profile.isMember("font"))
        {
            std::set<std::string> fontKeys;
#define SETUP_KEY(type, name, key, ...) fontKeys.emplace(key);
            MTSM_FONT_SETTINGS(SETUP_KEY)
#undef SETUP_KEY
            Keys(profile["font"], fontKeys, "font");
        }
        if (profile.isMember("unfocusedAppearance") && !profile["unfocusedAppearance"].isNull()) { ValidateAppearance(profile["unfocusedAppearance"]); }
        if (profile["colorScheme"].isObject()) { Keys(profile["colorScheme"], { "dark", "light" }, "color scheme selection"); }
    }

    inline void ValidateSettings(const Json::Value& settings)
    {
        std::set<std::string> keys{ "$schema", "$help", "profiles", "defaultProfile", "actions", "keybindings", "schemes", "themes", "backslash.savedTabGroups", "backslash.sshConnections", HostSshVisibility::Key };
#define SETUP_KEY(type, name, key, ...) keys.emplace(key);
        MTSM_GLOBAL_SETTINGS(SETUP_KEY)
#undef SETUP_KEY
        Keys(settings, keys, "settings");
        if (settings["profiles"].isArray()) { throw std::invalid_argument("Legacy profiles arrays are not supported by setup transfer. Save the current settings in this Backslash version first."); }
        Keys(settings["profiles"], { "defaults", "list" }, "profiles");
        ValidateProfile(settings["profiles"]["defaults"], true);
        const auto& profiles = settings["profiles"]["list"];
        if (!profiles.isArray() || profiles.size() > MaxProfiles) { throw std::invalid_argument("Setup profiles must be an array with at most 512 entries."); }
        std::set<std::string> ids;
        std::vector<std::string> names;
        for (const auto& profile : profiles)
        {
            ValidateProfile(profile, false);
            if (!ids.emplace(SavedTabGroups::Guid(profile["guid"].asString())).second) { throw std::invalid_argument("Setup contains duplicate profile GUIDs."); }
            const auto name = profile["name"].asString();
            if (std::any_of(names.begin(), names.end(), [&](const auto& prior) { return SavedTabGroups::SameName(prior, name); })) { throw std::invalid_argument("Setup contains duplicate profile names. Rename them before exporting/importing."); }
            names.push_back(name);
        }
        if (settings.isMember(HostSshVisibility::Key))
        {
            const auto normalized = HostSshVisibility::Read(settings[HostSshVisibility::Key]);
            for (const auto& id : settings[HostSshVisibility::Key].getMemberNames())
            {
                const auto& original = settings[HostSshVisibility::Key][id];
                if (original.isObject() && original != normalized[SavedTabGroups::Guid(id)]) { throw std::invalid_argument("Host SSH preferences contain unsupported fields."); }
            }
        }
        if (settings.isMember("backslash.sshConnections")) { WorkspaceSsh::ReadRecords(settings["backslash.sshConnections"]); }
        if (settings.isMember("backslash.savedTabGroups")) { SavedTabGroups::ReadValue(settings["backslash.savedTabGroups"]); }
        for (const auto* key : { "actions", "keybindings", "schemes", "themes" })
        {
            if (settings.isMember(key) && !settings[key].isArray()) { throw std::invalid_argument(std::string{ key } + " must be an array."); }
        }
        for (const auto* key : { "schemes", "themes" })
        {
            std::vector<std::string> resourceNames;
            for (const auto& definition : settings[key])
            {
                if (!definition.isObject() || !definition["name"].isString() || definition["name"].asString().empty()) { throw std::invalid_argument(std::string{ key } + " definitions need nonempty names."); }
                const auto name = definition["name"].asString();
                if (std::any_of(resourceNames.begin(), resourceNames.end(), [&](const auto& prior) { return SavedTabGroups::SameName(prior, name); })) { throw std::invalid_argument(std::string{ "Duplicate " } + key + " name: " + name); }
                resourceNames.push_back(name);
            }
        }
        // Startup/action definitions are preferences; live session and Pin
        // ownership arguments are never transferable setup data.
        const std::function<void(const Json::Value&)> noRuntime = [&](const Json::Value& value) {
            if (value.isObject())
            {
                for (const auto& key : value.getMemberNames())
                {
                    if (key == "sessionId" || key == "__content" || key.starts_with("__backslashPin")) { throw std::invalid_argument("Setup actions cannot contain live session or Pin ownership arguments."); }
                    noRuntime(value[key]);
                }
            }
            else if (value.isArray()) { for (const auto& item : value) { noRuntime(item); } }
        };
        noRuntime(settings["actions"]);
        noRuntime(settings["keybindings"]);
    }

    struct Document
    {
        Json::Value settings;
        Pins::Catalog pins;
    };

    inline void CheckReferences(const Json::Value& settings, const Pins::Catalog& pins, std::vector<std::string>& conflicts);

    inline Document Parse(std::string_view content)
    {
        auto root = ReadJson(content);
        Keys(root, { "format", "version", "settings", "pins" }, "setup document");
        if (!root["format"].isString() || root["format"].asString() != "backslash.setup" || !root["version"].isUInt() || root["version"].asUInt() != 1) { throw std::invalid_argument("This is not a supported Backslash setup document (expected backslash.setup version 1)."); }
        ValidateSettings(root["settings"]);
        return { std::move(root["settings"]), Pins::Catalog::Read(JsonText(root["pins"])) };
    }

    inline std::string Export(Json::Value explicitSettings, const Pins::Catalog& pins)
    {
        explicitSettings.removeMember("$schema");
        explicitSettings.removeMember("$help");
        ValidateSettings(explicitSettings);
        std::vector<std::string> conflicts;
        CheckReferences(explicitSettings, pins, conflicts);
        if (!conflicts.empty()) { throw std::invalid_argument("Setup export has unresolved references: " + conflicts.front()); }
        Json::Value root{ Json::objectValue };
        root["format"] = "backslash.setup";
        root["version"] = 1;
        root["settings"] = std::move(explicitSettings);
        root["pins"] = ReadJson(pins.Write());
        auto content = JsonText(root);
        Parse(content);
        return content;
    }

    struct Plan
    {
        Json::Value settings;
        Pins::Catalog pins;
        std::vector<std::string> conflicts;
        std::vector<std::string> warnings;
        size_t profilesAdded = 0, profilesUpdated = 0, groupsAdded = 0, groupsUpdated = 0, pinsAdded = 0, pinsUpdated = 0;
        bool changed = false;
        bool CanApply() const noexcept { return conflicts.empty(); }
    };

    inline std::string ProfileId(const Json::Value& value) { return SavedTabGroups::Guid(value["guid"].asString()); }
    inline std::string ProfileSource(const Json::Value& value) { return value.isMember("source") ? value["source"].asString() : std::string{}; }

    inline void CheckReferences(const Json::Value& settings, const Pins::Catalog& pins, std::vector<std::string>& conflicts)
    {
        std::map<std::string, const Json::Value*> profiles;
        for (const auto& profile : settings["profiles"]["list"]) { profiles.emplace(ProfileId(profile), &profile); }
        auto available = [&](const std::string& id, const std::string& source, const std::string& context) {
            const auto found = profiles.find(SavedTabGroups::Guid(id));
            if (found == profiles.end() || ProfileSource(*found->second) != source || (*found->second)["hidden"].asBool()) { conflicts.push_back(context + " references a missing, hidden or differently sourced profile: " + id); }
        };
        std::set<std::string> groups;
        if (settings.isMember("backslash.savedTabGroups"))
        {
            for (const auto& group : SavedTabGroups::ReadValue(settings["backslash.savedTabGroups"]).groups)
            {
                groups.emplace(group.id);
                for (const auto& tab : group.tabs)
                for (const auto& step : tab)
                {
                    const auto kind = step["kind"].asString();
                    if (kind == "newTab" || kind == "split") { available(step["profile"].asString(), step["source"].asString(), "Group '" + group.name + "'"); }
                }
            }
        }
        for (const auto& pin : pins.pins) { available(pin.profile, pin.source, "Pin '" + pin.label + "'"); }
        if (settings["backslash.startupWithSavedGroup"].asBool())
        {
            const auto selected = settings["backslash.startupGroup"].isString() ? settings["backslash.startupGroup"].asString() : std::string{};
            if (!std::any_of(groups.begin(), groups.end(), [&](const auto& id) { return SavedTabGroups::SameName(id, selected); })) { conflicts.emplace_back("The selected startup Group is missing. Choose an existing Group or disable startup with a Group."); }
        }
        if (settings.isMember("defaultProfile") && settings["defaultProfile"].isString())
        {
            const auto selected = settings["defaultProfile"].asString();
            const bool found = std::any_of(profiles.begin(), profiles.end(), [&](const auto& entry) { return SavedTabGroups::SameName(entry.second->operator[]("name").asString(), selected) || SavedTabGroups::SameName(entry.first, selected); });
            if (!found) { conflicts.emplace_back("The selected default profile is missing: " + selected); }
        }
    }

    // Optional caller validation runs before a Plan is returned. The UI uses
    // the real settings model to reject typed values or unsupported action data.
    inline Plan Prepare(const Document& document, const Json::Value& currentSettings, const Pins::Catalog& currentPins,
                        const std::function<void(const Json::Value&)>& validateModel = {})
    {
        ValidateSettings(document.settings);
        ValidateSettings(currentSettings);
        Pins::Catalog::Read(document.pins.Write());
        Pins::Catalog::Read(currentPins.Write());
        Plan plan;
        plan.settings = currentSettings;
        plan.pins = currentPins;
        for (const auto& key : document.settings.getMemberNames())
        {
            if (key != "profiles" && key != "backslash.savedTabGroups" && key != "backslash.sshConnections" && key != "$schema" && key != "$help" && key != "schemes" && key != "themes") { plan.settings[key] = document.settings[key]; }
        }
        if (document.settings.isMember("backslash.sshConnections"))
        {
            auto records = plan.settings.isMember("backslash.sshConnections") ? WorkspaceSsh::ReadRecords(plan.settings["backslash.sshConnections"]) : std::vector<WorkspaceSsh::Record>{};
            for (const auto& incoming : WorkspaceSsh::ReadRecords(document.settings["backslash.sshConnections"]))
            {
                const auto found = std::find_if(records.begin(), records.end(), [&](const auto& record) { return WorkspaceSsh::Lower(record.alias) == WorkspaceSsh::Lower(incoming.alias); });
                if (found == records.end()) { records.push_back(incoming); }
                else if (WorkspaceSsh::Lower(found->host) != WorkspaceSsh::Lower(incoming.host) || found->user != incoming.user || found->port != incoming.port || found->hostAlias != incoming.hostAlias || WorkspaceSsh::Lower(found->profile) != WorkspaceSsh::Lower(incoming.profile))
                {
                    plan.conflicts.push_back("Workspace SSH alias '" + incoming.alias + "' has different connection details. Nothing will be overwritten; resolve the alias conflict first.");
                }
            }
            if (records.size() > 512) { throw std::invalid_argument("Too many workspace SSH records."); }
            Json::Value values{ Json::arrayValue }; for (const auto& record : records) { values.append(WorkspaceSsh::Value(record)); }
            plan.settings["backslash.sshConnections"] = std::move(values);
        }
        // Definitions referenced by unrelated retained profiles must survive.
        // Schemes/themes use names as their existing stable identity.
        for (const auto* key : { "schemes", "themes" })
        {
            if (!document.settings.isMember(key)) { continue; }
            if (!plan.settings.isMember(key)) { plan.settings[key] = Json::Value{ Json::arrayValue }; }
            for (const auto& incoming : document.settings[key])
            {
                if (!incoming.isObject() || !incoming["name"].isString() || incoming["name"].asString().empty()) { throw std::invalid_argument(std::string{ key } + " definitions need nonempty names."); }
                Json::ArrayIndex index = plan.settings[key].size();
                for (Json::ArrayIndex i = 0; i < plan.settings[key].size(); ++i)
                {
                    if (plan.settings[key][i]["name"].isString() && SavedTabGroups::SameName(plan.settings[key][i]["name"].asString(), incoming["name"].asString())) { index = i; break; }
                }
                if (index == plan.settings[key].size()) { plan.settings[key].append(incoming); }
                else { plan.settings[key][index] = incoming; }
            }
        }
        plan.settings["profiles"]["defaults"] = document.settings["profiles"]["defaults"];
        auto& mergedProfiles = plan.settings["profiles"]["list"];
        for (const auto& incoming : document.settings["profiles"]["list"])
        {
            const auto id = ProfileId(incoming);
            Json::ArrayIndex index = mergedProfiles.size();
            for (Json::ArrayIndex i = 0; i < mergedProfiles.size(); ++i)
            {
                const auto existingId = ProfileId(mergedProfiles[i]);
                if (existingId == id) { index = i; }
                else if (SavedTabGroups::SameName(mergedProfiles[i]["name"].asString(), incoming["name"].asString()))
                {
                    plan.conflicts.emplace_back("Profile name '" + incoming["name"].asString() + "' belongs to another GUID. Rename the imported or destination profile, or resolve the GUID explicitly.");
                }
            }
            if (index == mergedProfiles.size()) { mergedProfiles.append(incoming); ++plan.profilesAdded; }
            else if (mergedProfiles[index] != incoming) { mergedProfiles[index] = incoming; ++plan.profilesUpdated; }
        }
        if (document.settings.isMember("backslash.savedTabGroups"))
        {
            auto merged = plan.settings.isMember("backslash.savedTabGroups") ? SavedTabGroups::ReadValue(plan.settings["backslash.savedTabGroups"]) : SavedTabGroups{};
            for (const auto& incoming : SavedTabGroups::ReadValue(document.settings["backslash.savedTabGroups"]).groups)
            {
                auto found = std::find_if(merged.groups.begin(), merged.groups.end(), [&](const auto& group) { return group.id == incoming.id; });
                for (const auto& group : merged.groups)
                {
                    if (group.id != incoming.id && SavedTabGroups::SameName(group.name, incoming.name)) { plan.conflicts.emplace_back("Group name '" + incoming.name + "' belongs to another ID. Rename a Group or resolve its ID explicitly."); }
                }
                if (found == merged.groups.end()) { merged.groups.push_back(incoming); ++plan.groupsAdded; }
                else
                {
                    SavedTabGroups before, after; before.groups.push_back(*found); after.groups.push_back(incoming);
                    if (before.ToValue() != after.ToValue()) { *found = incoming; ++plan.groupsUpdated; }
                }
            }
            plan.settings["backslash.savedTabGroups"] = merged.ToValue();
        }
        for (const auto& incoming : document.pins.pins)
        {
            auto found = std::find_if(plan.pins.pins.begin(), plan.pins.pins.end(), [&](const auto& pin) { return pin.id == incoming.id; });
            if (found == plan.pins.pins.end())
            {
                const auto sameDestination = std::find_if(plan.pins.pins.begin(), plan.pins.pins.end(), [&](const auto& pin) { return pin.profile == incoming.profile && pin.source == incoming.source && pin.kind == incoming.kind && pin.host == incoming.host && pin.path == incoming.path; });
                if (sameDestination != plan.pins.pins.end()) { plan.conflicts.emplace_back("An imported Pin destination already exists under another ID. Resolve the Pin IDs explicitly before importing."); }
                plan.pins.pins.push_back(incoming); ++plan.pinsAdded;
            }
            else if (*found != incoming) { *found = incoming; ++plan.pinsUpdated; }
        }
        if (mergedProfiles.size() > MaxProfiles || plan.pins.pins.size() > Pins::Catalog::MaxPins) { plan.conflicts.emplace_back("The merged setup exceeds the profile or Pin limit."); }
        if (plan.CanApply())
        {
            ValidateSettings(plan.settings);
            Pins::Catalog::Read(plan.pins.Write());
            CheckReferences(plan.settings, plan.pins, plan.conflicts);
        }
        if (plan.CanApply() && validateModel) { validateModel(plan.settings); }
        plan.changed = plan.settings != currentSettings || plan.pins.pins != currentPins.pins;
        plan.warnings.emplace_back("Defaults and matching profiles replace explicit settings; omitted profile fields inherit again. Unrelated profiles, Groups and Pins remain. Omitted global preferences remain unchanged.");
        if (document.settings.isMember("startupActions") || document.settings["backslash.startupWithSavedGroup"].asBool()) { plan.warnings.emplace_back("Imported startup preferences are retained but this import does not launch commands, Groups or sessions."); }
        return plan;
    }
}
