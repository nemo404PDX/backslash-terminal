#define BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
#include "../../../src/cascadia/inc/SftpProcess.h"
#include "FixtureLog.h"
#include <filesystem>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
int wmain(int argc,wchar_t** argv){for(int i=1;i<argc;i++)if(std::wstring_view(argv[i])==L"--stderr-flood"){std::string block(1024,'x');DWORD n=0;for(int j=0;j<256;j++)if(!WriteFile(GetStdHandle(STD_ERROR_HANDLE),block.data(),static_cast<DWORD>(block.size()),&n,nullptr))return 2;const char answer='Z';WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),&answer,1,&n,nullptr);Sleep(INFINITE);return 0;}
 try{if(argc!=2)throw std::runtime_error("Owned test directory required");std::filesystem::path root=argv[1];if(std::filesystem::exists(root))throw std::runtime_error("Collision");std::filesystem::create_directory(root);wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);
 {FixtureLog log((root/L"cap.log").c_str());{ProcessTransport p(exe,{L"--stderr-flood"},3000,[&](std::string_view bytes){log.Append(bytes);});uint8_t b=0;p.Read(std::span<uint8_t>(&b,1));if(b!='Z')throw std::runtime_error("Protocol output lost");}if(!log.Good()||!log.Truncated())throw std::runtime_error("Cap predicate failed");}
 if(std::filesystem::file_size(root/L"cap.log")!=FixtureLog::Limit)throw std::runtime_error("Log cap failed");
 {ProcessTransport p(exe,{L"--stderr-flood"},3000,[](std::string_view){throw std::runtime_error("Sink refused");});uint8_t b=0;p.Read(std::span<uint8_t>(&b,1));if(b!='Z')throw std::runtime_error("Throwing sink stopped drain");}
 std::cout<<"PASS: 256KiB child stderr drained; 64KiB exact file cap; protocol output/cancellation and throwing sink teardown. Owned fake child only.\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
