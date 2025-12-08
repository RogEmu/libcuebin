#pragma once

#include <filesystem>
#include <string_view>

#include "libcuebin/cueTypes.hpp"
#include "libcuebin/error.hpp"

namespace cuebin {

class CueParser {
public:
    static Result<CueSheet> parseFile(const std::filesystem::path& path);
    static Result<CueSheet> parseString(std::string_view content);
};

} // namespace cuebin
