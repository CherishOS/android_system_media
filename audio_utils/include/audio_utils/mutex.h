/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <android-base/thread_annotations.h>
#include <audio_utils/threads.h>
#include <utils/Log.h>
#include <utils/Timers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <sys/syscall.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#pragma push_macro("LOG_TAG")
#undef LOG_TAG
#define LOG_TAG "audio_utils::mutex"

namespace android::audio_utils {

// Define global capabilities for thread-safety annotation.
//
// These can be manually modified, or
// compile generate_mutex_order.cpp in the tests directory
// to generate this.

// --- Begin generated section

// Lock order
enum class MutexOrder : uint32_t {
    kEffectHandle_Mutex = 0,
    kEffectBase_PolicyMutex = 1,
    kAudioFlinger_Mutex = 2,
    kAudioFlinger_HardwareMutex = 3,
    kDeviceEffectManager_Mutex = 4,
    kPatchCommandThread_Mutex = 5,
    kThreadBase_Mutex = 6,
    kAudioFlinger_ClientMutex = 7,
    kMelReporter_Mutex = 8,
    kEffectChain_Mutex = 9,
    kDeviceEffectProxy_ProxyMutex = 10,
    kEffectBase_Mutex = 11,
    kAudioFlinger_UnregisteredWritersMutex = 12,
    kAsyncCallbackThread_Mutex = 13,
    kConfigEvent_Mutex = 14,
    kOutputTrack_TrackMetadataMutex = 15,
    kPassthruPatchRecord_ReadMutex = 16,
    kPatchCommandThread_ListenerMutex = 17,
    kPlaybackThread_AudioTrackCbMutex = 18,
    kMediaLogNotifier_Mutex = 19,
    kOtherMutex = 20,
    kSize = 21,
};

// Lock by name
inline constexpr const char* const gMutexNames[] = {
    "EffectHandle_Mutex",
    "EffectBase_PolicyMutex",
    "AudioFlinger_Mutex",
    "AudioFlinger_HardwareMutex",
    "DeviceEffectManager_Mutex",
    "PatchCommandThread_Mutex",
    "ThreadBase_Mutex",
    "AudioFlinger_ClientMutex",
    "MelReporter_Mutex",
    "EffectChain_Mutex",
    "DeviceEffectProxy_ProxyMutex",
    "EffectBase_Mutex",
    "AudioFlinger_UnregisteredWritersMutex",
    "AsyncCallbackThread_Mutex",
    "ConfigEvent_Mutex",
    "OutputTrack_TrackMetadataMutex",
    "PassthruPatchRecord_ReadMutex",
    "PatchCommandThread_ListenerMutex",
    "PlaybackThread_AudioTrackCbMutex",
    "MediaLogNotifier_Mutex",
    "OtherMutex",
};

// Forward declarations
class AudioMutexAttributes;
template <typename T> class mutex_impl;
using mutex = mutex_impl<AudioMutexAttributes>;

// Capabilities in priority order
// (declaration only, value is nullptr)
inline mutex* EffectHandle_Mutex;
inline mutex* EffectBase_PolicyMutex
        ACQUIRED_AFTER(android::audio_utils::EffectHandle_Mutex);
inline mutex* AudioFlinger_Mutex
        ACQUIRED_AFTER(android::audio_utils::EffectBase_PolicyMutex);
inline mutex* AudioFlinger_HardwareMutex
        ACQUIRED_AFTER(android::audio_utils::AudioFlinger_Mutex);
inline mutex* DeviceEffectManager_Mutex
        ACQUIRED_AFTER(android::audio_utils::AudioFlinger_HardwareMutex);
inline mutex* PatchCommandThread_Mutex
        ACQUIRED_AFTER(android::audio_utils::DeviceEffectManager_Mutex);
inline mutex* ThreadBase_Mutex
        ACQUIRED_AFTER(android::audio_utils::PatchCommandThread_Mutex);
inline mutex* AudioFlinger_ClientMutex
        ACQUIRED_AFTER(android::audio_utils::ThreadBase_Mutex);
inline mutex* MelReporter_Mutex
        ACQUIRED_AFTER(android::audio_utils::AudioFlinger_ClientMutex);
inline mutex* EffectChain_Mutex
        ACQUIRED_AFTER(android::audio_utils::MelReporter_Mutex);
inline mutex* DeviceEffectProxy_ProxyMutex
        ACQUIRED_AFTER(android::audio_utils::EffectChain_Mutex);
inline mutex* EffectBase_Mutex
        ACQUIRED_AFTER(android::audio_utils::DeviceEffectProxy_ProxyMutex);
inline mutex* AudioFlinger_UnregisteredWritersMutex
        ACQUIRED_AFTER(android::audio_utils::EffectBase_Mutex);
inline mutex* AsyncCallbackThread_Mutex
        ACQUIRED_AFTER(android::audio_utils::AudioFlinger_UnregisteredWritersMutex);
inline mutex* ConfigEvent_Mutex
        ACQUIRED_AFTER(android::audio_utils::AsyncCallbackThread_Mutex);
inline mutex* OutputTrack_TrackMetadataMutex
        ACQUIRED_AFTER(android::audio_utils::ConfigEvent_Mutex);
inline mutex* PassthruPatchRecord_ReadMutex
        ACQUIRED_AFTER(android::audio_utils::OutputTrack_TrackMetadataMutex);
inline mutex* PatchCommandThread_ListenerMutex
        ACQUIRED_AFTER(android::audio_utils::PassthruPatchRecord_ReadMutex);
inline mutex* PlaybackThread_AudioTrackCbMutex
        ACQUIRED_AFTER(android::audio_utils::PatchCommandThread_ListenerMutex);
inline mutex* MediaLogNotifier_Mutex
        ACQUIRED_AFTER(android::audio_utils::PlaybackThread_AudioTrackCbMutex);
inline mutex* OtherMutex
        ACQUIRED_AFTER(android::audio_utils::MediaLogNotifier_Mutex);

// Exclusion by capability
#define EXCLUDES_BELOW_OtherMutex
#define EXCLUDES_OtherMutex \
    EXCLUDES(android::audio_utils::OtherMutex) \
    EXCLUDES_BELOW_OtherMutex

#define EXCLUDES_BELOW_MediaLogNotifier_Mutex \
    EXCLUDES_OtherMutex
#define EXCLUDES_MediaLogNotifier_Mutex \
    EXCLUDES(android::audio_utils::MediaLogNotifier_Mutex) \
    EXCLUDES_BELOW_MediaLogNotifier_Mutex

#define EXCLUDES_BELOW_PlaybackThread_AudioTrackCbMutex \
    EXCLUDES_MediaLogNotifier_Mutex
#define EXCLUDES_PlaybackThread_AudioTrackCbMutex \
    EXCLUDES(android::audio_utils::PlaybackThread_AudioTrackCbMutex) \
    EXCLUDES_BELOW_PlaybackThread_AudioTrackCbMutex

#define EXCLUDES_BELOW_PatchCommandThread_ListenerMutex \
    EXCLUDES_PlaybackThread_AudioTrackCbMutex
#define EXCLUDES_PatchCommandThread_ListenerMutex \
    EXCLUDES(android::audio_utils::PatchCommandThread_ListenerMutex) \
    EXCLUDES_BELOW_PatchCommandThread_ListenerMutex

#define EXCLUDES_BELOW_PassthruPatchRecord_ReadMutex \
    EXCLUDES_PatchCommandThread_ListenerMutex
#define EXCLUDES_PassthruPatchRecord_ReadMutex \
    EXCLUDES(android::audio_utils::PassthruPatchRecord_ReadMutex) \
    EXCLUDES_BELOW_PassthruPatchRecord_ReadMutex

#define EXCLUDES_BELOW_OutputTrack_TrackMetadataMutex \
    EXCLUDES_PassthruPatchRecord_ReadMutex
#define EXCLUDES_OutputTrack_TrackMetadataMutex \
    EXCLUDES(android::audio_utils::OutputTrack_TrackMetadataMutex) \
    EXCLUDES_BELOW_OutputTrack_TrackMetadataMutex

#define EXCLUDES_BELOW_ConfigEvent_Mutex \
    EXCLUDES_OutputTrack_TrackMetadataMutex
#define EXCLUDES_ConfigEvent_Mutex \
    EXCLUDES(android::audio_utils::ConfigEvent_Mutex) \
    EXCLUDES_BELOW_ConfigEvent_Mutex

#define EXCLUDES_BELOW_AsyncCallbackThread_Mutex \
    EXCLUDES_ConfigEvent_Mutex
#define EXCLUDES_AsyncCallbackThread_Mutex \
    EXCLUDES(android::audio_utils::AsyncCallbackThread_Mutex) \
    EXCLUDES_BELOW_AsyncCallbackThread_Mutex

#define EXCLUDES_BELOW_AudioFlinger_UnregisteredWritersMutex \
    EXCLUDES_AsyncCallbackThread_Mutex
#define EXCLUDES_AudioFlinger_UnregisteredWritersMutex \
    EXCLUDES(android::audio_utils::AudioFlinger_UnregisteredWritersMutex) \
    EXCLUDES_BELOW_AudioFlinger_UnregisteredWritersMutex

#define EXCLUDES_BELOW_EffectBase_Mutex \
    EXCLUDES_AudioFlinger_UnregisteredWritersMutex
#define EXCLUDES_EffectBase_Mutex \
    EXCLUDES(android::audio_utils::EffectBase_Mutex) \
    EXCLUDES_BELOW_EffectBase_Mutex

#define EXCLUDES_BELOW_DeviceEffectProxy_ProxyMutex \
    EXCLUDES_EffectBase_Mutex
#define EXCLUDES_DeviceEffectProxy_ProxyMutex \
    EXCLUDES(android::audio_utils::DeviceEffectProxy_ProxyMutex) \
    EXCLUDES_BELOW_DeviceEffectProxy_ProxyMutex

#define EXCLUDES_BELOW_EffectChain_Mutex \
    EXCLUDES_DeviceEffectProxy_ProxyMutex
#define EXCLUDES_EffectChain_Mutex \
    EXCLUDES(android::audio_utils::EffectChain_Mutex) \
    EXCLUDES_BELOW_EffectChain_Mutex

#define EXCLUDES_BELOW_MelReporter_Mutex \
    EXCLUDES_EffectChain_Mutex
#define EXCLUDES_MelReporter_Mutex \
    EXCLUDES(android::audio_utils::MelReporter_Mutex) \
    EXCLUDES_BELOW_MelReporter_Mutex

#define EXCLUDES_BELOW_AudioFlinger_ClientMutex \
    EXCLUDES_MelReporter_Mutex
#define EXCLUDES_AudioFlinger_ClientMutex \
    EXCLUDES(android::audio_utils::AudioFlinger_ClientMutex) \
    EXCLUDES_BELOW_AudioFlinger_ClientMutex

#define EXCLUDES_BELOW_ThreadBase_Mutex \
    EXCLUDES_AudioFlinger_ClientMutex
#define EXCLUDES_ThreadBase_Mutex \
    EXCLUDES(android::audio_utils::ThreadBase_Mutex) \
    EXCLUDES_BELOW_ThreadBase_Mutex

#define EXCLUDES_BELOW_PatchCommandThread_Mutex \
    EXCLUDES_ThreadBase_Mutex
#define EXCLUDES_PatchCommandThread_Mutex \
    EXCLUDES(android::audio_utils::PatchCommandThread_Mutex) \
    EXCLUDES_BELOW_PatchCommandThread_Mutex

#define EXCLUDES_BELOW_DeviceEffectManager_Mutex \
    EXCLUDES_PatchCommandThread_Mutex
#define EXCLUDES_DeviceEffectManager_Mutex \
    EXCLUDES(android::audio_utils::DeviceEffectManager_Mutex) \
    EXCLUDES_BELOW_DeviceEffectManager_Mutex

#define EXCLUDES_BELOW_AudioFlinger_HardwareMutex \
    EXCLUDES_DeviceEffectManager_Mutex
#define EXCLUDES_AudioFlinger_HardwareMutex \
    EXCLUDES(android::audio_utils::AudioFlinger_HardwareMutex) \
    EXCLUDES_BELOW_AudioFlinger_HardwareMutex

#define EXCLUDES_BELOW_AudioFlinger_Mutex \
    EXCLUDES_AudioFlinger_HardwareMutex
#define EXCLUDES_AudioFlinger_Mutex \
    EXCLUDES(android::audio_utils::AudioFlinger_Mutex) \
    EXCLUDES_BELOW_AudioFlinger_Mutex

#define EXCLUDES_BELOW_EffectBase_PolicyMutex \
    EXCLUDES_AudioFlinger_Mutex
#define EXCLUDES_EffectBase_PolicyMutex \
    EXCLUDES(android::audio_utils::EffectBase_PolicyMutex) \
    EXCLUDES_BELOW_EffectBase_PolicyMutex

#define EXCLUDES_BELOW_EffectHandle_Mutex \
    EXCLUDES_EffectBase_PolicyMutex
#define EXCLUDES_EffectHandle_Mutex \
    EXCLUDES(android::audio_utils::EffectHandle_Mutex) \
    EXCLUDES_BELOW_EffectHandle_Mutex

#define EXCLUDES_AUDIO_ALL \
    EXCLUDES_EffectHandle_Mutex

// --- End generated section

/**
 * AudioMutexAttributes is a collection of types and constexpr configuration
 * used for the Android audio mutex.
 *
 * A different AudioMutexAttributes configuration will instantiate a completely
 * independent set of mutex strategies, statics and thread locals,
 * for a different type of mutexes.
 */

class AudioMutexAttributes {
public:
    // Order types, name arrays.
    using order_t = MutexOrder;
    static constexpr auto& order_names_ = gMutexNames;
    static constexpr size_t order_size_ = static_cast<size_t>(MutexOrder::kSize);
    static constexpr order_t order_default_ = MutexOrder::kOtherMutex;

