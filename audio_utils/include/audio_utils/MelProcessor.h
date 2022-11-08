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

#pragma once

#include <array>
#include <mutex>

#include <audio_utils/BiquadFilter.h>
#include <system/audio.h>
#include <utils/RefBase.h>

namespace android::audio_utils {

/**
 * Class used to capture the MEL (momentary exposure levels) values as defined
 * by IEC62368-1 3rd edition. MELs are computed for each second.
 *
 * The public methods are internally protected by a mutex to be thread-safe.
 */
class MelProcessor : public RefBase {
public:

    static constexpr int kCascadeBiquadNumber = 3;
    static constexpr int32_t kMaxMelValues = 30;

    /**
     * An interface through which the MelProcessor client will be notified about
     * important events.
     */
    class MelCallback : public RefBase {
    public:
        ~MelCallback() override = default;
        /**
         * Called with a time-continuous vector of computed MEL values
         *
         * \param mels   contains MELs (one per second) with values above RS1.
         * \param offset the offset in mels for new MEL data.
         * \param length the number of valid MEL values in the vector starting at offset. The
         *               maximum number of elements in mels is defined in the MelProcessor
         *               constructor.
         */
        virtual void onNewMelValues(const std::vector<int32_t>& mels,
                                    size_t offset,
                                    size_t length) const = 0;
    };

    /**
     * \brief Creates a MelProcessor object.
     *
     * \param sampleRate        sample rate of the audio data.
     * \param channelCount      channel count of the audio data.
     * \param format            format of the audio data. It must be allowed by
     *                          audio_utils_is_compute_mel_format_supported()
     *                          else the constructor will abort.
     * \param callback          reports back the new mel values.
     * \param maxMelsCallback   the number of max elements a callback can have.
     */
    MelProcessor(uint32_t sampleRate,
                 uint32_t channelCount,
                 audio_format_t format,
                 sp<MelCallback> callback,
                 size_t maxMelsCallback = kMaxMelValues);

    /**
     * \brief Computes the MEL values for the given buffer and triggers a
     * callback with time-continuous MEL values when: MEL buffer is full or if
     * there is a discontinue in MEL calculation (e.g.: MEL is under RS1)
     *
     * \param buffer            pointer to the audio data buffer.
     * \param bytes            buffer size in bytes.
     *
     * \return the number of bytes that were processed. Note: the method will
     *   output 0 for sample rates that are not supported.
     */
    int32_t process(const void* buffer, size_t bytes);

private:
    bool isSampleRateSupported();
    void createBiquads();
    void applyAWeight(const void* buffer, size_t frames);
    float getCombinedChannelEnergy();
    void addMelValue(float mel);


    const uint32_t mSampleRate;                // audio data sample rate
    const uint32_t mChannelCount;              // audio data channel count
    const audio_format_t mFormat;              // audio data format
    const size_t mFramesPerMelValue;           // number of audio frames per MEL value
    const sp<MelCallback> mCallback;           // callback to notify about new MEL values
                                               // and momentary exposure warning
    mutable std::mutex mLock;                  // monitor mutex
    size_t mCurrentSamples;                    // number of samples in the energy

    std::vector<float> mCurrentChannelEnergy;  // local energy accumulation
    std::vector<float> mAWeightSamples;        // contains the A-weighted input samples
    std::vector<float> mFloatSamples;          // contains the input samples converted to float
    std::vector<int32_t> mMelValues;           // accumulated MEL values
    uint32_t mCurrentIndex;                    // current index to store the MEL values

    using DefaultBiquadFilter = BiquadFilter<float, true, details::DefaultBiquadConstOptions>;
    std::array<std::unique_ptr<DefaultBiquadFilter>, kCascadeBiquadNumber>
        mCascadedBiquads;                      // Biquads used for the A-weighting
};

}  // namespace android::audio_utils
