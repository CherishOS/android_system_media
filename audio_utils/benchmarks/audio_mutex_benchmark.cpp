/*
 * Copyright 2023 The Android Open Source Project
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

#include <audio_utils/mutex.h>

#include <benchmark/benchmark.h>
#include <thread>
#include <utils/Timers.h>

/*
On Pixel 7 U arm64-v8a

Note: to bump up the scheduler clock frequency, one can use the toybox uclampset:
$ adb shell uclampset -m 1024 /data/benchmarktest64/audio_mutex_benchmark/audio_mutex_benchmark

For simplicity these tests use the regular invocation:
$ atest audio_mutex_benchmark

PI disabled
Benchmark                                                     Time        CPU        Iteration
audio_mutex_benchmark:
  #BM_gettid                                                 2.114707270012698 ns     2.1060503125712704 ns     316574041
  #BM_systemTime                                             45.21805864916908 ns     45.026921817247256 ns      15550399
  #BM_thread_8_variables                                    2.8218655987348464 ns     2.8084059508955304 ns     249245377
  #BM_thread_local_8_variables                              2.8193214063325636 ns      2.808094924629991 ns     249270437
  #BM_StdMutexLockUnlock                                    18.284456262132316 ns     18.198265636345102 ns      38452720
  #BM_AudioUtilsMutexLockUnlock                              65.64444199789075 ns      65.27982033200155 ns      10727119
  #BM_StdMutexInitializationLockUnlock                      27.511005854186497 ns     27.400197055351832 ns      25576570
  #BM_AudioUtilsMutexInitializationLockUnlock                80.13035874526041 ns       79.7832739530702 ns       8771433
  #BM_StdMutexBlockingConditionVariable/threads:2           20162.129644197645 ns     23610.966860300927 ns         49578
  #BM_AudioUtilsMutexBlockingConditionVariable/threads:2    15137.825614929703 ns     17290.483328991908 ns         53746
  #BM_StdMutexScopedLockUnlock/threads:2                     237.3259685000022 ns     471.24025449999993 ns       2000000
  #BM_AudioUtilsMutexScopedLockUnlock/threads:2             1166.2574495961544 ns     2286.2857883742913 ns        696078
  #BM_StdMutexReverseScopedLockUnlock/threads:2              78.13460285213144 ns      147.4075991604524 ns       4367114
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:2       652.4407601321186 ns      1260.665055303518 ns        729610
  #BM_empty_while                                           0.3525216479999926 ns    0.35102758499999887 ns    1000000000

PI enabled
Benchmark                                                     Time        CPU        Iteration
audio_mutex_benchmark:
  #BM_gettid                                                2.1172475906830703 ns    2.1060892051984452 ns     320818635
  #BM_systemTime                                            45.199132456479795 ns     45.00638566260978 ns      15548269
  #BM_thread_8_variables                                    2.8225443366713554 ns     2.808487087931257 ns     249225013
  #BM_thread_local_8_variables                               2.821986069144024 ns    2.8082593311342396 ns     249257908
  #BM_StdMutexLockUnlock                                     18.40590257629023 ns    18.181680287238002 ns      38568989
  #BM_AudioUtilsMutexLockUnlock                              67.93231825441279 ns      67.0807303508343 ns      10434985
  #BM_StdMutexInitializationLockUnlock                       27.68553624974349 ns    27.411678887121656 ns      25531919
  #BM_AudioUtilsMutexInitializationLockUnlock                87.67312115717117 ns     86.83834669971819 ns       8058524
  #BM_StdMutexBlockingConditionVariable/threads:2            17272.46593515828 ns     19906.88809450987 ns         78468
  #BM_AudioUtilsMutexBlockingConditionVariable/threads:2    32005.869201264206 ns     37545.03395758123 ns         44320
  #BM_StdMutexScopedLockUnlock/threads:2                    486.46992430399723 ns     956.4308063689062 ns       1080374
  #BM_AudioUtilsMutexScopedLockUnlock/threads:2              2054.297687080403 ns    2381.4833355667856 ns        462662
  #BM_StdMutexReverseScopedLockUnlock/threads:2              82.48454541925697 ns     156.5002537866742 ns       3502942
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:2       7202.760827499759 ns     8601.367344999997 ns        200000
  #BM_empty_while                                           0.3630271000000107 ns    0.3611862729999995 ns    1000000000

*/

// ---

// Benchmark gettid().  The mutex class uses this to get the linux thread id.
static void BM_gettid(benchmark::State &state) {
    int32_t value = 0;
    while (state.KeepRunning()) {
        value ^= android::audio_utils::gettid_wrapper();  // ensure the return value used.
    }
    ALOGD("%s: value:%d", __func__, value);
}

