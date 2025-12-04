#include "libcuebin/msf.hpp"

#include <charconv>
#include <cstdio>

namespace cuebin {

Result<MSF> MSF::parse(std::string_view str)
{
    // Expected format: MM:SS:FF
    if (str.size() < 5) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
            "MSF string too short: '" + std::string(str) + "'");
    }

    auto parsePart = [&](std::string_view part, const char* label) -> Result<uint8_t> {
        uint8_t val = 0;
        auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), val);
        if (ec != std::errc{} || ptr != part.data() + part.size()) {
            return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
                std::string("Invalid ") + label + " in MSF: '" + std::string(part) + "'");
        }
        return val;
    };

    auto colon1 = str.find(':');
    if (colon1 == std::string_view::npos) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
            "Missing first colon in MSF: '" + std::string(str) + "'");
    }
    auto colon2 = str.find(':', colon1 + 1);
    if (colon2 == std::string_view::npos) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
            "Missing second colon in MSF: '" + std::string(str) + "'");
    }

    auto mResult = parsePart(str.substr(0, colon1), "minute");
    if (!mResult) return mResult.error();

    auto sResult = parsePart(str.substr(colon1 + 1, colon2 - colon1 - 1), "second");
    if (!sResult) return sResult.error();

    auto fResult = parsePart(str.substr(colon2 + 1), "frame");
    if (!fResult) return fResult.error();

    uint8_t m = *mResult;
    uint8_t s = *sResult;
    uint8_t f = *fResult;

    if (s >= 60) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
            "Seconds out of range in MSF: " + std::to_string(s));
    }
    if (f >= 75) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidMSF,
            "Frames out of range in MSF: " + std::to_string(f));
    }

    return MSF(m, s, f);
}

std::string MSF::toString() const
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u", minute, second, frame);
    return buf;
}

} // namespace cuebin
