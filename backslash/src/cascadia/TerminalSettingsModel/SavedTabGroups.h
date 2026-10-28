// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include <winrt/Microsoft.Terminal.Settings.Model.h>
#include <json/json.h>
#include "../../types/inc/utils.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Microsoft::Terminal::Backslash
{
    namespace M = winrt::Microsoft::Terminal::Settings::Model;
    struct SavedTabGroup
    {
        std::string id;
        std::string name;
        bool capturedFolders = false;
        uint32_t focusedTab = 0;
        std::vector<Json::Value> tabs;
    };

    // A private, restricted layout schema. Never deserialize arbitrary startup
    // actions: command lines, session/content IDs, environments and terminal data
    // are deliberately absent. Unknown versions remain raw in global settings.
    struct SavedTabGroups
    {
        std::vector<SavedTabGroup> groups;
        static bool SafeText(const std::string& value, size_t limit)
        {
            return value.size() <= limit && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
        }
        static bool LocalFolder(const std::string& value)
        {
            return SafeText(value, 32760) && value.size() >= 3 &&
                ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z')) &&
                value[1] == ':' && (value[2] == '\\' || value[2] == '/') && value.find('%') == std::string::npos;
        }
        static std::string Guid(const std::string& value)
        {
            if (value.size() != 38 || value.front() != '{' || value.back() != '}') { throw std::invalid_argument("A saved group or profile has an invalid GUID."); }
            const winrt::guid id = ::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(value).c_str());
            if (id == winrt::guid{}) { throw std::invalid_argument("A saved group or profile has an empty GUID."); }
            return winrt::to_string(::Microsoft::Console::Utils::GuidToString(id));
        }
        static std::string Name(std::string value)
        {
            const auto first = value.find_first_not_of(" \t");
            if (first == std::string::npos) { throw std::invalid_argument("Enter a group name."); }
            value = value.substr(first, value.find_last_not_of(" \t") - first + 1);
            if (!SafeText(value, 320) || winrt::to_hstring(value).size() > 80) { throw std::invalid_argument("Group names must contain 1 to 80 characters without control characters."); }
            return value;
        }
        static bool SameName(const std::string& a, const std::string& b)
        {
            const auto aw = winrt::to_hstring(a), bw = winrt::to_hstring(b);
            return CompareStringOrdinal(aw.c_str(), static_cast<int>(aw.size()), bw.c_str(), static_cast<int>(bw.size()), TRUE) == CSTR_EQUAL;
        }
        static void Keys(const Json::Value& value, const std::set<std::string>& allowed)
        {
            if (!value.isObject()) { throw std::invalid_argument("Invalid saved group layout object."); }
            for (const auto& key : value.getMemberNames())
            {
                if (!allowed.contains(key)) { throw std::invalid_argument("Unrecognized saved group data; it was preserved but cannot be opened or edited."); }
            }
        }
        static std::string Text(const Json::Value& value, const char* key)
        {
            if (!value[key].isString()) { throw std::invalid_argument("Invalid saved group text field."); }
            return value[key].asString();
        }
        static SavedTabGroups ReadValue(const Json::Value& value)
        {
            Keys(value, { "version", "groups" });
            if (!value["version"].isUInt() || value["version"].asUInt() != 1) { throw std::invalid_argument("Unsupported saved group version. Its data will be preserved unchanged."); }
            if (!value["groups"].isArray() || value["groups"].size() > 50) { throw std::invalid_argument("Saved groups must be an array of at most 50 groups."); }
            SavedTabGroups result;
            uint32_t totalPanes = 0;
            for (const auto& item : value["groups"])
            {
                Keys(item, { "id", "name", "folderMode", "focusedTab", "tabs" });
                SavedTabGroup group;
                group.id = Guid(Text(item, "id")); group.name = Name(Text(item, "name"));
                const auto mode = Text(item, "folderMode");
                if (mode != "profileDefault" && mode != "captured") { throw std::invalid_argument("Invalid saved group folder mode."); }
                group.capturedFolders = mode == "captured";
                if (!item["tabs"].isArray() || item["tabs"].empty() || item["tabs"].size() > 32 ||
                    !item["focusedTab"].isUInt() || item["focusedTab"].asUInt() >= item["tabs"].size()) { throw std::invalid_argument("Invalid saved group tab order or focus."); }
                group.focusedTab = item["focusedTab"].asUInt();
                for (const auto& tab : item["tabs"])
                {
                    if (!tab.isArray() || tab.empty() || tab.size() > 512) { throw std::invalid_argument("Invalid saved group pane layout."); }
                    uint32_t panes = 0;
                    Json::Value clean{ Json::arrayValue };
                    for (auto step : tab)
                    {
                        const auto kind = Text(step, "kind");
                        if (kind == "newTab" || kind == "split")
                        {
                            Keys(step, { "kind", "profile", "source", "folder", "direction", "size" });
                            if ((kind == "newTab") != (panes == 0)) { throw std::invalid_argument("Each group tab must start with exactly one new-tab step."); }
                            step["profile"] = Guid(Text(step, "profile"));
                            if (!SafeText(Text(step, "source"), 1024)) { throw std::invalid_argument("Invalid profile source."); }
                            if (step.isMember("folder") && (step["source"] == "Windows.Terminal.SSH" || step["source"] == "Backslash.Terminal.SSH" || step["source"] == "Windows.Terminal.Wsl" || !LocalFolder(Text(step, "folder")))) { throw std::invalid_argument("Only bounded, absolute local drive folders can be captured."); }
                            if (kind == "split")
                            {
                                const auto direction = Text(step, "direction");
                                if ((direction != "right" && direction != "down") || !step["size"].isNumeric() || !std::isfinite(step["size"].asFloat()) || step["size"].asFloat() <= 0 || step["size"].asFloat() >= 1) { throw std::invalid_argument("Invalid saved split direction or ratio."); }
                            }
                            else if (step.isMember("direction") || step.isMember("size")) { throw std::invalid_argument("Unexpected new-tab geometry."); }
                            if (++panes > 64 || ++totalPanes > 1024) { throw std::invalid_argument("Saved group pane limit exceeded."); }
                        }
                        else if (kind == "focus")
                        {
                            Keys(step, { "kind", "id" });
                            if (!step["id"].isUInt() || step["id"].asUInt() >= panes) { throw std::invalid_argument("Invalid saved pane focus."); }
                        }
                        else if (kind == "moveFocus")
                        {
                            Keys(step, { "kind", "direction" });
                            const auto direction = Text(step, "direction");
                            if (!panes || (direction != "previous" && direction != "next")) { throw std::invalid_argument("Invalid saved layout traversal."); }
                        }
                        else { throw std::invalid_argument("Unsupported action in a saved tab group."); }
                        clean.append(std::move(step));
                    }
                    group.tabs.emplace_back(std::move(clean));
                }
                for (const auto& prior : result.groups)
                {
                    if (prior.id == group.id || SameName(prior.name, group.name)) { throw std::invalid_argument("A group with that ID or name already exists."); }
                }
                result.groups.emplace_back(std::move(group));
            }
            return result;
        }
        static SavedTabGroups Read(const winrt::hstring& text)
        {
            Json::CharReaderBuilder builder; Json::Value value; std::string errors;
            const auto utf8 = winrt::to_string(text); const std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
            if (!reader->parse(utf8.data(), utf8.data() + utf8.size(), &value, &errors)) { throw std::invalid_argument("Saved groups contain invalid JSON."); }
            return ReadValue(value);
        }
        Json::Value ToValue() const
        {
            Json::Value root{ Json::objectValue };root["version"] = 1;root["groups"] = Json::Value{ Json::arrayValue };
            for (const auto& group : groups)
            {
                Json::Value item{ Json::objectValue };item["id"] = group.id;item["name"] = group.name;
                item["folderMode"] = group.capturedFolders ? "captured" : "profileDefault";item["focusedTab"] = group.focusedTab;
                item["tabs"] = Json::Value{ Json::arrayValue };for (const auto& tab : group.tabs) { item["tabs"].append(tab); }
                root["groups"].append(std::move(item));
            }
            return root;
        }
        winrt::hstring Json() const
        {
            const auto value = ToValue(); ReadValue(value);
            Json::StreamWriterBuilder builder; return winrt::to_hstring(Json::writeString(builder, value));
        }
        // Capture receives safe per-pane args from the application factory. It
        // still copies an allowlist, so future upstream fields cannot leak in.
        static Json::Value CaptureTab(const std::vector<M::ActionAndArgs>& actions, const std::function<winrt::hstring(const winrt::guid&)>& source)
        {
            Json::Value tab{ Json::arrayValue };
            for (const auto& action : actions)
            {
                Json::Value step{ Json::objectValue }; M::NewTerminalArgs terminal{ nullptr };
                if (action.Action() == M::ShortcutAction::NewTab)
                {
                    step["kind"] = "newTab"; terminal = action.Args().as<M::NewTabArgs>().ContentArgs().as<M::NewTerminalArgs>();
                }
                else if (action.Action() == M::ShortcutAction::SplitPane)
                {
                    const auto split = action.Args().as<M::SplitPaneArgs>();
                    if (split.SplitMode() != M::SplitType::Manual || (split.SplitDirection() != M::SplitDirection::Right && split.SplitDirection() != M::SplitDirection::Down)) { throw std::invalid_argument("Unsupported saved split."); }
                    step["kind"] = "split";step["direction"] = split.SplitDirection() == M::SplitDirection::Right ? "right" : "down";step["size"] = split.SplitSize();
                    terminal = split.ContentArgs().as<M::NewTerminalArgs>();
                }
                else if (action.Action() == M::ShortcutAction::FocusPane) { step["kind"] = "focus";step["id"] = action.Args().as<M::FocusPaneArgs>().Id(); }
                else if (action.Action() == M::ShortcutAction::MoveFocus)
                {
                    const auto direction = action.Args().as<M::MoveFocusArgs>().FocusDirection();
                    if (direction != M::FocusDirection::PreviousInOrder && direction != M::FocusDirection::NextInOrder) { throw std::invalid_argument("Unsupported group focus traversal."); }
                    step["kind"] = "moveFocus";step["direction"] = direction == M::FocusDirection::PreviousInOrder ? "previous" : "next";
                }
                else if (action.Action() == M::ShortcutAction::RenameTab || action.Action() == M::ShortcutAction::SetTabColor || action.Action() == M::ShortcutAction::TogglePaneZoom) { continue; }
                else { throw std::invalid_argument("Only terminal pane layouts can be saved."); }
                if (terminal)
                {
                    step["profile"] = Guid(winrt::to_string(terminal.Profile()));
                    const winrt::guid id = ::Microsoft::Console::Utils::GuidFromString(terminal.Profile().c_str());step["source"] = winrt::to_string(source(id));
                    if (!terminal.StartingDirectory().empty()) { step["folder"] = winrt::to_string(terminal.StartingDirectory()); }
                }
                tab.append(std::move(step));
            }
            return tab;
        }
        struct Restore
        {
            std::vector<std::vector<M::ActionAndArgs>> tabs;
            uint32_t folderFallbacks = 0;
        };
        static Restore Build(const SavedTabGroup& group, const std::function<bool(const winrt::guid&, const winrt::hstring&)>& available,
                             const std::function<bool(const winrt::hstring&)>& folderExists)
        {
            SavedTabGroups check;check.groups.push_back(group);check.Json();
            Restore result;
            for (const auto& tab : group.tabs)
            {
                std::vector<M::ActionAndArgs> actions;
                for (const auto& step : tab)
                {
                    M::ActionAndArgs action;const auto kind = step["kind"].asString();
                    if (kind == "newTab" || kind == "split")
                    {
                        const auto profile = winrt::to_hstring(step["profile"].asString());
                        const winrt::guid id = ::Microsoft::Console::Utils::GuidFromString(profile.c_str());
                        if (!available(id, winrt::to_hstring(step["source"].asString()))) { throw std::invalid_argument("A referenced profile is missing, disabled, replaced or requires elevation. Restore the exact profile before opening this group: " + step["profile"].asString()); }
                        M::NewTerminalArgs args;args.Profile(profile);
                        if (group.capturedFolders)
                        {
                            if (step.isMember("folder") && folderExists(winrt::to_hstring(step["folder"].asString()))) { args.StartingDirectory(winrt::to_hstring(step["folder"].asString())); }
                            else { ++result.folderFallbacks; }
                        }
                        if (kind == "newTab") { action.Action(M::ShortcutAction::NewTab);action.Args(M::NewTabArgs{ args }); }
                        else { action.Action(M::ShortcutAction::SplitPane);action.Args(M::SplitPaneArgs{ M::SplitType::Manual, step["direction"] == "right" ? M::SplitDirection::Right : M::SplitDirection::Down, step["size"].asFloat(), args }); }
                    }
                    else if (kind == "focus") { action.Action(M::ShortcutAction::FocusPane);action.Args(M::FocusPaneArgs{ step["id"].asUInt() }); }
                    else { action.Action(M::ShortcutAction::MoveFocus);action.Args(M::MoveFocusArgs{ step["direction"] == "previous" ? M::FocusDirection::PreviousInOrder : M::FocusDirection::NextInOrder }); }
                    actions.emplace_back(std::move(action));
                }
                result.tabs.emplace_back(std::move(actions));
            }
            return result;
        }
    };
}
