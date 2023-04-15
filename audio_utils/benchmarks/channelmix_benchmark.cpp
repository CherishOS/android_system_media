/*
 * Copyright 2021 The Android Open Source Project
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

#include <audio_utils/ChannelMix.h>

#include <random>
#include <vector>

#include <benchmark/benchmark.h>
#include <log/log.h>

static constexpr audio_channel_mask_t kChannelPositionMasks[] = {
    AUDIO_CHANNEL_OUT_FRONT_LEFT,
    AUDIO_CHANNEL_OUT_FRONT_CENTER,
    AUDIO_CHANNEL_OUT_STEREO,
    AUDIO_CHANNEL_OUT_2POINT1,
    AUDIO_CHANNEL_OUT_2POINT0POINT2,
    AUDIO_CHANNEL_OUT_QUAD, // AUDIO_CHANNEL_OUT_QUAD_BACK
    AUDIO_CHANNEL_OUT_QUAD_SIDE,
    AUDIO_CHANNEL_OUT_SURROUND,
    AUDIO_CHANNEL_OUT_2POINT1POINT2,
    AUDIO_CHANNEL_OUT_3POINT0POINT2,
    AUDIO_CHANNEL_OUT_PENTA,
    AUDIO_CHANNEL_OUT_3POINT1POINT2,
    AUDIO_CHANNEL_OUT_5POINT1, // AUDIO_CHANNEL_OUT_5POINT1_BACK
    AUDIO_CHANNEL_OUT_5POINT1_SIDE,
    AUDIO_CHANNEL_OUT_6POINT1,
    AUDIO_CHANNEL_OUT_5POINT1POINT2,
    AUDIO_CHANNEL_OUT_7POINT1,
    AUDIO_CHANNEL_OUT_5POINT1POINT4,
    AUDIO_CHANNEL_OUT_7POINT1POINT2,
    AUDIO_CHANNEL_OUT_7POINT1POINT4,
    AUDIO_CHANNEL_OUT_13POINT_360RA,
    AUDIO_CHANNEL_OUT_22POINT2,
};

/*
$ adb shell /data/benchmarktest64/channelmix_benchmark/channelmix_benchmark
Pixel 7 arm64 benchmark

-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
channelmix_benchmark:
  #BM_ChannelMix/0     2249 ns    2238 ns       305820
  #BM_ChannelMix/1     2250 ns    2239 ns       312618
  #BM_ChannelMix/2      255 ns     254 ns      2752128
  #BM_ChannelMix/3     2962 ns    2947 ns       237469
  #BM_ChannelMix/4     3271 ns    3252 ns       215272
  #BM_ChannelMix/5      814 ns     810 ns       863256
  #BM_ChannelMix/6      814 ns     810 ns       863196
  #BM_ChannelMix/7     3264 ns    3249 ns       215351
  #BM_ChannelMix/8     3714 ns    3698 ns       189533
  #BM_ChannelMix/9     3746 ns    3715 ns       189286
  #BM_ChannelMix/10    3692 ns    3674 ns       190043
  #BM_ChannelMix/11    4092 ns    4074 ns       171760
  #BM_ChannelMix/12    1209 ns    1203 ns       581577
  #BM_ChannelMix/13    1207 ns    1201 ns       582529
  #BM_ChannelMix/14    4673 ns    4655 ns       150373
  #BM_ChannelMix/15    1308 ns    1301 ns       537770
  #BM_ChannelMix/16    1307 ns    1301 ns       537788
  #BM_ChannelMix/17    2059 ns    2049 ns       341548
  #BM_ChannelMix/18    2046 ns    2036 ns       343601
  #BM_ChannelMix/19    2442 ns    2430 ns       288046
  #BM_ChannelMix/20    7889 ns    7854 ns        89054
  #BM_ChannelMix/21    6134 ns    6104 ns       114691
*/

static void BM_ChannelMix(benchmark::State& state) {
    const audio_channel_mask_t channelMask = kChannelPositionMasks[state.range(0)];
    using namespace ::android::audio_utils::channels;
    ChannelMix<AUDIO_CHANNEL_OUT_STEREO> channelMix(channelMask);
    constexpr size_t frameCount = 1024;
    size_t inChannels = audio_channel_count_from_out_mask(channelMask);
    std::vector<float> input(inChannels * frameCount);
    std::vector<float> output(FCC_2 * frameCount);
    constexpr float amplitude = 0.01f;

    std::minstd_rand gen(channelMask);
    std::uniform_real_distribution<> dis(-amplitude, amplitude);
    for (auto& in : input) {
        in = dis(gen);
    }

    assert(channelMix.getInputChannelMask() != AUDIO_CHANNEL_NONE);
    // Run the test
    for (auto _ : state) {
        benchmark::DoNotOptimize(input.data());
        benchmark::DoNotOptimize(output.data());
        channelMix.process(input.data(), output.data(), frameCount, false /* accumulate */);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(inChannels);
    state.SetLabel(audio_channel_out_mask_to_string(channelMask));
}

static void ChannelMixArgs(benchmark::internal::Benchmark* b) {
    for (int i = 0; i < (int)std::size(kChannelPositionMasks); i++) {
        b->Args({i});
    }
}

BENCHMARK(BM_ChannelMix)->Apply(ChannelMixArgs);

BENCHMARK_MAIN();
