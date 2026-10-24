// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include <cstdint>
#include <initializer_list>

namespace Microsoft::Terminal::Backslash
{
    constexpr float BezelBadgeSeam = 1;
    constexpr float BezelBadgeHorizontalContentInset = 10;
    constexpr float BezelBadgeVerticalContentInset = 2;
    constexpr std::uint16_t BezelBadgeFontWeight = 700;

    constexpr int BezelPaletteCount = 13;
    constexpr bool IsDarkBezelPalette(int color) noexcept { return color >= 4 && color < BezelPaletteCount; }

    constexpr std::uint32_t BezelPaletteArgb(int color) noexcept
    {
        // Original plastic swatches keep their indices and RGB values.
        constexpr std::uint32_t colors[] = {
            0xffe8e8dd, 0xffe5dabd, 0xffd9c796, 0xffc4ab68,
            0xff666b6c, 0xff414648, 0xff222629, 0xff566572, 0xff626a55,
            0xff7c5955, 0xff95814f, 0xff3b0b66, 0xff073d42
        };
        static_assert(sizeof(colors) / sizeof(colors[0]) == BezelPaletteCount);
        return colors[color >= 0 && color < BezelPaletteCount ? color : 0];
    }

    constexpr std::uint32_t BlendBezelRgb(std::uint32_t casing, std::uint32_t tint, unsigned int percent) noexcept
    {
        auto result = std::uint32_t{ 0xff000000 };
        for (const auto shift : { 0u, 8u, 16u })
        {
            const auto channel = (((casing >> shift) & 255u) * (100u - percent) +
                                  ((tint >> shift) & 255u) * percent) / 100u;
            result |= channel << shift;
        }
        return result;
    }

    struct DarkBezelCasingMaterial
    {
        std::uint32_t top, bottom, rimTop, rimBottom, recessTop, recessBottom;
    };

    constexpr DarkBezelCasingMaterial GetDarkBezelCasingMaterial(int color) noexcept
    {
        const auto casing = BezelPaletteArgb(color);
        // Restrained neutral highlights retain hue and depth on dark finishes.
        // Every layer is opaque and independent of the terminal background brush.
        return { BlendBezelRgb(casing, 0xffc5ccc7, 10),
                 BlendBezelRgb(casing, 0xff000000, 14),
                 BlendBezelRgb(casing, 0xffe2e6dd, 27),
                 BlendBezelRgb(casing, 0xff000000, 45),
                 BlendBezelRgb(casing, 0xff000000, 68),
                 BlendBezelRgb(casing, 0xffa9b1a9, 16) };
    }

    struct BezelGradientDirection
    {
        float startX, startY, endX, endY;
    };
    constexpr BezelGradientDirection BezelCasingGradient{ 0, 0.5f, 1, 0.5f };
    constexpr BezelGradientDirection BezelRecessGradient{ 0, 1, 1, 0 };

    struct BezelCasingMaterial
    {
        std::uint32_t leftShadow, rightHighlight;
    };

    constexpr BezelCasingMaterial GetBezelCasingMaterial(int color) noexcept
    {
        if (IsDarkBezelPalette(color))
        {
            const auto casing = GetDarkBezelCasingMaterial(color);
            return { casing.bottom, casing.top };
        }
        const auto casing = BezelPaletteArgb(color);
        return { BlendBezelRgb(casing, 0xff000000, 9), BlendBezelRgb(casing, 0xffffffff, 14) };
    }

    struct BezelRecessMaterial
    {
        std::uint32_t shadow, highlight;
    };

    constexpr BezelRecessMaterial GetBezelRecessMaterial(int color) noexcept
    {
        if (IsDarkBezelPalette(color))
        {
            const auto casing = GetDarkBezelCasingMaterial(color);
            return { casing.recessTop, casing.recessBottom };
        }
        const auto casing = BezelPaletteArgb(color);
        // A broad gap stays shadowed across its width, with a softer lower edge.
        return { BlendBezelRgb(casing, 0xff000000, 63), BlendBezelRgb(casing, 0xff000000, 45) };
    }

    struct BezelBadgeMaterial
    {
        std::uint32_t face, seam, lowerSeam, lettering;
    };

    constexpr BezelBadgeMaterial GetBezelBadgeMaterial(int color) noexcept
    {
        const auto casing = BezelPaletteArgb(color);
        if (IsDarkBezelPalette(color))
        {
            return { BlendBezelRgb(casing, 0xff111517, 22),
                     BlendBezelRgb(casing, 0xff000000, 60),
                     BlendBezelRgb(casing, 0xff9aa29a, 12),
                     0xffeeefe5 };
        }
        // Original light-plastic material stays unchanged.
        // Equivalent to a restrained neutral-gray overlay on the casing. Bake the
        // result into opaque colors so terminal acrylic/opacity never affects it.
        return { BlendBezelRgb(casing, 0xff707070, 22),
                 BlendBezelRgb(casing, 0xff000000, 60),
                 BlendBezelRgb(casing, 0xff000000, 42),
                 0xff2a2d2b };
    }
}
