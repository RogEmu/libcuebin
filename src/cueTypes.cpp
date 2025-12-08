#include "libcuebin/cueTypes.hpp"

#include <algorithm>

namespace cuebin {

uint16_t sectorSizeForMode(TrackMode mode) noexcept
{
    switch (mode) {
        case TrackMode::Audio:      return 2352;
        case TrackMode::CDG:        return 2448;
        case TrackMode::Mode1_2048: return 2048;
        case TrackMode::Mode1_2352: return 2352;
        case TrackMode::Mode2_2336: return 2336;
        case TrackMode::Mode2_2352: return 2352;
        case TrackMode::CDI_2336:   return 2336;
        case TrackMode::CDI_2352:   return 2352;
    }
    return 2352;
}

Result<TrackMode> parseTrackMode(std::string_view str)
{
    // Uppercase comparison
    std::string upper(str);
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "AUDIO")      return TrackMode::Audio;
    if (upper == "CDG")        return TrackMode::CDG;
    if (upper == "MODE1/2048") return TrackMode::Mode1_2048;
    if (upper == "MODE1/2352") return TrackMode::Mode1_2352;
    if (upper == "MODE2/2336") return TrackMode::Mode2_2336;
    if (upper == "MODE2/2352") return TrackMode::Mode2_2352;
    if (upper == "CDI/2336")   return TrackMode::CDI_2336;
    if (upper == "CDI/2352")   return TrackMode::CDI_2352;

    return LIBCUEBIN_ERROR(ErrorCode::InvalidTrackMode,
        "Unknown track mode: '" + std::string(str) + "'");
}

Result<FileType> parseFileType(std::string_view str)
{
    std::string upper(str);
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "BINARY")   return FileType::Binary;
    if (upper == "MOTOROLA") return FileType::Motorola;
    if (upper == "AIFF")     return FileType::Aiff;
    if (upper == "WAVE")     return FileType::Wave;
    if (upper == "MP3")      return FileType::MP3;

    return LIBCUEBIN_ERROR(ErrorCode::InvalidFileType,
        "Unknown file type: '" + std::string(str) + "'");
}

std::string_view trackModeToString(TrackMode mode) noexcept
{
    switch (mode) {
        case TrackMode::Audio:      return "AUDIO";
        case TrackMode::CDG:        return "CDG";
        case TrackMode::Mode1_2048: return "MODE1/2048";
        case TrackMode::Mode1_2352: return "MODE1/2352";
        case TrackMode::Mode2_2336: return "MODE2/2336";
        case TrackMode::Mode2_2352: return "MODE2/2352";
        case TrackMode::CDI_2336:   return "CDI/2336";
        case TrackMode::CDI_2352:   return "CDI/2352";
    }
    return "UNKNOWN";
}

std::string_view fileTypeToString(FileType type) noexcept
{
    switch (type) {
        case FileType::Binary:   return "BINARY";
        case FileType::Motorola: return "MOTOROLA";
        case FileType::Aiff:     return "AIFF";
        case FileType::Wave:     return "WAVE";
        case FileType::MP3:      return "MP3";
    }
    return "UNKNOWN";
}

} // namespace cuebin
