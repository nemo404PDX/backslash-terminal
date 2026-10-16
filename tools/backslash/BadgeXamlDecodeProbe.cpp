// Owned offscreen SVG decoder probe. No desktop input, screenshot, network or registration.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetCurrentTime
#include <string>
#include <memory>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <wil/resource.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../../src/cascadia/inc/BezelBadgeSvg.h"

template<typename T> auto Wait(T const& operation)
{
    const auto end=GetTickCount64()+15000;
    while(operation.Status()==winrt::Windows::Foundation::AsyncStatus::Started)
    {
        MSG message;
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message);DispatchMessageW(&message); }
        if(GetTickCount64()>end) { throw winrt::hresult_error{HRESULT_FROM_WIN32(WAIT_TIMEOUT)}; }
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    }
    return operation.GetResults();
}

int wmain()
{
    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        auto manager=winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        const auto& assets=Microsoft::Terminal::Backslash::BezelBadgeAssets();
        for(size_t i=0;i<assets.size();++i)
        {
            using namespace winrt::Windows::Storage::Streams;
            using namespace winrt::Windows::UI::Xaml::Media::Imaging;
            SvgImageSource source;
            InMemoryRandomAccessStream stream;
            DataWriter writer{stream};
            const auto& svg=assets[i];
            writer.WriteBytes({reinterpret_cast<const uint8_t*>(svg.data()),static_cast<uint32_t>(svg.size())});
            Wait(writer.StoreAsync());writer.DetachStream();stream.Seek(0);
            const auto status=Wait(source.SetSourceAsync(stream));
            std::wcout<<L"style="<<i<<L" bytes="<<svg.size()<<L" SvgImageSourceLoadStatus="<<static_cast<int>(status)<<L"\n";
            if(status!=SvgImageSourceLoadStatus::Success) return 3;
        }
        manager.Close();
        std::wcout<<L"PASS: all three styles decoded through native XAML SvgImageSource\n";
        return 0;
    }
    catch(winrt::hresult_error const& e)
    {
        std::wcerr<<L"HRESULT="<<std::hex<<static_cast<unsigned long>(e.code().value)<<L" "<<e.message().c_str()<<L"\n";
        return 2;
    }
}
