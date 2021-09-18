/*
 * Copyright 2020 The Android Open Source Project
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

#include <array>
#include <climits>
#include <cstdlib>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <audio_utils/BiquadFilter.h>
#include <audio_utils/format.h>

#pragma GCC diagnostic ignored "-Wunused-function"  // we use override array assignment

static constexpr size_t DATA_SIZE = 1024;
// The coefficients is a HPF with sampling frequency as 48000, center frequency as 600,
// and Q as 0.707. As all the coefficients are not zero, they can be used to benchmark
// the non-zero optimization of BiquadFilter.
// The benchmark test will iterate the channel count from 1 to 2. The occupancy will be
// iterate from 1 to 31. In that case, it is possible to test the performance of cases
// with different coefficients as zero.
static constexpr float REF_COEFS[] = {0.9460f, -1.8919f, 0.9460f, -1.8890f, 0.8949f};

static void BM_BiquadFilter1D(benchmark::State& state) {
    using android::audio_utils::BiquadFilter;

    bool doParallel = (state.range(0) == 1);
    // const size_t channelCount = state.range(1);
    const size_t filters = 1;

    std::vector<float> input(DATA_SIZE);
    std::array<float, android::audio_utils::kBiquadNumCoefs> coefs;

    // Initialize input buffer and coefs with deterministic pseudo-random values
    constexpr std::minstd_rand::result_type SEED = 42; // arbitrary choice.
    std::minstd_rand gen(SEED);
    constexpr float amplitude = 1.0f;
    std::uniform_real_distribution<> dis(-amplitude, amplitude);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        input[i] = dis(gen);
    }

    android::audio_utils::BiquadFilter parallel(filters, coefs);
    std::vector<std::unique_ptr<BiquadFilter<float>>> biquads(filters);
    for (auto& biquad : biquads) {
        biquad.reset(new BiquadFilter<float>(1, coefs));
    }

    // Run the test
    float *data = input.data();
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(data);
        if (doParallel) {
            parallel.process1D(data, DATA_SIZE);
        } else {
            for (auto& biquad : biquads) {
                biquad->process(data, data, DATA_SIZE);
            }
        }
        benchmark::ClobberMemory();
    }
}

static void BiquadFilter1DArgs(benchmark::internal::Benchmark* b) {
    for (int k = 0; k < 2; k++) // 0 for normal random data, 1 for subnormal random data
         b->Args({k});
}

BENCHMARK(BM_BiquadFilter1D)->Apply(BiquadFilter1DArgs);

/*******************************************************************
 A test result running on Pixel 4XL for comparison.
 The first parameter indicates the input data is subnormal or not.
 0 for normal input data, 1 for subnormal input data.
 The second parameter indicates the channel count.
 The third parameter indicates the occupancy of the coefficients.

-----------------------------------------------------------------------------------
Benchmark                                         Time             CPU   Iterations
-----------------------------------------------------------------------------------
BM_BiquadFilter1D/0                             556 ns          555 ns      1263112
BM_BiquadFilter1D/1                             560 ns          558 ns      1253287
BM_BiquadFilterFloatOptimized/0/1/31           2178 ns         2172 ns       322245
BM_BiquadFilterFloatOptimized/0/2/31           5013 ns         4999 ns       140023
BM_BiquadFilterFloatOptimized/0/3/31           4938 ns         4924 ns       142153
BM_BiquadFilterFloatOptimized/0/4/31           4996 ns         4981 ns       140506
BM_BiquadFilterFloatOptimized/0/5/31           4931 ns         4917 ns       142358
BM_BiquadFilterFloatOptimized/0/6/31           5222 ns         5208 ns       134401
BM_BiquadFilterFloatOptimized/0/7/31           4694 ns         4681 ns       149552
BM_BiquadFilterFloatOptimized/0/8/31           5174 ns         5159 ns       135656
BM_BiquadFilterFloatOptimized/0/9/31           5604 ns         5589 ns       125174
BM_BiquadFilterFloatOptimized/0/10/31          6136 ns         6118 ns       114547
BM_BiquadFilterFloatOptimized/0/11/31          6080 ns         6065 ns       115425
BM_BiquadFilterFloatOptimized/0/12/31          6114 ns         6098 ns       114790
BM_BiquadFilterFloatOptimized/0/13/31          7247 ns         7229 ns        96798
BM_BiquadFilterFloatOptimized/0/14/31          7539 ns         7515 ns        93137
BM_BiquadFilterFloatOptimized/0/15/31         12787 ns        12748 ns        55041
BM_BiquadFilterFloatOptimized/0/16/31          7493 ns         7470 ns        93688
BM_BiquadFilterFloatOptimized/0/17/31          9797 ns         9766 ns        71597
BM_BiquadFilterFloatOptimized/0/18/31         12563 ns        12524 ns        55862
BM_BiquadFilterFloatOptimized/0/19/31         12560 ns        12521 ns        55846
BM_BiquadFilterFloatOptimized/0/20/31         12560 ns        12523 ns        55926
BM_BiquadFilterFloatOptimized/0/21/31         12576 ns        12543 ns        55795
BM_BiquadFilterFloatOptimized/0/22/31         12881 ns        12845 ns        54408
BM_BiquadFilterFloatOptimized/0/23/31         12681 ns        12635 ns        55410
BM_BiquadFilterFloatOptimized/0/24/31         12749 ns        12712 ns        55041
BM_BiquadFilterFloatOptimized/0/1/1             557 ns          555 ns      1260939
BM_BiquadFilterFloatOptimized/0/1/2             652 ns          650 ns      1077181
BM_BiquadFilterFloatOptimized/0/1/3             652 ns          650 ns      1077352
BM_BiquadFilterFloatOptimized/0/1/4             833 ns          831 ns       840290
BM_BiquadFilterFloatOptimized/0/1/5             835 ns          833 ns       840171
BM_BiquadFilterFloatOptimized/0/1/6             836 ns          833 ns       840106
BM_BiquadFilterFloatOptimized/0/1/7             835 ns          832 ns       840200
BM_BiquadFilterFloatOptimized/0/1/8            1813 ns         1808 ns       387100
BM_BiquadFilterFloatOptimized/0/1/9            1813 ns         1808 ns       387152
BM_BiquadFilterFloatOptimized/0/1/10           2552 ns         2544 ns       275176
BM_BiquadFilterFloatOptimized/0/1/11           2551 ns         2544 ns       275192
BM_BiquadFilterFloatOptimized/0/1/12           2178 ns         2172 ns       322335
BM_BiquadFilterFloatOptimized/0/1/13           2179 ns         2172 ns       322286
BM_BiquadFilterFloatOptimized/0/1/14           2178 ns         2172 ns       322252
BM_BiquadFilterFloatOptimized/0/1/15           2178 ns         2172 ns       322285
BM_BiquadFilterFloatOptimized/0/1/16           2175 ns         2169 ns       322716
BM_BiquadFilterFloatOptimized/0/1/17           2174 ns         2169 ns       322730
BM_BiquadFilterFloatOptimized/0/1/18           2175 ns         2169 ns       322719
BM_BiquadFilterFloatOptimized/0/1/19           2175 ns         2169 ns       322741
BM_BiquadFilterFloatOptimized/0/1/20           2178 ns         2172 ns       322336
BM_BiquadFilterFloatOptimized/0/1/21           2178 ns         2172 ns       322315
BM_BiquadFilterFloatOptimized/0/1/22           2178 ns         2172 ns       322328
BM_BiquadFilterFloatOptimized/0/1/23           2178 ns         2172 ns       322306
BM_BiquadFilterFloatOptimized/0/1/24           2175 ns         2169 ns       322752
BM_BiquadFilterFloatOptimized/0/1/25           2174 ns         2169 ns       322721
BM_BiquadFilterFloatOptimized/0/1/26           2174 ns         2169 ns       322722
BM_BiquadFilterFloatOptimized/0/1/27           2175 ns         2169 ns       322704
BM_BiquadFilterFloatOptimized/0/1/28           2178 ns         2172 ns       322317
BM_BiquadFilterFloatOptimized/0/1/29           2178 ns         2172 ns       322308
BM_BiquadFilterFloatOptimized/0/1/30           2179 ns         2172 ns       322300
BM_BiquadFilterFloatOptimized/0/1/31           2178 ns         2172 ns       322271
BM_BiquadFilterFloatOptimized/0/2/1             737 ns          734 ns       953033
BM_BiquadFilterFloatOptimized/0/2/2            1085 ns         1082 ns       647110
BM_BiquadFilterFloatOptimized/0/2/3            1085 ns         1082 ns       646630
BM_BiquadFilterFloatOptimized/0/2/4            1538 ns         1534 ns       456015
BM_BiquadFilterFloatOptimized/0/2/5            1536 ns         1532 ns       456137
BM_BiquadFilterFloatOptimized/0/2/6            1537 ns         1532 ns       456168
BM_BiquadFilterFloatOptimized/0/2/7            1536 ns         1532 ns       456982
BM_BiquadFilterFloatOptimized/0/2/8            1974 ns         1969 ns       355506
BM_BiquadFilterFloatOptimized/0/2/9            1974 ns         1969 ns       355489
BM_BiquadFilterFloatOptimized/0/2/10           4345 ns         4333 ns       161562
BM_BiquadFilterFloatOptimized/0/2/11           4344 ns         4332 ns       161564
BM_BiquadFilterFloatOptimized/0/2/12           5014 ns         4999 ns       140035
BM_BiquadFilterFloatOptimized/0/2/13           5014 ns         4999 ns       139958
BM_BiquadFilterFloatOptimized/0/2/14           5012 ns         4999 ns       139996
BM_BiquadFilterFloatOptimized/0/2/15           5013 ns         4999 ns       140021
BM_BiquadFilterFloatOptimized/0/2/16           3985 ns         3973 ns       176193
BM_BiquadFilterFloatOptimized/0/2/17           3984 ns         3973 ns       176178
BM_BiquadFilterFloatOptimized/0/2/18           3984 ns         3973 ns       176178
BM_BiquadFilterFloatOptimized/0/2/19           3984 ns         3973 ns       176179
BM_BiquadFilterFloatOptimized/0/2/20           5013 ns         4999 ns       140011
BM_BiquadFilterFloatOptimized/0/2/21           5013 ns         4999 ns       140042
BM_BiquadFilterFloatOptimized/0/2/22           5012 ns         4999 ns       140027
BM_BiquadFilterFloatOptimized/0/2/23           5011 ns         4999 ns       140028
BM_BiquadFilterFloatOptimized/0/2/24           3984 ns         3973 ns       176189
BM_BiquadFilterFloatOptimized/0/2/25           3984 ns         3973 ns       176199
BM_BiquadFilterFloatOptimized/0/2/26           3979 ns         3971 ns       176263
BM_BiquadFilterFloatOptimized/0/2/27           3984 ns         3973 ns       176206
BM_BiquadFilterFloatOptimized/0/2/28           5013 ns         4999 ns       140019
BM_BiquadFilterFloatOptimized/0/2/29           5013 ns         4999 ns       140032
BM_BiquadFilterFloatOptimized/0/2/30           5013 ns         4999 ns       140031
BM_BiquadFilterFloatOptimized/0/2/31           5012 ns         4999 ns       140021
BM_BiquadFilterFloatOptimized/0/3/1            1010 ns         1007 ns       695238
BM_BiquadFilterFloatOptimized/0/3/2            1760 ns         1755 ns       409554
BM_BiquadFilterFloatOptimized/0/3/3            1750 ns         1745 ns       391924
BM_BiquadFilterFloatOptimized/0/3/4            2315 ns         2308 ns       303349
BM_BiquadFilterFloatOptimized/0/3/5            2315 ns         2309 ns       303177
BM_BiquadFilterFloatOptimized/0/3/6            2316 ns         2309 ns       303026
BM_BiquadFilterFloatOptimized/0/3/7            2315 ns         2309 ns       303133
BM_BiquadFilterFloatOptimized/0/3/8            3052 ns         3044 ns       229836
BM_BiquadFilterFloatOptimized/0/3/9            3052 ns         3044 ns       229888
BM_BiquadFilterFloatOptimized/0/3/10           4345 ns         4333 ns       161546
BM_BiquadFilterFloatOptimized/0/3/11           4344 ns         4333 ns       161549
BM_BiquadFilterFloatOptimized/0/3/12           4937 ns         4924 ns       142178
BM_BiquadFilterFloatOptimized/0/3/13           4933 ns         4923 ns       142166
BM_BiquadFilterFloatOptimized/0/3/14           4937 ns         4924 ns       142174
BM_BiquadFilterFloatOptimized/0/3/15           4937 ns         4924 ns       142139
BM_BiquadFilterFloatOptimized/0/3/16           4068 ns         4058 ns       172507
BM_BiquadFilterFloatOptimized/0/3/17           4068 ns         4057 ns       172495
BM_BiquadFilterFloatOptimized/0/3/18           4069 ns         4058 ns       172509
BM_BiquadFilterFloatOptimized/0/3/19           4070 ns         4059 ns       172495
BM_BiquadFilterFloatOptimized/0/3/20           4937 ns         4924 ns       142161
BM_BiquadFilterFloatOptimized/0/3/21           4937 ns         4924 ns       142171
BM_BiquadFilterFloatOptimized/0/3/22           4937 ns         4923 ns       142172
BM_BiquadFilterFloatOptimized/0/3/23           4938 ns         4924 ns       142191
BM_BiquadFilterFloatOptimized/0/3/24           4072 ns         4058 ns       172484
BM_BiquadFilterFloatOptimized/0/3/25           4070 ns         4058 ns       172532
BM_BiquadFilterFloatOptimized/0/3/26           4068 ns         4058 ns       172543
BM_BiquadFilterFloatOptimized/0/3/27           4069 ns         4058 ns       172503
BM_BiquadFilterFloatOptimized/0/3/28           4937 ns         4924 ns       142173
BM_BiquadFilterFloatOptimized/0/3/29           4940 ns         4924 ns       142160
BM_BiquadFilterFloatOptimized/0/3/30           4937 ns         4924 ns       142168
BM_BiquadFilterFloatOptimized/0/3/31           4937 ns         4924 ns       142171
BM_BiquadFilterFloatOptimized/0/4/1             555 ns          553 ns      1264721
BM_BiquadFilterFloatOptimized/0/4/2             736 ns          734 ns       953947
BM_BiquadFilterFloatOptimized/0/4/3             736 ns          734 ns       953825
BM_BiquadFilterFloatOptimized/0/4/4            1357 ns         1353 ns       517334
BM_BiquadFilterFloatOptimized/0/4/5            1357 ns         1353 ns       517339
BM_BiquadFilterFloatOptimized/0/4/6            1357 ns         1353 ns       517307
BM_BiquadFilterFloatOptimized/0/4/7            1357 ns         1353 ns       517153
BM_BiquadFilterFloatOptimized/0/4/8            1901 ns         1896 ns       369069
BM_BiquadFilterFloatOptimized/0/4/9            1902 ns         1897 ns       369100
BM_BiquadFilterFloatOptimized/0/4/10           3984 ns         3972 ns       176207
BM_BiquadFilterFloatOptimized/0/4/11           3984 ns         3972 ns       176209
BM_BiquadFilterFloatOptimized/0/4/12           4998 ns         4982 ns       140517
BM_BiquadFilterFloatOptimized/0/4/13           4996 ns         4982 ns       140523
BM_BiquadFilterFloatOptimized/0/4/14           4996 ns         4982 ns       140527
BM_BiquadFilterFloatOptimized/0/4/15           4995 ns         4982 ns       140510
BM_BiquadFilterFloatOptimized/0/4/16           3984 ns         3973 ns       176180
BM_BiquadFilterFloatOptimized/0/4/17           3985 ns         3973 ns       176195
BM_BiquadFilterFloatOptimized/0/4/18           3985 ns         3973 ns       176206
BM_BiquadFilterFloatOptimized/0/4/19           3984 ns         3973 ns       176193
BM_BiquadFilterFloatOptimized/0/4/20           4999 ns         4984 ns       140465
BM_BiquadFilterFloatOptimized/0/4/21           4997 ns         4982 ns       140518
BM_BiquadFilterFloatOptimized/0/4/22           4997 ns         4982 ns       140541
BM_BiquadFilterFloatOptimized/0/4/23           4995 ns         4982 ns       140518
BM_BiquadFilterFloatOptimized/0/4/24           3984 ns         3973 ns       176197
BM_BiquadFilterFloatOptimized/0/4/25           3983 ns         3973 ns       176182
BM_BiquadFilterFloatOptimized/0/4/26           3984 ns         3973 ns       176193
BM_BiquadFilterFloatOptimized/0/4/27           3985 ns         3973 ns       176205
BM_BiquadFilterFloatOptimized/0/4/28           4997 ns         4982 ns       140507
BM_BiquadFilterFloatOptimized/0/4/29           4996 ns         4982 ns       140515
BM_BiquadFilterFloatOptimized/0/4/30           4996 ns         4983 ns       140517
BM_BiquadFilterFloatOptimized/0/4/31           4998 ns         4982 ns       140519
BM_BiquadFilterFloatOptimized/1/1/1             557 ns          555 ns      1261214
BM_BiquadFilterFloatOptimized/1/1/2             652 ns          650 ns      1077578
BM_BiquadFilterFloatOptimized/1/1/3             652 ns          650 ns      1077688
BM_BiquadFilterFloatOptimized/1/1/4             834 ns          832 ns       841263
BM_BiquadFilterFloatOptimized/1/1/5             836 ns          833 ns       840264
BM_BiquadFilterFloatOptimized/1/1/6             836 ns          833 ns       840002
BM_BiquadFilterFloatOptimized/1/1/7             835 ns          833 ns       840209
BM_BiquadFilterFloatOptimized/1/1/8            1813 ns         1808 ns       387140
BM_BiquadFilterFloatOptimized/1/1/9            1814 ns         1808 ns       387077
BM_BiquadFilterFloatOptimized/1/1/10           2552 ns         2544 ns       275164
BM_BiquadFilterFloatOptimized/1/1/11           2552 ns         2545 ns       275177
BM_BiquadFilterFloatOptimized/1/1/12           2178 ns         2172 ns       322211
BM_BiquadFilterFloatOptimized/1/1/13           2178 ns         2172 ns       322244
BM_BiquadFilterFloatOptimized/1/1/14           2179 ns         2172 ns       322290
BM_BiquadFilterFloatOptimized/1/1/15           2179 ns         2172 ns       322318
BM_BiquadFilterFloatOptimized/1/1/16           2175 ns         2169 ns       322771
BM_BiquadFilterFloatOptimized/1/1/17           2176 ns         2169 ns       322723
BM_BiquadFilterFloatOptimized/1/1/18           2175 ns         2169 ns       322752
BM_BiquadFilterFloatOptimized/1/1/19           2175 ns         2169 ns       322712
BM_BiquadFilterFloatOptimized/1/1/20           2178 ns         2172 ns       322229
BM_BiquadFilterFloatOptimized/1/1/21           2178 ns         2172 ns       322263
BM_BiquadFilterFloatOptimized/1/1/22           2178 ns         2172 ns       322271
BM_BiquadFilterFloatOptimized/1/1/23           2178 ns         2172 ns       322302
BM_BiquadFilterFloatOptimized/1/1/24           2176 ns         2169 ns       322749
BM_BiquadFilterFloatOptimized/1/1/25           2175 ns         2169 ns       322653
BM_BiquadFilterFloatOptimized/1/1/26           2175 ns         2169 ns       322739
BM_BiquadFilterFloatOptimized/1/1/27           2175 ns         2169 ns       322709
BM_BiquadFilterFloatOptimized/1/1/28           2178 ns         2172 ns       322242
BM_BiquadFilterFloatOptimized/1/1/29           2178 ns         2172 ns       322286
BM_BiquadFilterFloatOptimized/1/1/30           2177 ns         2172 ns       322259
BM_BiquadFilterFloatOptimized/1/1/31           2178 ns         2172 ns       322321
BM_BiquadFilterFloatOptimized/1/2/1             737 ns          734 ns       953000
BM_BiquadFilterFloatOptimized/1/2/2            1085 ns         1082 ns       646529
BM_BiquadFilterFloatOptimized/1/2/3            1086 ns         1082 ns       646983
BM_BiquadFilterFloatOptimized/1/2/4            1537 ns         1533 ns       456082
BM_BiquadFilterFloatOptimized/1/2/5            1538 ns         1533 ns       457062
BM_BiquadFilterFloatOptimized/1/2/6            1539 ns         1534 ns       457137
BM_BiquadFilterFloatOptimized/1/2/7            1539 ns         1534 ns       457042
BM_BiquadFilterFloatOptimized/1/2/8            1975 ns         1969 ns       355538
BM_BiquadFilterFloatOptimized/1/2/9            1975 ns         1969 ns       355560
BM_BiquadFilterFloatOptimized/1/2/10           4347 ns         4333 ns       161568
BM_BiquadFilterFloatOptimized/1/2/11           4345 ns         4333 ns       161551
BM_BiquadFilterFloatOptimized/1/2/12           5014 ns         4999 ns       139998
BM_BiquadFilterFloatOptimized/1/2/13           5014 ns         4999 ns       140001
BM_BiquadFilterFloatOptimized/1/2/14           5016 ns         5000 ns       140022
BM_BiquadFilterFloatOptimized/1/2/15           5013 ns         4999 ns       140019
BM_BiquadFilterFloatOptimized/1/2/16           3986 ns         3973 ns       176177
BM_BiquadFilterFloatOptimized/1/2/17           3985 ns         3973 ns       176194
BM_BiquadFilterFloatOptimized/1/2/18           3984 ns         3973 ns       176174
BM_BiquadFilterFloatOptimized/1/2/19           3984 ns         3973 ns       176167
BM_BiquadFilterFloatOptimized/1/2/20           5012 ns         4999 ns       140029
BM_BiquadFilterFloatOptimized/1/2/21           5014 ns         4999 ns       140026
BM_BiquadFilterFloatOptimized/1/2/22           5013 ns         4999 ns       140013
BM_BiquadFilterFloatOptimized/1/2/23           5014 ns         5000 ns       139998
BM_BiquadFilterFloatOptimized/1/2/24           3986 ns         3973 ns       176163
BM_BiquadFilterFloatOptimized/1/2/25           3984 ns         3973 ns       176201
BM_BiquadFilterFloatOptimized/1/2/26           3983 ns         3973 ns       176186
BM_BiquadFilterFloatOptimized/1/2/27           3986 ns         3973 ns       176174
BM_BiquadFilterFloatOptimized/1/2/28           5013 ns         4999 ns       140001
BM_BiquadFilterFloatOptimized/1/2/29           5014 ns         4999 ns       140033
BM_BiquadFilterFloatOptimized/1/2/30           5012 ns         4999 ns       140018
BM_BiquadFilterFloatOptimized/1/2/31           5014 ns         4999 ns       140003
BM_BiquadFilterFloatOptimized/1/3/1            1010 ns         1007 ns       695126
BM_BiquadFilterFloatOptimized/1/3/2            1753 ns         1748 ns       401120
BM_BiquadFilterFloatOptimized/1/3/3            1765 ns         1759 ns       403787
BM_BiquadFilterFloatOptimized/1/3/4            2312 ns         2307 ns       303354
BM_BiquadFilterFloatOptimized/1/3/5            2317 ns         2309 ns       303095
BM_BiquadFilterFloatOptimized/1/3/6            2318 ns         2311 ns       302366
BM_BiquadFilterFloatOptimized/1/3/7            2315 ns         2309 ns       303183
BM_BiquadFilterFloatOptimized/1/3/8            3053 ns         3044 ns       229914
BM_BiquadFilterFloatOptimized/1/3/9            3053 ns         3044 ns       229952
BM_BiquadFilterFloatOptimized/1/3/10           4346 ns         4333 ns       161527
BM_BiquadFilterFloatOptimized/1/3/11           4345 ns         4333 ns       161578
BM_BiquadFilterFloatOptimized/1/3/12           4938 ns         4924 ns       142144
BM_BiquadFilterFloatOptimized/1/3/13           4938 ns         4924 ns       142160
BM_BiquadFilterFloatOptimized/1/3/14           4938 ns         4924 ns       142173
BM_BiquadFilterFloatOptimized/1/3/15           4938 ns         4924 ns       142171
BM_BiquadFilterFloatOptimized/1/3/16           4072 ns         4058 ns       172551
BM_BiquadFilterFloatOptimized/1/3/17           4071 ns         4059 ns       172535
BM_BiquadFilterFloatOptimized/1/3/18           4071 ns         4059 ns       172451
BM_BiquadFilterFloatOptimized/1/3/19           4072 ns         4059 ns       172440
BM_BiquadFilterFloatOptimized/1/3/20           4938 ns         4925 ns       142159
BM_BiquadFilterFloatOptimized/1/3/21           4940 ns         4924 ns       142162
BM_BiquadFilterFloatOptimized/1/3/22           4938 ns         4924 ns       142152
BM_BiquadFilterFloatOptimized/1/3/23           4939 ns         4924 ns       142166
BM_BiquadFilterFloatOptimized/1/3/24           4070 ns         4058 ns       172556
BM_BiquadFilterFloatOptimized/1/3/25           4069 ns         4058 ns       172463
BM_BiquadFilterFloatOptimized/1/3/26           4071 ns         4058 ns       172489
BM_BiquadFilterFloatOptimized/1/3/27           4070 ns         4058 ns       172506
BM_BiquadFilterFloatOptimized/1/3/28           4938 ns         4924 ns       142152
BM_BiquadFilterFloatOptimized/1/3/29           4939 ns         4924 ns       142164
BM_BiquadFilterFloatOptimized/1/3/30           4937 ns         4924 ns       142172
BM_BiquadFilterFloatOptimized/1/3/31           4939 ns         4924 ns       142156
BM_BiquadFilterFloatOptimized/1/4/1             555 ns          553 ns      1264784
BM_BiquadFilterFloatOptimized/1/4/2             736 ns          734 ns       953628
BM_BiquadFilterFloatOptimized/1/4/3             736 ns          734 ns       953966
BM_BiquadFilterFloatOptimized/1/4/4            1357 ns         1353 ns       517294
BM_BiquadFilterFloatOptimized/1/4/5            1357 ns         1353 ns       517252
BM_BiquadFilterFloatOptimized/1/4/6            1357 ns         1353 ns       517358
BM_BiquadFilterFloatOptimized/1/4/7            1357 ns         1353 ns       517367
BM_BiquadFilterFloatOptimized/1/4/8            1902 ns         1896 ns       369039
BM_BiquadFilterFloatOptimized/1/4/9            1903 ns         1897 ns       368999
BM_BiquadFilterFloatOptimized/1/4/10           3984 ns         3972 ns       176223
BM_BiquadFilterFloatOptimized/1/4/11           3985 ns         3972 ns       176227
BM_BiquadFilterFloatOptimized/1/4/12           4996 ns         4982 ns       140498
BM_BiquadFilterFloatOptimized/1/4/13           4996 ns         4982 ns       140514
BM_BiquadFilterFloatOptimized/1/4/14           4995 ns         4982 ns       140497
BM_BiquadFilterFloatOptimized/1/4/15           4995 ns         4982 ns       140514
BM_BiquadFilterFloatOptimized/1/4/16           3984 ns         3973 ns       176199
BM_BiquadFilterFloatOptimized/1/4/17           3984 ns         3973 ns       176183
BM_BiquadFilterFloatOptimized/1/4/18           3985 ns         3973 ns       176198
BM_BiquadFilterFloatOptimized/1/4/19           3986 ns         3973 ns       176194
BM_BiquadFilterFloatOptimized/1/4/20           4998 ns         4984 ns       140422
BM_BiquadFilterFloatOptimized/1/4/21           4997 ns         4982 ns       140519
BM_BiquadFilterFloatOptimized/1/4/22           4995 ns         4982 ns       140514
BM_BiquadFilterFloatOptimized/1/4/23           4996 ns         4982 ns       140516
BM_BiquadFilterFloatOptimized/1/4/24           3984 ns         3973 ns       176184
BM_BiquadFilterFloatOptimized/1/4/25           3983 ns         3972 ns       176191
BM_BiquadFilterFloatOptimized/1/4/26           3985 ns         3973 ns       176189
BM_BiquadFilterFloatOptimized/1/4/27           3985 ns         3973 ns       176195
BM_BiquadFilterFloatOptimized/1/4/28           4996 ns         4982 ns       140504
BM_BiquadFilterFloatOptimized/1/4/29           4996 ns         4982 ns       140513
BM_BiquadFilterFloatOptimized/1/4/30           4995 ns         4982 ns       140510
BM_BiquadFilterFloatOptimized/1/4/31           4997 ns         4982 ns       140504
BM_BiquadFilterFloatNonOptimized/0/1/31        2178 ns         2172 ns       322337
BM_BiquadFilterFloatNonOptimized/0/2/31        4353 ns         4342 ns       161208
BM_BiquadFilterFloatNonOptimized/0/3/31        6529 ns         6509 ns       107546
BM_BiquadFilterFloatNonOptimized/0/4/31        8700 ns         8677 ns        80685
BM_BiquadFilterFloatNonOptimized/0/5/31       10874 ns        10844 ns        64535
BM_BiquadFilterFloatNonOptimized/0/6/31       13072 ns        13030 ns        53723
BM_BiquadFilterFloatNonOptimized/0/7/31       15226 ns        15184 ns        46111
BM_BiquadFilterFloatNonOptimized/0/8/31       17416 ns        17371 ns        40292
BM_BiquadFilterFloatNonOptimized/0/9/31       19595 ns        19545 ns        35814
BM_BiquadFilterFloatNonOptimized/0/10/31      21774 ns        21713 ns        32242
BM_BiquadFilterFloatNonOptimized/0/11/31      23971 ns        23908 ns        29279
BM_BiquadFilterFloatNonOptimized/0/12/31      26170 ns        26092 ns        26825
BM_BiquadFilterFloatNonOptimized/0/13/31      28384 ns        28304 ns        24732
BM_BiquadFilterFloatNonOptimized/0/14/31      30585 ns        30495 ns        22956
BM_BiquadFilterFloatNonOptimized/0/15/31      32811 ns        32724 ns        21391
BM_BiquadFilterFloatNonOptimized/0/16/31      35082 ns        34987 ns        20007
BM_BiquadFilterFloatNonOptimized/0/17/31      37629 ns        37527 ns        18653
BM_BiquadFilterFloatNonOptimized/0/18/31      40442 ns        40328 ns        17366
BM_BiquadFilterFloatNonOptimized/0/19/31      42448 ns        42335 ns        16532
BM_BiquadFilterFloatNonOptimized/0/20/31      45171 ns        45045 ns        15536
BM_BiquadFilterFloatNonOptimized/0/21/31      46966 ns        46835 ns        14950
BM_BiquadFilterFloatNonOptimized/0/22/31      48604 ns        48466 ns        14449
BM_BiquadFilterFloatNonOptimized/0/23/31      50446 ns        50294 ns        13915
BM_BiquadFilterFloatNonOptimized/0/24/31      52667 ns        52495 ns        13339
BM_BiquadFilterDoubleOptimized/0/1/31          2180 ns         2173 ns       322151
BM_BiquadFilterDoubleOptimized/0/2/31          5002 ns         4987 ns       140369
BM_BiquadFilterDoubleOptimized/0/3/31          4919 ns         4906 ns       142292
BM_BiquadFilterDoubleOptimized/0/4/31          5225 ns         5210 ns       134286
BM_BiquadFilterDoubleNonOptimized/0/1/31       2177 ns         2171 ns       322374
BM_BiquadFilterDoubleNonOptimized/0/2/31       4353 ns         4341 ns       161217
BM_BiquadFilterDoubleNonOptimized/0/3/31       6537 ns         6516 ns       107442
BM_BiquadFilterDoubleNonOptimized/0/4/31       8715 ns         8691 ns        80545

 *******************************************************************/

