#pragma once

#include "installation.hpp"
#include "settings.hpp"

#include <filesystem>

bool launch(
    const Installation& installation,
    const std::filesystem::path& project_path,
    const Settings& settings);
