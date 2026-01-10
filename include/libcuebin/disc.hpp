#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "libcuebin/cueTypes.hpp"
#include "libcuebin/error.hpp"
#include "libcuebin/msf.hpp"
#include "libcuebin/sector.hpp"
#include "libcuebin/track.hpp"

namespace cuebin {

class Disc {
public:
    static Result<Disc> fromCue(const std::filesystem::path& cuePath);

    ~Disc();
    Disc(Disc&& other) noexcept;
    Disc& operator=(Disc&& other) noexcept;

    Disc(const Disc&) = delete;
    Disc& operator=(const Disc&) = delete;

    size_t trackCount() const noexcept;
    const Track* track(uint8_t trackNumber) const noexcept;
    std::span<const Track> tracks() const noexcept;
    uint8_t firstTrackNumber() const noexcept;
    uint8_t lastTrackNumber() const noexcept;

    int32_t totalSectors() const noexcept;
    Result<SectorData> readSector(int32_t lba) const;
    Result<SectorData> readSector(MSF address) const;
    Result<std::vector<SectorData>> readSectors(int32_t lba, int32_t count) const;
    const Track* findTrack(int32_t lba) const noexcept;
    int32_t leadOutLba() const noexcept;

    std::optional<std::string_view> title() const noexcept;
    std::optional<std::string_view> performer() const noexcept;
    std::optional<std::string_view> catalog() const noexcept;
    const CueSheet& cueSheet() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    explicit Disc(std::unique_ptr<Impl> impl);
};

} // namespace cuebin
