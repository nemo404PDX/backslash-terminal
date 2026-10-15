// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "SavedTabGroups.h"
#include <optional>

namespace Microsoft::Terminal::Backslash::GroupSavePolicy
{
    enum class PaneKind { Terminal, Settings, Unsupported };
    struct Selection
    {
        std::vector<uint32_t> indices;
        uint32_t focusedTab = 0;
        bool skippedSettings = false;
    };
    inline Selection Select(const std::vector<std::vector<PaneKind>>& tabs, uint32_t focused)
    {
        Selection result;
        for (uint32_t i = 0; i < tabs.size(); ++i)
        {
            const auto& panes = tabs[i];
            if (panes.size() == 1 && panes.front() == PaneKind::Settings) { result.skippedSettings = true;continue; }
            if (panes.empty() || std::any_of(panes.begin(),panes.end(),[](auto kind) { return kind != PaneKind::Terminal; }))
            { throw std::invalid_argument("Only terminal pane layouts can be saved. Mixed Settings or other nonterminal panes cannot be omitted safely."); }
            if (i == focused) { result.focusedTab = static_cast<uint32_t>(result.indices.size()); }
            result.indices.push_back(i);
        }
        if (result.indices.empty()) { throw std::invalid_argument("No terminal tabs to save."); }
        return result; // Settings/invalid focus deterministically uses the first eligible tab.
    }
    inline bool ShowSettingsNotice(const Selection& selection, bool dismissed, bool alreadyShown)
    { return selection.skippedSettings && !dismissed && !alreadyShown; }
    inline bool PersistNoticeChoice(bool continued, bool checked, bool saved)
    { return continued && checked && saved; }
    inline bool SameGroup(const SavedTabGroup& a, const SavedTabGroup& b)
    {
        SavedTabGroups left,right;left.groups.push_back(a);right.groups.push_back(b);
        return left.ToValue() == right.ToValue();
    }
    inline bool AppendOnly(const SavedTabGroups& before, const SavedTabGroups& after)
    {
        if (after.groups.size() <= before.groups.size()) { return false; }
        for (size_t i = 0; i < before.groups.size(); ++i) { if (!SameGroup(before.groups[i],after.groups[i])) { return false; } }
        return true;
    }
    inline winrt::hstring MergeAddedGroups(const winrt::hstring& beforeJson, const winrt::hstring& afterJson, const winrt::hstring& draftJson)
    {
        const auto before = SavedTabGroups::Read(beforeJson),after = SavedTabGroups::Read(afterJson);
        if (!AppendOnly(before,after)) { throw std::invalid_argument("Saved groups changed. Save or reset the Settings draft before trying again."); }
        auto draft = SavedTabGroups::Read(draftJson);
        for (size_t i = before.groups.size(); i < after.groups.size(); ++i)
        {
            const auto& addition = after.groups[i];
            const auto existing = std::find_if(draft.groups.begin(),draft.groups.end(),[&](const auto& group) { return group.id == addition.id; });
            if (existing != draft.groups.end())
            {
                if (!SameGroup(*existing,addition)) { throw std::invalid_argument("This group conflicts with unsaved Settings edits. Choose another name or resolve those edits first."); }
                continue; // Watcher replay must not duplicate an already merged addition.
            }
            if (std::any_of(draft.groups.begin(),draft.groups.end(),[&](const auto& group) { return SavedTabGroups::SameName(group.name,addition.name); }))
            { throw std::invalid_argument("This group name conflicts with unsaved Settings edits. Choose another name or resolve those edits first."); }
            draft.groups.push_back(addition);
        }
        return draft.Json(); // Validate bounds without changing the caller's draft.
    }
    inline bool OnlyGroupsDiffer(Json::Value before, Json::Value after)
    {
        before.removeMember("backslash.savedTabGroups");after.removeMember("backslash.savedTabGroups");return before == after;
    }
}
