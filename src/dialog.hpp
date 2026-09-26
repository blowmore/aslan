#pragma once

#include "installation.hpp"
#include "settings.hpp"

#include <optional>
#include <string>
#include <vector>

struct DialogResult
{
    std::size_t selected_installation = 0;
    Settings settings;
};

std::optional<DialogResult> dialog(
    const std::vector<Installation>& installations,
    const std::wstring& message,
    const Settings& settings,
    std::optional<std::size_t> preferred_installation = std::nullopt);
