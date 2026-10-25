// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
// LOCAL TEST FIXTURE ONLY. This program cannot connect to any server.
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

static constexpr auto Identity = "BACKSLASH LOCAL SSH ARGUMENT CAPTURE FIXTURE";

static std::string utf8(const wchar_t* input)
{
    const auto length = WideCharToMultiByte(CP_UTF8, 0, input, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input, -1, result.data(), length, nullptr, nullptr);
    result.pop_back();
    return result;
}

static std::string quote(const wchar_t* input)
{
    std::string result{ "\"" };
    for (const auto ch : utf8(input))
    {
        switch (ch)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\r': result += "\\r"; break;
        case '\n': result += "\\n"; break;
        case '\t': result += "\\t"; break;
        default: result += ch; break;
        }
    }
    result += '"';
    return result;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && std::wstring_view{ argv[1] } == L"--fixture-identity")
    {
        std::cout << Identity;
        return 0;
    }
    wchar_t capture[2048];
    const auto length = GetEnvironmentVariableW(L"BACKSLASH_TEST_SSH_CAPTURE_PATH", capture, 2048);
    if (length == 0 || length >= 2048)
    {
        return 2;
    }
    std::ofstream file{ std::filesystem::path{ capture }, std::ios::binary };
    file << '[';
    for (int i = 0; i < argc; ++i)
    {
        if (i) { file << ','; }
        file << quote(argv[i]);
    }
    file << ']';
    return file.good() ? 0 : 3;
}
