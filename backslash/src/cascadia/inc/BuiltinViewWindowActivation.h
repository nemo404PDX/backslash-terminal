// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <windows.h>

namespace Microsoft::Terminal::Backslash
{
    // Focusing an existing built-in view must not undo maximization or move
    // its host. Restore only an iconic window, then request activation.
    template<typename IsMinimized, typename Restore, typename Activate>
    void ActivateBuiltinViewWindow(HWND window, IsMinimized isMinimized, Restore restore, Activate activate)
    {
        if (!window) { return; }
        if (isMinimized(window)) { restore(window); }
        activate(window);
    }

    inline void ActivateBuiltinViewWindow(HWND window)
    {
        ActivateBuiltinViewWindow(window,
            [](HWND value) { return ::IsIconic(value) != FALSE; },
            [](HWND value) { ::ShowWindow(value, SW_RESTORE); },
            [](HWND value) { ::SetForegroundWindow(value); });
    }
}
