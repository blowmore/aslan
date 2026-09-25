#define WIN32_LEAN_AND_MEAN

#include "launch.hpp"

#include <windows.h>

#include <iostream>
#include <string>
#include <system_error>

namespace
{

std::wstring quote_windows_argument(const std::wstring& value)
{
    return L"\"" + value + L"\"";
}

std::wstring widen_error(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

void remove_session_state(const std::filesystem::path& project_path)
{
    const std::filesystem::path project_directory =
        std::filesystem::absolute(project_path).parent_path();
    const std::filesystem::path settings_directory =
        project_directory / L"Temp" / L"Settings";

    std::error_code error;
    if (!std::filesystem::exists(settings_directory, error) || error)
    {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             settings_directory,
             std::filesystem::directory_options::skip_permission_denied,
             error))
    {
        if (error)
        {
            error.clear();
            continue;
        }

        const std::wstring filename = entry.path().filename().wstring();
        const bool is_session_file =
            filename == L"ProjectDesktop.state" || filename == L"Workspace.state";
        const bool is_open_file = entry.path().extension() == L".isopen";

        if (entry.is_regular_file(error) && (is_session_file || is_open_file))
        {
            std::filesystem::remove(entry.path(), error);
            if (error)
            {
                std::wcerr << L"Could not remove session/open-state file "
                           << entry.path().wstring() << L": "
                           << widen_error(error.message()) << L"\n";
                error.clear();
            }
        }
    }
}

void remove_project_open_state(const std::filesystem::path& project_path)
{
    std::filesystem::path open_state_path = std::filesystem::absolute(project_path);
    open_state_path.replace_extension(L".isopen");

    std::error_code error;
    if (!std::filesystem::is_regular_file(open_state_path, error))
    {
        if (error)
        {
            std::wcerr << L"Could not check project open-state file "
                       << open_state_path.wstring() << L": "
                       << widen_error(error.message()) << L"\n";
        }
        return;
    }

    std::filesystem::remove(open_state_path, error);
    if (error)
    {
        std::wcerr << L"Could not remove project open-state file "
                   << open_state_path.wstring() << L": "
                   << widen_error(error.message()) << L"\n";
    }
}

} // namespace

bool launch(const Installation& installation, const std::filesystem::path& project_path)
{
    const std::filesystem::path executable_path(installation.executable_path);
    const std::wstring absolute_project_path = std::filesystem::absolute(project_path).wstring();
    remove_session_state(project_path);
    remove_project_open_state(project_path);
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
