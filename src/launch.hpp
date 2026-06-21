#pragma once

#include "installation.hpp"

#include <filesystem>

bool launch(const Installation& installation, const std::filesystem::path& project_path);
