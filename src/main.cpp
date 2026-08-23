#include "installation.hpp"
#include "launch.hpp"
#include "parse.hpp"
#include "dialog.hpp"

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

int launch_project(const Installation& installation, const std::filesystem::path& project_path)
{
    std::wcout << L"Executable: " << installation.executable_path;
    if (!installation.executable_exists)
    {
        std::wcout << L" (missing)";
    }
    std::wcout << L"\n";

    if (!installation.executable_exists)
    {
        return 1;
    }

    std::wcout << L"Launching project...\n";
    return launch(installation, project_path) ? 0 : 1;
}

int choose_and_launch_project(
    const std::vector<Installation>& installations,
    const std::wstring& message,
    const std::filesystem::path& project_path)
{
    const std::optional<std::size_t> selected_installation = dialog(installations, message);

    if (!selected_installation || *selected_installation >= installations.size())
    {
        std::wcout << L"No Automation Studio version selected.\n";
        return 1;
    }

    const Installation& installation = installations[*selected_installation];
    std::wcout << L"Selected version: "
               << (installation.version.empty() ? L"(unknown)" : installation.version) << L"\n";
    return launch_project(installation, project_path);
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
        const std::optional<Version> project_version = parse(project_path);

        if (!project_version)
        {
            const std::wstring message =
                L"Could not read an Automation Studio version from this project.";
            std::wcout << message << L" Path: " << project_path.wstring() << L"\n";
            return choose_and_launch_project(installations, message, project_path);
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
            return launch_project(*matching_installation, project_path);
        }
        else
        {
            const std::wstring message =
                L"No installed Automation Studio version matches " + required_version + L".";
            std::wcout << message << L"\n";
            return choose_and_launch_project(installations, message, project_path);
        }
    }

    return installations.empty() ? 1 : 0;
}
