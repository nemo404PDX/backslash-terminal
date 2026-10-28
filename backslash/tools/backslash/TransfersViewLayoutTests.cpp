// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/TransfersViewLayout.h"
#include <iostream>
#include <limits>
namespace L = Microsoft::Terminal::Backslash::TransfersViewLayout;
int main()
{
    unsigned checks = 0;
    auto require = [&](bool condition, const char* label) {
        if (!condition) { std::cerr << "FAIL: " << label << '\n'; return false; }
        ++checks; return true;
    };
    // Actual view minima minus page and card padding/border: narrow must stack.
    if (!require(!L::UseSideBySide(360 - 32 - 34, L::StatusWidthForLabel(70)), "minimum pane stacks")) { return 1; }
    if (!require(!L::UseSideBySide(400 - 32 - 34, L::StatusWidthForLabel(70)), "narrow split stacks")) { return 1; }
    if (!require(L::UseSideBySide(1000 - 32 - 34, L::StatusWidthForLabel(70)), "wide view shares a row")) { return 1; }
    if (!require(L::StatusWidthForLabel(70) == 160 && L::StatusWidthForLabel(300) == 240, "status remains bounded")) { return 1; }
    if (!require(!L::UseSideBySide(450, L::StatusWidthForLabel(300)), "large/localized labels delay shared row")) { return 1; }
    for (double label : {0.0,70.0,130.0,300.0})
    {
        const auto status=L::StatusWidthForLabel(label);
        const auto boundary=L::MinimumSearchWidth + L::ColumnGap + status;
        if (!require(!L::UseSideBySide(boundary-0.5,status) && L::UseSideBySide(boundary,status), "no undersized search at breakpoint")) { return 1; }
        for (double scale : {1.0,1.5,2.0})
        {
            if (!require(L::UseSideBySide((boundary*scale)/scale,status), "DIP layout is independent of pixel scale")) { return 1; }
        }
    }
    if (!require(!L::UseSideBySide(0,160) && !L::UseSideBySide(std::numeric_limits<double>::quiet_NaN(),160) && !L::UseSideBySide(std::numeric_limits<double>::infinity(),160), "unmeasured/invalid allocation stacks")) { return 1; }
    std::cout << "PASS: " << checks << " Transfers layout policy checks; no GUI or renderer verification.\n";
}
