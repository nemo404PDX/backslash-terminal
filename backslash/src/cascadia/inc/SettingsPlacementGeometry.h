// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <limits>
namespace Microsoft::Terminal::Backslash
{
    // Dimensions are the actual active leaf allocation, not the entire tab.
    // Allow the maximum two 2-DIP pane borders in each dimension.
    constexpr bool SettingsFitsRightSplit(double activeWidth, double activeHeight, double minimumWidth, double minimumHeight) noexcept
    {
        return activeWidth <= std::numeric_limits<double>::max() && activeHeight <= std::numeric_limits<double>::max() &&
               minimumWidth <= std::numeric_limits<double>::max() && minimumHeight <= std::numeric_limits<double>::max() &&
               activeWidth > 0 && activeHeight > 0 && minimumWidth > 0 && minimumHeight > 0 &&
               activeWidth * 0.5 >= minimumWidth + 4 && activeHeight >= minimumHeight + 4;
    }
}