template <typename F>
static void BM_BiquadFilter(benchmark::State& state, bool optimized) {
    bool isSubnormal = (state.range(0) == 1);
    const size_t channelCount = state.range(1);
    const size_t occupancy = state.range(2);

    std::vector<F> input(DATA_SIZE * channelCount);
    std::vector<F> output(DATA_SIZE * channelCount);
    std::array<F, android::audio_utils::kBiquadNumCoefs> coefs;

    // Initialize input buffer and coefs with deterministic pseudo-random values
    std::minstd_rand gen(occupancy);
    const F amplitude = isSubnormal ? std::numeric_limits<F>::min() * 0.1 : 1.;
    std::uniform_real_distribution<> dis(-amplitude, amplitude);
    for (size_t i = 0; i < DATA_SIZE * channelCount; ++i) {
        input[i] = dis(gen);
    }
    for (size_t i = 0; i < coefs.size(); ++i) {
        coefs[i] = (occupancy >> i & 1) * REF_COEFS[i];
    }

    android::audio_utils::BiquadFilter<F> biquadFilter(channelCount, coefs, optimized);

    // Run the test
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(input.data());
        benchmark::DoNotOptimize(output.data());
        biquadFilter.process(output.data(), input.data(), DATA_SIZE);
        benchmark::ClobberMemory();
    }
    state.SetComplexityN(state.range(1));  // O(channelCount)
}

