#pragma once

#include <array>
#include <cstdint>

#include "libcuebin/cueTypes.hpp"

namespace cuebin {

static constexpr size_t RAW_SECTOR_SIZE = 2352;

struct SectorData {
    std::array<uint8_t, RAW_SECTOR_SIZE> data{};
    TrackMode mode = TrackMode::Audio;
};

} // namespace cuebin
