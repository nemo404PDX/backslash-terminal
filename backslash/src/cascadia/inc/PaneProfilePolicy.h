// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
namespace Microsoft::Terminal::Backslash::PaneProfilePolicy
{
    constexpr bool Eligible(bool terminalLeaf, bool readOnly, bool pinned, bool moving, bool busy) noexcept
    {
        return terminalLeaf && !readOnly && !pinned && !moving && !busy;
    }
    constexpr bool SaveSuppression(bool affirmative, bool targetValid, bool committed, bool checked) noexcept
    {
        return affirmative && targetValid && committed && checked;
    }
    constexpr bool UpdateWindowAppearance(bool selectedTab) noexcept { return selectedTab; }
    constexpr bool RestoreKeyboardFocus(bool selectedTab, bool windowActive) noexcept { return selectedTab && windowActive; }
    // Commit only after wiring and refreshing succeed. Pre-commit failure restores
    // the exact old content; close is called once and only after the commit.
    template<typename Content, typename Exchange, typename Wire, typename Refresh, typename Close, typename Restore>
    void Replace(Content expected, Content replacement, Exchange exchange, Wire wire, Refresh refresh, Close close, Restore restore)
    {
        Content old{};
        try
        {
            old = exchange(replacement);
            wire(replacement);
            refresh();
        }
        catch (...)
        {
            if (old) { exchange(old); }
            wire(expected);
            restore(); // Undo title/progress/MRU/window changes made before refresh failed.
            throw;
        }
        close(old);
    }
}