static void BM_BiquadFilterFloatOptimized(benchmark::State& state) {
    BM_BiquadFilter<float>(state, true /* optimized */);
}

static void BM_BiquadFilterFloatNonOptimized(benchmark::State& state) {
    BM_BiquadFilter<float>(state, false /* optimized */);
}

static void BM_BiquadFilterDoubleOptimized(benchmark::State& state) {
    BM_BiquadFilter<double>(state, true /* optimized */);
}

static void BM_BiquadFilterDoubleNonOptimized(benchmark::State& state) {
    BM_BiquadFilter<double>(state, false /* optimized */);
}

static void BiquadFilterQuickArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 24;
    for (int k = 0; k < 1; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            int j = (1 << android::audio_utils::kBiquadNumCoefs) - 1; // Full
            b->Args({k, i, j});
        }
    }
}

static void BiquadFilterFullArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 4;
    for (int k = 0; k < 2; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            for (int j = 1; j < (1 << android::audio_utils::kBiquadNumCoefs); ++j) { // Occupancy
                b->Args({k, i, j});
            }
        }
    }
}

static void BiquadFilterDoubleArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 4;
    for (int k = 0; k < 1; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            int j = (1 << android::audio_utils::kBiquadNumCoefs) - 1; // Full
            b->Args({k, i, j});
        }
    }
}

BENCHMARK(BM_BiquadFilterFloatOptimized)->Apply(BiquadFilterQuickArgs);
BENCHMARK(BM_BiquadFilterFloatOptimized)->Apply(BiquadFilterFullArgs);
// Other tests of interest
BENCHMARK(BM_BiquadFilterFloatNonOptimized)->Apply(BiquadFilterQuickArgs);
BENCHMARK(BM_BiquadFilterDoubleOptimized)->Apply(BiquadFilterDoubleArgs);
BENCHMARK(BM_BiquadFilterDoubleNonOptimized)->Apply(BiquadFilterDoubleArgs);

BENCHMARK_MAIN();
