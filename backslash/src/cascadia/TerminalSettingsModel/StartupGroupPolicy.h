// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "SavedTabGroups.h"
#include <optional>
#include <cwctype>

namespace Microsoft::Terminal::Backslash::StartupGroupPolicy
{
    struct Context
    {
        bool initialAppLaunch = false;
        bool explicitCommand = false;
        bool connection = false;
        bool content = false;
        bool restoredLayout = false;
        bool settingsActions = false;
    };
    constexpr bool Eligible(const Context& c)
    {
        return c.initialAppLaunch && !c.explicitCommand && !c.connection && !c.content && !c.restoredLayout && !c.settingsActions;
    }
    struct Prepared
    {
        std::optional<SavedTabGroup> group;
        SavedTabGroups::Restore restore;
        std::string notice;
    };
    inline Prepared Prepare(const winrt::hstring& catalog, const winrt::hstring& selected,
                            const std::function<bool(const winrt::guid&, const winrt::hstring&)>& available,
                            const std::function<bool(const winrt::hstring&)>& folderExists)
    {
        Prepared p;
        try
        {
            const auto id = SavedTabGroups::Guid(winrt::to_string(selected));
            const auto store = SavedTabGroups::Read(catalog);
            const auto found = std::find_if(store.groups.begin(), store.groups.end(), [&](const auto& g) { return g.id == id; });
            if (found == store.groups.end()) { throw std::invalid_argument("The selected startup group is no longer available."); }
            p.restore = SavedTabGroups::Build(*found, available, folderExists);p.group = *found;
        }
        catch (const std::exception& e) { p.notice = std::string{ e.what() } + " Starting with the default profile instead."; }
        return p;
    }
    inline void FolderChoice(Prepared& p, bool accepted)
    {
        if (p.group && p.restore.folderFallbacks && !accepted)
        {
            p.group.reset();p.restore = {};p.notice = "Startup group canceled. Starting with the default profile instead.";
        }
    }
    inline bool Matches(std::wstring_view name, std::wstring_view query)
    {
        std::wstring a{ name },b{ query };
        const auto fold=[](wchar_t c) { return static_cast<wchar_t>(towlower(c)); };
        std::transform(a.begin(),a.end(),a.begin(),fold);std::transform(b.begin(),b.end(),b.begin(),fold);
        return a.find(b) != a.npos;
    }
    struct PickerChoice
    {
        winrt::hstring id;
        winrt::hstring name;
    };
    struct PickerState
    {
        std::vector<PickerChoice> choices;
        std::optional<PickerChoice> selected;
        std::string notice;
    };
    inline PickerState Picker(const winrt::hstring& catalog, const winrt::hstring& selected, std::wstring_view query = {})
    {
        PickerState result;
        try
        {
            const auto store = SavedTabGroups::Read(catalog);
            // Build suggestions even if the persisted selection is invalid, so
            // choosing another real group can repair it without losing the ID.
            std::string selectedId;
            try { if (!selected.empty()) { selectedId = SavedTabGroups::Guid(winrt::to_string(selected)); } }
            catch (...) { result.notice = "The startup group selection is invalid. Choose a listed group; the saved value is preserved until Save."; }
            for (const auto& group : store.groups)
            {
                PickerChoice choice{ winrt::to_hstring(group.id), winrt::to_hstring(group.name) };
                if (group.id == selectedId) { result.selected = choice; }
                if (Matches(choice.name, query)) { result.choices.emplace_back(std::move(choice)); }
            }
            if (!selected.empty() && !result.selected && result.notice.empty()) { result.notice = "The selected startup group is unavailable. Choose a listed group; otherwise startup uses the default profile with a notice."; }
        }
        catch (const std::exception& e) { result.notice = e.what(); }
        catch (const winrt::hresult_error& e) { result.notice = winrt::to_string(e.message()); }
        return result;
    }
    inline std::optional<PickerChoice> Choose(const winrt::hstring& catalog, const winrt::hstring& chosenId)
    {
        // An empty typed query is never a selection; names never resolve IDs.
        return chosenId.empty() ? std::nullopt : Picker(catalog, chosenId).selected;
    }
}
