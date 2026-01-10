#include <gtest/gtest.h>
#include "libcuebin/disc.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>

using namespace cuebin;

static const std::filesystem::path DATA_DIR = TEST_DATA_DIR;

namespace {

// Helper to create a dummy BIN file of the given size
void createBinFile(const std::filesystem::path& path, size_t size, uint8_t fill = 0xAA) {
    std::ofstream f(path, std::ios::binary);
    std::vector<uint8_t> data(size, fill);
    // Write sector markers: first byte of each 2352-byte sector = sector index
    for (size_t i = 0; i < size / 2352; ++i) {
        data[i * 2352] = static_cast<uint8_t>(i & 0xFF);
    }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

class DiscTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create bin files that match our test CUE sheets
        // singleTrack.cue: "singleTrack.bin" MODE2/2352
        createBinFile(DATA_DIR / "singleTrack.bin", 2352 * 100);

        // multiTrack.cue: "multiTrack.bin" - all tracks in one file
        // Track 1: INDEX 01 00:00:00 (MODE2/2352)
        // Track 2: INDEX 00 23:24:25, INDEX 01 23:26:25 (AUDIO)
        // Track 3: PREGAP 00:02:00, INDEX 01 27:44:50 (AUDIO)
        // Total sectors needed: at least MSF(27,44,50).toLba() + some extra
        int32_t totalFrames = MSF(28, 0, 0).toLba();
        createBinFile(DATA_DIR / "multiTrack.bin", static_cast<size_t>(totalFrames) * 2352);

        // multiFile.cue: separate files
        createBinFile(DATA_DIR / "data.bin", 2352 * 300);
        createBinFile(DATA_DIR / "audio02.bin", 2352 * 200);
        createBinFile(DATA_DIR / "audio03.bin", 2352 * 200);

        // metadata.cue: "metadata.bin"
        int32_t metaFrames = MSF(40, 0, 0).toLba();
        createBinFile(DATA_DIR / "metadata.bin", static_cast<size_t>(metaFrames) * 2352);
    }

    void TearDown() override {
        // Clean up generated bin files
        std::filesystem::remove(DATA_DIR / "singleTrack.bin");
        std::filesystem::remove(DATA_DIR / "multiTrack.bin");
        std::filesystem::remove(DATA_DIR / "data.bin");
        std::filesystem::remove(DATA_DIR / "audio02.bin");
        std::filesystem::remove(DATA_DIR / "audio03.bin");
        std::filesystem::remove(DATA_DIR / "metadata.bin");
    }
};

} // anonymous namespace

TEST_F(DiscTest, LoadSingleTrack) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.trackCount(), 1u);
    EXPECT_EQ(disc.firstTrackNumber(), 1);
    EXPECT_EQ(disc.lastTrackNumber(), 1);
    EXPECT_EQ(disc.totalSectors(), 100);

    const auto* t = disc.track(1);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->number(), 1);
    EXPECT_EQ(t->mode(), TrackMode::Mode2_2352);
    EXPECT_EQ(t->sectorSize(), 2352u);
    EXPECT_TRUE(t->isData());
    EXPECT_FALSE(t->isAudio());
    EXPECT_EQ(t->startLba(), 0);
    EXPECT_EQ(t->lengthSectors(), 100);
}

TEST_F(DiscTest, LoadMultiTrack) {
    auto result = Disc::fromCue(DATA_DIR / "multiTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.trackCount(), 3u);
    EXPECT_EQ(disc.firstTrackNumber(), 1);
    EXPECT_EQ(disc.lastTrackNumber(), 3);

    // Track 1: data
    const auto* t1 = disc.track(1);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->isData());
    EXPECT_EQ(t1->startLba(), 0);

    // Track 2: audio
    const auto* t2 = disc.track(2);
    ASSERT_NE(t2, nullptr);
    EXPECT_TRUE(t2->isAudio());

    // Track 3: audio with pregap
    const auto* t3 = disc.track(3);
    ASSERT_NE(t3, nullptr);
    EXPECT_TRUE(t3->isAudio());
    EXPECT_EQ(t3->pregapSectors(), 150); // 00:02:00 = 150 frames
}

