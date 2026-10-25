// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <windows.h>

namespace Microsoft::Console::Render::Atlas
{
    enum class PostProcessMode
    {
        None,
        Retro,
        Custom,
    };

    inline PostProcessMode SelectPostProcess(const std::wstring_view customPath, const bool retroEnabled, const float glow, const float scanlines, const float tint = 0.0f) noexcept
    {
        // A custom path retains precedence, including neutral built-in strengths and errors.
        if (!customPath.empty()) { return PostProcessMode::Custom; }
        return retroEnabled && (glow > 0.0f || scanlines > 0.0f || tint > 0.0f) ? PostProcessMode::Retro : PostProcessMode::None;
    }

    struct ShaderFileStamp
    {
        bool available = false;
        uint64_t size = 0;
        uint64_t modified = 0;
        bool operator==(const ShaderFileStamp&) const noexcept = default;
    };

    inline ShaderFileStamp GetShaderFileStamp(const std::wstring& path) noexcept
    {
        WIN32_FILE_ATTRIBUTE_DATA data{};
        if (!path.empty() && GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
        {
            return { true, (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow,
                (static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime };
        }
        return {};
    }
}
