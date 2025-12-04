#include <gtest/gtest.h>
#include "libcuebin/msf.hpp"

using namespace cuebin;

TEST(MSFTest, DefaultConstruction) {
    MSF msf;
    EXPECT_EQ(msf.minute, 0);
    EXPECT_EQ(msf.second, 0);
    EXPECT_EQ(msf.frame, 0);
}

TEST(MSFTest, ValueConstruction) {
    MSF msf(2, 30, 50);
    EXPECT_EQ(msf.minute, 2);
    EXPECT_EQ(msf.second, 30);
    EXPECT_EQ(msf.frame, 50);
}

TEST(MSFTest, ToLBA) {
    // 00:00:00 -> 0
    EXPECT_EQ(MSF(0, 0, 0).toLba(), 0);
    // 00:00:01 -> 1
    EXPECT_EQ(MSF(0, 0, 1).toLba(), 1);
    // 00:01:00 -> 75
    EXPECT_EQ(MSF(0, 1, 0).toLba(), 75);
    // 01:00:00 -> 4500
    EXPECT_EQ(MSF(1, 0, 0).toLba(), 4500);
    // 00:02:00 -> 150 (standard pregap)
    EXPECT_EQ(MSF(0, 2, 0).toLba(), 150);
    // 72:00:00 -> 324000
    EXPECT_EQ(MSF(72, 0, 0).toLba(), 324000);
    // Composite: 01:02:03 -> 4500 + 150 + 3 = 4653
    EXPECT_EQ(MSF(1, 2, 3).toLba(), 4653);
}

TEST(MSFTest, FromLBA) {
    auto msf = MSF::fromLba(0);
    EXPECT_EQ(msf, MSF(0, 0, 0));

    msf = MSF::fromLba(1);
    EXPECT_EQ(msf, MSF(0, 0, 1));

    msf = MSF::fromLba(75);
    EXPECT_EQ(msf, MSF(0, 1, 0));

    msf = MSF::fromLba(4500);
    EXPECT_EQ(msf, MSF(1, 0, 0));

    msf = MSF::fromLba(4653);
    EXPECT_EQ(msf, MSF(1, 2, 3));
}

TEST(MSFTest, RoundTrip) {
    for (int32_t lba = 0; lba < 1000; ++lba) {
        EXPECT_EQ(MSF::fromLba(lba).toLba(), lba);
    }
}

TEST(MSFTest, ParseValid) {
    auto result = MSF::parse("00:00:00");
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, MSF(0, 0, 0));

    result = MSF::parse("01:02:03");
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, MSF(1, 2, 3));

    result = MSF::parse("72:59:74");
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, MSF(72, 59, 74));
}

TEST(MSFTest, ParseInvalid) {
    // Missing colons
    EXPECT_FALSE(MSF::parse("000000").ok());
    // Too short
    EXPECT_FALSE(MSF::parse("0:0").ok());
    // Seconds out of range
    EXPECT_FALSE(MSF::parse("00:60:00").ok());
    // Frames out of range
    EXPECT_FALSE(MSF::parse("00:00:75").ok());
    // Non-numeric
    EXPECT_FALSE(MSF::parse("aa:bb:cc").ok());
}

TEST(MSFTest, ToString) {
    EXPECT_EQ(MSF(0, 0, 0).toString(), "00:00:00");
    EXPECT_EQ(MSF(1, 2, 3).toString(), "01:02:03");
    EXPECT_EQ(MSF(72, 59, 74).toString(), "72:59:74");
}

TEST(MSFTest, ToPhysicalMSF) {
    // LBA 0 + 150 = 150 = 00:02:00
    auto msf = MSF::toPhysicalMsf(0);
    EXPECT_EQ(msf, MSF(0, 2, 0));

    // LBA 150 + 150 = 300 = 00:04:00
    msf = MSF::toPhysicalMsf(150);
    EXPECT_EQ(msf, MSF(0, 4, 0));
}

TEST(MSFTest, ComparisonOperators) {
    EXPECT_TRUE(MSF(0, 0, 0) < MSF(0, 0, 1));
    EXPECT_TRUE(MSF(0, 0, 0) <= MSF(0, 0, 0));
    EXPECT_TRUE(MSF(0, 0, 1) > MSF(0, 0, 0));
    EXPECT_TRUE(MSF(0, 0, 0) >= MSF(0, 0, 0));
    EXPECT_TRUE(MSF(1, 0, 0) > MSF(0, 59, 74));
}

TEST(MSFTest, Constexpr) {
    constexpr MSF msf(1, 2, 3);
    constexpr int32_t lba = msf.toLba();
    static_assert(lba == 4653);

    constexpr MSF roundtrip = MSF::fromLba(4653);
    static_assert(roundtrip == MSF(1, 2, 3));

    constexpr MSF physical = MSF::toPhysicalMsf(0);
    static_assert(physical == MSF(0, 2, 0));
}