    // verify order information
    static_assert(std::size(order_names_) == order_size_);
    static_assert(static_cast<size_t>(order_default_) < order_size_);

    // Set mutex_tracking_enabled_ to true to enable mutex
    // statistics and debugging (order checking) features.
    static constexpr bool mutex_tracking_enabled_ = true;

    // Control the depth of the mutex stack per thread (the mutexes
    // we track).  Set this to the maximum expected
    // number of mutexes held by a thread.  If the depth is too small,
    // deadlock detection, order checking, and recursion checking
    // may result in a false negative.  This is a static configuration
    // because reallocating memory for the stack requires a lock for
    // the reader.
    static constexpr size_t mutex_stack_depth_ = 16;

    // Enable or disable log always fatal.
    // This also requires the mutex feature flag to be set.
    static constexpr bool abort_on_order_check_ = true;
    static constexpr bool abort_on_recursion_check_ = true;
    static constexpr bool abort_on_invalid_unlock_ = true;
};

/**
 * Helper method to accumulate floating point values to an atomic
 * prior to C++23 support of atomic<float> atomic<double> accumulation.
 */
template <typename AccumulateType, typename ValueType>
void atomic_add_to(std::atomic<AccumulateType> &dst, ValueType src) {
    static_assert(std::atomic<AccumulateType>::is_always_lock_free);
    AccumulateType expected;
    do {
        expected = dst;
    } while (!dst.compare_exchange_weak(expected, expected + src));
}

/**
 * mutex_stat is a struct composed of atomic members associated
 * with usage of a particular mutex order.
 *
 * Access of a snapshot of this does not have a global lock, so the reader
 * may experience temporal shear. Use of this by a different reader thread
 * is for informative purposes only.
 */

// CounterType == uint64_t, AccumulatorType == double
template <typename CounterType, typename AccumulatorType>
struct mutex_stat {
    static_assert(std::is_floating_point_v<AccumulatorType>);
    static_assert(std::is_integral_v<CounterType>);
    std::atomic<CounterType> locks = 0;        // number of times locked
    std::atomic<CounterType> unlocks = 0;      // number of times unlocked
    std::atomic<CounterType> waits = 0;         // number of locks that waitedwa
    std::atomic<AccumulatorType> wait_sum_ns = 0.;    // sum of time waited.
    std::atomic<AccumulatorType> wait_sumsq_ns = 0.;  // sumsq of time waited.

