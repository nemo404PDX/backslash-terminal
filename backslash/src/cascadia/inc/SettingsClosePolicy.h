// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <cstddef>
#include <limits>
namespace Microsoft::Terminal::Backslash::SettingsClosePolicy
{
    enum class Decision { Wait, Close, ConfirmDiscard };
    constexpr Decision Evaluate(bool operationBusy, bool awaitingSaveReload, bool staleDraft, bool changed) noexcept
    {
        if (operationBusy || awaitingSaveReload) { return Decision::Wait; }
        return staleDraft || changed ? Decision::ConfirmDiscard : Decision::Close;
    }
    constexpr bool ProtectTabClose(bool activeSettings, std::size_t leafCount) noexcept { return activeSettings && leafCount > 1; }
    // Footer layout follows measured localized action widths.
    constexpr bool StackFooterActions(double availableWidth, double saveWidth, double discardWidth) noexcept
    {
        const auto maximum = std::numeric_limits<double>::max();
        return !(availableWidth > 0 && availableWidth <= maximum && saveWidth > 0 && saveWidth <= maximum && discardWidth > 0 && discardWidth <= maximum) || saveWidth + discardWidth + 8 > availableWidth;
    }
}
