#define WIN32_LEAN_AND_MEAN

#include "installation.hpp"
#include "project_parser.hpp"

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
            return installations.empty() ? 1 : 0;
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
        }
        else
        {
            std::wcout << L"No installed Automation Studio version matches " << required_version << L".\n";
        }
    }

    return installations.empty() ? 1 : 0;
}
