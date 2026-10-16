// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
// Native composed control and preview tests. Captures owned XAML elements only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetCurrentTime
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <wil/resource.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <wincodec.h>
#include <wrl/client.h>
#include "../../src/cascadia/inc/RetroBezel.h"
#include "../../src/cascadia/inc/AppearancePreviewLayout.h"

using namespace winrt;
using namespace winrt::Windows::UI::Xaml;
namespace B=::Microsoft::Terminal::Backslash;

static void Require(bool value,char const* message)
{
    if(!value){std::cerr<<"FAIL: "<<message<<"\n";throw hresult_error{E_FAIL};}
}
static void Pump(unsigned milliseconds)
{
    const auto end=GetTickCount64()+milliseconds;
    do { MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT); }while(GetTickCount64()<end);
}
template<typename T> auto Wait(T const& op)
{
    const auto end=GetTickCount64()+15000;
    while(op.Status()==winrt::Windows::Foundation::AsyncStatus::Started){Pump(10);if(GetTickCount64()>end)throw hresult_error{HRESULT_FROM_WIN32(WAIT_TIMEOUT)};}
    return op.GetResults();
}
static Controls::Image FindImage(UIElement const& element)
{
    if(auto image=element.try_as<Controls::Image>())return image;
    if(auto panel=element.try_as<Controls::Panel>())for(auto child:panel.Children())if(auto result=FindImage(child))return result;
    if(auto border=element.try_as<Controls::Border>())if(border.Child())return FindImage(border.Child());
    return nullptr;
}
struct Pixels{int width,height;std::vector<uint8_t> data;};
static Pixels Capture(UIElement const& element)
{
    Media::Imaging::RenderTargetBitmap bitmap;Wait(bitmap.RenderAsync(element));
    const auto buffer=Wait(bitmap.GetPixelsAsync());std::vector<uint8_t> pixels(buffer.Length());
    winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer).ReadBytes(pixels);
    return {bitmap.PixelWidth(),bitmap.PixelHeight(),std::move(pixels)};
}
static void SavePng(std::filesystem::path const& path,Pixels const& pixels)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)));
    ComPtr<IWICStream> stream;check_hresult(factory->CreateStream(&stream));check_hresult(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE));
    ComPtr<IWICBitmapEncoder> encoder;check_hresult(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder));check_hresult(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache));
    ComPtr<IWICBitmapFrameEncode> frame;check_hresult(encoder->CreateNewFrame(&frame,nullptr));check_hresult(frame->Initialize(nullptr));check_hresult(frame->SetSize(static_cast<UINT>(pixels.width),static_cast<UINT>(pixels.height)));
    auto format=GUID_WICPixelFormat32bppBGRA;check_hresult(frame->SetPixelFormat(&format));check_hresult(frame->WritePixels(static_cast<UINT>(pixels.height),static_cast<UINT>(pixels.width*4),static_cast<UINT>(pixels.data.size()),const_cast<BYTE*>(pixels.data.data())));check_hresult(frame->Commit());check_hresult(encoder->Commit());
}
struct Ink{int left,top,right,bottom,changed,white,black,whiteColumns;int letterSamples=0,transparentLetters=0;};
static Ink Inspect(Pixels const& background,Pixels const& painted,winrt::Windows::Foundation::Rect rect,double sx,double sy,int style,Pixels const* solid)
{
    Require(background.width==painted.width && background.height==painted.height,"Stable captured size");
    Ink ink{painted.width,painted.height,-1,-1,0,0,0,0};std::vector<bool> columns(static_cast<size_t>(painted.width));
    for(int y=0;y<painted.height;++y)for(int x=0;x<painted.width;++x)
    {
        const auto i=static_cast<size_t>((y*painted.width+x)*4);int delta=0;
        for(size_t c=0;c<4;++c)delta=std::max(delta,std::abs(static_cast<int>(background.data[i+c])-painted.data[i+c]));
        const bool inBadgeRegion=x>=rect.X*sx-2 && x<(rect.X+rect.Width)*sx+2 && y>=rect.Y*sy-2 && y<(rect.Y+rect.Height)*sy+2;
        if(delta>8 && inBadgeRegion){++ink.changed;ink.left=std::min(ink.left,x);ink.top=std::min(ink.top,y);ink.right=std::max(ink.right,x);ink.bottom=std::max(ink.bottom,y);}
        if(x>=rect.X*sx && x<(rect.X+rect.Width)*sx && y>=rect.Y*sy && y<(rect.Y+rect.Height)*sy)
        {
            const bool white=painted.data[i]>235 && painted.data[i+1]>235 && painted.data[i+2]>235 && painted.data[i+3]>240;
            if(white){++ink.white;columns[static_cast<size_t>(x)]=true;}
            if(painted.data[i]<32 && painted.data[i+1]<32 && painted.data[i+2]<32 && painted.data[i+3]>240)++ink.black;
        }
    }
    for(bool c:columns)ink.whiteColumns+=c;
    std::cout<<"Pixel evidence: changed="<<ink.changed<<" white="<<ink.white<<" black="<<ink.black<<" whiteColumns="<<ink.whiteColumns<<"\n";
    Require(ink.changed>100,"Badge silhouette differs from unbranded casing");
    if(style==2)Require(ink.white>50 && ink.whiteColumns>20,"CurrentColor backing and letter faces resolve to white");
    else Require(ink.black>50,"Dark artwork backing is painted");
    if(style==1)
    {
        Require(solid && solid->width==painted.width && solid->height==painted.height,"Cutout has matching solid reference");
        // Use the solid style's opaque letter/slash interiors as the hole oracle.
        // Erode one pixel to exclude antialiased path edges. Transparent holes
        // must reproduce the actual unbranded casing, not white SVG lettering.
        for(int y=1;y<painted.height-1;++y)for(int x=1;x<painted.width-1;++x)
        {
            if(x<rect.X*sx || x>=(rect.X+rect.Width)*sx || y<rect.Y*sy || y>=(rect.Y+rect.Height)*sy)continue;
            bool letter=true;
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
            {
                const auto k=static_cast<size_t>(((y+dy)*painted.width+x+dx)*4);
                letter=letter && solid->data[k]>235 && solid->data[k+1]>235 && solid->data[k+2]>235 && solid->data[k+3]>240;
            }
            if(!letter)continue;
            ++ink.letterSamples;const auto k=static_cast<size_t>((y*painted.width+x)*4);int delta=0;
            for(size_t c=0;c<4;++c)delta=std::max(delta,std::abs(static_cast<int>(painted.data[k+c])-background.data[k+c]));
            if(delta<=8)++ink.transparentLetters;
        }
        Require(ink.letterSamples>50,"Letter/slash mask has meaningful interior samples");
        Require(ink.transparentLetters*100>=ink.letterSamples*95,"Cutout letter/slash interiors expose the underlying casing");
        std::cout<<"Cutout transparency: "<<ink.transparentLetters<<"/"<<ink.letterSamples<<" interior samples match casing\n";
    }
    else
    {
        Require(ink.white>20 && ink.whiteColumns>20,"Solid/currentColor retain their authored visible letter faces");
    }
    const auto width=ink.right-ink.left+1,height=ink.bottom-ink.top+1;
    Require(width>rect.Width*sx*0.85 && width<rect.Width*sx*1.06+2,"Painted badge has full wide aspect, not a square");
    Require(height>rect.Height*sy*0.75 && height<rect.Height*sy*1.06+2,"Painted badge height matches layout");
    Require(std::abs(ink.left-rect.X*sx)<3 && std::abs(ink.top-rect.Y*sy)<3,"Painted placement matches actual Image transform");
    return ink;
}

