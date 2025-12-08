#include "libcuebin/cueParser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace cuebin {

std::string_view trim(std::string_view sv)
{
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
        sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
        sv.remove_suffix(1);
    return sv;
}

std::string toUpper(std::string_view sv)
{
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return s;
}

// Extract the next token from the line, handling quoted strings.
// Advances `line` past the consumed token.
std::string nextToken(std::string_view& line)
{
    line = trim(line);
    if (line.empty()) return {};

    if (line.front() == '"') {
        auto end = line.find('"', 1);
        if (end == std::string_view::npos) {
            // Unterminated quote -- take rest of line
            std::string tok(line.substr(1));
            line = {};
            return tok;
        }
        std::string tok(line.substr(1, end - 1));
        line.remove_prefix(end + 1);
        return tok;
    }

    auto end = line.find_first_of(" \t");
    if (end == std::string_view::npos) {
        std::string tok(line);
        line = {};
        return tok;
    }
    std::string tok(line.substr(0, end));
    line.remove_prefix(end);
    return tok;
}

// Consume the rest of the line as a single token (for quoted values after a keyword).
std::string restAsString(std::string_view& line)
{
    line = trim(line);
    if (line.empty()) return {};

    if (line.front() == '"') {
        auto end = line.find('"', 1);
        if (end == std::string_view::npos) {
            std::string s(line.substr(1));
            line = {};
            return s;
        }
        std::string s(line.substr(1, end - 1));
        line.remove_prefix(end + 1);
        return s;
    }

    std::string s(line);
    line = {};
    return s;
}

Result<uint8_t> parseUint8(std::string_view sv)
{
    uint8_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidCueFormat,
            "Expected number, got: '" + std::string(sv) + "'");
    }
    return val;
}

uint8_t parseFlags(std::string_view line)
{
    uint8_t flags = 0;
    while (!line.empty()) {
        auto tok = toUpper(nextToken(line));
        if (tok == "DCP")       flags |= static_cast<uint8_t>(TrackFlag::DCP);
        else if (tok == "4CH")  flags |= static_cast<uint8_t>(TrackFlag::CH4);
        else if (tok == "PRE")  flags |= static_cast<uint8_t>(TrackFlag::PRE);
        else if (tok == "SCMS") flags |= static_cast<uint8_t>(TrackFlag::SCMS);
        else spdlog::warn("Unknown track flag: '{}'", tok);
    }
    return flags;
}

Result<CueSheet> CueParser::parseFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return LIBCUEBIN_ERROR(ErrorCode::FileNotFound,
            "Cannot open CUE file: " + path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return parseString(ss.str());
}