    template <typename WaitTimeType>
    void add_wait_time(WaitTimeType wait_ns) {
        AccumulatorType value_ns = wait_ns;
        atomic_add_to(wait_sum_ns, value_ns);
        atomic_add_to(wait_sumsq_ns, value_ns * value_ns);
    }

    std::string to_string() const {
        CounterType uncontested = locks - waits;
        AccumulatorType recip = waits == 0 ? 0. : 1. / waits;
        AccumulatorType avg_wait_ms = waits == 0 ? 0. : wait_sum_ns * 1e-6 * recip;
        AccumulatorType std_wait_ms = waits < 2 ? 0. :
                std::sqrt(wait_sumsq_ns * recip * 1e-12 - avg_wait_ms * avg_wait_ms);
        return std::string("locks: ").append(std::to_string(locks))
            .append("\nuncontested: ").append(std::to_string(uncontested))
            .append("\nwaits: ").append(std::to_string(waits))
            .append("\nunlocks: ").append(std::to_string(unlocks))
            .append("\navg_wait_ms: ").append(std::to_string(avg_wait_ms))
            .append("\nstd_wait_ms: ").append(std::to_string(std_wait_ms))
            .append("\n");
    }
};

/**
 * atomic_stack is a single writer, multiple reader object.
 * Readers not on the same thread as the writer may experience temporal shear,
 * but individual members are accessed atomic-safe, i.e. no partial member
 * reads or delayed writes due to caching.
 *
 * For use with mutex checking, the atomic_stack maintains an ordering on
 * P (payload) such that the top item pushed must always be greater than or
 * equal to the P (payload) of items below it.
 *
 * Pushes always go to the top of the stack.  Removes can occur
 * from any place in the stack, but typically near the top.
 *
 * The atomic_stack never reallocates beyond its fixed capacity of N.
 * This prevents a lockless reader from accessing invalid memory because
 * the address region does not change.
 *
 * If the number of pushes exceed the capacity N, then items may be discarded.
 * In that case, the stack is a subset stack of the "true" unlimited
 * capacity stack.  Nevertheless, a subset of an ordered stack
 * with items deleted is also ordered.
 *
 * The size() of the atomic_stack is the size of the subset stack of tracked items.
 * The true_size() is the size of the number of items pushed minus the
 * number of items removed (the "true" size if capacity were unlimited).
 * Since the capacity() is constant the true_size() may include
 * items we don't track except by count.  If true_size() == size() then
 * the subset stack is complete.
 *
 * In this single writer, multiple reader model, we could get away with
 * memory_order_relaxed as the reader is purely informative,
 * but we choose memory_order_seq_cst which imposes the most
 * restrictions on the compiler (variable access reordering) and the
 * processor (memory access reordering).  This means operations take effect
 * in the order written.  However, this isn't strictly needed - as there is
 * only one writer, a read-modify-write operation is safe (no need for special
 * memory instructions), and there isn't the acquire-release semantics with
 * non-atomic memory access needed for a lockless fifo, for example.
 */

 /*
  * For audio mutex purposes, one question arises - why don't we use
  * a bitmask to represent the capabilities taken by a thread
  * instead of a stack?
  *
  * A bitmask arrangement works if there exists a one-to-one relationship
  * from a physical mutex to its capability.  That may exist for some
  * projects, but not AudioFlinger.
  *
  * As a consequence, we need the actual count and handle:
  *
  * 1) A single thread may hold multiple instances of some capabilities
  * (e.g. ThreadBase_Mutex and EffectChain_Mutex).
  * For example there may be multiple effect chains locked during mixing.
  * There may be multiple PlaybackThreads locked during effect chain movement.
  * A bit per capability can't count beyond 1.
  *
  * 2) Deadlock detection requires tracking the actual MutexHandle (a void*)
  * to form a cycle, because there may be many mutexes associated with a
  * given capability order.
  * For example, each PlaybackThread or RecordThread will have its own mutex
  * with the ThreadBase_Mutex capability.
  *
  */

template <typename Item, typename Payload, size_t N>
class atomic_stack {
public:
    using item_payload_pair_t = std::pair<std::atomic<Item>, std::atomic<Payload>>;

