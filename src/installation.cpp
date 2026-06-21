#include "installation.hpp"
#include "registry_key.hpp"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <iostream>

namespace
{

std::wstring to_lower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch)
    {
        return static_cast<wchar_t>(std::towlower(ch));
    });

    return value;
}

bool contains_case_insensitive(const std::wstring& value, const std::wstring& needle)
{
    return to_lower(value).find(to_lower(needle)) != std::wstring::npos;
}

bool file_exists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring append_path(const std::wstring& base, const std::wstring& child)
{
    if (base.empty())
    {
        return child;
    }

    const wchar_t last = base.back();
    if (last == L'\\' || last == L'/')
    {
        return base + child;
    }

    return base + L"\\" + child;
}

std::vector<Installation> find_installations_in_root(
    const std::wstring& registry_root,
    REGSAM access)
{
    RegistryKey root = open_local_machine_key(registry_root, access);
    if (!root)
    {
        return {};
    }

    std::vector<Installation> installations;
    for (const std::wstring& sub_key_name : enumerate_sub_keys(root.get()))
    {
        if (!contains_case_insensitive(sub_key_name, L"BR_AS"))
        {
            continue;
        }

        RegistryKey as_key = open_child_key(root.get(), sub_key_name);
        if (!as_key)
        {
            continue;
        }

        std::wstring install_path = read_registry_string(as_key.get(), L"BuRSharedFilesPath");
        if (install_path.empty())
        {
            continue;
        }

        RegistryKey control_studio_key = open_child_key(as_key.get(), L"ControlStudio");
        std::wstring version;
        if (control_studio_key)
        {
            version = read_registry_string(control_studio_key.get(), L"ProgrVersion");
        }

        std::wstring executable_path = append_path(install_path, L"bin-en\\.exe");
        installations.push_back({version, install_path, executable_path, file_exists(executable_path)});
    }

    std::sort(installations.begin(), installations.end(), [](const auto& left, const auto& right)
    {
        return left.version < right.version;
    });

    return installations;
}

} // namespace

std::vector<Installation> find_installations()
{
    std::vector<Installation> installations =
        find_installations_in_root(L"SOFTWARE", KEY_READ | KEY_WOW64_32KEY);

    if (installations.empty())
    {
        installations = find_installations_in_root(L"SOFTWARE\\WOW6432Node", KEY_READ);
    }

    if (installations.empty())
    {
        installations = find_installations_in_root(L"SOFTWARE", KEY_READ);
    }

    return installations;
}

void print_installations(const std::vector<Installation>& installations)
{
    std::wcout << L"Automation Studio installations\n\n";

    if (installations.empty())
    {
        std::wcout << L"No Automation Studio installations found.\n";
        return;
    }

    for (std::size_t index = 0; index < installations.size(); ++index)
    {
        const Installation& installation = installations[index];

        std::wcout << index + 1 << L". Version: "
                   << (installation.version.empty() ? L"(unknown)" : installation.version) << L"\n";
        std::wcout << L"   Install path: " << installation.install_path << L"\n";
        std::wcout << L"   Executable:   " << installation.executable_path;
        if (!installation.executable_exists)
        {
            std::wcout << L" (missing)";
        }
        std::wcout << L"\n\n";
    }
}

bool starts_with(const std::wstring& value, const std::wstring& prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}
