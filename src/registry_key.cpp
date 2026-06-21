#include "registry_key.hpp"

#include <vector>

RegistryKey::RegistryKey(HKEY handle)
    : handle_(handle)
{
}

RegistryKey::RegistryKey(RegistryKey&& other) noexcept
    : handle_(other.handle_)
{
    other.handle_ = nullptr;
}

RegistryKey& RegistryKey::operator=(RegistryKey&& other) noexcept
{
    if (this != &other)
    {
        close();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    return *this;
}

RegistryKey::~RegistryKey()
{
    close();
}

HKEY RegistryKey::get() const
{
    return handle_;
}

RegistryKey::operator bool() const
{
    return handle_ != nullptr;
}

void RegistryKey::close()
{
    if (handle_)
    {
        RegCloseKey(handle_);
        handle_ = nullptr;
    }
}

RegistryKey open_local_machine_key(const std::wstring& sub_key, REGSAM access)
{
    HKEY handle = nullptr;
    const LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub_key.c_str(), 0, access, &handle);
    return status == ERROR_SUCCESS ? RegistryKey(handle) : RegistryKey();
}

RegistryKey open_child_key(HKEY parent, const std::wstring& sub_key)
{
    HKEY handle = nullptr;
    const LSTATUS status = RegOpenKeyExW(parent, sub_key.c_str(), 0, KEY_READ, &handle);
    return status == ERROR_SUCCESS ? RegistryKey(handle) : RegistryKey();
}

std::wstring read_registry_string(HKEY key, const wchar_t* value_name)
{
    DWORD type = 0;
    DWORD byte_count = 0;
    DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;

    LSTATUS status = RegGetValueW(key, nullptr, value_name, flags, &type, nullptr, &byte_count);
    if (status != ERROR_SUCCESS || byte_count == 0)
    {
        return L"";
    }

    std::wstring value(byte_count / sizeof(wchar_t), L'\0');
    status = RegGetValueW(key, nullptr, value_name, flags, &type, value.data(), &byte_count);
    if (status != ERROR_SUCCESS)
    {
        return L"";
    }

    value.resize(byte_count / sizeof(wchar_t));
    while (!value.empty() && value.back() == L'\0')
    {
        value.pop_back();
    }

    return value;
}

std::vector<std::wstring> enumerate_sub_keys(HKEY key)
{
    DWORD sub_key_count = 0;
    DWORD max_sub_key_length = 0;
    const LSTATUS query_status = RegQueryInfoKeyW(
        key,
        nullptr,
        nullptr,
        nullptr,
        &sub_key_count,
        &max_sub_key_length,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (query_status != ERROR_SUCCESS)
    {
        return {};
    }

    std::vector<std::wstring> names;
    std::vector<wchar_t> buffer(max_sub_key_length + 1);

    for (DWORD index = 0; index < sub_key_count; ++index)
    {
        DWORD name_length = static_cast<DWORD>(buffer.size());
        const LSTATUS enum_status = RegEnumKeyExW(
            key,
            index,
            buffer.data(),
            &name_length,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        if (enum_status == ERROR_SUCCESS)
        {
            names.emplace_back(buffer.data(), name_length);
        }
    }

    return names;
}