    /**
     * Puts the item at the top of the stack.
     *
     * If the stack depth is exceeded the item
     * replaces the top.
     *
     * Mutexes when locked are always placed on the top of the stack;
     * however, they may be unlocked in a non last-in-first-out (LIFO)
     * order.  It is rare to see a non LIFO order, but it can happen.
     */
    void push(const Item& item, const Payload& payload) {
        size_t location = top_;
        size_t increment = 1;
        if (location >= N) {
            // we exceed the top of stack.
            //
            // although we could ignore this item (subset is the oldest),
            // the better solution is to replace the topmost entry as
            // it allows quicker removal.
            location = N - 1;
            increment = 0;
        }
        // issue the operations close together.
        pairs_[location].first = item;
        pairs_[location].second = payload;
        ++true_top_;
        top_ += increment;
    }

    /**
     * Removes the item which is expected at the top of the stack
     * but may be lower.  Mutexes are generally unlocked in stack
     * order (LIFO), but this is not a strict requirement.
     */
    bool remove(const Item& item) {
        if (true_top_ == 0) {
            return false;  // cannot remove.
        }
        // there is a temporary benign read race here where true_top_ != top_.
        --true_top_;
        for (size_t i = top_; i > 0; ) {
            if (item == pairs_[--i].first) {
                // We shift to preserve order.
                // A reader may temporarily see a "duplicate" entry
                // but that is preferable to a "missing" entry
                // for the purposes of deadlock detection.
                const size_t limit = top_ - 1;
                while (i < limit) {  // using atomics, we need to assign first, second separately.
                    pairs_[i].first = pairs_[i + 1].first.load();
                    pairs_[i].second = pairs_[i + 1].second.load();
                    ++i;
                }
                --top_; // now we restrict our range.
                // on relaxed semantics, it might be better to clear out the last
                // pair, but we are seq_cst.
                return true;
            }
        }
        // not found in our subset.
        //
        // we return true upon correct removal (true_top_ must always be >= top_).
        if (true_top_ >= top_) return true;

        // else recover and return false to notify that removal was invalid.
        true_top_ = top_.load();
        return false;
    }

    /**
     * return the top of our atomic subset stack
     * or the invalid_ (zero-initialized) entry if it doesn't exist.
     */
    // Consideration of using std::optional<> is a possibility
    // but as std::atomic doesn't have a copy ctor (and does not make sense),
    // we would want to directly return an optional on the non-atomic values,
    // in a custom pair.
    const item_payload_pair_t& top(size_t offset = 0) const {
        const ssize_t top = static_cast<ssize_t>(top_) - static_cast<ssize_t>(offset);
        if (top > 0 && top <= static_cast<ssize_t>(N)) return pairs_[top - 1];
        return invalid_;  // we don't know anything.
    }

    /**
     * return the bottom (or base) of our atomic subset stack
     * or the invalid_ (zero-initialized) entry if it doesn't exist.
     */
    const item_payload_pair_t& bottom(size_t offset = 0) const {
        if (offset < top_) return pairs_[offset];
        return invalid_;  // we don't know anything.
    }

    /**
     * prints the contents of the stack starting from the most recent first.
     *
     * If the thread is not the same as the writer thread, there could be
     * temporal shear in the data printed.
     */
    std::string to_string() const {
        std::string s("size: ");
        s.append(std::to_string(size()))
            .append(" true_size: ").append(std::to_string(true_size()))
            .append(" items: [");
        for (size_t i = 0; i < top_; ++i) {
            s.append("{ ")
            .append(std::to_string(reinterpret_cast<uintptr_t>(pairs_[i].first.load())))
            .append(", ")
            .append(std::to_string(static_cast<size_t>(pairs_[i].second.load())))
            .append(" } ");
        }
        s.append("]");
        return s;
    }

   /*
    * stack configuration
    */
    static size_t capacity() { return N; }
    size_t true_size() const { return true_top_; }
    size_t size() const { return top_; }
    const auto& invalid() const { return invalid_; }

private:
    std::atomic<size_t> top_ = 0;       // ranges from 0 to N - 1
    std::atomic<size_t> true_top_ = 0;  // always >= top_.
    // if true_top_ == top_ the subset stack is complete.

    /*
     * The subset stack entries are a pair of atomics rather than an atomic<pair>
     * to prevent lock requirements if T and P are small enough, i.e. <= sizeof(size_t).
     *
     * As atomics are not composable from smaller atomics, there may be some
     * temporary inconsistencies when reading from a different thread than the writer.
     */
    item_payload_pair_t pairs_[N]{};

    /*
     * The invalid pair is returned when top() is called without a tracked item.
     * This might occur with an empty subset of the "true" stack.
     */
    static constexpr item_payload_pair_t invalid_{};
};

/**
 * thread_mutex_info is a struct that is associated with every
 * thread the first time a mutex is used on it.  Writing will be through
 * a single thread (essentially thread_local), but the thread_registry
 * debug methods may access this through a different reader thread.
 *
 * If the thread does not use the audio_utils mutex, the allocation of this
 * struct never occurs, although there is approx 16 bytes for a shared ptr and
 * 1 byte for a thread local once bool.
 *
 * Here, we use for the MutexHandle a void*, which is used as an opaque unique ID
 * representing the mutex.
 *
 * Since there is no global locking, the validity of the mutex* associated to
 * the void* is unknown -- the mutex* could be deallocated in a different
 * thread.  Nevertheless the opaque ID can still be used to check deadlocks
 * realizing there could be a false positive on a potential reader race
 * where a new mutex is created at the same storage location.
 */
template <typename MutexHandle, typename Order, size_t N>
class thread_mutex_info {
public:
    using atomic_stack_t = atomic_stack<MutexHandle, Order, N>;

    thread_mutex_info(pid_t tid) : tid_(tid) {}

    // the destructor releases the thread_mutex_info.
    // declared here, defined below due to use of thread_registry.
    ~thread_mutex_info();

    void reset_waiter(MutexHandle waiter = nullptr) {
        mutex_wait_ = waiter;
    }

    /**
     * check_held returns the stack pair that conflicts
     * with the existing mutex handle and order, or the invalid
     * stack pair (empty mutex handle and empty order).
     */
    const typename atomic_stack_t::item_payload_pair_t&
    check_held(MutexHandle mutex, Order order) const {
        // validate mutex order.
        const size_t size = mutexes_held_.size();
        for (size_t i = 0; i < size; ++i) {
            const auto& top = mutexes_held_.top(i);
            const auto top_order = top.second.load();

            if (top_order < order) break;              // ok
            if (top_order > order) return top;         // inverted order
            if (top.first.load() == mutex) return top; // recursive mutex
        }
        return mutexes_held_.invalid();
    }

    /*
     * This is unverified push.  Use check_held() prior to this to
     * verify no lock inversion or replication.
     */
    void push_held(MutexHandle mutex, Order order) {
        mutexes_held_.push(mutex, order);
    }

    bool remove_held(MutexHandle mutex) {
        return mutexes_held_.remove(mutex);
    }

    /*
     * Due to the fact that the thread_mutex_info contents are not globally locked,
     * there may be temporal shear.  The string representation is
     * informative only.
     */
    std::string to_string() const {
        std::string s;
        s.append("tid: ").append(std::to_string(static_cast<int>(tid_)));
        s.append("\nwaiting: ").append(std::to_string(
                reinterpret_cast<uintptr_t>(mutex_wait_.load())));
        s.append("\nheld: ").append(mutexes_held_.to_string());
        return s;
    }

