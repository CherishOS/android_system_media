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

#include <android-base/thread_annotations.h>
#include <audio_utils/BiquadFilter.h>
#include <system/audio.h>
#include <utils/Errors.h>
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
    /** Should represent the minimal value after which a 1% CSD change can occur. */
    static constexpr int32_t kMaxMelValues = 3;

    /**
     * An interface through which the MelProcessor client will be notified about
     * important events.
     */
    class MelCallback : public virtual RefBase {
    public:
        ~MelCallback() override = default;
        /**
         * Called with a time-continuous vector of computed MEL values
         *
         * \param mels     contains MELs (one per second) with values above RS1.
         * \param offset   the offset in mels for new MEL data.
         * \param length   the number of valid MEL values in the vector starting at offset. The
         *                 maximum number of elements in mels is defined in the MelProcessor
         *                 constructor.
         * \param deviceId id of device where the samples were processed
         */
        virtual void onNewMelValues(const std::vector<float>& mels,
                                    size_t offset,
                                    size_t length,
                                    audio_port_handle_t deviceId) const = 0;

        /**
         * Called when the momentary exposure exceeds the RS2 value.
         *
         * Note: RS2 is configurable vie MelProcessor#setOutputRs2.
         */
        virtual void onMomentaryExposure(float currentMel, audio_port_handle_t deviceId) const = 0;
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
     * \param deviceId          the device ID for the MEL callbacks
     * \param rs2Value          initial RS2 value to use
     * \param maxMelsCallback   the number of max elements a callback can have.
     */
    MelProcessor(uint32_t sampleRate,
                 uint32_t channelCount,
                 audio_format_t format,
                 const MelCallback& callback,
                 audio_port_handle_t deviceId,
                 float rs2Value,
                 size_t maxMelsCallback = kMaxMelValues);

    /**
     * Sets the output RS2 value for momentary exposure warnings. Default value
     * is 100dBA as specified in IEC62368-1 3rd edition. Must not be higher than
     * 100dBA and not lower than 80dBA.
     *
     * \param rs2Value value to use for momentary exposure
     * \return NO_ERROR if rs2Value is between 80dBA amd 100dBA or BAD_VALUE
     *   otherwise
     */
    status_t setOutputRs2(float rs2Value);

    /** Returns the RS2 value used for momentary exposures. */
    float getOutputRs2() const;

    /** Updates the device id. */
    void setDeviceId(audio_port_handle_t deviceId);

    /** Returns the device id. */
    audio_port_handle_t getDeviceId();

    /**
     * \brief Computes the MEL values for the given buffer and triggers a
     * callback with time-continuous MEL values when: MEL buffer is full or if
     * there is a discontinue in MEL calculation (e.g.: MEL is under RS1)
     *
     * \param buffer           pointer to the audio data buffer.
     * \param bytes            buffer size in bytes.
     *
     * \return the number of bytes that were processed. Note: the method will
     *   output 0 for sample rates that are not supported.
     */
    int32_t process(const void* buffer, size_t bytes);

    /**
     * Sets the given attenuation for the MEL calculation. This can be used when
     * the audio framework is operating in absolute volume mode.
     *
     * @param attenuationDB    attenuation to use on computed MEL values
     */
    void setAttenuation(float attenuationDB);

private:
    bool isSampleRateSupported();
    void createBiquads();
    void applyAWeight_l(const void* buffer, size_t frames) REQUIRES(mLock);
    float getCombinedChannelEnergy_l() REQUIRES(mLock);
    void addMelValue_l(float mel, std::vector<std::function<void()>>& callbacks) REQUIRES(mLock);


    const uint32_t mSampleRate;                // audio data sample rate
    const uint32_t mChannelCount;              // audio data channel count
    const audio_format_t mFormat;              // audio data format
    const size_t mFramesPerMelValue;           // number of audio frames per MEL value
    const MelCallback& mCallback;              // callback to notify about new MEL values
                                               // and momentary exposure warning
                                               // does not own the callback, must outlive
    mutable std::mutex mLock;                  // monitor mutex

    // number of samples in the energy
    size_t mCurrentSamples GUARDED_BY(mLock);

    float mAttenuationDB GUARDED_BY(mLock) = 0.f;

    // local energy accumulation
    std::vector<float> mCurrentChannelEnergy GUARDED_BY(mLock);
    // contains the A-weighted input samples
    std::vector<float> mAWeightSamples GUARDED_BY(mLock);
    // contains the input samples converted to float
    std::vector<float> mFloatSamples GUARDED_BY(mLock);
    // accumulated MEL values
    std::vector<float> mMelValues GUARDED_BY(mLock);
    // current index to store the MEL values
    uint32_t mCurrentIndex GUARDED_BY(mLock);

    using DefaultBiquadFilter = BiquadFilter<float, true, details::DefaultBiquadConstOptions>;
    // Biquads used for the A-weighting
    std::array<std::unique_ptr<DefaultBiquadFilter>, kCascadeBiquadNumber>
        mCascadedBiquads GUARDED_BY(mLock);
    // device id used for the callbacks
    audio_port_handle_t mDeviceId GUARDED_BY(mLock);
    // Value used for momentary exposure
    float mRs2Value GUARDED_BY(mLock);
};

}  // namespace android::audio_utils
