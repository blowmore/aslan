#pragma once

#include <filesystem>
#include <optional>
#include <string>

std::optional<std::string> read_project_automation_studio_version(const std::filesystem::path& project_path);