    /*
     * empty() indicates that the thread is not waiting for or
     * holding any mutexes.
     */
    bool empty() const {
        return mutex_wait_ == nullptr && mutexes_held_.size() == 0;
    }

    const auto& stack() const {
        return mutexes_held_;
    }

    const pid_t tid_;                                   // me
    std::atomic<MutexHandle> mutex_wait_{};             // mutex waiting for
    atomic_stack_t mutexes_held_;  // mutexes held
};


/**
 * deadlock_info_t encapsulates the mutex wait / cycle information from
 * thread_registry::deadlock_detection().
 *
 * If a cycle is detected, the last element of the vector chain represents
 * a tid that is repeated somewhere earlier in the vector.
 */
struct deadlock_info_t {
public:
    explicit deadlock_info_t(pid_t tid_param) : tid(tid_param) {}

    bool empty() const {
        return chain.empty();
    }

    std::string to_string() const {
        std::string description;

        if (has_cycle) {
            description.append("mutex cycle found (last tid repeated) ");
        } else {
            description.append("mutex wait chain ");
        }
        description.append("[ ").append(std::to_string(tid));
        // Note: when we dump here, we add the timeout tid to the start of the wait chain.
        for (const auto& [ tid2, name ] : chain) {
            description.append(", ").append(std::to_string(tid2))
                    .append(" (holding ").append(name).append(")");
        }
        description.append(" ]");
        return description;
    }

    const pid_t tid;         // tid for which the deadlock was checked
    bool has_cycle = false;  // true if there is a cycle detected
    std::vector<std::pair<pid_t, std::string>> chain;  // wait chain of tids and mutexes.
};

/**
 * The thread_registry is a thread-safe locked structure that
 * maintains a list of the threads that contain thread_mutex_info.
 *
 * Only first mutex access from a new thread and the destruction of that
 * thread will trigger an access to the thread_registry map.
 *
 * The debug methods to_string() and deadlock_detection() will also lock the struct
 * long enough to copy the map and safely obtain the weak pointers,
 * and then deal with the thread local data afterwards.
 *
 * It is recommended to keep a static singleton of the thread_registry for the
 * type desired.  The singleton should be associated properly with the object
 * it should be unique for, which in this case is the mutex_impl template.
 * This enables access to the elements as needed.
 */
template <typename ThreadInfo>
class thread_registry {
public:
    bool add_to_registry(const std::shared_ptr<ThreadInfo>& tminfo) EXCLUDES(mutex_) {
        ALOGV("%s: registered for %d", __func__, tminfo->tid_);
        std::lock_guard l(mutex_);
        if (registry_.count(tminfo->tid_) > 0) {
            ALOGW_IF("%s: tid %d already exists", __func__, tminfo->tid_);
            return false;
        }
        registry_[tminfo->tid_] = tminfo;
        return true;
    }

    bool remove_from_registry(pid_t tid) EXCLUDES(mutex_) {
        ALOGV("%s: unregistered for %d", __func__, tid);
        std::lock_guard l(mutex_);
        // don't crash here because it might be a test app.
        const bool success = registry_.erase(tid) == 1;
        ALOGW_IF(!success, "%s: Cannot find entry for tid:%d", __func__, tid);
        return success;
    }

    // Returns a std::unordered_map for easy access on tid.
    auto copy_map() EXCLUDES(mutex_) {
        std::lock_guard l(mutex_);
        return registry_;
    }

    // Returns a std::map sorted on tid for easy debug reading.
    auto copy_ordered_map() EXCLUDES(mutex_) {
        std::lock_guard l(mutex_);
        std::map<pid_t, std::weak_ptr<ThreadInfo>> sorted(registry_.begin(), registry_.end());
        return sorted;
    }

    /**
     * Returns a string containing the thread mutex info for each
     * thread that has accessed the audio_utils mutex.
     */
    std::string to_string() {
        // for debug purposes it is much easier to see the tids in numeric order.
        const auto registry_map = copy_ordered_map();
        ALOGV("%s: dumping tids: %zu", __func__, registry_map.size());
        std::string s("thread count: ");
        s.append(std::to_string(registry_map.size())).append("\n");

        std::vector<pid_t> empty;
        for (const auto& [tid, weak_info] : registry_map) {
            const auto info = weak_info.lock();
            if (info) {
                if (info->empty()) {
                    empty.push_back(tid);
                } else {
                    s.append(info->to_string()).append("\n");
                }
            }
        }

        // dump remaining empty tids out
        s.append("tids without current activity [ ");
        for (const auto tid : empty) {
            s.append(std::to_string(tid)).append(" ");
        }
        s.append("]\n");
        return s;
    }

    /**
     * Returns the tid mutex pointer (a void*) if it is waiting.
     *
     * It should use a copy of the registry map which is not changing
     * as it does not take any lock.
     */
    static void* tid_to_mutex_wait(
            const std::unordered_map<pid_t, std::weak_ptr<ThreadInfo>>& registry_map,
            pid_t tid) {
        const auto it = registry_map.find(tid);
        if (it == registry_map.end()) return {};
        const auto& weak_info = it->second;  // unmapped returns empty weak_ptr.
        const auto info = weak_info.lock();
        if (!info) return {};
        return info->mutex_wait_.load();
    }

