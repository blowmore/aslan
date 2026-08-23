#pragma once

#include "installation.hpp"

#include <optional>
#include <string>
#include <vector>

std::optional<std::size_t> dialog(const std::vector<Installation>& installations, const std::wstring& message);
