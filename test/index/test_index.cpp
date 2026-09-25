#include <gtest/gtest.h>
#include "index/history_index.h"
#include <algorithm>

TEST(HistoryIndexTest, EmptyIndex) {
    HistoryIndex idx;
    EXPECT_EQ(idx.size(), 0);
    EXPECT_FALSE(idx.contains("user1"));
    EXPECT_EQ(idx.latestBefore("user1", 1000), nullptr);
    EXPECT_TRUE(idx.list().empty());
}

TEST(HistoryIndexTest, AddVersionsAndLatest) {
    HistoryIndex idx;
    idx.addVersion("user1", {1000, 64});
    idx.addVersion("user1", {2000, 128});
    idx.addVersion("user1", {3000, 192});

    EXPECT_TRUE(idx.contains("user1"));
    EXPECT_EQ(idx.size(), 1);

    const RecordVersion& latest = idx.latest("user1");
    EXPECT_EQ(latest.timestamp, 3000);
    EXPECT_EQ(latest.offset, 192);

    const auto& hist = idx.getHistory("user1");
    ASSERT_EQ(hist.size(), 3);
    EXPECT_EQ(hist[0].timestamp, 1000);
    EXPECT_EQ(hist[1].timestamp, 2000);
    EXPECT_EQ(hist[2].timestamp, 3000);
}

TEST(HistoryIndexTest, LatestBeforeBinarySearch) {
    HistoryIndex idx;
    idx.addVersion("k", {100, 10});
    idx.addVersion("k", {200, 20});
    idx.addVersion("k", {300, 30});

    // Before any version
    EXPECT_EQ(idx.latestBefore("k", 50), nullptr);

    // Exactly at version 1
    const RecordVersion* v1 = idx.latestBefore("k", 100);
    ASSERT_NE(v1, nullptr);
    EXPECT_EQ(v1->timestamp, 100);
    EXPECT_EQ(v1->offset, 10);

    // Between version 1 and 2
    const RecordVersion* vMid = idx.latestBefore("k", 150);
    ASSERT_NE(vMid, nullptr);
    EXPECT_EQ(vMid->timestamp, 100);

    // Exactly at version 2
    const RecordVersion* v2 = idx.latestBefore("k", 200);
    ASSERT_NE(v2, nullptr);
    EXPECT_EQ(v2->timestamp, 200);
    EXPECT_EQ(v2->offset, 20);

    // After all versions
    const RecordVersion* vAfter = idx.latestBefore("k", 999);
    ASSERT_NE(vAfter, nullptr);
    EXPECT_EQ(vAfter->timestamp, 300);
    EXPECT_EQ(vAfter->offset, 30);

    // Non-existent key
    EXPECT_EQ(idx.latestBefore("nonexistent", 200), nullptr);
}

TEST(HistoryIndexTest, MultipleKeysAndList) {
    HistoryIndex idx;
    idx.addVersion("alpha", {100, 10});
    idx.addVersion("beta", {200, 20});

    EXPECT_EQ(idx.size(), 2);
    std::vector<std::string> keys = idx.list();
    EXPECT_EQ(keys.size(), 2);
    EXPECT_NE(std::find(keys.begin(), keys.end(), "alpha"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "beta"), keys.end());
}
