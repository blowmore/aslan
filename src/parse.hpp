#pragma once

#include <filesystem>
#include <optional>
#include <string>

typedef std::string Version;

std::optional<Version> parse(const std::filesystem::path& project_path);
