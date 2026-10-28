// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace Microsoft::Terminal::Backslash
{
    // Straight sRGB companion to PhosphorTint in custom_shader_ps.hlsl.
    // Callers preserve alpha/brush opacity; zero strength is exact identity.
    inline std::array<float, 3> MapPhosphorRgb(std::array<float, 3> source, std::array<float, 3> endpoint, float strength) noexcept
    {
        if (strength <= 0) { return source; }
        const auto decode = [](float v) { return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
        const auto encode = [](float v) { return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f; };
        for (auto& v : source) { v = decode(std::clamp(v, 0.0f, 1.0f)); }
        for (auto& v : endpoint) { v = decode(v); }
        const auto luminance = std::clamp(source[0] * 0.2126f + source[1] * 0.7152f + source[2] * 0.0722f, 0.0f, 1.0f);
        if (endpoint[0] == 0 && endpoint[1] == 0 && endpoint[2] == 0) { endpoint = { 1, 1, 1 }; }
        for (size_t i = 0; i < source.size(); ++i)
        {
            source[i] = encode(std::clamp(source[i] + (endpoint[i] * luminance - source[i]) * strength, 0.0f, 1.0f));
        }
        return source;
    }
}
