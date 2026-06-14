#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <vector>

struct AutomationStudioInstallation {
    std::wstring version;
    std::wstring installPath;
    std::wstring executablePath;
    bool executableExists = false;
};

class RegistryKey {
public:
    RegistryKey() = default;
    explicit RegistryKey(HKEY handle) : handle_(handle) {}

    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;

    RegistryKey(RegistryKey&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    RegistryKey& operator=(RegistryKey&& other) noexcept
    {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    ~RegistryKey()
    {
        close();
    }

    HKEY get() const
    {
        return handle_;
    }

    explicit operator bool() const
    {
        return handle_ != nullptr;
    }

private:
    void close()
    {
        if (handle_) {
            RegCloseKey(handle_);
            handle_ = nullptr;
        }
    }

    HKEY handle_ = nullptr;
};

RegistryKey openLocalMachineKey(const std::wstring& subKey, REGSAM access = KEY_READ)
{
    HKEY handle = nullptr;
    const LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, access, &handle);
    return status == ERROR_SUCCESS ? RegistryKey(handle) : RegistryKey();
}

RegistryKey openChildKey(HKEY parent, const std::wstring& subKey)
{
    HKEY handle = nullptr;
    const LSTATUS status = RegOpenKeyExW(parent, subKey.c_str(), 0, KEY_READ, &handle);
    return status == ERROR_SUCCESS ? RegistryKey(handle) : RegistryKey();
}

std::wstring readRegistryString(HKEY key, const wchar_t* valueName)
{
    DWORD type = 0;
    DWORD byteCount = 0;
    DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;

    LSTATUS status = RegGetValueW(key, nullptr, valueName, flags, &type, nullptr, &byteCount);
    if (status != ERROR_SUCCESS || byteCount == 0) {
        return L"";
    }

    std::wstring value(byteCount / sizeof(wchar_t), L'\0');
    status = RegGetValueW(key, nullptr, valueName, flags, &type, value.data(), &byteCount);
    if (status != ERROR_SUCCESS) {
        return L"";
    }

    value.resize(byteCount / sizeof(wchar_t));
    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }

    return value;
}

std::vector<std::wstring> enumerateSubKeys(HKEY key)
{
    DWORD subKeyCount = 0;
    DWORD maxSubKeyLength = 0;
    const LSTATUS queryStatus = RegQueryInfoKeyW(
        key,
        nullptr,
        nullptr,
        nullptr,
        &subKeyCount,
        &maxSubKeyLength,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (queryStatus != ERROR_SUCCESS) {
        return {};
    }

    std::vector<std::wstring> names;
    std::vector<wchar_t> buffer(maxSubKeyLength + 1);

    for (DWORD index = 0; index < subKeyCount; ++index) {
        DWORD nameLength = static_cast<DWORD>(buffer.size());
        const LSTATUS enumStatus = RegEnumKeyExW(
            key,
            index,
            buffer.data(),
            &nameLength,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        if (enumStatus == ERROR_SUCCESS) {
            names.emplace_back(buffer.data(), nameLength);
        }
    }

    return names;
}

std::wstring toLower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });

    return value;
}

bool containsCaseInsensitive(const std::wstring& value, const std::wstring& needle)
{
    return toLower(value).find(toLower(needle)) != std::wstring::npos;
}

bool fileExists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring appendPath(const std::wstring& base, const std::wstring& child)
{
    if (base.empty()) {
        return child;
    }

    const wchar_t last = base.back();
    if (last == L'\\' || last == L'/') {
        return base + child;
    }

    return base + L"\\" + child;
}

std::vector<AutomationStudioInstallation> findAutomationStudioInstallationsInRoot(
    const std::wstring& registryRoot,
    REGSAM access)
{
    RegistryKey root = openLocalMachineKey(registryRoot, access);
    if (!root) {
        return {};
    }

    std::vector<AutomationStudioInstallation> installations;
    for (const std::wstring& subKeyName : enumerateSubKeys(root.get())) {
        if (!containsCaseInsensitive(subKeyName, L"BR_AS")) {
            continue;
        }

        RegistryKey asKey = openChildKey(root.get(), subKeyName);
        if (!asKey) {
            continue;
        }

        std::wstring installPath = readRegistryString(asKey.get(), L"BuRSharedFilesPath");
        if (installPath.empty()) {
            continue;
        }

        RegistryKey controlStudioKey = openChildKey(asKey.get(), L"ControlStudio");
        std::wstring version;
        if (controlStudioKey) {
            version = readRegistryString(controlStudioKey.get(), L"ProgrVersion");
        }

        std::wstring executablePath = appendPath(installPath, L"bin-en\\AutomationStudio.exe");
        installations.push_back({ version, installPath, executablePath, fileExists(executablePath) });
    }

    std::sort(installations.begin(), installations.end(), [](const auto& left, const auto& right) {
        return left.version < right.version;
    });

    return installations;
}