    /**
     * Returns a deadlock_info_t struct describing the mutex wait / cycle information.
     *
     * The deadlock_detection() method is not exceptionally fast
     * and is not designed to be called for every mutex locked (and contended).
     * It is designed to run as a diagnostic routine to enhance
     * dumping for watchdogs, like TimeCheck, when a tid is believed blocked.
     *
     * Access of state is through atomics, so has minimal overhead on
     * concurrent execution, with the possibility of (mostly) false
     * negatives due to race.
     *
     * \param tid target tid which may be in a cycle or blocked.
     * \param mutex_names a string array of mutex names indexed on capability order.
     * \return a deadlock_info_t struct, which contains whether a cycle was found and
     *         a vector of tids and mutex names in the mutex wait chain.
     */
    template <typename StringArray>
    deadlock_info_t deadlock_detection(pid_t tid, const StringArray& mutex_names) {
        const auto registry_map = copy_map();
        deadlock_info_t deadlock_info{tid};

        // if tid not waiting, return.
        void* m = tid_to_mutex_wait(registry_map, tid);
        if (m == nullptr) return deadlock_info;

        bool subset = false; // do we have missing mutex data per thread?

        // Create helper map from mutex to tid.
        //
        // The helper map is built up from thread_local info rather than from
        // a global mutex list.
        //
        // There are multiple reasons behind this.
        // 1) There are many mutexes (mostly not held). We don't want to keep and
        //    manage a "global" list of them.
        // 2) The mutex pointer itself may be deallocated from a different thread
        //    from the reader. To keep it alive requires either a mutex, or a
        //    weak_ptr to shared_ptr promotion.
        //    Lifetime management is expensive on a per-mutex basis as there are many
        //    of them, but cheaper on a per-thread basis as the threads are fewer.
        // 3) The thread_local lookup is very inexpensive for thread info (special
        //    acceleration by C++ and the OS), but more complex for a mutex list
        //    which at best is a static concurrent hash map.
        //
        // Note that the mutex_ptr handle is opaque -- it may be deallocated from
        // a different thread, so we use the tid from the thread registry map.
        //
        using pid_order_index_pair_t = std::pair<pid_t, size_t>;
        std::unordered_map<void*, pid_order_index_pair_t> mutex_to_tid;
        for (const auto& [tid2, weak_info] : registry_map) {
            const auto info = weak_info.lock();
            if (info == nullptr) continue;
            const auto& stack = info->mutexes_held_;
            subset = subset || stack.size() != stack.true_size();
            for (size_t i = 0; i < stack.size(); ++i) {
                const auto& mutex_order_pair = stack.bottom(i);
                // if this method is not called by the writer thread
                // it is possible for data to change.
                const auto mutex_ptr = mutex_order_pair.first.load();
                const auto order = static_cast<size_t>(mutex_order_pair.second.load());
                if (mutex_ptr != nullptr) {
                    mutex_to_tid[mutex_ptr] = pid_order_index_pair_t{tid2, order};
                }
            }
        }
        ALOGD_IF(subset, "%s: mutex info only subset, deadlock detection may be inaccurate",
                __func__);

        // traverse from tid -> waiting mutex, then
        // mutex -> tid holding
        // until we get no more tids, or a tid cycle.
        std::unordered_set<pid_t> visited;
        visited.insert(tid);  // mark the original tid, we start there for cycle detection.
        while (true) {
            // no tid associated with the mutex.
            if (mutex_to_tid.count(m) == 0) return deadlock_info;
            const auto [tid2, order] = mutex_to_tid[m];

            // add to chain.
            const auto name = order < std::size(mutex_names) ? mutex_names[order] : "unknown";
            deadlock_info.chain.emplace_back(tid2, name);

            // cycle detected
            if (visited.count(tid2)) {
                deadlock_info.has_cycle = true;
                return deadlock_info;
            }
            visited.insert(tid2);

            // if tid not waiting return (could be blocked on binder).
            m = tid_to_mutex_wait(registry_map, tid2);
            if (m == nullptr) return deadlock_info;
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<pid_t, std::weak_ptr<ThreadInfo>> registry_ GUARDED_BY(mutex_);
};

// audio_utils::mutex, audio_utils::lock_guard, audio_utils::unique_lock,
// and audio_utils::condition_variable are method compatible versions
// of std::mutex, std::lock_guard, std::unique_lock, and std::condition_variable
// for optimizing audio thread performance and debugging.
//
// Note: we do not use std::timed_mutex as its Clang library implementation
// is inefficient.  One is better off making a custom timed implementation using
// pthread_mutex_timedlock() on the mutex::native_handle().

extern bool mutex_get_enable_flag();

template <typename Attributes>
class CAPABILITY("mutex") mutex_impl {
public:
    using attributes_t = Attributes;

    // We use composition here.
    // No copy/move ctors as the member std::mutex has it deleted.
    mutex_impl(typename Attributes::order_t order = Attributes::order_default_)
        : order_(order)
        , stat_{get_mutex_stat_array()[static_cast<size_t>(order)]}
    {
        LOG_ALWAYS_FATAL_IF(static_cast<size_t>(order) >= Attributes::order_size_,
                "mutex order %u is equal to or greater than order limit:%zu",
                order, Attributes::order_size_);

        if (!mutex_get_enable_flag()) return;

        pthread_mutexattr_t attr;
        int ret = pthread_mutexattr_init(&attr);
        if (ret != 0) {
            ALOGW("%s, pthread_mutexattr_init returned %d", __func__, ret);
            return;
        }

        ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
        if (ret != 0) {
            ALOGW("%s, pthread_mutexattr_setprotocol returned %d", __func__, ret);
            return;
        }

        // use of the native_handle() is implementation defined.
        const auto handle = m_.native_handle();
        ret = pthread_mutex_init(handle, &attr);
        if (ret != 0) {
            ALOGW("%s, pthread_mutex_init returned %d", __func__, ret);
        }
        ALOGV("%s: audio_mutex initialized: ret:%d  order:%u", __func__, ret, order_);
    }

    ~mutex_impl() {
        // Note: std::mutex behavior is undefined if released holding ownership.
    }

    auto native_handle() {
        return m_.native_handle();
    }

    void lock() ACQUIRE() {
        lock_scoped_stat_t::pre_lock(*this);
        if (!m_.try_lock()) {  // if we directly use futex, we can optimize this with m_.lock().
            // lock_scoped_stat_t accumulates waiting time for the mutex lock call.
            lock_scoped_stat_t ls(*this);
            m_.lock();
        }
        lock_scoped_stat_t::post_lock(*this);
    }

    void unlock() RELEASE() {
        lock_scoped_stat_t::pre_unlock(*this);
        m_.unlock();
    }

    bool try_lock(int64_t timeout_ns = 0) TRY_ACQUIRE(true) {
        lock_scoped_stat_t::pre_lock(*this);
        if (timeout_ns <= 0) {
            if (!m_.try_lock()) return false;
        } else {
            const int64_t deadline_ns = timeout_ns + systemTime(SYSTEM_TIME_REALTIME);
            const struct timespec ts = {
                .tv_sec = static_cast<time_t>(deadline_ns / 1'000'000'000),
                .tv_nsec = static_cast<long>(deadline_ns % 1'000'000'000),
            };
            lock_scoped_stat_t ls(*this);
            if (pthread_mutex_timedlock(m_.native_handle(), &ts) != 0) {
                ls.ignoreWaitTime();  // didn't get lock, don't count wait time
                return false;
            }
        }
        lock_scoped_stat_t::post_lock(*this);
        return true;
    }

    // additional method to obtain the underlying std::mutex.
    std::mutex& std_mutex() {
        return m_;
    }

    using mutex_stat_t = mutex_stat<uint64_t, double>;

    mutex_stat_t& get_stat() const {
        return stat_;
    }

    /**
     * Returns the locking statistics per mutex capability category.
     */
    static std::string all_stats_to_string() {
        std::string out("mutex stats: priority inheritance ");
        out.append(mutex_get_enable_flag() ? "enabled" : "disabled")
            .append("\n");
        const auto& stat_array = get_mutex_stat_array();
        for (size_t i = 0; i < stat_array.size(); ++i) {
            if (stat_array[i].locks != 0) {
                out.append("Capability: ").append(Attributes::order_names_[i]).append("\n")
                    .append(stat_array[i].to_string());
            }
        }
        return out;
    }

    /**
     * Returns the thread locks held per tid.
     */
    static std::string all_threads_to_string() {
        return get_registry().to_string();
    }

    /**
     * Returns a pair of bool (whether a cycle is detected) and a vector
     * of mutex wait dependencies.
     *
     * If a cycle is detected, the last element of the vector represents
     * a tid that is repeated somewhere earlier in the vector.
     *
     * The deadlock_detection() method is not exceptionally fast
     * and is not designed to be called for every mutex locked (and contended).
     * It is designed to run as a diagnostic routine to enhance
     * dumping for watchdogs, like TimeCheck, when a tid is believed blocked.
     *
     * Access of state is through atomics, so has minimal overhead on
     * concurrent execution, with the possibility of (mostly) false
     * negatives due to race.
     */
    static deadlock_info_t
    deadlock_detection(pid_t tid) {
        return get_registry().deadlock_detection(tid, Attributes::order_names_);
    }

    using thread_mutex_info_t = thread_mutex_info<
            void* /* mutex handle */, MutexOrder, Attributes::mutex_stack_depth_>;

    // get_thread_mutex_info is a thread-local "singleton".
    //
    // We write it like a Meyer's singleton with a single thread_local
    // assignment that is guaranteed to be called on first time initialization.
    // Since the variables are thread_local, there is no thread contention
    // for initialization that would happen with a traditional Meyer's singleton,
    // so really a simple thread-local bool will do for a once_flag.
    static const std::shared_ptr<thread_mutex_info_t>& get_thread_mutex_info() {
        thread_local std::shared_ptr<thread_mutex_info_t> tminfo = []() {
            auto info = std::make_shared<thread_mutex_info_t>(gettid_wrapper());
            get_registry().add_to_registry(info);
            return info;
        }();
        return tminfo;
    }

    // helper class for registering statistics for a mutex lock.

    class lock_scoped_stat_enabled {
    public:
        explicit lock_scoped_stat_enabled(mutex& m)
            : mutex_(m)
            , time_(systemTime()) {
           ++mutex_.stat_.waits;
           mutex_.get_thread_mutex_info()->reset_waiter(&mutex_);
        }

        ~lock_scoped_stat_enabled() {
           if (!discard_wait_time_) mutex_.stat_.add_wait_time(systemTime() - time_);
           mutex_.get_thread_mutex_info()->reset_waiter();
        }

        void ignoreWaitTime() {
            discard_wait_time_ = true;
        }

        static void pre_unlock(mutex& m) {
            ++m.stat_.unlocks;
            const bool success = m.get_thread_mutex_info()->remove_held(&m);
            LOG_ALWAYS_FATAL_IF(Attributes::abort_on_invalid_unlock_
                    && mutex_get_enable_flag()
                    && !success,
                    "%s: invalid mutex unlock when not previously held", __func__);
        }

        // before we lock, we check order and recursion.
        static void pre_lock(mutex& m) {
            if constexpr (!Attributes::abort_on_order_check_ &&
                    !Attributes::abort_on_recursion_check_) return;

            const auto& p = m.get_thread_mutex_info()->check_held(&m, m.order_);
            if (p.first == nullptr) return;  // no problematic mutex.

            // problem!
            const size_t p_order = static_cast<size_t>(p.second.load());
            const size_t m_order = static_cast<size_t>(m.order_);

            // lock inversion
            LOG_ALWAYS_FATAL_IF(Attributes::abort_on_order_check_
                    && mutex_get_enable_flag()
                    && p_order > m_order,
                    "%s: invalid mutex order (previous) %zu %s> (new) %zu %s",
                    __func__, p_order, Attributes::order_names_[p_order],
                    m_order, Attributes::order_names_[m_order]);

            // lock recursion
            LOG_ALWAYS_FATAL_IF(Attributes::abort_on_recursion_check_
                    && mutex_get_enable_flag()
                    && p_order == m_order,
                    "%s: recursive mutex access detected (order: %zu %s)",
                    __func__, p_order, Attributes::order_names_[p_order]);
        }

        static void post_lock(mutex& m) {
            ++m.stat_.locks;
            m.get_thread_mutex_info()->push_held(&m, m.order_);
        }

    private:
        mutex& mutex_;
        const int64_t time_;
        bool discard_wait_time_ = false;
    };

    class lock_scoped_stat_disabled {
    public:
        explicit lock_scoped_stat_disabled(mutex&) {}

        void ignoreWaitTime() {}

        static void pre_unlock(mutex&) {}

        static void pre_lock(mutex&) {}

        static void post_lock(mutex&) {}
    };

    using lock_scoped_stat_t = std::conditional_t<Attributes::mutex_tracking_enabled_,
            lock_scoped_stat_enabled, lock_scoped_stat_disabled>;

    // helper class for registering statistics for a cv wait.
    class cv_wait_scoped_stat_enabled {
    public:
        explicit cv_wait_scoped_stat_enabled(mutex& m) : mutex_(m) {
            ++mutex_.stat_.unlocks;
            const bool success = mutex_.get_thread_mutex_info()->remove_held(&mutex_);
            LOG_ALWAYS_FATAL_IF(Attributes::abort_on_invalid_unlock_
                    && mutex_get_enable_flag()
                    && !success,
                    "%s: invalid mutex unlock when not previously held", __func__);
        }

        ~cv_wait_scoped_stat_enabled() {
            ++mutex_.stat_.locks;
            mutex_.get_thread_mutex_info()->push_held(&mutex_, mutex_.order_);
        }
    private:
        mutex& mutex_;
    };

    class cv_wait_scoped_stat_disabled {
        explicit cv_wait_scoped_stat_disabled(mutex&) {}
    };

    using cv_wait_scoped_stat_t = std::conditional_t<Attributes::mutex_tracking_enabled_,
            cv_wait_scoped_stat_enabled, cv_wait_scoped_stat_disabled>;

    using thread_registry_t = thread_registry<thread_mutex_info_t>;

    // One per-process thread registry, one instance per template typename.
    // Declared here but must be defined in a .cpp otherwise there will be multiple
    // instances if the header is included into different shared libraries.
    static thread_registry_t& get_registry();

    using stat_array_t = std::array<mutex_stat_t, Attributes::order_size_>;

    // One per-process mutex statistics array, one instance per template typename.
    // Declared here but must be defined in a .cpp otherwise there will be multiple
    // instances if the header is included into different shared libraries.
    static stat_array_t& get_mutex_stat_array();

private:

    std::mutex m_;
    const typename Attributes::order_t order_;
    mutex_stat_t& stat_;  // set in ctor
};

// define the destructor to remove from registry.
template <typename MutexHandle, typename Order, size_t N>
inline thread_mutex_info<MutexHandle, Order, N>::~thread_mutex_info() {
    if (tid_ != 0) {
        mutex::get_registry().remove_from_registry(tid_);
    }
}

// audio_utils::lock_guard only works with the defined mutex.
using lock_guard = std::lock_guard<mutex>;

// audio_utils::unique_lock is based on std::unique_lock<std::mutex>
// because std::condition_variable is optimized for std::unique_lock<std::mutex>
//
// Note: std::unique_lock **does not** have thread safety annotations.
// We annotate correctly here.  Essentially, this is the same as an annotated
// using unique_lock = std::unique_lock<std::mutex>;
//
// We omit swap(), release() and move methods which don't have thread
// safety annotations.

class SCOPED_CAPABILITY unique_lock {
public:
    explicit unique_lock(mutex& m) ACQUIRE(m)
        : ul_(m.std_mutex(), std::defer_lock)
        , mutex_(m) {
        lock();
    }

    ~unique_lock() RELEASE() {
        if (held) unlock();
    }

    void lock() ACQUIRE() {
        mutex::lock_scoped_stat_t::pre_lock(mutex_);
        if (!ul_.try_lock()) {
            mutex::lock_scoped_stat_t ls(mutex_);
            ul_.lock();
        }
        mutex::lock_scoped_stat_t::post_lock(mutex_);
        held = true;
    }

    void unlock() RELEASE() {
        mutex::lock_scoped_stat_t::pre_unlock(mutex_);
        held = false;
        ul_.unlock();
    }

    bool try_lock() TRY_ACQUIRE(true) {
        mutex::lock_scoped_stat_t::pre_lock(mutex_);
        if (!ul_.try_lock()) return false;
        mutex::lock_scoped_stat_t::post_lock(mutex_);
        held = true;
        return true;
    }

    template<class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration)
            TRY_ACQUIRE(true) {
        mutex::lock_scoped_stat_t::pre_lock(mutex_);
        if (!ul_.try_lock_for(timeout_duration)) return false;
        mutex::lock_scoped_stat_t::post_lock(mutex_);
        held = true;
        return true;
    }

    template<class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time)
            TRY_ACQUIRE(true) {
        mutex::lock_scoped_stat_t::pre_lock(mutex_);
        if (!ul_.try_lock_until(timeout_time)) return false;
        mutex::lock_scoped_stat_t::post_lock(mutex_);
        held = true;
        return true;
    }

    // additional method to obtain the underlying std::unique_lock
    std::unique_lock<std::mutex>& std_unique_lock() {
        return ul_;
    }

    // additional method to obtain the underlying mutex
    mutex& native_mutex() {
        return mutex_;
    }

private:
    std::unique_lock<std::mutex> ul_;
    bool held = false;
    mutex& mutex_;
};

// audio_utils::condition_variable uses the optimized version of
// std::condition_variable for std::unique_lock<std::mutex>
// It is possible to use std::condition_variable_any for a generic mutex type,
// but it is less efficient.

class condition_variable {
public:
    void notify_one() noexcept {
        cv_.notify_one();
    }

    void notify_all() noexcept {
        cv_.notify_all();
    }

    void wait(unique_lock& lock) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        cv_.wait(lock.std_unique_lock());
    }

    template<typename Predicate>
    void wait(unique_lock& lock, Predicate stop_waiting) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        cv_.wait(lock.std_unique_lock(), std::move(stop_waiting));
    }

