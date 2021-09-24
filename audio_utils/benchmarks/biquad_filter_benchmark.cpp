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

 Parameterized Test BM_BiquadFilter1D/A
 <A> is 0 or 1 indicating if the input data is subnormal or not.

 Parameterized Test BM_BiquadFilter<TYPE>/A/B/C
 <A> is 0 or 1 indicating if the input data is subnormal or not.
 <B> is the channel count, starting from 1
 <C> indicates the occupancy of the coefficients as a bitmask (1 - 31) representing
     b0, b1, b2, a0, a1.  31 indicates all Biquad coefficients are non-zero.

-----------------------------------------------------------------------------------
Benchmark                                         Time             CPU   Iterations
-----------------------------------------------------------------------------------
BM_BiquadFilter1D/0                             558 ns          556 ns      1258922
BM_BiquadFilter1D/1                             561 ns          560 ns      1251090
BM_BiquadFilterFloatOptimized/0/1/31           2499 ns         2493 ns       280808
BM_BiquadFilterFloatOptimized/0/2/31           3174 ns         3166 ns       221128
BM_BiquadFilterFloatOptimized/0/3/31           3497 ns         3487 ns       200739
BM_BiquadFilterFloatOptimized/0/4/31           3165 ns         3157 ns       221768
BM_BiquadFilterFloatOptimized/0/5/31           3424 ns         3415 ns       204909
BM_BiquadFilterFloatOptimized/0/6/31           3539 ns         3530 ns       198271
BM_BiquadFilterFloatOptimized/0/7/31           4311 ns         4300 ns       162593
BM_BiquadFilterFloatOptimized/0/8/31           3501 ns         3492 ns       200490
BM_BiquadFilterFloatOptimized/0/9/31           4310 ns         4299 ns       162317
BM_BiquadFilterFloatOptimized/0/10/31          4487 ns         4476 ns       156406
BM_BiquadFilterFloatOptimized/0/11/31          5589 ns         5575 ns       125644
BM_BiquadFilterFloatOptimized/0/12/31          4457 ns         4445 ns       157532
BM_BiquadFilterFloatOptimized/0/13/31          5600 ns         5586 ns       125403
BM_BiquadFilterFloatOptimized/0/14/31          5834 ns         5819 ns       120309
BM_BiquadFilterFloatOptimized/0/15/31          7089 ns         7070 ns        98986
BM_BiquadFilterFloatOptimized/0/16/31          5644 ns         5627 ns       124364
BM_BiquadFilterFloatOptimized/0/17/31          8244 ns         8223 ns        85126
BM_BiquadFilterFloatOptimized/0/18/31          8900 ns         8874 ns        78853
BM_BiquadFilterFloatOptimized/0/19/31          9385 ns         9360 ns        74775
BM_BiquadFilterFloatOptimized/0/20/31          8783 ns         8760 ns        79901
BM_BiquadFilterFloatOptimized/0/21/31          9335 ns         9305 ns        75239
BM_BiquadFilterFloatOptimized/0/22/31          9561 ns         9535 ns        73368
BM_BiquadFilterFloatOptimized/0/23/31         10334 ns        10307 ns        67876
BM_BiquadFilterFloatOptimized/0/24/31          9266 ns         9241 ns        75692
BM_BiquadFilterFloatOptimized/0/1/1             557 ns          556 ns      1259656
BM_BiquadFilterFloatOptimized/0/1/2             651 ns          649 ns      1078575
BM_BiquadFilterFloatOptimized/0/1/3             650 ns          648 ns      1079479
BM_BiquadFilterFloatOptimized/0/1/4             805 ns          803 ns       918780
BM_BiquadFilterFloatOptimized/0/1/5             984 ns          981 ns       736887
BM_BiquadFilterFloatOptimized/0/1/6             797 ns          795 ns       882135
BM_BiquadFilterFloatOptimized/0/1/7             792 ns          790 ns       897376
BM_BiquadFilterFloatOptimized/0/1/8            1974 ns         1969 ns       355501
BM_BiquadFilterFloatOptimized/0/1/9            1973 ns         1968 ns       355606
BM_BiquadFilterFloatOptimized/0/1/10           2709 ns         2703 ns       259268
BM_BiquadFilterFloatOptimized/0/1/11           2613 ns         2607 ns       268435
BM_BiquadFilterFloatOptimized/0/1/12           2499 ns         2493 ns       280813
BM_BiquadFilterFloatOptimized/0/1/13           2497 ns         2491 ns       280990
BM_BiquadFilterFloatOptimized/0/1/14           2499 ns         2493 ns       280818
BM_BiquadFilterFloatOptimized/0/1/15           2499 ns         2493 ns       280815
BM_BiquadFilterFloatOptimized/0/1/16           2327 ns         2321 ns       301566
BM_BiquadFilterFloatOptimized/0/1/17           2326 ns         2321 ns       301606
BM_BiquadFilterFloatOptimized/0/1/18           2326 ns         2321 ns       301606
BM_BiquadFilterFloatOptimized/0/1/19           2327 ns         2321 ns       301606
BM_BiquadFilterFloatOptimized/0/1/20           2499 ns         2493 ns       280810
BM_BiquadFilterFloatOptimized/0/1/21           2497 ns         2491 ns       280989
BM_BiquadFilterFloatOptimized/0/1/22           2499 ns         2493 ns       280796
BM_BiquadFilterFloatOptimized/0/1/23           2499 ns         2493 ns       280807
BM_BiquadFilterFloatOptimized/0/1/24           2327 ns         2321 ns       301596
BM_BiquadFilterFloatOptimized/0/1/25           2327 ns         2321 ns       301600
BM_BiquadFilterFloatOptimized/0/1/26           2327 ns         2321 ns       301597
BM_BiquadFilterFloatOptimized/0/1/27           2327 ns         2321 ns       301588
BM_BiquadFilterFloatOptimized/0/1/28           2500 ns         2493 ns       280761
BM_BiquadFilterFloatOptimized/0/1/29           2499 ns         2492 ns       280951
BM_BiquadFilterFloatOptimized/0/1/30           2500 ns         2493 ns       280787
BM_BiquadFilterFloatOptimized/0/1/31           2500 ns         2493 ns       280808
BM_BiquadFilterFloatOptimized/0/2/1             440 ns          439 ns      1595281
BM_BiquadFilterFloatOptimized/0/2/2             633 ns          631 ns      1108368
BM_BiquadFilterFloatOptimized/0/2/3             633 ns          631 ns      1108778
BM_BiquadFilterFloatOptimized/0/2/4            1523 ns         1518 ns       461120
BM_BiquadFilterFloatOptimized/0/2/5            1523 ns         1518 ns       461075
BM_BiquadFilterFloatOptimized/0/2/6            1522 ns         1518 ns       461059
BM_BiquadFilterFloatOptimized/0/2/7            1523 ns         1518 ns       461068
BM_BiquadFilterFloatOptimized/0/2/8            2854 ns         2845 ns       248471
BM_BiquadFilterFloatOptimized/0/2/9            2809 ns         2800 ns       250019
BM_BiquadFilterFloatOptimized/0/2/10           4412 ns         4398 ns       159164
BM_BiquadFilterFloatOptimized/0/2/11           4413 ns         4399 ns       159138
BM_BiquadFilterFloatOptimized/0/2/12           3177 ns         3167 ns       221023
BM_BiquadFilterFloatOptimized/0/2/13           3164 ns         3154 ns       221972
BM_BiquadFilterFloatOptimized/0/2/14           3225 ns         3211 ns       217654
BM_BiquadFilterFloatOptimized/0/2/15           3178 ns         3167 ns       221055
BM_BiquadFilterFloatOptimized/0/2/16           3726 ns         3714 ns       188557
BM_BiquadFilterFloatOptimized/0/2/17           3726 ns         3716 ns       188151
BM_BiquadFilterFloatOptimized/0/2/18           3734 ns         3721 ns       188243
BM_BiquadFilterFloatOptimized/0/2/19           3723 ns         3710 ns       188560
BM_BiquadFilterFloatOptimized/0/2/20           3178 ns         3167 ns       221083
BM_BiquadFilterFloatOptimized/0/2/21           3163 ns         3154 ns       221947
BM_BiquadFilterFloatOptimized/0/2/22           3224 ns         3214 ns       218373
BM_BiquadFilterFloatOptimized/0/2/23           3177 ns         3167 ns       221028
BM_BiquadFilterFloatOptimized/0/2/24           3727 ns         3714 ns       188443
BM_BiquadFilterFloatOptimized/0/2/25           3735 ns         3721 ns       188131
BM_BiquadFilterFloatOptimized/0/2/26           3732 ns         3719 ns       188374
BM_BiquadFilterFloatOptimized/0/2/27           3721 ns         3710 ns       188619
BM_BiquadFilterFloatOptimized/0/2/28           3176 ns         3167 ns       221067
BM_BiquadFilterFloatOptimized/0/2/29           3164 ns         3154 ns       221953
BM_BiquadFilterFloatOptimized/0/2/30           3225 ns         3214 ns       217988
BM_BiquadFilterFloatOptimized/0/2/31           3176 ns         3167 ns       221015
BM_BiquadFilterFloatOptimized/0/3/1             877 ns          874 ns       800012
BM_BiquadFilterFloatOptimized/0/3/2            1218 ns         1214 ns       576381
BM_BiquadFilterFloatOptimized/0/3/3            1217 ns         1214 ns       577767
BM_BiquadFilterFloatOptimized/0/3/4            2281 ns         2274 ns       307760
BM_BiquadFilterFloatOptimized/0/3/5            2285 ns         2278 ns       307313
BM_BiquadFilterFloatOptimized/0/3/6            2285 ns         2278 ns       307254
BM_BiquadFilterFloatOptimized/0/3/7            2280 ns         2273 ns       307865
BM_BiquadFilterFloatOptimized/0/3/8            2966 ns         2957 ns       236544
BM_BiquadFilterFloatOptimized/0/3/9            2945 ns         2936 ns       238459
BM_BiquadFilterFloatOptimized/0/3/10           4613 ns         4597 ns       152280
BM_BiquadFilterFloatOptimized/0/3/11           4612 ns         4597 ns       152296
BM_BiquadFilterFloatOptimized/0/3/12           3499 ns         3489 ns       200637
BM_BiquadFilterFloatOptimized/0/3/13           3498 ns         3486 ns       200771
BM_BiquadFilterFloatOptimized/0/3/14           3569 ns         3557 ns       196782
BM_BiquadFilterFloatOptimized/0/3/15           3500 ns         3489 ns       200662
BM_BiquadFilterFloatOptimized/0/3/16           3809 ns         3797 ns       184356
BM_BiquadFilterFloatOptimized/0/3/17           3817 ns         3804 ns       184009
BM_BiquadFilterFloatOptimized/0/3/18           3818 ns         3804 ns       183988
BM_BiquadFilterFloatOptimized/0/3/19           3809 ns         3797 ns       184373
BM_BiquadFilterFloatOptimized/0/3/20           3501 ns         3489 ns       200657
BM_BiquadFilterFloatOptimized/0/3/21           3497 ns         3486 ns       200769
BM_BiquadFilterFloatOptimized/0/3/22           3567 ns         3556 ns       196867
BM_BiquadFilterFloatOptimized/0/3/23           3500 ns         3489 ns       200647
BM_BiquadFilterFloatOptimized/0/3/24           3808 ns         3796 ns       184354
BM_BiquadFilterFloatOptimized/0/3/25           3816 ns         3805 ns       184002
BM_BiquadFilterFloatOptimized/0/3/26           3816 ns         3804 ns       184006
BM_BiquadFilterFloatOptimized/0/3/27           3809 ns         3797 ns       184416
BM_BiquadFilterFloatOptimized/0/3/28           3500 ns         3488 ns       200657
BM_BiquadFilterFloatOptimized/0/3/29           3498 ns         3486 ns       200786
BM_BiquadFilterFloatOptimized/0/3/30           3568 ns         3557 ns       196887
BM_BiquadFilterFloatOptimized/0/3/31           3500 ns         3488 ns       200663
BM_BiquadFilterFloatOptimized/0/4/1             558 ns          556 ns      1257930
BM_BiquadFilterFloatOptimized/0/4/2             652 ns          650 ns      1076427
BM_BiquadFilterFloatOptimized/0/4/3             651 ns          648 ns      1079429
BM_BiquadFilterFloatOptimized/0/4/4             831 ns          829 ns       844257
BM_BiquadFilterFloatOptimized/0/4/5             829 ns          826 ns       847191
BM_BiquadFilterFloatOptimized/0/4/6             829 ns          826 ns       847010
BM_BiquadFilterFloatOptimized/0/4/7             832 ns          829 ns       843914
BM_BiquadFilterFloatOptimized/0/4/8            1881 ns         1875 ns       373166
BM_BiquadFilterFloatOptimized/0/4/9            1910 ns         1904 ns       367626
BM_BiquadFilterFloatOptimized/0/4/10           2247 ns         2239 ns       312581
BM_BiquadFilterFloatOptimized/0/4/11           2246 ns         2238 ns       312874
BM_BiquadFilterFloatOptimized/0/4/12           3170 ns         3158 ns       221666
BM_BiquadFilterFloatOptimized/0/4/13           3159 ns         3150 ns       222273
BM_BiquadFilterFloatOptimized/0/4/14           3149 ns         3139 ns       222959
BM_BiquadFilterFloatOptimized/0/4/15           3168 ns         3158 ns       221668
BM_BiquadFilterFloatOptimized/0/4/16           2278 ns         2271 ns       308250
BM_BiquadFilterFloatOptimized/0/4/17           2280 ns         2273 ns       308036
BM_BiquadFilterFloatOptimized/0/4/18           2280 ns         2273 ns       308016
BM_BiquadFilterFloatOptimized/0/4/19           2278 ns         2271 ns       308301
BM_BiquadFilterFloatOptimized/0/4/20           3168 ns         3158 ns       221671
BM_BiquadFilterFloatOptimized/0/4/21           3159 ns         3150 ns       222270
BM_BiquadFilterFloatOptimized/0/4/22           3149 ns         3139 ns       223010
BM_BiquadFilterFloatOptimized/0/4/23           3168 ns         3158 ns       221652
BM_BiquadFilterFloatOptimized/0/4/24           2279 ns         2271 ns       308191
BM_BiquadFilterFloatOptimized/0/4/25           2281 ns         2273 ns       307942
BM_BiquadFilterFloatOptimized/0/4/26           2280 ns         2272 ns       308012
BM_BiquadFilterFloatOptimized/0/4/27           2279 ns         2271 ns       308357
BM_BiquadFilterFloatOptimized/0/4/28           3169 ns         3158 ns       221700
BM_BiquadFilterFloatOptimized/0/4/29           3159 ns         3149 ns       222286
BM_BiquadFilterFloatOptimized/0/4/30           3149 ns         3139 ns       222997
BM_BiquadFilterFloatOptimized/0/4/31           3168 ns         3158 ns       221672
BM_BiquadFilterFloatOptimized/1/1/1             558 ns          556 ns      1259230
BM_BiquadFilterFloatOptimized/1/1/2             651 ns          649 ns      1078239
BM_BiquadFilterFloatOptimized/1/1/3             651 ns          649 ns      1078731
BM_BiquadFilterFloatOptimized/1/1/4             771 ns          768 ns       898703
BM_BiquadFilterFloatOptimized/1/1/5            1020 ns         1017 ns       712070
BM_BiquadFilterFloatOptimized/1/1/6             796 ns          794 ns       867607
BM_BiquadFilterFloatOptimized/1/1/7             816 ns          814 ns       895946
BM_BiquadFilterFloatOptimized/1/1/8            1976 ns         1970 ns       355331
BM_BiquadFilterFloatOptimized/1/1/9            1976 ns         1969 ns       355435
BM_BiquadFilterFloatOptimized/1/1/10           2709 ns         2700 ns       259919
BM_BiquadFilterFloatOptimized/1/1/11           2617 ns         2608 ns       268279
BM_BiquadFilterFloatOptimized/1/1/12           2501 ns         2494 ns       280784
BM_BiquadFilterFloatOptimized/1/1/13           2500 ns         2492 ns       280890
BM_BiquadFilterFloatOptimized/1/1/14           2502 ns         2493 ns       280685
BM_BiquadFilterFloatOptimized/1/1/15           2502 ns         2493 ns       280729
BM_BiquadFilterFloatOptimized/1/1/16           2329 ns         2322 ns       301460
BM_BiquadFilterFloatOptimized/1/1/17           2330 ns         2322 ns       301456
BM_BiquadFilterFloatOptimized/1/1/18           2329 ns         2322 ns       301447
BM_BiquadFilterFloatOptimized/1/1/19           2329 ns         2322 ns       301456
BM_BiquadFilterFloatOptimized/1/1/20           2502 ns         2494 ns       280714
BM_BiquadFilterFloatOptimized/1/1/21           2501 ns         2492 ns       280834
BM_BiquadFilterFloatOptimized/1/1/22           2502 ns         2494 ns       280713
BM_BiquadFilterFloatOptimized/1/1/23           2502 ns         2494 ns       280691
BM_BiquadFilterFloatOptimized/1/1/24           2329 ns         2322 ns       301435
BM_BiquadFilterFloatOptimized/1/1/25           2330 ns         2322 ns       301438
BM_BiquadFilterFloatOptimized/1/1/26           2329 ns         2322 ns       301470
BM_BiquadFilterFloatOptimized/1/1/27           2330 ns         2322 ns       301493
BM_BiquadFilterFloatOptimized/1/1/28           2502 ns         2493 ns       280702
BM_BiquadFilterFloatOptimized/1/1/29           2500 ns         2492 ns       280940
BM_BiquadFilterFloatOptimized/1/1/30           2502 ns         2494 ns       280740
BM_BiquadFilterFloatOptimized/1/1/31           2502 ns         2494 ns       280719
BM_BiquadFilterFloatOptimized/1/2/1             440 ns          439 ns      1595119
BM_BiquadFilterFloatOptimized/1/2/2             634 ns          631 ns      1109077
BM_BiquadFilterFloatOptimized/1/2/3             633 ns          631 ns      1108421
BM_BiquadFilterFloatOptimized/1/2/4            1523 ns         1518 ns       460928
BM_BiquadFilterFloatOptimized/1/2/5            1524 ns         1518 ns       461034
BM_BiquadFilterFloatOptimized/1/2/6            1524 ns         1518 ns       460936
BM_BiquadFilterFloatOptimized/1/2/7            1524 ns         1519 ns       460956
BM_BiquadFilterFloatOptimized/1/2/8            2871 ns         2862 ns       243633
BM_BiquadFilterFloatOptimized/1/2/9            2808 ns         2800 ns       249997
BM_BiquadFilterFloatOptimized/1/2/10           4412 ns         4397 ns       159195
BM_BiquadFilterFloatOptimized/1/2/11           4412 ns         4398 ns       159154
BM_BiquadFilterFloatOptimized/1/2/12           3177 ns         3167 ns       221084
BM_BiquadFilterFloatOptimized/1/2/13           3164 ns         3154 ns       221939
BM_BiquadFilterFloatOptimized/1/2/14           3217 ns         3210 ns       218007
BM_BiquadFilterFloatOptimized/1/2/15           3177 ns         3167 ns       221047
BM_BiquadFilterFloatOptimized/1/2/16           3726 ns         3713 ns       188559
BM_BiquadFilterFloatOptimized/1/2/17           3733 ns         3720 ns       188289
BM_BiquadFilterFloatOptimized/1/2/18           3733 ns         3721 ns       188122
BM_BiquadFilterFloatOptimized/1/2/19           3724 ns         3712 ns       188522
BM_BiquadFilterFloatOptimized/1/2/20           3177 ns         3167 ns       221061
BM_BiquadFilterFloatOptimized/1/2/21           3164 ns         3154 ns       221952
BM_BiquadFilterFloatOptimized/1/2/22           3224 ns         3213 ns       217980
BM_BiquadFilterFloatOptimized/1/2/23           3178 ns         3167 ns       221046
BM_BiquadFilterFloatOptimized/1/2/24           3726 ns         3714 ns       188525
BM_BiquadFilterFloatOptimized/1/2/25           3732 ns         3720 ns       188234
BM_BiquadFilterFloatOptimized/1/2/26           3732 ns         3719 ns       188156
BM_BiquadFilterFloatOptimized/1/2/27           3726 ns         3714 ns       188613
BM_BiquadFilterFloatOptimized/1/2/28           3177 ns         3167 ns       221042
BM_BiquadFilterFloatOptimized/1/2/29           3164 ns         3154 ns       221970
BM_BiquadFilterFloatOptimized/1/2/30           3226 ns         3215 ns       217798
BM_BiquadFilterFloatOptimized/1/2/31           3178 ns         3167 ns       221042
BM_BiquadFilterFloatOptimized/1/3/1             885 ns          882 ns       795133
BM_BiquadFilterFloatOptimized/1/3/2            1219 ns         1214 ns       576293
BM_BiquadFilterFloatOptimized/1/3/3            1218 ns         1214 ns       576722
BM_BiquadFilterFloatOptimized/1/3/4            2282 ns         2274 ns       307745
BM_BiquadFilterFloatOptimized/1/3/5            2286 ns         2278 ns       307324
BM_BiquadFilterFloatOptimized/1/3/6            2286 ns         2278 ns       307308
BM_BiquadFilterFloatOptimized/1/3/7            2282 ns         2274 ns       307912
BM_BiquadFilterFloatOptimized/1/3/8            2962 ns         2952 ns       237180
BM_BiquadFilterFloatOptimized/1/3/9            2946 ns         2935 ns       238462
BM_BiquadFilterFloatOptimized/1/3/10           4612 ns         4597 ns       152246
BM_BiquadFilterFloatOptimized/1/3/11           4613 ns         4596 ns       152286
BM_BiquadFilterFloatOptimized/1/3/12           3501 ns         3489 ns       200662
BM_BiquadFilterFloatOptimized/1/3/13           3497 ns         3486 ns       200784
BM_BiquadFilterFloatOptimized/1/3/14           3569 ns         3557 ns       196804
BM_BiquadFilterFloatOptimized/1/3/15           3499 ns         3488 ns       200661
BM_BiquadFilterFloatOptimized/1/3/16           3809 ns         3797 ns       184350
BM_BiquadFilterFloatOptimized/1/3/17           3816 ns         3804 ns       184028
BM_BiquadFilterFloatOptimized/1/3/18           3815 ns         3804 ns       184008
BM_BiquadFilterFloatOptimized/1/3/19           3808 ns         3796 ns       184333
BM_BiquadFilterFloatOptimized/1/3/20           3502 ns         3489 ns       200636
BM_BiquadFilterFloatOptimized/1/3/21           3499 ns         3486 ns       200768
BM_BiquadFilterFloatOptimized/1/3/22           3569 ns         3557 ns       196840
BM_BiquadFilterFloatOptimized/1/3/23           3501 ns         3488 ns       200657
BM_BiquadFilterFloatOptimized/1/3/24           3807 ns         3796 ns       184403
BM_BiquadFilterFloatOptimized/1/3/25           3816 ns         3804 ns       184040
BM_BiquadFilterFloatOptimized/1/3/26           3816 ns         3804 ns       184021
BM_BiquadFilterFloatOptimized/1/3/27           3808 ns         3796 ns       184385
BM_BiquadFilterFloatOptimized/1/3/28           3500 ns         3488 ns       200666
BM_BiquadFilterFloatOptimized/1/3/29           3497 ns         3485 ns       200811
BM_BiquadFilterFloatOptimized/1/3/30           3571 ns         3558 ns       196974
BM_BiquadFilterFloatOptimized/1/3/31           3499 ns         3488 ns       200710
BM_BiquadFilterFloatOptimized/1/4/1             558 ns          556 ns      1259007
BM_BiquadFilterFloatOptimized/1/4/2             652 ns          650 ns      1076207
BM_BiquadFilterFloatOptimized/1/4/3             650 ns          648 ns      1079464
BM_BiquadFilterFloatOptimized/1/4/4             831 ns          828 ns       847251
BM_BiquadFilterFloatOptimized/1/4/5             829 ns          826 ns       847543
BM_BiquadFilterFloatOptimized/1/4/6             829 ns          826 ns       847037
BM_BiquadFilterFloatOptimized/1/4/7             832 ns          829 ns       844307
BM_BiquadFilterFloatOptimized/1/4/8            1879 ns         1873 ns       378908
BM_BiquadFilterFloatOptimized/1/4/9            1910 ns         1905 ns       367554
BM_BiquadFilterFloatOptimized/1/4/10           2246 ns         2240 ns       312471
BM_BiquadFilterFloatOptimized/1/4/11           2244 ns         2238 ns       312719
BM_BiquadFilterFloatOptimized/1/4/12           3167 ns         3157 ns       221689
BM_BiquadFilterFloatOptimized/1/4/13           3159 ns         3149 ns       222292
BM_BiquadFilterFloatOptimized/1/4/14           3148 ns         3138 ns       223041
BM_BiquadFilterFloatOptimized/1/4/15           3167 ns         3157 ns       221705
BM_BiquadFilterFloatOptimized/1/4/16           2278 ns         2271 ns       308275
BM_BiquadFilterFloatOptimized/1/4/17           2280 ns         2273 ns       308050
BM_BiquadFilterFloatOptimized/1/4/18           2280 ns         2272 ns       307994
BM_BiquadFilterFloatOptimized/1/4/19           2278 ns         2270 ns       308324
BM_BiquadFilterFloatOptimized/1/4/20           3168 ns         3157 ns       221734
BM_BiquadFilterFloatOptimized/1/4/21           3159 ns         3149 ns       222273
BM_BiquadFilterFloatOptimized/1/4/22           3148 ns         3139 ns       222991
BM_BiquadFilterFloatOptimized/1/4/23           3166 ns         3157 ns       221723
BM_BiquadFilterFloatOptimized/1/4/24           2278 ns         2271 ns       308395
BM_BiquadFilterFloatOptimized/1/4/25           2279 ns         2272 ns       308055
BM_BiquadFilterFloatOptimized/1/4/26           2280 ns         2272 ns       308098
BM_BiquadFilterFloatOptimized/1/4/27           2278 ns         2271 ns       308274
BM_BiquadFilterFloatOptimized/1/4/28           3168 ns         3157 ns       221710
BM_BiquadFilterFloatOptimized/1/4/29           3158 ns         3149 ns       222311
BM_BiquadFilterFloatOptimized/1/4/30           3148 ns         3138 ns       223009
BM_BiquadFilterFloatOptimized/1/4/31           3167 ns         3157 ns       221723
BM_BiquadFilterFloatNonOptimized/0/1/31        2500 ns         2493 ns       280839
BM_BiquadFilterFloatNonOptimized/0/2/31        4996 ns         4983 ns       140491
BM_BiquadFilterFloatNonOptimized/0/3/31        7491 ns         7468 ns        93734
BM_BiquadFilterFloatNonOptimized/0/4/31        9988 ns         9955 ns        70314
BM_BiquadFilterFloatNonOptimized/0/5/31       12475 ns        12440 ns        56266
BM_BiquadFilterFloatNonOptimized/0/6/31       14977 ns        14927 ns        46888
BM_BiquadFilterFloatNonOptimized/0/7/31       17540 ns        17486 ns        40039
BM_BiquadFilterFloatNonOptimized/0/8/31       19997 ns        19937 ns        35114
BM_BiquadFilterFloatNonOptimized/0/9/31       22510 ns        22444 ns        31185
BM_BiquadFilterFloatNonOptimized/0/10/31      25029 ns        24949 ns        28059
BM_BiquadFilterFloatNonOptimized/0/11/31      27520 ns        27436 ns        25514
BM_BiquadFilterFloatNonOptimized/0/12/31      30048 ns        29959 ns        23368
BM_BiquadFilterFloatNonOptimized/0/13/31      32524 ns        32428 ns        21586
BM_BiquadFilterFloatNonOptimized/0/14/31      35051 ns        34949 ns        20029
BM_BiquadFilterFloatNonOptimized/0/15/31      37546 ns        37436 ns        18697
BM_BiquadFilterFloatNonOptimized/0/16/31      40115 ns        39978 ns        17510
BM_BiquadFilterFloatNonOptimized/0/17/31      42624 ns        42492 ns        16473
BM_BiquadFilterFloatNonOptimized/0/18/31      45142 ns        45008 ns        15550
BM_BiquadFilterFloatNonOptimized/0/19/31      47667 ns        47508 ns        14732
BM_BiquadFilterFloatNonOptimized/0/20/31      50150 ns        50005 ns        13999
BM_BiquadFilterFloatNonOptimized/0/21/31      52661 ns        52492 ns        13336
BM_BiquadFilterFloatNonOptimized/0/22/31      55160 ns        54977 ns        12732
BM_BiquadFilterFloatNonOptimized/0/23/31      57717 ns        57556 ns        12194
BM_BiquadFilterFloatNonOptimized/0/24/31      60105 ns        59986 ns        11684
BM_BiquadFilterDoubleOptimized/0/1/31          2498 ns         2491 ns       281105
BM_BiquadFilterDoubleOptimized/0/2/31          3123 ns         3112 ns       224898
BM_BiquadFilterDoubleOptimized/0/3/31          3435 ns         3425 ns       204393
BM_BiquadFilterDoubleOptimized/0/4/31          3567 ns         3556 ns       196854
BM_BiquadFilterDoubleNonOptimized/0/1/31       2498 ns         2490 ns       281119
BM_BiquadFilterDoubleNonOptimized/0/2/31       5019 ns         5004 ns       100000
BM_BiquadFilterDoubleNonOptimized/0/3/31       7500 ns         7478 ns        93607
BM_BiquadFilterDoubleNonOptimized/0/4/31      10010 ns         9981 ns        70129

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
