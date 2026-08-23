#pragma once

#include <string>
#include <vector>

struct Installation
{
    std::wstring version;
    std::wstring install_path;
    std::wstring executable_path;
    bool executable_exists = false;
};

std::vector<Installation> find_installations();
bool starts_with(const std::wstring& value, const std::wstring& prefix);