    template<typename Rep, typename Period>
    std::cv_status wait_for(unique_lock& lock,
            const std::chrono::duration<Rep, Period>& rel_time) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        return cv_.wait_for(lock.std_unique_lock(), rel_time);
    }

    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(unique_lock& lock,
            const std::chrono::duration<Rep, Period>& rel_time,
            Predicate stop_waiting) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        return cv_.wait_for(lock.std_unique_lock(), rel_time, std::move(stop_waiting));
    }

    template<typename Clock, typename Duration>
    std::cv_status wait_until(unique_lock& lock,
            const std::chrono::time_point<Clock, Duration>& timeout_time) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        return cv_.wait_until(lock.std_unique_lock(), timeout_time);
    }

    template<typename Clock, typename Duration, typename Predicate>
    bool wait_until(unique_lock& lock,
            const std::chrono::time_point<Clock, Duration>& timeout_time,
            Predicate stop_waiting) {
        mutex::cv_wait_scoped_stat_t ws(lock.native_mutex());
        return cv_.wait_until(lock.std_unique_lock(), timeout_time, std::move(stop_waiting));
    }

    auto native_handle() {
        return cv_.native_handle();
    }

private:
    std::condition_variable cv_;
};

// audio_utils::scoped_lock has proper thread safety annotations.
// std::scoped_lock does not have thread safety annotations for greater than 1 lock
// since the variadic template doesn't convert to the variadic macro def.

