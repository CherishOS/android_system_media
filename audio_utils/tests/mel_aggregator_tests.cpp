/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0
#define LOG_TAG "audio_utils_mel_aggregator_tests"

#include <audio_utils/MelAggregator.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace android::audio_utils {
namespace {

using ::testing::ElementsAre;

TEST(MelAggregatorTest, GetCallbackForExistingStream) {
    MelAggregator aggregator{1};
    sp<MelProcessor::MelCallback> callback1 =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/1, /*streamHandle=*/1);
    sp<MelProcessor::MelCallback> callback2 =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/2, /*streamHandle=*/1);

    EXPECT_EQ(callback1, callback2);
}

TEST(MelAggregatorTest, AggregateValuesFromDifferentStreams) {
    MelAggregator aggregator{2};
    sp<MelProcessor::MelCallback> callback1 =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/1, /*streamHandle=*/1);
    sp<MelProcessor::MelCallback> callback2 =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/1, /*streamHandle=*/2);

    callback1->onNewMelValues({10, 10}, 0, 2);
    callback2->onNewMelValues({10, 10}, 0, 2);

    ASSERT_EQ(aggregator.getMelRecordsSize(), size_t{1});
    aggregator.foreach([](const MelRecord &value) {
        EXPECT_EQ(value.portId, 1);
        EXPECT_THAT(value.mels, ElementsAre(13, 13));
    });
}

TEST(MelAggregatorTest, RemoveOlderValuesAndAggregate) {
    MelAggregator aggregator{2};
    sp<MelProcessor::MelCallback> callback =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/1, /*streamHandle=*/1);

    callback->onNewMelValues({1, 1}, 0, 2);
    // second mel array contains values that are too old and will be removed
    callback->onNewMelValues({2, 2, 2}, 0, 3);

    ASSERT_EQ(aggregator.getMelRecordsSize(), size_t{1});
    aggregator.foreach([](const MelRecord &value) {
        EXPECT_EQ(value.portId, 1);
        EXPECT_THAT(value.mels, ElementsAre(5, 5));
    });
}

TEST(MelAggregatorTest, CheckMelIntervalSplit) {
    MelAggregator aggregator{4};
    sp<MelProcessor::MelCallback> callback =
        aggregator.getOrCreateCallbackForDevice(/*deviceId=*/1, /*streamHandle=*/1);

    callback->onNewMelValues({3, 3}, 0, 2);
    sleep(1);
    callback->onNewMelValues({3, 3, 3, 3}, 0, 4);

    ASSERT_EQ(aggregator.getMelRecordsSize(), size_t{3});

    int index = 0;
    aggregator.foreach([&index](const MelRecord &value) {
        EXPECT_EQ(value.portId, 1);
        if (index == 1) {
            EXPECT_THAT(value.mels, ElementsAre(6, 6));
        } else {
            EXPECT_THAT(value.mels, ElementsAre(3));
        }
        ++ index;
    });
}

}  // namespace
}  // namespace android
