#pragma once

#include <windows.h>
#include <string>
#include <vector>

class RegistryKey
{
public:
    RegistryKey() = default;
    explicit RegistryKey(HKEY handle);
    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;
    RegistryKey(RegistryKey&& other) noexcept;
    RegistryKey& operator=(RegistryKey&& other) noexcept;
    ~RegistryKey();

    HKEY get() const;
    explicit operator bool() const;

private:
    void close();
    HKEY handle_ = nullptr;
};

RegistryKey open_local_machine_key(const std::wstring& sub_key, REGSAM access = KEY_READ);
RegistryKey open_child_key(HKEY parent, const std::wstring& sub_key);
std::wstring read_registry_string(HKEY key, const wchar_t* value_name);
std::vector<std::wstring> enumerate_sub_keys(HKEY key);
