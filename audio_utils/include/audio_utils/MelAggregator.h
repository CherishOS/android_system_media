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

#include <audio_utils/MelProcessor.h>

#include <android-base/thread_annotations.h>
#include <map>
#include <mutex>
#include <unordered_map>

namespace android::audio_utils {

struct MelRecord {
    /** The port ID of the audio device where the MEL value was recorded. */
    audio_port_handle_t portId;
    /**
     * Array of continuously recorded MEL values >= RS1 (1 per second). First
     * value in the array was recorded at time: timestamp.
     */
    std::vector<int32_t> mels;
    /** Corresponds to the time when the first MEL entry in MelRecord was recorded. */
    int64_t timestamp;

    explicit MelRecord(audio_port_handle_t portId,
                      std::vector<int32_t> mels,
                      int64_t timestamp) :
        portId(portId), mels(std::move(mels)), timestamp(timestamp) {}
};

/**
 * Class used to aggregate MEL values from different streams that play sounds
 * simultaneously.
 *
 * The public methods are internally protected by a mutex to be thread-safe.
 */
class MelAggregator : public RefBase {
public:
    explicit MelAggregator(size_t storageDurationSeconds)
        : mStorageDurationSeconds(storageDurationSeconds) {};

    /**
     * \brief Creates or gets the callback assigned to the streamHandle
     *
     * \param deviceId          id for the devices where the stream is active.
     * \param streanHandle      handle to the stream
     */
    sp<MelProcessor::MelCallback> getOrCreateCallbackForDevice(
        audio_port_handle_t deviceId,
        audio_io_handle_t streamHandle);

    /**
     * \brief Removes stream callback when MEL computation is not needed anymore
     *
     * \param streanHandle      handle to the stream
     */
    void removeStreamCallback(audio_io_handle_t streamHandle);

    /**
     * \brief Returns the size of the stored MEL records.
     */
    size_t getMelRecordsSize() const;

    /**
     * \brief Iterate over the MelRecords and applies function f.
     *
     * \param f      function to apply on the iterated MelRecord's sorted by timestamp
     */
     void foreach(const std::function<void(const MelRecord&)>& f) const;

private:
    /**
     * An implementation of the MelProcessor::MelCallback that is assigned to a
     * specific device.
     */
    class Callback : public MelProcessor::MelCallback {
    public:
        explicit Callback(MelAggregator& melAggregator, audio_port_handle_t deviceHandle)
            : mMelAggregator(melAggregator), mDeviceHandle(deviceHandle) {}

        void onNewMelValues(const std::vector<int32_t>& mels,
                            size_t offset,
                            size_t length) const override;

        MelAggregator& mMelAggregator;
        audio_port_handle_t mDeviceHandle;
    };

    /**
     * New value are stored and MEL values that correspond to the same timestamp
     * will be aggregated.
     */
    void aggregateAndAddNewMelRecord_l(MelRecord value) REQUIRES(mLock);

    /** Insert new MelRecord sorted into mMelRecords. */
    void insertSorted_l(std::vector<int32_t>& mels,
                        int64_t timeBegin,
                        int64_t timeEnd,
                        int64_t timestamp,
                        audio_port_handle_t portId) REQUIRES(mLock);

    const size_t mStorageDurationSeconds;

    mutable std::mutex mLock;

    std::map<int64_t, MelRecord> mMelRecords GUARDED_BY(mLock);
    std::unordered_map<audio_io_handle_t, sp<Callback>> mActiveCallbacks GUARDED_BY(mLock);
};

}  // naemspace android::audio_utils
