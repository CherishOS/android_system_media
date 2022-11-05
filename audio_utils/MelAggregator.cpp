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
#define LOG_TAG "audio_utils_MelAggregator"

#include <audio_utils/MelAggregator.h>
#include <audio_utils/power.h>
#include <utils/Log.h>

namespace android::audio_utils {
namespace {

/**
 * Checking the intersection of the time intervals of v1 and v2. Each MelRecord v
 * spawns an interval [t1, t2) if and only if:
 *    v.timestamp == t1 && v.mels.size() == t2 - t1
 **/
std::pair<int64_t, int64_t> intersectRegion(const MelRecord& v1, const MelRecord& v2)
{
    const int64_t maxStart = std::max(v1.timestamp, v2.timestamp);
    const int64_t v1End = v1.timestamp + v1.mels.size();
    const int64_t v2End = v2.timestamp + v2.mels.size();
    const int64_t minEnd = std::min(v1End, v2End);
    return {maxStart, minEnd};
}

int64_t getCurrentSecond() {
    struct timespec now_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &now_ts) != 0) {
        ALOGE("%s: cannot get timestamp", __func__);
        return -1;
    }
    return now_ts.tv_sec;
}

int32_t aggregateMels(const int32_t mel1, const int32_t mel2) {
    return roundf(audio_utils_power_from_energy(powf(10, mel1 / 10.0f) + powf(10, mel2 / 10.0f)));
}

}  // namespace

sp<MelProcessor::MelCallback> MelAggregator::getOrCreateCallbackForDevice(
        audio_port_handle_t deviceId,
        audio_io_handle_t streamHandle)
{
    std::lock_guard _l(mLock);

    auto streamHandleCallback = mActiveCallbacks.find(streamHandle);
    if (streamHandleCallback != mActiveCallbacks.end()) {
        ALOGV("%s: found callback for stream %d", __func__, streamHandle);
        auto callback = streamHandleCallback->second;
        callback->mDeviceHandle = deviceId;
        return callback;
    } else {
        ALOGV("%s: creating new callback for device %d", __func__, streamHandle);
        sp<Callback> melCallback = sp<Callback>::make(*this, deviceId);
        mActiveCallbacks[streamHandle] = melCallback;
        return melCallback;
    }
}

void MelAggregator::Callback::onNewMelValues(const std::vector<int32_t>& mels,
                                             size_t offset,
                                             size_t length) const
{
    ALOGV("%s", __func__);
    std::lock_guard _l(mMelAggregator.mLock);

    int64_t timestampSec = getCurrentSecond();
    mMelAggregator.aggregateAndAddNewMelRecord_l(MelRecord(mDeviceHandle,
                                                         std::vector<int32_t>(
                                                             mels.begin() + offset,
                                                             mels.begin() + offset + length),
                                                         timestampSec - length));
}

void MelAggregator::insertSorted_l(std::vector<int32_t>& mels,
                                   int64_t timeBegin,
                                   int64_t timeEnd,
                                   int64_t timestampMels,
                                   audio_port_handle_t portId)
{
    mMelRecords.emplace_hint(mMelRecords.end(), timeBegin, MelRecord(portId,
                               std::vector<int32_t>(
                                   mels.begin() + timeBegin - timestampMels,
                                   mels.begin() + timeEnd - timestampMels),
                               timeBegin));
}

void MelAggregator::aggregateAndAddNewMelRecord_l(MelRecord mel)
{
    for (auto& storedMel: mMelRecords) {
        auto aggregateInterval = intersectRegion(storedMel.second, mel);
        if (aggregateInterval.first >= aggregateInterval.second) {
            // no overlap
            continue;
        }

        for (int64_t aggregateTime = aggregateInterval.first;
             aggregateTime < aggregateInterval.second; ++aggregateTime) {
            const int offsetStored = aggregateTime - storedMel.second.timestamp;
            const int offsetNew = aggregateTime - mel.timestamp;
            storedMel.second.mels[offsetStored] =
                aggregateMels(storedMel.second.mels[offsetStored], mel.mels[offsetNew]);

            // invalidate new value since it was aggregated
            mel.mels[offsetNew] = -1;
        }
    }

    int64_t timeBegin = mel.timestamp;
    int64_t timeEnd = mel.timestamp;
    int64_t lastTimestamp = mel.timestamp + mel.mels.size();
    mel.mels.push_back(-1);  // for last interval termination
    for (; timeEnd <= lastTimestamp; ++ timeEnd) {
        if (mel.mels[timeEnd - mel.timestamp] == -1) {
            if (timeEnd - timeBegin > 0) {
                // new interval to add
                insertSorted_l(mel.mels, timeBegin, timeEnd, mel.timestamp, mel.portId);
            }
            timeBegin = timeEnd + 1;
        }
    }

    // clean up older values
    // TODO: optimize MelRecord to use deque. This will allow to remove single MELs.
    while (mMelRecords.size() > 1 && lastTimestamp - mMelRecords.begin()->first
            > mStorageDurationSeconds) {
        mMelRecords.erase(mMelRecords.begin());
    }
}

void MelAggregator::removeStreamCallback(audio_io_handle_t streamHandle)
{
    std::lock_guard _l(mLock);
    mActiveCallbacks.erase(streamHandle);
}

size_t MelAggregator::getMelRecordsSize() const
{
    std::lock_guard _l(mLock);
    return mMelRecords.size();
}

void MelAggregator::foreach(const std::function<void(const MelRecord&)>& f) const
{
     std::lock_guard _l(mLock);
     for (const auto &melRecord : mMelRecords) {
         f(melRecord.second);
     }
}

}  // namespace android::audio_utils