TEST_F(DiscTest, LoadMultiFile) {
    auto result = Disc::fromCue(DATA_DIR / "multiFile.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.trackCount(), 3u);

    // Each track should be from a different file
    const auto* t1 = disc.track(1);
    const auto* t2 = disc.track(2);
    const auto* t3 = disc.track(3);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    ASSERT_NE(t3, nullptr);

    EXPECT_EQ(t1->fileIndex(), 0u);
    EXPECT_EQ(t2->fileIndex(), 1u);
    EXPECT_EQ(t3->fileIndex(), 2u);
}

TEST_F(DiscTest, Metadata) {
    auto result = Disc::fromCue(DATA_DIR / "metadata.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.title(), "Final Fantasy VII");
    EXPECT_EQ(disc.performer(), "Square");
    EXPECT_EQ(disc.catalog(), "0000000000000");

    const auto* t1 = disc.track(1);
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1->title(), "Data Track");
    EXPECT_EQ(t1->performer(), "Square");
    EXPECT_EQ(t1->isrc(), "JPSMK0100001");

    const auto* t2 = disc.track(2);
    ASSERT_NE(t2, nullptr);
    EXPECT_EQ(t2->title(), "Opening - Bombing Mission");
    EXPECT_EQ(t2->performer(), "Nobuo Uematsu");
}

TEST_F(DiscTest, ReadSector) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    auto& disc = *result;

    // Read first sector
    auto sector = disc.readSector(0);
    ASSERT_TRUE(sector.ok()) << sector.error().message;
    EXPECT_EQ(sector->mode, TrackMode::Mode2_2352);
    // First byte should be sector marker (0 for sector 0)
    EXPECT_EQ(sector->data[0], 0);

    // Read second sector
    sector = disc.readSector(1);
    ASSERT_TRUE(sector.ok()) << sector.error().message;
    EXPECT_EQ(sector->data[0], 1);
}

TEST_F(DiscTest, ReadSectorMSF) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    auto& disc = *result;

    auto sector = disc.readSector(MSF(0, 0, 0));
    ASSERT_TRUE(sector.ok()) << sector.error().message;
    EXPECT_EQ(sector->data[0], 0);
}

TEST_F(DiscTest, ReadSectors) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    auto& disc = *result;

    auto sectors = disc.readSectors(0, 3);
    ASSERT_TRUE(sectors.ok()) << sectors.error().message;
    ASSERT_EQ(sectors->size(), 3u);
    EXPECT_EQ((*sectors)[0].data[0], 0);
    EXPECT_EQ((*sectors)[1].data[0], 1);
    EXPECT_EQ((*sectors)[2].data[0], 2);
}

TEST_F(DiscTest, ReadSectorOutOfRange) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    auto& disc = *result;

    auto sector = disc.readSector(-1);
    EXPECT_FALSE(sector.ok());
    EXPECT_EQ(sector.error().code, ErrorCode::LBAOutOfRange);

    sector = disc.readSector(100);
    EXPECT_FALSE(sector.ok());
    EXPECT_EQ(sector.error().code, ErrorCode::LBAOutOfRange);
}

TEST_F(DiscTest, FindTrack) {
    auto result = Disc::fromCue(DATA_DIR / "multiTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;

    // LBA 0 should be in track 1
    const auto* t = disc.findTrack(0);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->number(), 1);

    // LBA beyond all tracks should return nullptr
    t = disc.findTrack(disc.totalSectors());
    EXPECT_EQ(t, nullptr);
}

TEST_F(DiscTest, TrackSpan) {
    auto result = Disc::fromCue(DATA_DIR / "multiTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;

    auto span = disc.tracks();
    EXPECT_EQ(span.size(), 3u);
    EXPECT_EQ(span[0].number(), 1);
    EXPECT_EQ(span[1].number(), 2);
    EXPECT_EQ(span[2].number(), 3);
}

TEST_F(DiscTest, LeadOutLBA) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.leadOutLba(), disc.totalSectors());
}

TEST_F(DiscTest, NonexistentTrack) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    EXPECT_EQ(disc.track(99), nullptr);
}

TEST_F(DiscTest, CueSheetAccess) {
    auto result = Disc::fromCue(DATA_DIR / "metadata.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    const auto& disc = *result;
    const auto& sheet = disc.cueSheet();
    EXPECT_EQ(sheet.title, "Final Fantasy VII");
    EXPECT_FALSE(sheet.files.empty());
}

TEST_F(DiscTest, ErrorFileNotFound) {
    auto result = Disc::fromCue("/nonexistent/path.cue");
    EXPECT_FALSE(result.ok());
}

TEST_F(DiscTest, MoveConstruction) {
    auto result = Disc::fromCue(DATA_DIR / "singleTrack.cue");
    ASSERT_TRUE(result.ok()) << result.error().message;

    Disc disc1 = std::move(*result);
    EXPECT_EQ(disc1.trackCount(), 1u);

    Disc disc2 = std::move(disc1);
    EXPECT_EQ(disc2.trackCount(), 1u);
}
