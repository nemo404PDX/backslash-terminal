// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SftpService.h"
#include <winrt/Windows.UI.Xaml.Controls.h>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    inline winrt::Windows::UI::Xaml::Controls::ContentDialog ExitDialog(bool quitProcess)
    {
        using namespace winrt::Windows::UI::Xaml::Controls;
        ContentDialog dialog;dialog.Title(winrt::box_value(quitProcess?L"Quit Backslash with SFTP work?":L"Close this window with SFTP work?"));
        TextBlock text;text.TextWrapping(winrt::Windows::UI::Xaml::TextWrapping::Wrap);
        text.Text(quitProcess?L"Quitting stops active SFTP work. A file already being published is allowed to finish. Completed files remain; remote temporary files may remain. Progress and retained-file history are not saved. Review those paths in Transfers before quitting.":L"This closes the window and its terminal sessions. SFTP jobs continue only while Backslash remains running. If this closes the application, active work stops and remote temporary files may remain. Progress and retained-file history are not saved.");
        dialog.Content(text);dialog.PrimaryButtonText(quitProcess?L"Quit Backslash":L"Close window");dialog.CloseButtonText(L"Keep working");dialog.DefaultButton(ContentDialogButton::Close);return dialog;
    }
}
