#include "project_parser.hpp"

#include <fstream>
#include <iterator>
#include <regex>

std::optional<std::string> read_project_automation_studio_version(const std::filesystem::path& project_path)
{
    std::ifstream project_file(project_path, std::ios::binary);
    if (!project_file)
    {
        return std::nullopt;
    }

    const std::string project_contents{
        std::istreambuf_iterator<char>(project_file),
        std::istreambuf_iterator<char>()};

    const std::regex version_pattern(R"(AutomationStudio Version="?([0-9])\.([0-9]+)\.([0-9]+))");
    std::smatch match;
    if (!std::regex_search(project_contents, match, version_pattern))
    {
        return std::nullopt;
    }

    return match[1].str() + "." + match[2].str() + "." + match[3].str();
}
