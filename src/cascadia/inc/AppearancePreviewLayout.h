// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include "RetroBezelGeometry.h"
#include <algorithm>
#include <cmath>

namespace Microsoft::Terminal::Backslash
{
    constexpr float AppearancePreviewMinimumViewportWidth = 320;
    constexpr float AppearancePreviewMinimumViewportHeight = 220;
    constexpr float AppearancePreviewBorderExtent = 2; // One DIP stroke on each side.
    constexpr float AppearancePreviewPreferredWidth = 640;
    constexpr float AppearancePreviewMaximumPreferredWidth = 1000;
    constexpr int AppearancePreviewMinimumColumns = 40;
    constexpr int AppearancePreviewMinimumRows = 10;

    struct AppearancePreviewLayout
    {
        float width, height, viewportWidth, viewportHeight;
        bool requiresHorizontalScroll;
    };

    // GetProposedDimensions returns pixels including bezel, padding and scrollbar.
    // Convert once to DIPs; add only the preview's stroke and readable-sample floor.
    // Narrow hosts scroll this full-size preview rather than crushing the aperture.
    inline AppearancePreviewLayout GetAppearancePreviewLayout(float availableWidth,
                                                              BezelInsets bezel,
                                                              float requiredWidthPixels,
                                                              float requiredHeightPixels,
                                                              float rasterizationScale) noexcept
    {
        const auto scale = std::isfinite(rasterizationScale) && rasterizationScale > 0 ? rasterizationScale : 1.0f;
        const auto requiredWidth = std::isfinite(requiredWidthPixels) ? requiredWidthPixels / scale - bezel.Width() : 0.0f;
        const auto requiredHeight = std::isfinite(requiredHeightPixels) ? requiredHeightPixels / scale - bezel.Height() : 0.0f;
        const auto minimumWidth = std::max(AppearancePreviewMinimumViewportWidth, requiredWidth) + bezel.Width() + AppearancePreviewBorderExtent;
        const auto height = std::max(AppearancePreviewMinimumViewportHeight, requiredHeight) + bezel.Height() + AppearancePreviewBorderExtent;
        const auto hostWidth = std::isfinite(availableWidth) && availableWidth > 0 ? availableWidth : AppearancePreviewPreferredWidth;
        const auto width = std::max(minimumWidth, std::min(hostWidth, AppearancePreviewMaximumPreferredWidth));
        return { width, height, width - bezel.Width() - AppearancePreviewBorderExtent,
                 height - bezel.Height() - AppearancePreviewBorderExtent,
                 std::isfinite(availableWidth) && availableWidth > 0 && width > availableWidth };
    }
}
