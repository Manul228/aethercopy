#include <gtest/gtest.h>
#include "aethercopy/filter.h"

using namespace aethercopy;

TEST(FormatFilterTest, IncludeMode) {
    FormatFilter filter;
    filter.includeOnly({"image/jpeg", "image/png"});

    EXPECT_TRUE(filter.shouldCopy("image/jpeg"));
    EXPECT_TRUE(filter.shouldCopy("image/png"));
    EXPECT_FALSE(filter.shouldCopy("application/pdf"));
}

TEST(FormatFilterTest, ExcludeMode) {
    FormatFilter filter;
    filter.exclude({"video/mp4", "video/avi"});

    EXPECT_FALSE(filter.shouldCopy("video/mp4"));
    EXPECT_FALSE(filter.shouldCopy("video/avi"));
    EXPECT_TRUE(filter.shouldCopy("image/jpeg"));
}

TEST(FormatFilterTest, EmptyFilter) {
    FormatFilter filter;
    filter.exclude({});  // ничего не исключаем

    EXPECT_TRUE(filter.shouldCopy("anything"));
}