// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

namespace Microsoft::Terminal::Backslash
{
    // The surround is separate from the terminal's opacity/acrylic brush.
    constexpr unsigned int BezelSurroundArgb = 0xff000000;
    constexpr float BezelOuterRadius = 0; // External casing silhouette stays square.
    constexpr float BezelRecessOuterRadius = 6;
    constexpr float BezelInnerRadius = 16;
    constexpr float BezelInnerClearance = 6;
    constexpr float BezelRecessThickness = 18;
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
    // The actual screen corner lies clearance DIPs inside both opening edges.
    // When radius exceeds clearance, that corner must still lie inside the arc.
    constexpr bool BezelOpeningContainsInsetCorner(float radius, float clearance) noexcept
    {
        if (radius <= clearance) { return true; }
        const auto delta = radius - clearance;
        return 2 * delta * delta <= radius * radius;
    }
    static_assert(BezelOpeningContainsInsetCorner(BezelInnerRadius, BezelInnerClearance));

    // DIPs, independent of terminal text padding and renderer pixels.
    struct BezelInsets
    {
        float left = 0, top = 0, right = 0, bottom = 0;
        constexpr float Width() const noexcept { return left + right; }
        constexpr float Height() const noexcept { return top + bottom; }
    };

    // This preset describes visible casing, outside the separately sized recess.
    constexpr float GetVisibleBezelCasingWidth(int size) noexcept
    {
        return size == 0 ? 40.0f : size == 2 ? 72.0f : 56.0f;
    }

    constexpr BezelInsets GetBezelInsets(bool enabled, int size, int badge, bool balance = false, int filler = -1, int top = -1, int right = -1, int bottom = -1, int left = -1) noexcept
    {
        if (!enabled) { return {}; }
        const auto inset = [size](int overrideSize) constexpr {
            return GetVisibleBezelCasingWidth(overrideSize >= 0 && overrideSize <= 2 ? overrideSize : size) + BezelRecessThickness + BezelInnerClearance;
        };
        BezelInsets result{ inset(left), inset(top), inset(right), inset(bottom) };
        constexpr auto chin = 28.0f;
        // Omitted filler retains this helper's historical call contract. Product
        // paths always pass the independent profile filler explicitly.
        const auto extra = filler < 0 ? (badge == 1 ? 1 : badge == 2 ? 2 : 0) : filler;
        if (extra == 1) { result.bottom += chin; if (balance) { result.top += chin; } }
        if (extra == 2) { result.top += chin; if (balance) { result.bottom += chin; } }
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

    struct BezelApertureBounds
    {
        BezelScreenBounds screen, opening, recess;
    };

    constexpr BezelApertureBounds GetBezelApertureBounds(float width, float height, BezelInsets insets) noexcept
    {
        const auto screen = GetBezelScreenBounds(width, height, insets);
        const BezelScreenBounds opening{ screen.left - BezelInnerClearance, screen.top - BezelInnerClearance,
                                         screen.right + BezelInnerClearance, screen.bottom + BezelInnerClearance };
        const BezelScreenBounds recess{ opening.left - BezelRecessThickness, opening.top - BezelRecessThickness,
                                        opening.right + BezelRecessThickness, opening.bottom + BezelRecessThickness };
        return { screen, opening, recess };
    }

    // Window mode is an input only: changing the profile setting cannot change it.
    constexpr BezelInsets GetBezelInsetsForMode(bool enabled, int size, int badge, bool center, bool fullscreen, int filler = -1, int top = -1, int right = -1, int bottom = -1, int left = -1) noexcept
    {
        return GetBezelInsets(enabled, size, badge, center && fullscreen, filler, top, right, bottom, left);
    }

}
