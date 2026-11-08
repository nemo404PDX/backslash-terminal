// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../src/cascadia/inc/BuiltinViewWindowActivation.h"
#include <iostream>
#include <stdexcept>
#include <string>
namespace B=Microsoft::Terminal::Backslash;
static void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){try{
    int cases=0;
    // Placement modes choose where the view lives; focus always acts on its
    // existing owner. Injected native calls model their documented side effects.
    for(const auto view:{"Settings","Transfers"})for(const auto mode:{"tab","split","separate-window"})for(const int state:{0,1,2}){
        struct Window{bool minimized;bool maximized;int x,y,width,height;};
        Window owner{state==2,state==1,75,125,1200,800};
        const Window caller{false,true,10,20,1920,1080};
        const auto hwnd=reinterpret_cast<HWND>(&owner);
        int restores=0,activations=0;std::string order;
        for(int repeat=0;repeat<5;++repeat){
            B::ActivateBuiltinViewWindow(hwnd,[&](HWND w){Require(w==hwnd,"Query owner only");return owner.minimized;},
                [&](HWND w){Require(w==hwnd,"Restore owner only");++restores;order+='R';owner.minimized=false;owner.maximized=false;},
                [&](HWND w){Require(w==hwnd,"Activate owner only");++activations;order+='A';});
            Require(owner.x==75&&owner.y==125&&owner.width==1200&&owner.height==800,"Focus preserves owner rectangle");
            Require(owner.maximized==(state==1),"Focus preserves maximized state");
            Require(caller.maximized&&caller.x==10&&caller.y==20,"Separate owner leaves caller unchanged");
            ++cases;
        }
        Require(restores==(state==2?1:0)&&activations==5,"Only minimized owner is restored once");
        Require(order==(state==2?"RAAAAA":"AAAAA"),"Restore precedes activation");
        std::cout<<"PASS "<<view<<" "<<mode<<" state="<<state<<" repeats=5\n";
    }
    B::ActivateBuiltinViewWindow(nullptr,[](HWND)->bool{throw std::runtime_error("Null query");},[](HWND){throw std::runtime_error("Null restore");},[](HWND){throw std::runtime_error("Null activation");});
    // Real hidden owned HWNDs, with native iconic-state query but activation
    // suppressed: no user window, foreground focus, placement or preferences touched.
    for(const DWORD state:{DWORD{0},DWORD{WS_MAXIMIZE}}){
        const auto hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Owned activation regression",WS_OVERLAPPEDWINDOW|state,-32000,-32000,800,600,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Require(hwnd!=nullptr,"Create hidden owned HWND");
        WINDOWPLACEMENT before{sizeof(WINDOWPLACEMENT)},after{sizeof(WINDOWPLACEMENT)};
        RECT rectBefore{},rectAfter{};GetWindowPlacement(hwnd,&before);GetWindowRect(hwnd,&rectBefore);
        const bool zoomed=IsZoomed(hwnd)!=FALSE;int activated=0;
        Require(zoomed==(state==WS_MAXIMIZE),"Owned HWND reached requested normal/maximized state");
        for(int i=0;i<5;++i)B::ActivateBuiltinViewWindow(hwnd,[](HWND w){return IsIconic(w)!=FALSE;},[](HWND){throw std::runtime_error("Non-iconic HWND must not restore");},[&](HWND w){Require(w==hwnd,"Owned native activation");++activated;});
        GetWindowPlacement(hwnd,&after);GetWindowRect(hwnd,&rectAfter);
        Require(before.showCmd==after.showCmd&&EqualRect(&before.rcNormalPosition,&after.rcNormalPosition)&&EqualRect(&rectBefore,&rectAfter)&&(IsZoomed(hwnd)!=FALSE)==zoomed&&activated==5,"Native normal/maximized placement unchanged");
        DestroyWindow(hwnd);++cases;
    }
    std::cout<<"PASS "<<cases<<" activation cases; native hidden HWND placement preserved; foreground calls suppressed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
