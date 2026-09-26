#define WIN32_LEAN_AND_MEAN

#include "settings.hpp"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>

#include <filesystem>
#include <iterator>
#include <string>

namespace
{
    std::filesystem::path settings_path()
    {
        PWSTR local_app_data = nullptr;
        const HRESULT result = SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            KF_FLAG_DEFAULT,
            nullptr,
            &local_app_data);

        if (FAILED(result))
        {
            return {};
        }

        const std::filesystem::path path =
            std::filesystem::path(local_app_data) / L"Aslan" / L"settings.ini";
        CoTaskMemFree(local_app_data);
        return path;
    }

    bool read_bool(const std::filesystem::path& path, const wchar_t* name, bool fallback)
    {
        wchar_t value[16]{};
        const DWORD length = GetPrivateProfileStringW(
            L"Launch",
            name,
            L"",
            value,
            static_cast<DWORD>(std::size(value)),
            path.c_str());

        if (length == 0)
        {
            return fallback;
        }

        const std::wstring text(value, length);
        if (text == L"1" || text == L"true" || text == L"True")
        {
            return true;
        }

        if (text == L"0" || text == L"false" || text == L"False")
        {
            return false;
        }

        return fallback;
    }
}

std::optional<Settings> load_settings()
{
    const std::filesystem::path path = settings_path();
    std::error_code error;
    if (path.empty() || !std::filesystem::is_regular_file(path, error) || error)
    {
        return std::nullopt;
    }

    Settings settings;
    settings.clear_user_state = read_bool(path, L"ClearUserState", settings.clear_user_state);
    return settings;
}

bool save_settings(const Settings& settings)
{
    const std::filesystem::path path = settings_path();
    if (path.empty())
    {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        return false;
    }

    return WritePrivateProfileStringW(
               L"Launch",
               L"ClearUserState",
               settings.clear_user_state ? L"1" : L"0",
               path.c_str()) != FALSE;
}
