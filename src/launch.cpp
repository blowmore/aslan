#define WIN32_LEAN_AND_MEAN

#include "launch.hpp"

#include <windows.h>

#include <iostream>
#include <string>

namespace
{

std::wstring quote_windows_argument(const std::wstring& value)
{
    return L"\"" + value + L"\"";
}

} // namespace

bool launch(const Installation& installation, const std::filesystem::path& project_path)
{
    const std::filesystem::path executable_path(installation.executable_path);
    const std::wstring absolute_project_path = std::filesystem::absolute(project_path).wstring();
    std::wstring command_line =
        quote_windows_argument(installation.executable_path) + L" " +
        quote_windows_argument(absolute_project_path);
    const std::wstring working_directory = executable_path.parent_path().wstring();

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info{};
    const BOOL created = CreateProcessW(
        installation.executable_path.c_str(),
        command_line.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        working_directory.empty() ? nullptr : working_directory.c_str(),
        &startup_info,
        &process_info);

    if (!created)
    {
        std::wcout << L"Failed to launch Automation Studio. Windows error: " << GetLastError() << L"\n";
        return false;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return true;
}
