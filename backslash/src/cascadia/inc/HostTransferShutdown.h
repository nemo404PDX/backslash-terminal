// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <Windows.h>
#include <functional>
namespace Microsoft::Terminal::Backslash::Shutdown
{
    enum class DrainResult { Drained, TimedOut, BridgeFailed };
    // Uses only the owning DLL's projected callbacks. Never include/create the
    // module-local transfer service in the WindowsTerminal executable.
    template<typename Begun,typename Drained,typename Pause,typename Clock>
    DrainResult WaitForDrain(Begun&& begun,Drained&& drained,Pause&& pause,Clock&& clock,ULONGLONG budget)
    {
        const auto start=clock();
        try{for(;;){if(begun()&&drained())return DrainResult::Drained;if(clock()-start>=budget)return DrainResult::TimedOut;pause();}}
        catch(...){return DrainResult::BridgeFailed;}
    }
    enum class GateResult { Drained, ForcedUnconfirmed };
    template<typename Begin,typename Begun,typename Drained,typename Pause,typename Clock,typename Timeout,typename Force>
    GateResult BeforeOwnerTermination(Begin&& begin,Begun&& begun,Drained&& drained,Pause&& pause,Clock&& clock,Timeout&& timeout,Force&& force,ULONGLONG budget)
    {
        for(;;){
            DrainResult result;
            try{begin();result=WaitForDrain(begun,drained,pause,clock,budget);}catch(...){result=DrainResult::BridgeFailed;}
            if(result==DrainResult::Drained)return GateResult::Drained;
            try{timeout(result);}catch(...){}
            bool consent=false;try{consent=force(result);}catch(...){}
            if(consent){try{if(begun()&&drained())return GateResult::Drained;}catch(...){}return GateResult::ForcedUnconfirmed;}
        }
    }
    inline bool ShouldPostQuit(int64_t windows,int64_t messages,bool allowHeadless)noexcept{return messages<=0&&windows<=0&&!allowHeadless;}
}