template <typename ...Mutexes>
class scoped_lock;

template <typename Mutex1>
class SCOPED_CAPABILITY scoped_lock<Mutex1>
    : private std::scoped_lock<Mutex1> {
public:
    explicit scoped_lock(Mutex1& m) ACQUIRE(m) : std::scoped_lock<Mutex1>(m) {}
    ~scoped_lock() RELEASE() = default;
};

template <typename Mutex1, typename Mutex2>
class SCOPED_CAPABILITY scoped_lock<Mutex1, Mutex2>
    : private std::scoped_lock<Mutex1, Mutex2> {
public:
    scoped_lock(Mutex1& m1, Mutex2& m2) ACQUIRE(m1, m2)
        : std::scoped_lock<Mutex1, Mutex2>(m1, m2) {}
    ~scoped_lock() RELEASE() = default;
};

template <typename Mutex1, typename Mutex2, typename Mutex3>
class SCOPED_CAPABILITY scoped_lock<Mutex1, Mutex2, Mutex3>
    : private std::scoped_lock<Mutex1, Mutex2, Mutex3> {
public:
    scoped_lock(Mutex1& m1, Mutex2& m2, Mutex3& m3) ACQUIRE(m1, m2, m3)
        : std::scoped_lock<Mutex1, Mutex2, Mutex3>(m1, m2, m3) {}
    ~scoped_lock() RELEASE() = default;
};

template <typename ...Mutexes>
class scoped_lock : private std::scoped_lock<Mutexes...> {
public:
    scoped_lock(Mutexes&... mutexes)
      : std::scoped_lock<Mutexes...>(mutexes...) {}
};

// audio_utils::lock_guard_no_thread_safety_analysis is used to lock
// the second mutex when the same global capability is aliased
// to 2 (or more) different mutexes.
//
// Example usage:
//
// // Suppose the interface IAfThreadBase::mutex() returns a global capability
// // ThreadBase_Mutex.
//
// sp<IAfThreadBase> srcThread, dstThread;
//
// lock_guard(srcThread->mutex());  // acquires global capability ThreadBase_Mutex;
// ...
// lock_guard_no_thread_safety_analysis(  // lock_guard would fail here as
//         dstThread->mutex());           // the same global capability is assigned to
//                                        // dstThread->mutex().
//                                        // lock_guard_no_thread_safety_analysis
//                                        // prevents a thread safety error.

template<typename Mutex1>
class lock_guard_no_thread_safety_analysis : private std::lock_guard<Mutex1> {
public:
    lock_guard_no_thread_safety_analysis(Mutex1& m) : std::lock_guard<Mutex1>(m) {}
};

} // namespace android::audio_utils

#pragma pop_macro("LOG_TAG")
