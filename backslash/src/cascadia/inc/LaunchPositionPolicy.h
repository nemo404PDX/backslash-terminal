// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

namespace Microsoft::Terminal::Backslash::LaunchPositionPolicy
{
    struct Inputs
    {
        bool enabled = false;
        bool settingPosition = false;
        bool commandlinePosition = false;
        bool persistedPosition = false;
        bool contentBounds = false;
    };
    // Center ordinary new windows only. Even a partial explicit coordinate is
    // placement intent; restoration and tear-out retain their native positions.
    constexpr bool ShouldCenter(const Inputs& value) noexcept
    {
        return value.enabled && !value.settingPosition && !value.commandlinePosition &&
               !value.persistedPosition && !value.contentBounds;
    }
}