std::vector<AutomationStudioInstallation> findAutomationStudioInstallations()
{
    // Automation Studio registers as a 32-bit application. This is the WinAPI
    // equivalent of the launch script's HKLM:\SOFTWARE\WOW6432Node scan.
    std::vector<AutomationStudioInstallation> installations =
        findAutomationStudioInstallationsInRoot(L"SOFTWARE", KEY_READ | KEY_WOW64_32KEY);

    if (installations.empty()) {
        installations = findAutomationStudioInstallationsInRoot(L"SOFTWARE\\WOW6432Node", KEY_READ);
    }

    if (installations.empty()) {
        installations = findAutomationStudioInstallationsInRoot(L"SOFTWARE", KEY_READ);
    }

    return installations;
}

std::optional<std::string> readProjectAutomationStudioVersion(const std::filesystem::path& projectPath)
{
    std::ifstream projectFile(projectPath, std::ios::binary);
    if (!projectFile) {
        return std::nullopt;
    }

    const std::string projectContents{
        std::istreambuf_iterator<char>(projectFile),
        std::istreambuf_iterator<char>()};

    const std::regex versionPattern(R"(AutomationStudio Version="?([0-9])\.([0-9]+)\.([0-9]+))");
    std::smatch match;
    if (!std::regex_search(projectContents, match, versionPattern)) {
        return std::nullopt;
    }

    return match[1].str() + "." + match[2].str() + "." + match[3].str();
}

std::wstring widenAscii(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

bool startsWith(const std::wstring& value, const std::wstring& prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

void printInstallations(const std::vector<AutomationStudioInstallation>& installations)
{
    std::wcout << L"Automation Studio installations\n\n";

    if (installations.empty()) {
        std::wcout << L"No Automation Studio installations found.\n";
        return;
    }

    for (std::size_t index = 0; index < installations.size(); ++index) {
        const AutomationStudioInstallation& installation = installations[index];

        std::wcout << index + 1 << L". Version: "
                   << (installation.version.empty() ? L"(unknown)" : installation.version) << L"\n";
        std::wcout << L"   Install path: " << installation.installPath << L"\n";
        std::wcout << L"   Executable:   " << installation.executablePath;
        if (!installation.executableExists) {
            std::wcout << L" (missing)";
        }
        std::wcout << L"\n\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    const std::vector<AutomationStudioInstallation> installations = findAutomationStudioInstallations();
    printInstallations(installations);

    if (argc > 1) {
        const std::filesystem::path projectPath(argv[1]);
        const std::optional<std::string> projectVersion = readProjectAutomationStudioVersion(projectPath);

        if (!projectVersion) {
            std::wcout << L"Could not read an Automation Studio version from: " << projectPath.wstring() << L"\n";
            return installations.empty() ? 1 : 0;
        }

        const std::wstring requiredVersion = widenAscii(*projectVersion);
        std::wcout << L"Project version: " << requiredVersion << L"\n";

        const auto matchingInstallation = std::find_if(
            installations.begin(),
            installations.end(),
            [&requiredVersion](const AutomationStudioInstallation& installation) {
                return startsWith(installation.version, requiredVersion);
            });

        if (matchingInstallation != installations.end()) {
            std::wcout << L"Matching executable: " << matchingInstallation->executablePath;
            if (!matchingInstallation->executableExists) {
                std::wcout << L" (missing)";
            }
            std::wcout << L"\n";
        }
        else {
            std::wcout << L"No installed Automation Studio version matches " << requiredVersion << L".\n";
        }
    }

    return installations.empty() ? 1 : 0;
}
