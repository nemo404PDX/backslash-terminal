// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <chrono>
#include "RenderVisibility.h"
#include "RenderDiagnostics.h"

using namespace Microsoft::Console::Render;
using namespace std::chrono_literals;
void require(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
    std::printf("PASS: %s\n",message);
}
template<typename F> bool until(F&& condition)
{
    for (int i=0; i<2000; ++i) { if (condition()) { return true; } std::this_thread::sleep_for(1ms); }
    return false;
}
int main(int argc, char** argv)
{
    require(argc==2,"Diagnostic fixture output argument");
    SetEnvironmentVariableW(L"BACKSLASH_RENDER_DIAGNOSTICS",nullptr);
    require(!RenderDiagnostics::FromEnvironment(),"Diagnostics disabled by default");
    SetEnvironmentVariableW(L"BACKSLASH_RENDER_DIAGNOSTICS",L"relative.jsonl");
    require(!RenderDiagnostics::FromEnvironment(),"Relative diagnostic path rejected");
    const std::wstring path(argv[1],argv[1]+std::char_traits<char>::length(argv[1]));
    SetEnvironmentVariableW(L"BACKSLASH_RENDER_DIAGNOSTICS",path.c_str());
    auto diagnostics=RenderDiagnostics::FromEnvironment();
    require(bool(diagnostics),"Absolute diagnostic path opts in");
    diagnostics->Write("fixture",true,false);
    SetEnvironmentVariableW(L"BACKSLASH_RENDER_DIAGNOSTICS",nullptr);

    RenderVisibility visibility;
    wil::unique_event shutdown;
    shutdown.create(wil::EventOptions::ManualReset);
    std::atomic<uint64_t> frames{0}, output{0}, renderedOutput{0}, resumeRedraws{0};
    std::thread worker([&] {
        while (WaitForSingleObject(shutdown.get(),0)!=WAIT_OBJECT_0 && visibility.Wait(shutdown.get()))
        {
            if (!visibility.IsVisible()) { continue; }
            if (visibility.TakeResumeRedraw()) { ++resumeRedraws; }
            renderedOutput=output.load();
            ++frames;
            std::this_thread::sleep_for(1ms);
        }
    });
    require(until([&]{return frames.load()>5;}),"Continuous worker starts while visible");
    visibility.SetVisible(false);
    require(until([&]{return visibility.IsSuspended();}),"Hidden worker acknowledges suspension");
    const auto hiddenFrames=frames.load();
    for(int i=0;i<200;++i) { ++output; }
    std::this_thread::sleep_for(30ms);
    require(frames.load()==hiddenFrames,"No hidden frames after suspension acknowledgement");
    require(output.load()==200,"Output processing continues independently while hidden");
    visibility.SetVisible(true);
    require(until([&]{return renderedOutput.load()==200;}),"Resume renders accumulated latest output");
    require(resumeRedraws.load()==1,"Hidden updates coalesce into one full resume redraw");
    for(int i=0;i<500;++i) { visibility.SetVisible(false); visibility.SetVisible(true); }
    require(until([&]{return frames.load()>hiddenFrames+5;}),"Rapid load/unload resumes without a lost wakeup");
    visibility.SetVisible(false);
    require(until([&]{return visibility.IsSuspended();}),"Worker suspends again before shutdown");
    shutdown.SetEvent();
    require(WaitForSingleObject(worker.native_handle(),2000)==WAIT_OBJECT_0,"Shutdown cancels hidden wait without resume");
    worker.join();
    require(!visibility.Wait(shutdown.get()),"Already-signaled shutdown wins a hidden wait");
    std::printf("Render visibility and diagnostic fixture passed\n");
}
