// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <algorithm>
#include <limits>
namespace Microsoft::Terminal::Backslash::TransfersViewLayout
{
    inline constexpr double MinimumSearchWidth = 240;
    inline constexpr double MinimumStatusWidth = 160;
    inline constexpr double MaximumStatusWidth = 240;
    inline constexpr double ColumnGap = 12;
    constexpr double StatusWidthForLabel(double textWidth) noexcept
    {
        if (!(textWidth >= 0 && textWidth <= std::numeric_limits<double>::max())) { return MinimumStatusWidth; }
        // Native content padding, dropdown chevron and breathing room.
        return std::clamp(textWidth + 64, MinimumStatusWidth, MaximumStatusWidth);
    }
    constexpr bool UseSideBySide(double availableWidth, double statusWidth) noexcept
    {
        return availableWidth > 0 && availableWidth <= std::numeric_limits<double>::max() &&
               statusWidth >= MinimumStatusWidth && statusWidth <= MaximumStatusWidth &&
               availableWidth >= MinimumSearchWidth + ColumnGap + statusWidth;
    }
}
