#include "libcuebin/disc.hpp"
#include "libcuebin/cueParser.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <mutex>

#include <spdlog/spdlog.h>

namespace cuebin {

struct FileHandle {
    std::filesystem::path path;
    int64_t fileSize = 0;
    mutable std::once_flag openFlag;
    mutable std::ifstream stream;
    mutable std::mutex mutex;
};

struct Disc::Impl {
    CueSheet sheet;
    std::filesystem::path baseDir;
    std::vector<Track> tracks;
    std::vector<std::unique_ptr<FileHandle>> fileHandles;
    int32_t totalSectors = 0;

    std::optional<std::string> title;
    std::optional<std::string> performer;
    std::optional<std::string> catalog;
};

Disc::Disc(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
Disc::~Disc() = default;
Disc::Disc(Disc&& other) noexcept = default;
Disc& Disc::operator=(Disc&& other) noexcept = default;

Result<Disc> Disc::fromCue(const std::filesystem::path& cuePath)
{
    auto sheetResult = CueParser::parseFile(cuePath);
    if (!sheetResult) return sheetResult.error();

    auto impl = std::make_unique<Impl>();
    impl->sheet = std::move(*sheetResult);
    impl->baseDir = cuePath.parent_path();

    if (impl->sheet.title) impl->title = *impl->sheet.title;
    if (impl->sheet.performer) impl->performer = *impl->sheet.performer;
    if (impl->sheet.catalog) impl->catalog = *impl->sheet.catalog;

    // Resolve file paths and get sizes
    for (const auto& cueFile : impl->sheet.files) {
        auto handle = std::make_unique<FileHandle>();
        handle->path = impl->baseDir / cueFile.filename;

        std::error_code ec;
        if (!std::filesystem::exists(handle->path, ec)) {
            return LIBCUEBIN_ERROR(ErrorCode::FileNotFound,
                "BIN file not found: " + handle->path.string());
        }
        handle->fileSize = static_cast<int64_t>(std::filesystem::file_size(handle->path, ec));
        if (ec) {
            return LIBCUEBIN_ERROR(ErrorCode::FileReadError,
                "Cannot get file size: " + handle->path.string());
        }

        impl->fileHandles.push_back(std::move(handle));
    }

    // Build tracks with LBA positions
    int32_t currentLba = 0;

    for (size_t fi = 0; fi < impl->sheet.files.size(); ++fi) {
        const auto& cueFile = impl->sheet.files[fi];

        for (size_t ti = 0; ti < cueFile.tracks.size(); ++ti) {
            const auto& ct = cueFile.tracks[ti];
            uint16_t ss = sectorSizeForMode(ct.mode);

            // Pregap: virtual sectors not in file, add to LBA
            int32_t pregap = 0;
            if (ct.pregap) {
                pregap = ct.pregap->toLba();
                currentLba += pregap;
            }

            // Find INDEX 00 and INDEX 01
            int32_t index01Offset = 0;
            for (const auto& idx : ct.indices) {
                if (idx.number == 1) index01Offset = idx.position.toLba();
            }

            // For the first track in a file, the byte offset starts at 0
            // and the INDEX positions are relative to the file start.
            // For subsequent tracks, positions are also file-relative.
            int64_t trackFileByteOffset;

            if (ti == 0) {
                // First track in file: index positions are from file start
                trackFileByteOffset = static_cast<int64_t>(index01Offset) * ss;
            } else {
                trackFileByteOffset = static_cast<int64_t>(index01Offset) * ss;
            }

            // Calculate track length
            int32_t trackSectors;
            if (ti + 1 < cueFile.tracks.size()) {
                // Next track in same file determines end
                const auto& next = cueFile.tracks[ti + 1];
                int32_t nextStart = 0;
                for (const auto& idx : next.indices) {
                    if (idx.number == 0) { nextStart = idx.position.toLba(); break; }
                    if (idx.number == 1) { nextStart = idx.position.toLba(); }
                }
                trackSectors = nextStart - index01Offset;
            } else {
                // Last track in file: use file size
                int64_t remainingBytes = impl->fileHandles[fi]->fileSize
                                        - static_cast<int64_t>(index01Offset) * ss;
                trackSectors = static_cast<int32_t>(remainingBytes / ss);
            }

            int32_t postgap = 0;
            if (ct.postgap) {
                postgap = ct.postgap->toLba();
            }

            impl->tracks.emplace_back(
                ct.number, ct.mode, ss,
                currentLba, trackSectors,
                pregap, postgap,
                ct.indices,
                ct.title, ct.performer, ct.isrc,
                fi, trackFileByteOffset,
                currentLba
            );

            currentLba += trackSectors + postgap;
        }
    }

    impl->totalSectors = currentLba;

    spdlog::info("Loaded CUE: {} tracks, {} total sectors",
                 impl->tracks.size(), impl->totalSectors);

    return Disc(std::move(impl));
}

size_t Disc::trackCount() const noexcept
{
    return m_impl->tracks.size();
}

const Track* Disc::track(uint8_t trackNumber) const noexcept
{
    for (const auto& t : m_impl->tracks) {
        if (t.number() == trackNumber) return &t;
    }
    return nullptr;
}

std::span<const Track> Disc::tracks() const noexcept
{
    return m_impl->tracks;
}

uint8_t Disc::firstTrackNumber() const noexcept
{
    if (m_impl->tracks.empty()) return 0;
    return m_impl->tracks.front().number();
}

uint8_t Disc::lastTrackNumber() const noexcept
{
    if (m_impl->tracks.empty()) return 0;
    return m_impl->tracks.back().number();
}

int32_t Disc::totalSectors() const noexcept
{
    return m_impl->totalSectors;
}

int32_t Disc::leadOutLba() const noexcept
{
    return m_impl->totalSectors;
}

const Track* Disc::findTrack(int32_t lba) const noexcept
{
    // Binary search: find last track whose startLba <= lba
    const Track* found = nullptr;
    int lo = 0;
    int hi = static_cast<int>(m_impl->tracks.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (m_impl->tracks[mid].startLba() <= lba) {
            found = &m_impl->tracks[mid];
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    if (found && lba < found->endLba()) return found;
    return nullptr;
}

Result<SectorData> Disc::readSector(int32_t lba) const
{
    if (lba < 0 || lba >= m_impl->totalSectors) {
        return LIBCUEBIN_ERROR(ErrorCode::LBAOutOfRange,
            "LBA " + std::to_string(lba) + " out of range [0, "
            + std::to_string(m_impl->totalSectors) + ")");
    }

    const Track* trk = findTrack(lba);
    if (!trk) {
        return LIBCUEBIN_ERROR(ErrorCode::TrackNotFound,
            "No track found for LBA " + std::to_string(lba));
    }

    auto& fh = *m_impl->fileHandles[trk->fileIndex()];

    // Lazy open
    std::call_once(fh.openFlag, [&fh]() {
        fh.stream.open(fh.path, std::ios::binary);
        if (fh.stream.is_open()) {
            spdlog::debug("Opened file: {}", fh.path.string());
        }
    });

    std::lock_guard<std::mutex> lock(fh.mutex);

    if (!fh.stream.is_open()) {
        return LIBCUEBIN_ERROR(ErrorCode::FileReadError,
            "Cannot open file: " + fh.path.string());
    }

    int64_t offset = trk->fileByteOffset()
                   + static_cast<int64_t>(lba - trk->fileStartLba()) * trk->sectorSize();

    fh.stream.seekg(offset);
    if (!fh.stream) {
        return LIBCUEBIN_ERROR(ErrorCode::FileSeekError,
            "Seek failed at offset " + std::to_string(offset));
    }

    SectorData sector;
    sector.mode = trk->mode();

    uint16_t readSize = trk->sectorSize();
    if (readSize > RAW_SECTOR_SIZE) readSize = RAW_SECTOR_SIZE;

    fh.stream.read(reinterpret_cast<char*>(sector.data.data()), readSize);
    if (!fh.stream) {
        // Allow partial read at end of file
        auto bytesRead = fh.stream.gcount();
        if (bytesRead == 0) {
            return LIBCUEBIN_ERROR(ErrorCode::FileReadError,
                "Read failed at offset " + std::to_string(offset));
        }
        // Zero-fill remainder
        std::memset(sector.data.data() + bytesRead, 0, RAW_SECTOR_SIZE - bytesRead);
        fh.stream.clear();
    }

    // For modes with smaller sector sizes (e.g., 2048), pad the rest with zeros
    if (readSize < RAW_SECTOR_SIZE) {
        std::memset(sector.data.data() + readSize, 0, RAW_SECTOR_SIZE - readSize);
    }

    return sector;
}

Result<SectorData> Disc::readSector(MSF address) const
{
    return readSector(address.toLba());
}

Result<std::vector<SectorData>> Disc::readSectors(int32_t lba, int32_t count) const
{
    if (count <= 0) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidArgument,
            "Sector count must be positive");
    }

    std::vector<SectorData> sectors;
    sectors.reserve(count);

    for (int32_t i = 0; i < count; ++i) {
        auto result = readSector(lba + i);
        if (!result) return result.error();
        sectors.push_back(std::move(*result));
    }

    return sectors;
}

std::optional<std::string_view> Disc::title() const noexcept
{
    if (m_impl->title) return std::string_view(*m_impl->title);
    return std::nullopt;
}

std::optional<std::string_view> Disc::performer() const noexcept
{
    if (m_impl->performer) return std::string_view(*m_impl->performer);
    return std::nullopt;
}

std::optional<std::string_view> Disc::catalog() const noexcept
{
    if (m_impl->catalog) return std::string_view(*m_impl->catalog);
    return std::nullopt;
}

const CueSheet& Disc::cueSheet() const noexcept
{
    return m_impl->sheet;
}

} // namespace cuebin
