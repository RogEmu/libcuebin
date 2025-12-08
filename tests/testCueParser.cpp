#include <gtest/gtest.h>
#include "libcuebin/cueParser.hpp"

#include <filesystem>

using namespace cuebin;

static const std::filesystem::path DATA_DIR = TEST_DATA_DIR;

TEST(CueParserTest, ParseSingleTrack) {
    auto result = CueParser::parseFile(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& sheet = *result;
    ASSERT_EQ(sheet.files.size(), 1u);
    EXPECT_EQ(sheet.files[0].filename, "singleTrack.bin");
    EXPECT_EQ(sheet.files[0].type, FileType::Binary);

    ASSERT_EQ(sheet.files[0].tracks.size(), 1u);
    const auto& track = sheet.files[0].tracks[0];
    EXPECT_EQ(track.number, 1);
    EXPECT_EQ(track.mode, TrackMode::Mode2_2352);
    ASSERT_EQ(track.indices.size(), 1u);
    EXPECT_EQ(track.indices[0].number, 1);
    EXPECT_EQ(track.indices[0].position, MSF(0, 0, 0));
}

TEST(CueParserTest, ParseMultiTrack) {
    auto result = CueParser::parseFile(DATA_DIR / "multiTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& sheet = *result;
    ASSERT_EQ(sheet.files.size(), 1u);
    ASSERT_EQ(sheet.files[0].tracks.size(), 3u);

    // Track 1: data
    EXPECT_EQ(sheet.files[0].tracks[0].number, 1);
    EXPECT_EQ(sheet.files[0].tracks[0].mode, TrackMode::Mode2_2352);

    // Track 2: audio with INDEX 00 and 01
    EXPECT_EQ(sheet.files[0].tracks[1].number, 2);
    EXPECT_EQ(sheet.files[0].tracks[1].mode, TrackMode::Audio);
    ASSERT_EQ(sheet.files[0].tracks[1].indices.size(), 2u);
    EXPECT_EQ(sheet.files[0].tracks[1].indices[0].number, 0);
    EXPECT_EQ(sheet.files[0].tracks[1].indices[1].number, 1);

    // Track 3: audio with pregap
    EXPECT_EQ(sheet.files[0].tracks[2].number, 3);
    EXPECT_TRUE(sheet.files[0].tracks[2].pregap.has_value());
    EXPECT_EQ(*sheet.files[0].tracks[2].pregap, MSF(0, 2, 0));
}

TEST(CueParserTest, ParseMultiFile) {
    auto result = CueParser::parseFile(DATA_DIR / "multiFile.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& sheet = *result;
    ASSERT_EQ(sheet.files.size(), 3u);
    EXPECT_EQ(sheet.files[0].filename, "data.bin");
    EXPECT_EQ(sheet.files[1].filename, "audio02.bin");
    EXPECT_EQ(sheet.files[2].filename, "audio03.bin");

    ASSERT_EQ(sheet.files[0].tracks.size(), 1u);
    ASSERT_EQ(sheet.files[1].tracks.size(), 1u);
    ASSERT_EQ(sheet.files[2].tracks.size(), 1u);
}

TEST(CueParserTest, ParseMetadata) {
    auto result = CueParser::parseFile(DATA_DIR / "metadata.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& sheet = *result;
    EXPECT_EQ(sheet.title, "Final Fantasy VII");
    EXPECT_EQ(sheet.performer, "Square");
    EXPECT_EQ(sheet.catalog, "0000000000000");
    ASSERT_GE(sheet.remarks.size(), 2u);

    ASSERT_EQ(sheet.files.size(), 1u);
    ASSERT_EQ(sheet.files[0].tracks.size(), 3u);

    // Track 1 metadata
    const auto& t1 = sheet.files[0].tracks[0];
    EXPECT_EQ(t1.title, "Data Track");
    EXPECT_EQ(t1.performer, "Square");
    EXPECT_EQ(t1.isrc, "JPSMK0100001");
    EXPECT_NE(t1.flags & static_cast<uint8_t>(TrackFlag::DCP), 0);

    // Track 2 metadata
    const auto& t2 = sheet.files[0].tracks[1];
    EXPECT_EQ(t2.title, "Opening - Bombing Mission");
    EXPECT_EQ(t2.performer, "Nobuo Uematsu");

    // Track 3 metadata with pregap and postgap
    const auto& t3 = sheet.files[0].tracks[2];
    EXPECT_EQ(t3.title, "Mako Reactor");
    EXPECT_TRUE(t3.pregap.has_value());
    EXPECT_EQ(*t3.pregap, MSF(0, 2, 0));
    EXPECT_TRUE(t3.postgap.has_value());
    EXPECT_EQ(*t3.postgap, MSF(0, 1, 0));
}

TEST(CueParserTest, ParseString) {
    const char* cue_text = R"(FILE "test.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
)";
    auto result = CueParser::parseString(cue_text);
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& sheet = *result;
    ASSERT_EQ(sheet.files.size(), 1u);
    EXPECT_EQ(sheet.files[0].tracks[0].mode, TrackMode::Mode1_2352);
}

TEST(CueParserTest, AllTrackModes) {
    const char* cue_text = R"(FILE "test.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 CDG
    INDEX 01 01:00:00
  TRACK 03 MODE1/2048
    INDEX 01 02:00:00
  TRACK 04 MODE1/2352
    INDEX 01 03:00:00
  TRACK 05 MODE2/2336
    INDEX 01 04:00:00
  TRACK 06 MODE2/2352
    INDEX 01 05:00:00
  TRACK 07 CDI/2336
    INDEX 01 06:00:00
  TRACK 08 CDI/2352
    INDEX 01 07:00:00
)";
    auto result = CueParser::parseString(cue_text);
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& tracks = result->files[0].tracks;
    ASSERT_EQ(tracks.size(), 8u);
    EXPECT_EQ(tracks[0].mode, TrackMode::Audio);
    EXPECT_EQ(tracks[1].mode, TrackMode::CDG);
    EXPECT_EQ(tracks[2].mode, TrackMode::Mode1_2048);
    EXPECT_EQ(tracks[3].mode, TrackMode::Mode1_2352);
    EXPECT_EQ(tracks[4].mode, TrackMode::Mode2_2336);
    EXPECT_EQ(tracks[5].mode, TrackMode::Mode2_2352);
    EXPECT_EQ(tracks[6].mode, TrackMode::CDI_2336);
    EXPECT_EQ(tracks[7].mode, TrackMode::CDI_2352);
}

TEST(CueParserTest, AllFileTypes) {
    const char* cue_text = R"(FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
FILE "b.bin" MOTOROLA
  TRACK 02 AUDIO
    INDEX 01 00:00:00
FILE "c.aiff" AIFF
  TRACK 03 AUDIO
    INDEX 01 00:00:00
FILE "d.wav" WAVE
  TRACK 04 AUDIO
    INDEX 01 00:00:00
FILE "e.mp3" MP3
  TRACK 05 AUDIO
    INDEX 01 00:00:00
)";
    auto result = CueParser::parseString(cue_text);
    ASSERT_TRUE(result.ok()) << result.error().message;

    ASSERT_EQ(result->files.size(), 5u);
    EXPECT_EQ(result->files[0].type, FileType::Binary);
    EXPECT_EQ(result->files[1].type, FileType::Motorola);
    EXPECT_EQ(result->files[2].type, FileType::Aiff);
    EXPECT_EQ(result->files[3].type, FileType::Wave);
    EXPECT_EQ(result->files[4].type, FileType::MP3);
}

TEST(CueParserTest, ErrorTrackBeforeFile) {
    const char* cue_text = R"(TRACK 01 AUDIO
    INDEX 01 00:00:00
)";
    auto result = CueParser::parseString(cue_text);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::UnexpectedDirective);
}

TEST(CueParserTest, ErrorDuplicateIndex) {
    const char* cue_text = R"(FILE "test.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
    INDEX 01 00:01:00
)";
    auto result = CueParser::parseString(cue_text);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::DuplicateIndex);
}

TEST(CueParserTest, ErrorInvalidTrackMode) {
    const char* cue_text = R"(FILE "test.bin" BINARY
  TRACK 01 INVALID_MODE
    INDEX 01 00:00:00
)";
    auto result = CueParser::parseString(cue_text);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidTrackMode);
}

TEST(CueParserTest, ErrorEmptyCueSheet) {
    auto result = CueParser::parseString("");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidCueFormat);
}

TEST(CueParserTest, ErrorFileNotFound) {
    auto result = CueParser::parseFile("/nonexistent/path.cue");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::FileNotFound);
}

TEST(CueParserTest, SectorSizes) {
    EXPECT_EQ(sectorSizeForMode(TrackMode::Audio), 2352u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::CDG), 2448u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::Mode1_2048), 2048u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::Mode1_2352), 2352u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::Mode2_2336), 2336u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::Mode2_2352), 2352u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::CDI_2336), 2336u);
    EXPECT_EQ(sectorSizeForMode(TrackMode::CDI_2352), 2352u);
}