int wmain(int argc,wchar_t** argv)
{
    if(argc!=2)return 2;
    std::ofstream report{std::filesystem::path{argv[1]}/L"composed-results.json"};report<<"{\"cases\":[";bool first=true;int passed=0,failures=0;
    try
    {
        init_apartment(apartment_type::single_threaded);
        auto manager=Hosting::WindowsXamlManager::InitializeForCurrentThread();
        const auto hwnd=CreateWindowExW(0,L"STATIC",L"Owned Backslash badge render tests",WS_OVERLAPPEDWINDOW,0,0,1600,1200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);if(!hwnd)throw_last_error();
        Hosting::DesktopWindowXamlSource island;auto native=island.as<IDesktopWindowXamlSourceNative>();check_hresult(native->AttachToWindow(hwnd));HWND child{};check_hresult(native->get_WindowHandle(&child));ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        const std::array<char const*,7> positions{"off","bottomCenter","topCenter","bottomLeft","bottomRight","topLeft","topRight"};
        for(int size=0;size<3;++size)
        for(int context=0;context<3;++context)
        {
            const auto contextName=context==0?"live-control":context==1?"preview-wide":"preview-narrow";
            B::RetroBezel bezel;Controls::Grid terminal;terminal.Background(Media::SolidColorBrush{winrt::Windows::UI::Color{255,0,0,0}});bezel.Content(terminal);
            Controls::Grid host;Controls::ScrollViewer scroller{nullptr};double hostWidth=1000,hostHeight=700;
            if(context==0){host.Children().Append(bezel.Root());}
            else
            {
                hostWidth=context==1?700:320;hostHeight=450;
                const auto layout=B::GetAppearancePreviewLayout(static_cast<float>(hostWidth),B::GetBezelInsets(true,size,0,false,0),0,0,1);
                Controls::Border border;border.Width(layout.width);border.Height(layout.height);border.BorderThickness({1,1,1,1});border.BorderBrush(Media::SolidColorBrush{winrt::Windows::UI::Color{255,100,100,100}});border.Child(bezel.Root());
                scroller=Controls::ScrollViewer{};scroller.HorizontalScrollBarVisibility(Controls::ScrollBarVisibility::Auto);scroller.HorizontalScrollMode(Controls::ScrollMode::Enabled);scroller.VerticalScrollBarVisibility(Controls::ScrollBarVisibility::Disabled);scroller.Content(border);host.Children().Append(scroller);
            }
            host.Width(hostWidth);host.Height(hostHeight);host.Background(Media::SolidColorBrush{winrt::Windows::UI::Color{255,50,50,50}});
            const auto scale=GetDpiForWindow(hwnd)/96.0;SetWindowPos(child,nullptr,0,0,static_cast<int>(hostWidth*scale),static_cast<int>(hostHeight*scale),SWP_SHOWWINDOW|SWP_NOACTIVATE);island.Content(host);Pump(150);host.UpdateLayout();
            auto image=FindImage(bezel.Root());Require(static_cast<bool>(image),"Production Image exists");Require(!image.IsHitTestVisible(),"Badge cannot intercept terminal hit tests");
            image.ImageFailed([&](auto const&,ExceptionRoutedEventArgs const& e){++failures;std::wcerr<<L"ImageFailed "<<e.ErrorMessage().c_str()<<L"\n";});
            unsigned imageOpened=0;
            image.ImageOpened([&](auto const&,auto const&){++imageOpened;});
            std::array<Pixels,6> solidReferences;
            for(int style=0;style<3;++style)for(int placement=1;placement<=6;++placement)
            {
                const auto previousSource=image.Source();const auto openedBefore=imageOpened;
                bezel.Update(true,size,0,placement,false,false,0,style);
                if(image.Source()!=previousSource)
                {
                    const auto deadline=GetTickCount64()+15000;
                    while(imageOpened==openedBefore && failures==0 && GetTickCount64()<deadline)Pump(20);
                    Require(imageOpened>openedBefore,"New badge style finishes ImageOpened before pixel assertions");
                }
                Pump(100);host.UpdateLayout();
                const auto bounds=image.TransformToVisual(host).TransformBounds({0,0,static_cast<float>(image.ActualWidth()),static_cast<float>(image.ActualHeight())});
                if(scroller){const auto target=std::clamp(static_cast<double>(bounds.X)+scroller.HorizontalOffset()+bounds.Width/2-hostWidth/2,0.0,scroller.ScrollableWidth());scroller.ChangeView(target,nullptr,nullptr,true);Pump(80);host.UpdateLayout();}
                bezel.Update(true,size,0,0,false,false,0,style);Pump(30);host.UpdateLayout();const auto background=Capture(host);
                bezel.Update(true,size,0,placement,false,false,0,style);Pump(80);host.UpdateLayout();const auto painted=Capture(host);
                const auto rect=image.TransformToVisual(host).TransformBounds({0,0,static_cast<float>(image.ActualWidth()),static_cast<float>(image.ActualHeight())});
                Require(rect.X>=-0.1 && rect.Y>=-0.1 && rect.X+rect.Width<=hostWidth+0.1 && rect.Y+rect.Height<=hostHeight+0.1,"Preview scroll exposes the whole badge without clipping");
                const auto local=image.TransformToVisual(bezel.Root()).TransformBounds({0,0,static_cast<float>(image.ActualWidth()),static_cast<float>(image.ActualHeight())});
                const auto opening=B::GetBezelApertureBounds(static_cast<float>(bezel.Root().ActualWidth()),static_cast<float>(bezel.Root().ActualHeight()),bezel.Insets).opening;
                Require(local.X >= opening.left+B::BezelBadgeCornerInset-0.01f && local.X+local.Width <= opening.right-B::BezelBadgeCornerInset+0.01f,"Composed badge excludes both bezel side rails");
                if(placement==3 || placement==5)Require(std::abs(local.X-opening.left-B::BezelBadgeCornerInset)<0.01f,"Left badge anchors to screen-opening span");
                if(placement==4 || placement==6)Require(std::abs(local.X+local.Width-opening.right+B::BezelBadgeCornerInset)<0.01f,"Right badge anchors to screen-opening span");
                const auto file=std::string{contextName}+"-size"+std::to_string(size)+"-style"+std::to_string(style)+"-"+positions[static_cast<size_t>(placement)]+".png";
                SavePng(std::filesystem::path{argv[1]}/file,painted);
                const auto ink=Inspect(background,painted,rect,painted.width/hostWidth,painted.height/hostHeight,style,
                                       style==1?&solidReferences[static_cast<size_t>(placement-1)]:nullptr);
                if(style==0)solidReferences[static_cast<size_t>(placement-1)]=painted;
                auto source=image.Source().as<Media::Imaging::SvgImageSource>();Require(source.RasterizePixelWidth()==527 && source.RasterizePixelHeight()==100,"Finite SVG raster viewport is retained");
                if(!first)report<<",";first=false;report<<"{\"context\":\""<<contextName<<"\",\"size\":"<<size<<",\"style\":"<<style<<",\"placement\":\""<<positions[static_cast<size_t>(placement)]<<"\",\"whitePixels\":"<<ink.white<<",\"blackPixels\":"<<ink.black<<",\"whiteColumns\":"<<ink.whiteColumns<<",\"letterSamples\":"<<ink.letterSamples<<",\"transparentLetters\":"<<ink.transparentLetters<<",\"paintedWidth\":"<<ink.right-ink.left+1<<",\"paintedHeight\":"<<ink.bottom-ink.top+1<<",\"png\":\""<<file<<"\"}";
                ++passed;std::cout<<"PASS "<<contextName<<" style="<<style<<" "<<positions[static_cast<size_t>(placement)]<<" silhouette="<<ink.right-ink.left+1<<"x"<<ink.bottom-ink.top+1<<" white="<<ink.white<<"\n";
            }
            island.Content(nullptr);Pump(50);
        }
        Require(failures==0,"No composed ImageFailed events");island.Close();DestroyWindow(hwnd);manager.Close();
        report<<"],\"passed\":"<<passed<<",\"imageFailures\":"<<failures<<",\"capture\":\"Native RenderTargetBitmap of production RetroBezel in live and preview containers; placeholder screen, no desktop capture\"}";
        std::cout<<"PASS: "<<passed<<" composed rendering cases\n";return 0;
    }
    catch(hresult_error const& e){report<<"],\"passed\":"<<passed<<",\"failed\":true}";std::wcerr<<L"HRESULT="<<std::hex<<static_cast<unsigned long>(e.code().value)<<L" "<<e.message().c_str()<<L"\n";return 1;}
}
