#pragma once

#include <string>
#include <vector>

struct AutomationStudioInstallation
{
    std::wstring version;
    std::wstring install_path;
    std::wstring executable_path;
    bool executable_exists = false;
};

std::vector<AutomationStudioInstallation> find_automation_studio_installations();
void print_installations(const std::vector<AutomationStudioInstallation>& installations);
bool starts_with(const std::wstring& value, const std::wstring& prefix);
