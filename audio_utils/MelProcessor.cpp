/*
 * Copyright 2022 The Android Open Source Project
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
#define LOG_TAG "audio_utils_MelProcessor"

#include <audio_utils/MelProcessor.h>

#include <audio_utils/format.h>
#include <audio_utils/power.h>
#include <log/log.h>

namespace android::audio_utils {

constexpr int32_t kSecondsPerMelValue = 1;
constexpr int32_t kMelAdjustmentDb = -3;

// Estimated offset defined in Table39 of IEC62368-1 3rd edition
// -30dBFS, -10dBFS should correspond to 80dBSPL, 100dBSPL
constexpr int32_t kMeldBFSTodBSPLOffset = 110;

constexpr int32_t kRs1OutputdBFS = 80;

// The following arrays contain the IIR biquad filter coefficients for performing A-weighting as
// described in IEC 61672:2003 for samples with 44.1kHz and 48kHz.
constexpr std::array<std::array<float, kBiquadNumCoefs>, 2> kBiquadCoefs1 =
    {{/* 44.1kHz= */{0.95616638497, -1.31960414122, 0.36343775625, -1.31861375911, 0.32059452332},
      /* 48kHz= */{0.96525096525, -1.34730163086, 0.38205066561, -1.34730722798, 0.34905752979}}};
constexpr std::array<std::array<float, kBiquadNumCoefs>, 2> kBiquadCoefs2 =
    {{/* 44.1kHz= */{0.94317138580, -1.88634277160, 0.94317138580, -1.88558607420, 0.88709946900},
      /* 48kHz= */{0.94696969696, -1.89393939393, 0.94696969696, -1.89387049481, 0.89515976917}}};
constexpr std::array<std::array<float, kBiquadNumCoefs>, 2> kBiquadCoefs3 =
    {{/* 44.1kHz= */{0.69736775447, -0.42552769920, -0.27184005527, -1.31859445445, 0.32058831623},
      /* 48kHz= */{0.64666542810, -0.38362237137, -0.26304305672, -1.34730722798, 0.34905752979}}};

MelProcessor::MelProcessor(uint32_t sampleRate,
        uint32_t channelCount,
        audio_format_t format,
        sp<MelCallback> callback,
        size_t maxMelsCallback)
    : mSampleRate(sampleRate),
      mChannelCount(channelCount),
      mFormat(format),
      mFramesPerMelValue(sampleRate * kSecondsPerMelValue),
      mCallback(std::move(callback)),
      mCurrentSamples(0),
      mCurrentChannelEnergy(channelCount, 0.0f),
      mAWeightSamples(mFramesPerMelValue * mChannelCount),
      mFloatSamples(mFramesPerMelValue * mChannelCount),
      mMelValues(maxMelsCallback),
      mCurrentIndex(0)
{
    createBiquads();
}

bool MelProcessor::isSampleRateSupported() {
    // For now only support 44.1 and 48kHz for Mel calculation
    if (mSampleRate != 44100 && mSampleRate != 48000) {
        return false;
    }

    return true;
}

void MelProcessor::createBiquads() {
    if (!isSampleRateSupported()) {
        return;
    }

    int coefsIndex = mSampleRate == 44100 ? 0 : 1;
    mCascadedBiquads =
              {std::make_unique<DefaultBiquadFilter>(mChannelCount, kBiquadCoefs1.at(coefsIndex)),
               std::make_unique<DefaultBiquadFilter>(mChannelCount, kBiquadCoefs2.at(coefsIndex)),
               std::make_unique<DefaultBiquadFilter>(mChannelCount, kBiquadCoefs3.at(coefsIndex))};
}

void MelProcessor::applyAWeight(const void* buffer, size_t samples)
{
    memcpy_by_audio_format(mFloatSamples.data(), AUDIO_FORMAT_PCM_FLOAT, buffer, mFormat, samples);

    float* tempFloat[2] = { mFloatSamples.data(), mAWeightSamples.data() };
    int inIdx = 1, outIdx = 0;
    const size_t frames = samples / mChannelCount;
    for (const auto& biquad : mCascadedBiquads) {
        outIdx ^= 1;
        inIdx ^= 1;
        biquad->process(tempFloat[outIdx], tempFloat[inIdx], frames);
    }

    // should not be the case since the size is odd
    if (!(mCascadedBiquads.size() & 1)) {
        std::swap(mFloatSamples, mAWeightSamples);
    }
}

float MelProcessor::getCombinedChannelEnergy() {
    float combinedEnergy = 0.0f;
    for (auto& energy: mCurrentChannelEnergy) {
        combinedEnergy += energy;
        energy = 0;
    }

    combinedEnergy /= (float) mFramesPerMelValue;
    return combinedEnergy;
}

void MelProcessor::addMelValue(float mel) {
    mMelValues[mCurrentIndex] = static_cast<int32_t>(roundf(mel));
    ALOGV("writing MEL %d at index %d", mMelValues[mCurrentIndex], mCurrentIndex);

    bool reportContinuousValues = false;
    if ((mMelValues[mCurrentIndex] < kRs1OutputdBFS && mCurrentIndex > 0)) {
        reportContinuousValues = true;
    } else if (mMelValues[mCurrentIndex] >= kRs1OutputdBFS) {
        // only store MEL that are above RS1
        ++mCurrentIndex;
    }

    if (reportContinuousValues || (mCurrentIndex > mMelValues.size() - 1)) {
        mCallback->onNewMelValues(mMelValues, /* offset= */0, mCurrentIndex);
        mCurrentIndex = 0;
    }
}

int32_t MelProcessor::process(const void* buffer, size_t bytes) {
    if (!isSampleRateSupported()) {
        return 0;
    }

    std::lock_guard<std::mutex> guard(mLock);

    const size_t bytes_per_sample = audio_bytes_per_sample(mFormat);
    size_t samples = bytes_per_sample > 0 ? bytes / bytes_per_sample : 0;
    while (samples > 0) {
        const size_t requiredSamples = mFramesPerMelValue * mChannelCount - mCurrentSamples;
        size_t processSamples = std::min(requiredSamples, samples);
        processSamples -= processSamples % mChannelCount;

        applyAWeight(buffer, processSamples);

        audio_utils_accumulate_energy(mAWeightSamples.data(),
                                      AUDIO_FORMAT_PCM_FLOAT,
                                      processSamples,
                                      mChannelCount,
                                      mCurrentChannelEnergy.data());
        mCurrentSamples += processSamples;

        ALOGV("required:%zu, process:%zu, mCurrentChannelEnergy[0]:%f, mCurrentSamples:%zu",
                  requiredSamples, processSamples, mCurrentChannelEnergy[0], mCurrentSamples);
        if (processSamples < requiredSamples) {
            return bytes;
        }

        addMelValue(fmaxf(audio_utils_power_from_energy(getCombinedChannelEnergy())
                              + kMelAdjustmentDb
                              + kMeldBFSTodBSPLOffset, 0.0f));

        samples -= processSamples;
        buffer = (const uint8_t *)buffer + mCurrentSamples * bytes_per_sample;
        mCurrentSamples = 0;
    }

    return bytes;
}

}   // namespace android
