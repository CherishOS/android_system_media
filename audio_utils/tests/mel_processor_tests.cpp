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
#define LOG_TAG "audio_utils_mel_processor_tests"

#include <audio_utils/MelProcessor.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <tuple>
#include <unordered_map>
#include <log/log.h>

namespace android::audio_utils {
namespace {

using ::testing::_;
using ::testing::Le;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

// Contains the sample rate and frequency for sine wave
using AudioParam = std::tuple<int32_t, int32_t>;


const std::unordered_map<int32_t, int32_t> kAWeightDelta1000 =
    {{80, 23}, {100, 19}, {500, 3}, {1000, 0}, {2000, 1}, {3000, 1},
     {8000, 1}};


class MelCallbackMock : public MelProcessor::MelCallback {
 public:
  MOCK_METHOD(void, onNewMelValues, (const std::vector<int32_t>&, size_t, size_t),
              (const override));
};

void appendSineWaveBuffer(std::vector<float>& buffer,
                          float frequency,
                          size_t samples,
                          int32_t sampleRate,
                          float attenuation = 1.0f) {
    float rad = 2.0f * (float) M_PI * frequency / (float) sampleRate;
    for (size_t j = 0; j < samples; ++j) {
        buffer.push_back(sinf(j * rad) * attenuation);
    }
}

class MelProcessorFixtureTest : public TestWithParam<AudioParam> {
protected:
    MelProcessorFixtureTest()
        : mSampleRate(std::get<0>(GetParam())),
          mFrequency(std::get<1>(GetParam())),
          mMelCallback(sp<MelCallbackMock>::make()),
          mProcessor(mSampleRate, 1, AUDIO_FORMAT_PCM_FLOAT, mMelCallback, mMaxMelsCallback) {}


    int32_t mSampleRate;
    int32_t mFrequency;
    int32_t mMaxMelsCallback = 2;

    sp<MelCallbackMock> mMelCallback;
    MelProcessor mProcessor;

    std::vector<float> mBuffer;
};


TEST(MelProcessorTest, UnsupportedSamplerateCheck) {
    EXPECT_DEATH({
        MelProcessor(1000, 1, AUDIO_FORMAT_PCM_FLOAT, nullptr, 1);
    }, "Unsupported sample rate: 1000");
}

TEST_P(MelProcessorFixtureTest, CheckNumberOfCallbacks) {
    if (mFrequency != 1000.0f) {
        ALOGV("NOTE: CheckNumberOfCallbacks disabeld for frequency %d", mFrequency);
        return;
    }

    appendSineWaveBuffer(mBuffer, 1000.0f, mSampleRate, mSampleRate * mMaxMelsCallback);
    appendSineWaveBuffer(mBuffer, 1000.0f, mSampleRate, mSampleRate * mMaxMelsCallback, 0.01f);

    EXPECT_CALL(*mMelCallback.get(), onNewMelValues(_, _, Le(size_t{2}))).Times(1);

    mProcessor.process(&mBuffer[0], mBuffer.size() * sizeof(float));
}

TEST_P(MelProcessorFixtureTest, CheckAWeightingFrequency) {
    appendSineWaveBuffer(mBuffer, mFrequency, mSampleRate, mSampleRate);
    appendSineWaveBuffer(mBuffer, 1000.0f, mSampleRate, mSampleRate);

    EXPECT_CALL(*mMelCallback.get(), onNewMelValues(_, _, _))
        .Times(1)
        .WillRepeatedly([&] (const std::vector<int32_t>& mel, size_t offset, size_t length) {
            EXPECT_EQ(offset, size_t{0});
            EXPECT_EQ(length, size_t{2});
            int32_t deltaValue = abs(mel[0] - mel[1]);
            ALOGV("MEL[%d] = %d,  MEL[1000] = %d\n", mFrequency, mel[0], mel[1]);
            EXPECT_TRUE(abs(deltaValue - kAWeightDelta1000.at(mFrequency)) <= 1);
        });

    mProcessor.process(&mBuffer[0], mBuffer.size() * sizeof(float));
}

INSTANTIATE_TEST_SUITE_P(MelProcessorTestSuite,
    MelProcessorFixtureTest,
    Combine(Values(44100, 48000), Values(80, 100, 500, 1000, 2000, 3000, 8000))
);

}  // namespace
}  // namespace android
