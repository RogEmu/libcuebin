#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "libcuebin/error.hpp"

namespace cuebin {

struct MSF {
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t frame = 0;

    static constexpr int FRAMES_PER_SECOND = 75;
    static constexpr int SECONDS_PER_MINUTE = 60;
    static constexpr int FRAMES_PER_MINUTE = FRAMES_PER_SECOND * SECONDS_PER_MINUTE;
    static constexpr int PREGAP_FRAMES = 150; // 2-second pregap (2 * 75)

    constexpr MSF() = default;
    constexpr MSF(uint8_t m, uint8_t s, uint8_t f)
        : minute(m), second(s), frame(f) {}

    constexpr int32_t toLba() const noexcept {
        return static_cast<int32_t>(minute) * FRAMES_PER_MINUTE
             + static_cast<int32_t>(second) * FRAMES_PER_SECOND
             + static_cast<int32_t>(frame);
    }

    static constexpr MSF fromLba(int32_t lba) noexcept {
        MSF msf;
        msf.minute = static_cast<uint8_t>(lba / FRAMES_PER_MINUTE);
        lba %= FRAMES_PER_MINUTE;
        msf.second = static_cast<uint8_t>(lba / FRAMES_PER_SECOND);
        msf.frame = static_cast<uint8_t>(lba % FRAMES_PER_SECOND);
        return msf;
    }

    static Result<MSF> parse(std::string_view str);

    std::string toString() const;

    static constexpr MSF toPhysicalMsf(int32_t lba) noexcept {
        return fromLba(lba + PREGAP_FRAMES);
    }

    constexpr bool operator==(const MSF& other) const noexcept {
        return minute == other.minute && second == other.second && frame == other.frame;
    }

    constexpr bool operator!=(const MSF& other) const noexcept {
        return !(*this == other);
    }

    constexpr bool operator<(const MSF& other) const noexcept {
        return toLba() < other.toLba();
    }

    constexpr bool operator<=(const MSF& other) const noexcept {
        return toLba() <= other.toLba();
    }

    constexpr bool operator>(const MSF& other) const noexcept {
        return toLba() > other.toLba();
    }

    constexpr bool operator>=(const MSF& other) const noexcept {
        return toLba() >= other.toLba();
    }
};

} // namespace cuebin
