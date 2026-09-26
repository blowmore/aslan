#include "installation.hpp"
#include "launch.hpp"
#include "parse.hpp"
#include "dialog.hpp"
#include "settings.hpp"

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

int launch_project(
    const Installation& installation,
    const std::filesystem::path& project_path,
    const Settings& settings)
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
    return launch(installation, project_path, settings) ? 0 : 1;
}

int choose_and_launch_project(
    const std::vector<Installation>& installations,
    const std::wstring& message,
    const std::filesystem::path& project_path,
    Settings& settings,
    std::optional<std::size_t> preferred_installation = std::nullopt)
{
    const std::optional<DialogResult> result =
        dialog(installations, message, settings, preferred_installation);

    if (!result || result->selected_installation >= installations.size())
    {
        std::wcout << L"No Automation Studio version selected.\n";
        return 1;
    }

    settings = result->settings;
    if (!save_settings(settings))
    {
        std::wcerr << L"Could not save settings.\n";
    }

    const Installation& installation = installations[result->selected_installation];
    std::wcout << L"Selected version: "
               << (installation.version.empty() ? L"(unknown)" : installation.version) << L"\n";
    return launch_project(installation, project_path, settings);
}

// Note: Old options
// RunAsAdministrator  false
// RestoreSession   true
// WaitExit         false
// WarnAboutPolicy  true


int wmain(int argc, wchar_t* argv[])
{
    const std::vector<Installation> installations = find_installations();
    const bool shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const std::optional<Settings> saved_settings = load_settings();
    Settings settings = saved_settings.value_or(Settings{});

    if (argc > 1)
    {
        const std::filesystem::path project_path(argv[1]);

        if (shift_pressed)
        {
            return choose_and_launch_project(
                installations,
                L"Select the Automation Studio version to use.",
                project_path,
                settings);
        }

        const std::optional<Version> project_version = parse(project_path);

        if (!project_version)
        {
            const std::wstring message =
                L"Could not read an Automation Studio version from this project.";
            std::wcout << message << L" Path: " << project_path.wstring() << L"\n";
            return choose_and_launch_project(
                installations,
                message,
                project_path,
                settings);
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
            if (!saved_settings)
            {
                const std::size_t preferred_installation =
                    static_cast<std::size_t>(matching_installation - installations.begin());
                return choose_and_launch_project(
                    installations,
                    L"Choose the launch options for this project.",
                    project_path,
                    settings,
                    preferred_installation);
            }

            return launch_project(*matching_installation, project_path, settings);
        }
        else
        {
            const std::wstring message =
                L"No installed Automation Studio version matches " + required_version + L".";
            std::wcout << message << L"\n";
            return choose_and_launch_project(
                installations,
                message,
                project_path,
                settings);
        }
    }

    return installations.empty() ? 1 : 0;
}
