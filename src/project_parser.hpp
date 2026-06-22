#pragma once

#include <filesystem>
#include <optional>
#include <string>

typedef std::string Version;

std::optional<std::string> read_project_version(const std::filesystem::path& project_path);