BENCHMARK(BM_gettid);

// Benchmark systemTime().  The mutex class uses this for timing.
static void BM_systemTime(benchmark::State &state) {
    int64_t value = 0;
    while (state.KeepRunning()) {
        value ^= systemTime();
    }
    ALOGD("%s: value:%lld", __func__, (long long)value);
}

BENCHMARK(BM_systemTime);

// Benchmark access to 8 thread local storage variables by compiler built_in __thread.
__thread volatile int tls_value1 = 1;
__thread volatile int tls_value2 = 2;
__thread volatile int tls_value3 = 3;
__thread volatile int tls_value4 = 4;
__thread volatile int tls_value5 = 5;
__thread volatile int tls_value6 = 6;
__thread volatile int tls_value7 = 7;
__thread volatile int tls_value8 = 8;

static void BM_thread_8_variables(benchmark::State &state) {
    while (state.KeepRunning()) {
        tls_value1 ^= tls_value1 ^ tls_value2 ^ tls_value3 ^ tls_value4 ^
                tls_value5 ^ tls_value6 ^ tls_value7 ^ tls_value8;
    }
    ALOGD("%s: value:%d", __func__, tls_value1);
}

BENCHMARK(BM_thread_8_variables);

// Benchmark access to 8 thread local storage variables through the
// the C/C++ 11 standard thread_local.
thread_local volatile int tlsa_value1 = 1;
thread_local volatile int tlsa_value2 = 2;
thread_local volatile int tlsa_value3 = 3;
thread_local volatile int tlsa_value4 = 4;
thread_local volatile int tlsa_value5 = 5;
thread_local volatile int tlsa_value6 = 6;
thread_local volatile int tlsa_value7 = 7;
thread_local volatile int tlsa_value8 = 8;

static void BM_thread_local_8_variables(benchmark::State &state) {
    while (state.KeepRunning()) {
        tlsa_value1 ^= tlsa_value1 ^ tlsa_value2 ^ tlsa_value3 ^ tlsa_value4 ^
                tlsa_value5 ^ tlsa_value6 ^ tlsa_value7 ^ tlsa_value8;
    }
    ALOGD("%s: value:%d", __func__, tlsa_value1);
}

BENCHMARK(BM_thread_local_8_variables);

// ---

template <typename Mutex>
void MutexLockUnlock(benchmark::State& state) {
    Mutex m;
    while (state.KeepRunning()) {
        m.lock();
        m.unlock();
    }
}

static void BM_StdMutexLockUnlock(benchmark::State &state) {
    MutexLockUnlock<std::mutex>(state);
}

// Benchmark repeated mutex lock/unlock from a single thread
// using std::mutex.
BENCHMARK(BM_StdMutexLockUnlock);

static void BM_AudioUtilsMutexLockUnlock(benchmark::State &state) {
    MutexLockUnlock<android::audio_utils::mutex>(state);
}

// Benchmark repeated mutex lock/unlock from a single thread
// using audio_utils::mutex.  This will differ when priority inheritance
// is used or not.
BENCHMARK(BM_AudioUtilsMutexLockUnlock);

// ---

template <typename Mutex>
void MutexInitializationLockUnlock(benchmark::State& state) {
    while (state.KeepRunning()) {
        Mutex m;
        m.lock();
        m.unlock();
    }
}

static void BM_StdMutexInitializationLockUnlock(benchmark::State &state) {
    MutexInitializationLockUnlock<std::mutex>(state);
}

// Benchmark repeated mutex creation then lock/unlock from a single thread
// using std::mutex.
BENCHMARK(BM_StdMutexInitializationLockUnlock);

static void BM_AudioUtilsMutexInitializationLockUnlock(benchmark::State &state) {
    MutexInitializationLockUnlock<android::audio_utils::mutex>(state);
}

// Benchmark repeated mutex creation then lock/unlock from a single thread
// using audio_utils::mutex.  This will differ when priority inheritance
// is used or not.
BENCHMARK(BM_AudioUtilsMutexInitializationLockUnlock);

// ---

static constexpr size_t THREADS = 2;

template <typename Mutex, typename UniqueLock, typename ConditionVariable>
class MutexBlockingConditionVariable {
    Mutex m;
    ConditionVariable cv[THREADS];
    bool wake[THREADS];

public:
    void run(benchmark::State& state) {
        const size_t local = state.thread_index();
        const size_t remote = (local + 1) % THREADS;
        if (local == 0) wake[local] = true;
        for (auto _ : state) {
            UniqueLock ul(m);
            cv[local].wait(ul, [&]{ return wake[local]; });
            wake[remote] = true;
            wake[local] = false;
            cv[remote].notify_one();
        }
        UniqueLock ul(m);
        wake[remote] = true;
        cv[remote].notify_one();
    }
};

