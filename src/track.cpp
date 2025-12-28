#include "libcuebin/track.hpp"

namespace cuebin {

Track::Track(uint8_t number, TrackMode mode, uint16_t sectorSize,
             int32_t startLba, int32_t lengthSectors,
             int32_t pregapSectors, int32_t postgapSectors,
             std::vector<CueIndex> indices,
             std::optional<std::string> title,
             std::optional<std::string> performer,
             std::optional<std::string> isrc,
             size_t fileIndex, int64_t fileByteOffset,
             int32_t fileStartLba)
    : m_number(number)
    , m_mode(mode)
    , m_sectorSize(sectorSize)
    , m_startLba(startLba)
    , m_lengthSectors(lengthSectors)
    , m_pregapSectors(pregapSectors)
    , m_postgapSectors(postgapSectors)
    , m_indices(std::move(indices))
    , m_title(std::move(title))
    , m_performer(std::move(performer))
    , m_isrc(std::move(isrc))
    , m_fileIndex(fileIndex)
    , m_fileByteOffset(fileByteOffset)
    , m_fileStartLba(fileStartLba)
{}

std::optional<std::string_view> Track::title() const noexcept
{
    if (m_title) return std::string_view(*m_title);
    return std::nullopt;
}

std::optional<std::string_view> Track::performer() const noexcept
{
    if (m_performer) return std::string_view(*m_performer);
    return std::nullopt;
}

std::optional<std::string_view> Track::isrc() const noexcept
{
    if (m_isrc) return std::string_view(*m_isrc);
    return std::nullopt;
}

} // namespace cuebin
