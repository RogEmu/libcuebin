#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "libcuebin/msf.hpp"

namespace cuebin {

enum class TrackMode {
    Audio,
    CDG,
    Mode1_2048,
    Mode1_2352,
    Mode2_2336,
    Mode2_2352,
    CDI_2336,
    CDI_2352,
};

enum class FileType {
    Binary,
    Motorola,
    Aiff,
    Wave,
    MP3,
};

enum class TrackFlag : uint8_t {
    DCP  = 1 << 0, // Digital copy permitted
    CH4  = 1 << 1, // Four channel audio
    PRE  = 1 << 2, // Pre-emphasis enabled
    SCMS = 1 << 3, // Serial copy management system
};

uint16_t sectorSizeForMode(TrackMode mode) noexcept;
Result<TrackMode> parseTrackMode(std::string_view str);
Result<FileType> parseFileType(std::string_view str);
std::string_view trackModeToString(TrackMode mode) noexcept;
std::string_view fileTypeToString(FileType type) noexcept;

struct CueIndex {
    uint8_t number = 0;
    MSF position;
};

struct CueTrack {
    uint8_t number = 0;
    TrackMode mode = TrackMode::Audio;
    std::vector<CueIndex> indices;
    std::optional<MSF> pregap;
    std::optional<MSF> postgap;
    uint8_t flags = 0;
    std::optional<std::string> isrc;
    std::optional<std::string> title;
    std::optional<std::string> performer;
    std::optional<std::string> songwriter;
};

struct CueFile {
    std::string filename;
    FileType type = FileType::Binary;
    std::vector<CueTrack> tracks;
};

struct CueSheet {
    std::vector<CueFile> files;
    std::optional<std::string> catalog;
    std::optional<std::string> cdtextfile;
    std::optional<std::string> title;
    std::optional<std::string> performer;
    std::optional<std::string> songwriter;
    std::vector<std::string> remarks;
};

} // namespace cuebin