Result<CueSheet> CueParser::parseString(std::string_view content)
{
    CueSheet sheet;
    CueFile* currentFile = nullptr;
    CueTrack* currentTrack = nullptr;
    int lineNum = 0;

    std::istringstream stream{std::string(content)};
    std::string lineBuf;

    while (std::getline(stream, lineBuf)) {
        ++lineNum;
        std::string_view line = trim(lineBuf);
        if (line.empty()) continue;

        std::string_view remaining = line;
        std::string keyword = toUpper(nextToken(remaining));

        if (keyword == "FILE") {
            std::string filename = nextToken(remaining);
            std::string typeStr = nextToken(remaining);
            if (filename.empty() || typeStr.empty()) {
                return LIBCUEBIN_ERROR(ErrorCode::InvalidCueFormat,
                    "FILE directive missing filename or type at line " + std::to_string(lineNum));
            }
            auto fileType = parseFileType(typeStr);
            if (!fileType) return fileType.error();

            sheet.files.emplace_back();
            currentFile = &sheet.files.back();
            currentFile->filename = std::move(filename);
            currentFile->type = *fileType;
            currentTrack = nullptr;
        }
        else if (keyword == "TRACK") {
            if (!currentFile) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "TRACK before FILE at line " + std::to_string(lineNum));
            }
            std::string numStr = nextToken(remaining);
            std::string modeStr = nextToken(remaining);
            if (numStr.empty() || modeStr.empty()) {
                return LIBCUEBIN_ERROR(ErrorCode::InvalidCueFormat,
                    "TRACK directive missing number or mode at line " + std::to_string(lineNum));
            }
            auto num = parseUint8(numStr);
            if (!num) return num.error();
            auto mode = parseTrackMode(modeStr);
            if (!mode) return mode.error();

            currentFile->tracks.emplace_back();
            currentTrack = &currentFile->tracks.back();
            currentTrack->number = *num;
            currentTrack->mode = *mode;
        }
        else if (keyword == "INDEX") {
            if (!currentTrack) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "INDEX before TRACK at line " + std::to_string(lineNum));
            }
            std::string numStr = nextToken(remaining);
            std::string msfStr = nextToken(remaining);
            if (numStr.empty() || msfStr.empty()) {
                return LIBCUEBIN_ERROR(ErrorCode::InvalidCueFormat,
                    "INDEX directive missing number or position at line " + std::to_string(lineNum));
            }
            auto num = parseUint8(numStr);
            if (!num) return num.error();
            auto msf = MSF::parse(msfStr);
            if (!msf) return msf.error();

            // Check for duplicate index
            for (const auto& idx : currentTrack->indices) {
                if (idx.number == *num) {
                    return LIBCUEBIN_ERROR(ErrorCode::DuplicateIndex,
                        "Duplicate INDEX " + std::to_string(*num) + " at line " + std::to_string(lineNum));
                }
            }

            currentTrack->indices.push_back({*num, *msf});
        }
        else if (keyword == "PREGAP") {
            if (!currentTrack) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "PREGAP before TRACK at line " + std::to_string(lineNum));
            }
            std::string msfStr = nextToken(remaining);
            auto msf = MSF::parse(msfStr);
            if (!msf) return msf.error();
            currentTrack->pregap = *msf;
        }
        else if (keyword == "POSTGAP") {
            if (!currentTrack) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "POSTGAP before TRACK at line " + std::to_string(lineNum));
            }
            std::string msfStr = nextToken(remaining);
            auto msf = MSF::parse(msfStr);
            if (!msf) return msf.error();
            currentTrack->postgap = *msf;
        }
        else if (keyword == "FLAGS") {
            if (!currentTrack) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "FLAGS before TRACK at line " + std::to_string(lineNum));
            }
            currentTrack->flags = parseFlags(remaining);
        }
        else if (keyword == "ISRC") {
            if (!currentTrack) {
                return LIBCUEBIN_ERROR(ErrorCode::UnexpectedDirective,
                    "ISRC before TRACK at line " + std::to_string(lineNum));
            }
            currentTrack->isrc = nextToken(remaining);
        }
        else if (keyword == "CATALOG") {
            sheet.catalog = nextToken(remaining);
        }
        else if (keyword == "CDTEXTFILE") {
            sheet.cdtextfile = restAsString(remaining);
        }
        else if (keyword == "TITLE") {
            std::string val = restAsString(remaining);
            if (currentTrack) {
                currentTrack->title = std::move(val);
            } else {
                sheet.title = std::move(val);
            }
        }
        else if (keyword == "PERFORMER") {
            std::string val = restAsString(remaining);
            if (currentTrack) {
                currentTrack->performer = std::move(val);
            } else {
                sheet.performer = std::move(val);
            }
        }
        else if (keyword == "SONGWRITER") {
            std::string val = restAsString(remaining);
            if (currentTrack) {
                currentTrack->songwriter = std::move(val);
            } else {
                sheet.songwriter = std::move(val);
            }
        }
        else if (keyword == "REM") {
            sheet.remarks.emplace_back(remaining);
        }
        else {
            spdlog::debug("Skipping unknown CUE directive '{}' at line {}", keyword, lineNum);
        }
    }

    if (sheet.files.empty()) {
        return LIBCUEBIN_ERROR(ErrorCode::InvalidCueFormat,
            "CUE sheet contains no FILE directives");
    }

    return sheet;
}

} // namespace cuebin
