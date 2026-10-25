// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include "RetroBezelGeometry.h"
#include <algorithm>
#include <cmath>

namespace Microsoft::Terminal::Backslash
{
    constexpr float BezelBadgeAspectRatio = 527.0f / 100.0f;
    constexpr float BezelBadgeCornerInset = 12;
    constexpr float BezelBadgeBandPadding = 4;

    struct BezelBadgeLayout
    {
        float left = 0, top = 0, width = 0, height = 0;
        bool visible = false;
    };

    // Values 1/2 retain the original Bottom/Top enum identities (now centered).
    // A Canvas hosts this decoration so it contributes no desired terminal size.
    inline BezelBadgeLayout GetBezelBadgeLayout(bool enabled, int placement, float width, float height, BezelInsets insets) noexcept
    {
        if (!enabled || placement < 1 || placement > 6 || !std::isfinite(width) || !std::isfinite(height)) { return {}; }
        const bool top = placement == 2 || placement == 5 || placement == 6;
        const auto shoulder = (top ? insets.top : insets.bottom) - BezelInnerClearance - BezelRecessThickness;
        const auto band = std::min(shoulder, height) - BezelOuterRimThickness;
        // Side rails are exclusion zones. Anchor and scale inside the horizontal
        // screen-opening span, with the same safe inset for every casing size.
        const auto opening = GetBezelApertureBounds(width, height, insets).opening;
        const auto allowedLeft = opening.left + BezelBadgeCornerInset;
        const auto allowedRight = opening.right - BezelBadgeCornerInset;
        const auto availableWidth = allowedRight - allowedLeft;
        const auto badgeHeight = std::min({ BezelBadgeHeight, availableWidth / BezelBadgeAspectRatio, band - 2 * BezelBadgeBandPadding });
        if (!(badgeHeight > 0)) { return {}; }
        const auto badgeWidth = badgeHeight * BezelBadgeAspectRatio;
        const auto left = placement == 3 || placement == 5 ? allowedLeft :
                          placement == 4 || placement == 6 ? allowedRight - badgeWidth : (allowedLeft + allowedRight - badgeWidth) / 2;
        const auto offset = BezelOuterRimThickness + (band - badgeHeight) / 2;
        return { left, top ? offset : height - offset - badgeHeight, badgeWidth, badgeHeight, true };
    }
}
