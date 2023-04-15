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
  #BM_ChannelMix_Stereo/0       2252 ns     2240 ns       306777
  #BM_ChannelMix_Stereo/1       2250 ns     2240 ns       312406
  #BM_ChannelMix_Stereo/2        255 ns      254 ns      2756139
  #BM_ChannelMix_Stereo/3       2961 ns     2948 ns       237611
  #BM_ChannelMix_Stereo/4       3266 ns     3251 ns       215297
  #BM_ChannelMix_Stereo/5        814 ns      810 ns       863179
  #BM_ChannelMix_Stereo/6        816 ns      810 ns       863279
  #BM_ChannelMix_Stereo/7       3268 ns     3248 ns       215509
  #BM_ChannelMix_Stereo/8       3721 ns     3697 ns       189238
  #BM_ChannelMix_Stereo/9       3714 ns     3697 ns       189348
  #BM_ChannelMix_Stereo/10      3695 ns     3676 ns       190422
  #BM_ChannelMix_Stereo/11      4092 ns     4072 ns       171887
  #BM_ChannelMix_Stereo/12      1208 ns     1203 ns       581912
  #BM_ChannelMix_Stereo/13      1229 ns     1207 ns       581412
  #BM_ChannelMix_Stereo/14      4675 ns     4655 ns       150372
  #BM_ChannelMix_Stereo/15      1307 ns     1301 ns       537850
  #BM_ChannelMix_Stereo/16      1307 ns     1301 ns       537851
  #BM_ChannelMix_Stereo/17      2057 ns     2047 ns       341706
  #BM_ChannelMix_Stereo/18      2040 ns     2030 ns       344685
  #BM_ChannelMix_Stereo/19      2429 ns     2417 ns       289546
  #BM_ChannelMix_Stereo/20      7893 ns     7861 ns        89059
  #BM_ChannelMix_Stereo/21      6142 ns     6106 ns       114677
  #BM_ChannelMix_5Point1/0      1673 ns     1665 ns       420383
  #BM_ChannelMix_5Point1/1      1677 ns     1664 ns       420459
  #BM_ChannelMix_5Point1/2       534 ns      531 ns      1314044
  #BM_ChannelMix_5Point1/3      3039 ns     3024 ns       231431
  #BM_ChannelMix_5Point1/4      3764 ns     3745 ns       186930
  #BM_ChannelMix_5Point1/5       726 ns      723 ns       974958
  #BM_ChannelMix_5Point1/6       657 ns      651 ns      1080526
  #BM_ChannelMix_5Point1/7      3794 ns     3776 ns       185233
  #BM_ChannelMix_5Point1/8      4420 ns     4399 ns       159621
  #BM_ChannelMix_5Point1/9      4429 ns     4405 ns       158864
  #BM_ChannelMix_5Point1/10     4438 ns     4415 ns       158596
  #BM_ChannelMix_5Point1/11     5159 ns     5140 ns       135009
  #BM_ChannelMix_5Point1/12      662 ns      659 ns      1064686
  #BM_ChannelMix_5Point1/13      666 ns      662 ns      1049301
  #BM_ChannelMix_5Point1/14     5818 ns     5791 ns       120490
  #BM_ChannelMix_5Point1/15      785 ns      782 ns       892017
  #BM_ChannelMix_5Point1/16      788 ns      783 ns       893725
  #BM_ChannelMix_5Point1/17     1229 ns     1224 ns       570784
  #BM_ChannelMix_5Point1/18     1012 ns     1007 ns       689270
  #BM_ChannelMix_5Point1/19     1380 ns     1374 ns       505533
  #BM_ChannelMix_5Point1/20    10208 ns    10149 ns        68964
  #BM_ChannelMix_5Point1/21     5380 ns     5354 ns       130651
*/

template<audio_channel_mask_t OUTPUT_CHANNEL_MASK>
static void BenchmarkChannelMix(benchmark::State& state) {
    const audio_channel_mask_t channelMask = kChannelPositionMasks[state.range(0)];
    using namespace ::android::audio_utils::channels;
    ChannelMix<OUTPUT_CHANNEL_MASK> channelMix(channelMask);
    const size_t outChannels = audio_channel_count_from_out_mask(OUTPUT_CHANNEL_MASK);
    constexpr size_t frameCount = 1024;
    size_t inChannels = audio_channel_count_from_out_mask(channelMask);
    std::vector<float> input(inChannels * frameCount);
    std::vector<float> output(outChannels * frameCount);
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

static void BM_ChannelMix_Stereo(benchmark::State& state) {
    BenchmarkChannelMix<AUDIO_CHANNEL_OUT_STEREO>(state);
}

static void BM_ChannelMix_5Point1(benchmark::State& state) {
    BenchmarkChannelMix<AUDIO_CHANNEL_OUT_5POINT1>(state);
}

static void ChannelMixArgs(benchmark::internal::Benchmark* b) {
    for (int i = 0; i < (int)std::size(kChannelPositionMasks); i++) {
        b->Args({i});
    }
}

BENCHMARK(BM_ChannelMix_Stereo)->Apply(ChannelMixArgs);

BENCHMARK(BM_ChannelMix_5Point1)->Apply(ChannelMixArgs);

BENCHMARK_MAIN();
