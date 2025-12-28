#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "libcuebin/cueTypes.hpp"
#include "libcuebin/msf.hpp"

namespace cuebin {

class Track {
public:
    Track(uint8_t number, TrackMode mode, uint16_t sectorSize,
          int32_t startLba, int32_t lengthSectors,
          int32_t pregapSectors, int32_t postgapSectors,
          std::vector<CueIndex> indices,
          std::optional<std::string> title,
          std::optional<std::string> performer,
          std::optional<std::string> isrc,
          size_t fileIndex, int64_t fileByteOffset,
          int32_t fileStartLba);

    uint8_t number() const noexcept { return m_number; }
    TrackMode mode() const noexcept { return m_mode; }
    uint16_t sectorSize() const noexcept { return m_sectorSize; }

    int32_t startLba() const noexcept { return m_startLba; }
    int32_t lengthSectors() const noexcept { return m_lengthSectors; }
    int32_t endLba() const noexcept { return m_startLba + m_lengthSectors; }

    int32_t pregapSectors() const noexcept { return m_pregapSectors; }
    int32_t postgapSectors() const noexcept { return m_postgapSectors; }

    const std::vector<CueIndex>& indices() const noexcept { return m_indices; }

    bool isAudio() const noexcept { return m_mode == TrackMode::Audio; }
    bool isData() const noexcept { return !isAudio(); }

    std::optional<std::string_view> title() const noexcept;
    std::optional<std::string_view> performer() const noexcept;
    std::optional<std::string_view> isrc() const noexcept;

    // Internal: used by Disc for I/O
    size_t fileIndex() const noexcept { return m_fileIndex; }
    int64_t fileByteOffset() const noexcept { return m_fileByteOffset; }
    int32_t fileStartLba() const noexcept { return m_fileStartLba; }

private:
    uint8_t m_number;
    TrackMode m_mode;
    uint16_t m_sectorSize;
    int32_t m_startLba;
    int32_t m_lengthSectors;
    int32_t m_pregapSectors;
    int32_t m_postgapSectors;
    std::vector<CueIndex> m_indices;
    std::optional<std::string> m_title;
    std::optional<std::string> m_performer;
    std::optional<std::string> m_isrc;
    size_t m_fileIndex;
    int64_t m_fileByteOffset;
    int32_t m_fileStartLba;
};

} // namespace cuebin