MutexBlockingConditionVariable<std::mutex,
            std::unique_lock<std::mutex>,
            std::condition_variable> CvStd;

static void BM_StdMutexBlockingConditionVariable(benchmark::State &state) {
    CvStd.run(state);
}

// Benchmark 2 threads that use condition variables to wake each other up,
// where only one thread is active at a given time.  This benchmark
// uses std::mutex, std::unique_lock, and std::condition_variable.
BENCHMARK(BM_StdMutexBlockingConditionVariable)->Threads(THREADS);

MutexBlockingConditionVariable<android::audio_utils::mutex,
        android::audio_utils::unique_lock,
        android::audio_utils::condition_variable> CvAu;

static void BM_AudioUtilsMutexBlockingConditionVariable(benchmark::State &state) {
    CvAu.run(state);
}

// Benchmark 2 threads that use condition variables to wake each other up,
// where only one thread is active at a given time.  This benchmark
// uses audio_utils::mutex, audio_utils::unique_lock, and audio_utils::condition_variable.
//
// Priority inheritance mutexes will affect the performance of this benchmark.
BENCHMARK(BM_AudioUtilsMutexBlockingConditionVariable)->Threads(THREADS);

// ---

template <typename Mutex, typename ScopedLock>
class MutexScopedLockUnlock {
    const bool reverse_;
    Mutex m1_, m2_;
    int counter_ = 0;

public:
    MutexScopedLockUnlock(bool reverse = false) : reverse_(reverse) {}

    void run(benchmark::State& state) {
        const size_t index = state.thread_index();
        for (auto _ : state) {
            if (!reverse_ || index & 1) {
                ScopedLock s1(m1_, m2_);
                ++counter_;
            } else {
                ScopedLock s1(m2_, m1_);
                ++counter_;
            }
        }
    }
};

MutexScopedLockUnlock<std::mutex,
        std::scoped_lock<std::mutex, std::mutex>> ScopedStd;

static void BM_StdMutexScopedLockUnlock(benchmark::State &state) {
    ScopedStd.run(state);
}

// Benchmark scoped_lock where two threads try to obtain the
// same 2 locks with the same initial acquisition order.
// This uses std::mutex, std::scoped_lock.
BENCHMARK(BM_StdMutexScopedLockUnlock)->Threads(THREADS);

MutexScopedLockUnlock<android::audio_utils::mutex,
        android::audio_utils::scoped_lock<
                android::audio_utils::mutex, android::audio_utils::mutex>> ScopedAu;

static void BM_AudioUtilsMutexScopedLockUnlock(benchmark::State &state) {
    ScopedAu.run(state);
}

// Benchmark scoped_lock where two threads try to obtain the
// same 2 locks with the same initial acquisition order.
// This uses audio_utils::mutex and audio_utils::scoped_lock.
//
// Priority inheritance mutexes will affect the performance of this benchmark.
BENCHMARK(BM_AudioUtilsMutexScopedLockUnlock)->Threads(THREADS);

MutexScopedLockUnlock<std::mutex,
        std::scoped_lock<std::mutex, std::mutex>> ReverseScopedStd(true);

static void BM_StdMutexReverseScopedLockUnlock(benchmark::State &state) {
    ReverseScopedStd.run(state);
}

// Benchmark scoped_lock with odd thread having reversed scoped_lock mutex acquisition order.
// This uses std::mutex, std::scoped_lock.
BENCHMARK(BM_StdMutexReverseScopedLockUnlock)->Threads(THREADS);

MutexScopedLockUnlock<android::audio_utils::mutex,
        android::audio_utils::scoped_lock<
                android::audio_utils::mutex, android::audio_utils::mutex>> ReverseScopedAu(true);

static void BM_AudioUtilsMutexReverseScopedLockUnlock(benchmark::State &state) {
    ReverseScopedAu.run(state);
}

// Benchmark scoped_lock with odd thread having reversed scoped_lock mutex acquisition order.
// This uses audio_utils::mutex and audio_utils::scoped_lock.
//
// Priority inheritance mutexes will affect the performance of this benchmark.
BENCHMARK(BM_AudioUtilsMutexReverseScopedLockUnlock)->Threads(THREADS);

static void BM_empty_while(benchmark::State &state) {

    while (state.KeepRunning()) {
        ;
    }
    ALOGD("%s", android::audio_utils::mutex::all_stats_to_string().c_str());
}

// Benchmark to see the cost of doing nothing.
BENCHMARK(BM_empty_while);

BENCHMARK_MAIN();
