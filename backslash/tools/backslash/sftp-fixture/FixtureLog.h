#pragma once
#include <Windows.h>
#include <string_view>
#include <mutex>
#include <algorithm>
#include <stdexcept>
// Fixture-only cap. Continue draining/discarding after cap or sink failure.
class FixtureLog {
 HANDLE h=INVALID_HANDLE_VALUE;size_t size=0;bool truncated=false,failed=false;std::mutex mu;
public:
 static constexpr size_t Limit=65536;
 explicit FixtureLog(const wchar_t* path){h=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Diagnostic log collision/open failure");}
 ~FixtureLog(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
 void Append(std::string_view bytes)noexcept {try{std::lock_guard lock(mu);const auto room=Limit-size;const auto n=(std::min)(room,bytes.size());truncated|=n<bytes.size();if(n&&!failed){DWORD wrote=0;if(!WriteFile(h,bytes.data(),static_cast<DWORD>(n),&wrote,nullptr)||wrote!=n)failed=true;size+=wrote;}}catch(...){failed=true;}}
 bool Good(){std::lock_guard lock(mu);return !failed;}
 bool Truncated(){std::lock_guard lock(mu);return truncated;}
};

