// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

namespace Microsoft::Terminal
{
    // Original TerminalPage tab policy, isolated for non-UI regression coverage.
    // Profile decoration/centering has no input to this policy.
    constexpr bool ShouldShowTerminalTabs(bool focusMode, bool fullscreen, bool showTabsFullscreen,
                                         bool showTabsInTitlebar, bool multipleTabs, bool alwaysShowTabs) noexcept
    {
        return !focusMode && (!fullscreen || showTabsFullscreen) &&
               (showTabsInTitlebar || multipleTabs || alwaysShowTabs);
    }
}
