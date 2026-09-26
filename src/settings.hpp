#pragma once

#include <optional>

struct Settings
{
    bool clear_user_state = true;
};

std::optional<Settings> load_settings();
bool save_settings(const Settings& settings);
