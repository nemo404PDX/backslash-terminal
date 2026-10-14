// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

namespace Microsoft::Terminal::Backslash
{
    // The surround is separate from the terminal's opacity/acrylic brush.
    constexpr unsigned int BezelSurroundArgb = 0xff000000;
    constexpr float BezelOuterRadius = 0; // External casing silhouette stays square.
    constexpr float BezelRecessOuterRadius = 3;
    constexpr float BezelInnerRadius = 6;
    constexpr float BezelInnerClearance = 6;
    constexpr float BezelRecessThickness = 3;
    constexpr float BezelOuterRimThickness = 2;
    constexpr float BezelBadgeHeight = 22;

    constexpr float ClampBezelCornerRadius(float requested, float width, float height) noexcept
    {
        if (!(requested > 0 && width > 0 && height > 0)) { return 0; }
        const auto limit = (width < height ? width : height) / 2;
        return requested < limit ? requested : limit;
    }

    struct BezelRecessRadii { float outer, inner; };
    constexpr BezelRecessRadii GetBezelRecessRadii(float openingWidth, float openingHeight) noexcept
    {
        const auto inner = ClampBezelCornerRadius(BezelInnerRadius, openingWidth, openingHeight);
        // Retain the smaller outer/larger inner relationship even in tiny layouts.
        const auto outer = ClampBezelCornerRadius(BezelRecessOuterRadius * inner / BezelInnerRadius,
                                                  openingWidth + 2 * BezelRecessThickness,
                                                  openingHeight + 2 * BezelRecessThickness);
        return { outer, inner };
    }

    // Flat chin runs from the outer rim to the recess shoulder.
    // Return distance from the top/bottom outer edge to the entire plate, not its label.
    constexpr float GetBezelBadgeOuterOffset(float sideInset) noexcept
    {
        const auto shoulder = sideInset - BezelInnerClearance - BezelRecessThickness;
        return (BezelOuterRimThickness + shoulder - BezelBadgeHeight) / 2;
    }
    static_assert(BezelInnerClearance >= BezelInnerRadius); // Arc never covers terminal cells.

    // DIPs, independent of terminal text padding and renderer pixels.
    struct BezelInsets
    {
        float left = 0, top = 0, right = 0, bottom = 0;
        constexpr float Width() const noexcept { return left + right; }
        constexpr float Height() const noexcept { return top + bottom; }
    };

    constexpr BezelInsets GetBezelInsets(bool enabled, int size, int badge, bool balance = false) noexcept
    {
        if (!enabled) { return {}; }
        const auto edge = size == 0 ? 18.0f : size == 2 ? 40.0f : 28.0f;
        BezelInsets result{ edge, edge, edge, edge };
        constexpr auto chin = 28.0f;
        if (badge == 1) { result.bottom += chin; if (balance) { result.top += chin; } }
        if (badge == 2) { result.top += chin; if (balance) { result.bottom += chin; } }
        return result;
    }
    struct BezelScreenBounds
    {
        float left, top, right, bottom;
        constexpr bool Contains(float x, float y) const noexcept
        {
            return x >= left && x < right && y >= top && y < bottom;
        }
    };

    constexpr BezelScreenBounds GetBezelScreenBounds(float width, float height, BezelInsets insets) noexcept
    {
        return { insets.left, insets.top, width - insets.right, height - insets.bottom };
    }

    // Window mode is an input only: changing the profile setting cannot change it.
    constexpr BezelInsets GetBezelInsetsForMode(bool enabled, int size, int badge, bool center, bool fullscreen) noexcept
    {
        return GetBezelInsets(enabled, size, badge, center && fullscreen);
    }

}
