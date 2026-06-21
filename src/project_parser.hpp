#pragma once

#include <filesystem>
#include <optional>
#include <string>

std::optional<std::string> read_project_version(const std::filesystem::path& project_path);
