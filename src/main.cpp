#define WIN32_LEAN_AND_MEAN

#include "installation.hpp"
#include "project_parser.hpp"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

std::wstring widen_ascii(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

std::wstring quote_windows_argument(const std::wstring& value)
{
    return L"\"" + value + L"\"";
}

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

// Note: Old options
// RunAsAdministrator  false
// RestoreSession   true
// WaitExit         false
// WarnAboutPolicy  true


int wmain(int argc, wchar_t* argv[])
{
    const std::vector<Installation> installations = find_installations();
    print_installations(installations);

    if (argc > 1)
    {
        const std::filesystem::path project_path(argv[1]);
        const std::optional<std::string> project_version = read_project_version(project_path);

        if (!project_version)
        {
            std::wcout << L"Could not read an Automation Studio version from: " << project_path.wstring() << L"\n";
            return 1;
        }

        const std::wstring required_version = widen_ascii(*project_version);
        std::wcout << L"Project version: " << required_version << L"\n";

        const auto matching_installation = std::find_if(
            installations.begin(),
            installations.end(),
            [&required_version](const Installation& installation)
            {
                return starts_with(installation.version, required_version);
            });

        if (matching_installation != installations.end())
        {
            std::wcout << L"Matching executable: " << matching_installation->executable_path;
            if (!matching_installation->executable_exists)
            {
                std::wcout << L" (missing)";
            }
            std::wcout << L"\n";

            if (!matching_installation->executable_exists)
            {
                return 1;
            }

            std::wcout << L"Launching project...\n";
            return launch(*matching_installation, project_path) ? 0 : 1;
        }
        else
        {
            std::wcout << L"No installed Automation Studio version matches " << required_version << L".\n";
            return 1;
        }
    }

    return installations.empty() ? 1 : 0;
}
