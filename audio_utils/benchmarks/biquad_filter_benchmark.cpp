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
BM_BiquadFilter1D/0                             559 ns          558 ns      1254596
BM_BiquadFilter1D/1                             563 ns          561 ns      1246670
BM_BiquadFilterFloatOptimized/0/1/31           2263 ns         2257 ns       310326
BM_BiquadFilterFloatOptimized/0/2/31           2471 ns         2465 ns       289044
BM_BiquadFilterFloatOptimized/0/3/31           2827 ns         2818 ns       248148
BM_BiquadFilterFloatOptimized/0/4/31           2443 ns         2436 ns       287402
BM_BiquadFilterFloatOptimized/0/5/31           2843 ns         2836 ns       246912
BM_BiquadFilterFloatOptimized/0/6/31           3188 ns         3180 ns       220494
BM_BiquadFilterFloatOptimized/0/7/31           3910 ns         3898 ns       179652
BM_BiquadFilterFloatOptimized/0/8/31           3167 ns         3157 ns       221764
BM_BiquadFilterFloatOptimized/0/9/31           3947 ns         3936 ns       177855
BM_BiquadFilterFloatOptimized/0/10/31          4204 ns         4193 ns       166963
BM_BiquadFilterFloatOptimized/0/11/31          5313 ns         5298 ns       131865
BM_BiquadFilterFloatOptimized/0/12/31          4205 ns         4193 ns       166925
BM_BiquadFilterFloatOptimized/0/13/31          5303 ns         5289 ns       132321
BM_BiquadFilterFloatOptimized/0/14/31          5501 ns         5487 ns       127623
BM_BiquadFilterFloatOptimized/0/15/31          6572 ns         6551 ns       106827
BM_BiquadFilterFloatOptimized/0/16/31          5438 ns         5426 ns       129016
BM_BiquadFilterFloatOptimized/0/17/31          7729 ns         7708 ns        90812
BM_BiquadFilterFloatOptimized/0/18/31          8007 ns         7984 ns        87910
BM_BiquadFilterFloatOptimized/0/19/31          8586 ns         8560 ns        81801
BM_BiquadFilterFloatOptimized/0/20/31          7811 ns         7789 ns        89871
BM_BiquadFilterFloatOptimized/0/21/31          8714 ns         8691 ns        80518
BM_BiquadFilterFloatOptimized/0/22/31          8851 ns         8820 ns        79287
BM_BiquadFilterFloatOptimized/0/23/31          9606 ns         9576 ns        73108
BM_BiquadFilterFloatOptimized/0/24/31          8676 ns         8647 ns        80944
BM_BiquadFilterFloatOptimized/0/1/1             559 ns          557 ns      1255379
BM_BiquadFilterFloatOptimized/0/1/2             649 ns          647 ns      1081153
BM_BiquadFilterFloatOptimized/0/1/3             649 ns          647 ns      1081862
BM_BiquadFilterFloatOptimized/0/1/4             848 ns          846 ns       827513
BM_BiquadFilterFloatOptimized/0/1/5             847 ns          844 ns       829861
BM_BiquadFilterFloatOptimized/0/1/6             844 ns          841 ns       829051
BM_BiquadFilterFloatOptimized/0/1/7             848 ns          845 ns       827492
BM_BiquadFilterFloatOptimized/0/1/8            2289 ns         2282 ns       307077
BM_BiquadFilterFloatOptimized/0/1/9            2296 ns         2288 ns       303532
BM_BiquadFilterFloatOptimized/0/1/10           2305 ns         2299 ns       304972
BM_BiquadFilterFloatOptimized/0/1/11           2287 ns         2281 ns       307199
BM_BiquadFilterFloatOptimized/0/1/12           2262 ns         2256 ns       310300
BM_BiquadFilterFloatOptimized/0/1/13           2263 ns         2257 ns       310400
BM_BiquadFilterFloatOptimized/0/1/14           2262 ns         2257 ns       310236
BM_BiquadFilterFloatOptimized/0/1/15           2262 ns         2256 ns       309767
BM_BiquadFilterFloatOptimized/0/1/16           2262 ns         2256 ns       310116
BM_BiquadFilterFloatOptimized/0/1/17           2262 ns         2256 ns       310345
BM_BiquadFilterFloatOptimized/0/1/18           2261 ns         2255 ns       310106
BM_BiquadFilterFloatOptimized/0/1/19           2263 ns         2256 ns       310242
BM_BiquadFilterFloatOptimized/0/1/20           2262 ns         2255 ns       310131
BM_BiquadFilterFloatOptimized/0/1/21           2262 ns         2256 ns       310250
BM_BiquadFilterFloatOptimized/0/1/22           2264 ns         2257 ns       310329
BM_BiquadFilterFloatOptimized/0/1/23           2264 ns         2257 ns       310011
BM_BiquadFilterFloatOptimized/0/1/24           2264 ns         2258 ns       310008
BM_BiquadFilterFloatOptimized/0/1/25           2263 ns         2256 ns       310203
BM_BiquadFilterFloatOptimized/0/1/26           2264 ns         2258 ns       310016
BM_BiquadFilterFloatOptimized/0/1/27           2264 ns         2258 ns       310046
BM_BiquadFilterFloatOptimized/0/1/28           2263 ns         2257 ns       310325
BM_BiquadFilterFloatOptimized/0/1/29           2264 ns         2257 ns       310048
BM_BiquadFilterFloatOptimized/0/1/30           2265 ns         2258 ns       310008
BM_BiquadFilterFloatOptimized/0/1/31           2263 ns         2257 ns       310046
BM_BiquadFilterFloatOptimized/0/2/1             613 ns          611 ns      1143690
BM_BiquadFilterFloatOptimized/0/2/2             801 ns          799 ns       877788
BM_BiquadFilterFloatOptimized/0/2/3             799 ns          797 ns       875867
BM_BiquadFilterFloatOptimized/0/2/4            1507 ns         1502 ns       465824
BM_BiquadFilterFloatOptimized/0/2/5            1505 ns         1501 ns       466685
BM_BiquadFilterFloatOptimized/0/2/6            1500 ns         1496 ns       468398
BM_BiquadFilterFloatOptimized/0/2/7            1505 ns         1501 ns       466086
BM_BiquadFilterFloatOptimized/0/2/8            2237 ns         2231 ns       313969
BM_BiquadFilterFloatOptimized/0/2/9            2233 ns         2227 ns       314388
BM_BiquadFilterFloatOptimized/0/2/10           2234 ns         2228 ns       314181
BM_BiquadFilterFloatOptimized/0/2/11           2236 ns         2230 ns       313480
BM_BiquadFilterFloatOptimized/0/2/12           2474 ns         2467 ns       287311
BM_BiquadFilterFloatOptimized/0/2/13           2448 ns         2441 ns       284113
BM_BiquadFilterFloatOptimized/0/2/14           2512 ns         2504 ns       283932
BM_BiquadFilterFloatOptimized/0/2/15           2448 ns         2442 ns       285294
BM_BiquadFilterFloatOptimized/0/2/16           2459 ns         2451 ns       283455
BM_BiquadFilterFloatOptimized/0/2/17           2466 ns         2459 ns       282706
BM_BiquadFilterFloatOptimized/0/2/18           2493 ns         2486 ns       279019
BM_BiquadFilterFloatOptimized/0/2/19           2453 ns         2446 ns       285647
BM_BiquadFilterFloatOptimized/0/2/20           2475 ns         2468 ns       285345
BM_BiquadFilterFloatOptimized/0/2/21           2506 ns         2499 ns       286030
BM_BiquadFilterFloatOptimized/0/2/22           2512 ns         2505 ns       280230
BM_BiquadFilterFloatOptimized/0/2/23           2466 ns         2458 ns       284371
BM_BiquadFilterFloatOptimized/0/2/24           2454 ns         2446 ns       286968
BM_BiquadFilterFloatOptimized/0/2/25           2478 ns         2470 ns       284297
BM_BiquadFilterFloatOptimized/0/2/26           2509 ns         2501 ns       281390
BM_BiquadFilterFloatOptimized/0/2/27           2477 ns         2469 ns       282065
BM_BiquadFilterFloatOptimized/0/2/28           2484 ns         2476 ns       284814
BM_BiquadFilterFloatOptimized/0/2/29           2443 ns         2435 ns       283155
BM_BiquadFilterFloatOptimized/0/2/30           2488 ns         2481 ns       283433
BM_BiquadFilterFloatOptimized/0/2/31           2452 ns         2444 ns       286627
BM_BiquadFilterFloatOptimized/0/3/1            1015 ns         1012 ns       691448
BM_BiquadFilterFloatOptimized/0/3/2            1362 ns         1357 ns       516073
BM_BiquadFilterFloatOptimized/0/3/3            1358 ns         1354 ns       516419
BM_BiquadFilterFloatOptimized/0/3/4            2305 ns         2298 ns       304561
BM_BiquadFilterFloatOptimized/0/3/5            2303 ns         2296 ns       304712
BM_BiquadFilterFloatOptimized/0/3/6            2292 ns         2285 ns       306149
BM_BiquadFilterFloatOptimized/0/3/7            2304 ns         2297 ns       304959
BM_BiquadFilterFloatOptimized/0/3/8            2318 ns         2311 ns       301399
BM_BiquadFilterFloatOptimized/0/3/9            2310 ns         2303 ns       303435
BM_BiquadFilterFloatOptimized/0/3/10           2313 ns         2306 ns       303399
BM_BiquadFilterFloatOptimized/0/3/11           2318 ns         2311 ns       302755
BM_BiquadFilterFloatOptimized/0/3/12           2831 ns         2823 ns       247948
BM_BiquadFilterFloatOptimized/0/3/13           2830 ns         2822 ns       248677
BM_BiquadFilterFloatOptimized/0/3/14           2850 ns         2842 ns       246677
BM_BiquadFilterFloatOptimized/0/3/15           2827 ns         2819 ns       248340
BM_BiquadFilterFloatOptimized/0/3/16           2834 ns         2825 ns       247686
BM_BiquadFilterFloatOptimized/0/3/17           2828 ns         2819 ns       247784
BM_BiquadFilterFloatOptimized/0/3/18           2842 ns         2833 ns       248007
BM_BiquadFilterFloatOptimized/0/3/19           2843 ns         2834 ns       248472
BM_BiquadFilterFloatOptimized/0/3/20           2829 ns         2820 ns       248677
BM_BiquadFilterFloatOptimized/0/3/21           2833 ns         2824 ns       248061
BM_BiquadFilterFloatOptimized/0/3/22           2849 ns         2841 ns       246611
BM_BiquadFilterFloatOptimized/0/3/23           2827 ns         2818 ns       247863
BM_BiquadFilterFloatOptimized/0/3/24           2830 ns         2820 ns       247550
BM_BiquadFilterFloatOptimized/0/3/25           2836 ns         2827 ns       248435
BM_BiquadFilterFloatOptimized/0/3/26           2859 ns         2850 ns       245727
BM_BiquadFilterFloatOptimized/0/3/27           2828 ns         2818 ns       247914
BM_BiquadFilterFloatOptimized/0/3/28           2818 ns         2809 ns       248637
BM_BiquadFilterFloatOptimized/0/3/29           2820 ns         2810 ns       249170
BM_BiquadFilterFloatOptimized/0/3/30           2858 ns         2849 ns       245760
BM_BiquadFilterFloatOptimized/0/3/31           2832 ns         2823 ns       247969
BM_BiquadFilterFloatOptimized/0/4/1             650 ns          648 ns      1080613
BM_BiquadFilterFloatOptimized/0/4/2             808 ns          805 ns       868597
BM_BiquadFilterFloatOptimized/0/4/3             809 ns          807 ns       867460
BM_BiquadFilterFloatOptimized/0/4/4             844 ns          841 ns       831865
BM_BiquadFilterFloatOptimized/0/4/5             845 ns          842 ns       833152
BM_BiquadFilterFloatOptimized/0/4/6             844 ns          842 ns       831276
BM_BiquadFilterFloatOptimized/0/4/7             843 ns          841 ns       831231
BM_BiquadFilterFloatOptimized/0/4/8            2193 ns         2186 ns       320234
BM_BiquadFilterFloatOptimized/0/4/9            2194 ns         2187 ns       320273
BM_BiquadFilterFloatOptimized/0/4/10           2193 ns         2185 ns       320448
BM_BiquadFilterFloatOptimized/0/4/11           2192 ns         2185 ns       320000
BM_BiquadFilterFloatOptimized/0/4/12           2448 ns         2440 ns       285447
BM_BiquadFilterFloatOptimized/0/4/13           2454 ns         2447 ns       282923
BM_BiquadFilterFloatOptimized/0/4/14           2359 ns         2352 ns       297360
BM_BiquadFilterFloatOptimized/0/4/15           2467 ns         2459 ns       286699
BM_BiquadFilterFloatOptimized/0/4/16           2442 ns         2435 ns       286231
BM_BiquadFilterFloatOptimized/0/4/17           2446 ns         2438 ns       287105
BM_BiquadFilterFloatOptimized/0/4/18           2361 ns         2354 ns       298082
BM_BiquadFilterFloatOptimized/0/4/19           2452 ns         2444 ns       284776
BM_BiquadFilterFloatOptimized/0/4/20           2459 ns         2451 ns       284338
BM_BiquadFilterFloatOptimized/0/4/21           2437 ns         2429 ns       289188
BM_BiquadFilterFloatOptimized/0/4/22           2358 ns         2350 ns       297415
BM_BiquadFilterFloatOptimized/0/4/23           2459 ns         2451 ns       287279
BM_BiquadFilterFloatOptimized/0/4/24           2452 ns         2444 ns       281625
BM_BiquadFilterFloatOptimized/0/4/25           2389 ns         2382 ns       290665
BM_BiquadFilterFloatOptimized/0/4/26           2360 ns         2354 ns       298354
BM_BiquadFilterFloatOptimized/0/4/27           2433 ns         2425 ns       288930
BM_BiquadFilterFloatOptimized/0/4/28           2447 ns         2440 ns       285827
BM_BiquadFilterFloatOptimized/0/4/29           2450 ns         2444 ns       282857
BM_BiquadFilterFloatOptimized/0/4/30           2360 ns         2353 ns       297327
BM_BiquadFilterFloatOptimized/0/4/31           2463 ns         2455 ns       281644
BM_BiquadFilterFloatOptimized/1/1/1             559 ns          558 ns      1255286
BM_BiquadFilterFloatOptimized/1/1/2             649 ns          647 ns      1081171
BM_BiquadFilterFloatOptimized/1/1/3             649 ns          647 ns      1081604
BM_BiquadFilterFloatOptimized/1/1/4             848 ns          845 ns       828382
BM_BiquadFilterFloatOptimized/1/1/5             847 ns          844 ns       827653
BM_BiquadFilterFloatOptimized/1/1/6             848 ns          845 ns       831001
BM_BiquadFilterFloatOptimized/1/1/7             848 ns          845 ns       828299
BM_BiquadFilterFloatOptimized/1/1/8            2291 ns         2284 ns       306513
BM_BiquadFilterFloatOptimized/1/1/9            2297 ns         2290 ns       305585
BM_BiquadFilterFloatOptimized/1/1/10           2310 ns         2303 ns       304022
BM_BiquadFilterFloatOptimized/1/1/11           2286 ns         2279 ns       307592
BM_BiquadFilterFloatOptimized/1/1/12           2263 ns         2256 ns       310010
BM_BiquadFilterFloatOptimized/1/1/13           2264 ns         2258 ns       310167
BM_BiquadFilterFloatOptimized/1/1/14           2264 ns         2257 ns       310392
BM_BiquadFilterFloatOptimized/1/1/15           2264 ns         2257 ns       310298
BM_BiquadFilterFloatOptimized/1/1/16           2263 ns         2256 ns       310058
BM_BiquadFilterFloatOptimized/1/1/17           2264 ns         2257 ns       310262
BM_BiquadFilterFloatOptimized/1/1/18           2263 ns         2256 ns       310337
BM_BiquadFilterFloatOptimized/1/1/19           2266 ns         2258 ns       310162
BM_BiquadFilterFloatOptimized/1/1/20           2264 ns         2257 ns       309968
BM_BiquadFilterFloatOptimized/1/1/21           2264 ns         2256 ns       310341
BM_BiquadFilterFloatOptimized/1/1/22           2266 ns         2258 ns       310193
BM_BiquadFilterFloatOptimized/1/1/23           2263 ns         2257 ns       310310
BM_BiquadFilterFloatOptimized/1/1/24           2265 ns         2257 ns       310182
BM_BiquadFilterFloatOptimized/1/1/25           2263 ns         2256 ns       309999
BM_BiquadFilterFloatOptimized/1/1/26           2265 ns         2258 ns       309943
BM_BiquadFilterFloatOptimized/1/1/27           2264 ns         2257 ns       310175
BM_BiquadFilterFloatOptimized/1/1/28           2265 ns         2258 ns       310119
BM_BiquadFilterFloatOptimized/1/1/29           2265 ns         2258 ns       310231
BM_BiquadFilterFloatOptimized/1/1/30           2266 ns         2258 ns       309976
BM_BiquadFilterFloatOptimized/1/1/31           2266 ns         2258 ns       310062
BM_BiquadFilterFloatOptimized/1/2/1             613 ns          611 ns      1144258
BM_BiquadFilterFloatOptimized/1/2/2             801 ns          798 ns       877198
BM_BiquadFilterFloatOptimized/1/2/3             800 ns          798 ns       877731
BM_BiquadFilterFloatOptimized/1/2/4            1508 ns         1503 ns       465854
BM_BiquadFilterFloatOptimized/1/2/5            1505 ns         1500 ns       466533
BM_BiquadFilterFloatOptimized/1/2/6            1502 ns         1497 ns       468915
BM_BiquadFilterFloatOptimized/1/2/7            1505 ns         1501 ns       466371
BM_BiquadFilterFloatOptimized/1/2/8            2238 ns         2231 ns       313808
BM_BiquadFilterFloatOptimized/1/2/9            2236 ns         2228 ns       313914
BM_BiquadFilterFloatOptimized/1/2/10           2235 ns         2228 ns       314155
BM_BiquadFilterFloatOptimized/1/2/11           2237 ns         2231 ns       313280
BM_BiquadFilterFloatOptimized/1/2/12           2482 ns         2474 ns       287057
BM_BiquadFilterFloatOptimized/1/2/13           2491 ns         2483 ns       280960
BM_BiquadFilterFloatOptimized/1/2/14           2520 ns         2513 ns       283093
BM_BiquadFilterFloatOptimized/1/2/15           2452 ns         2445 ns       285828
BM_BiquadFilterFloatOptimized/1/2/16           2503 ns         2495 ns       286339
BM_BiquadFilterFloatOptimized/1/2/17           2499 ns         2491 ns       285757
BM_BiquadFilterFloatOptimized/1/2/18           2509 ns         2502 ns       281775
BM_BiquadFilterFloatOptimized/1/2/19           2491 ns         2483 ns       281628
BM_BiquadFilterFloatOptimized/1/2/20           2466 ns         2458 ns       289553
BM_BiquadFilterFloatOptimized/1/2/21           2456 ns         2447 ns       284222
BM_BiquadFilterFloatOptimized/1/2/22           2502 ns         2494 ns       277208
BM_BiquadFilterFloatOptimized/1/2/23           2458 ns         2450 ns       281951
BM_BiquadFilterFloatOptimized/1/2/24           2474 ns         2466 ns       290638
BM_BiquadFilterFloatOptimized/1/2/25           2473 ns         2465 ns       285659
BM_BiquadFilterFloatOptimized/1/2/26           2504 ns         2497 ns       278294
BM_BiquadFilterFloatOptimized/1/2/27           2467 ns         2459 ns       279088
BM_BiquadFilterFloatOptimized/1/2/28           2487 ns         2479 ns       281668
BM_BiquadFilterFloatOptimized/1/2/29           2469 ns         2462 ns       274237
BM_BiquadFilterFloatOptimized/1/2/30           2502 ns         2495 ns       282294
BM_BiquadFilterFloatOptimized/1/2/31           2438 ns         2430 ns       285546
BM_BiquadFilterFloatOptimized/1/3/1            1016 ns         1012 ns       691507
BM_BiquadFilterFloatOptimized/1/3/2            1361 ns         1357 ns       514162
BM_BiquadFilterFloatOptimized/1/3/3            1359 ns         1355 ns       516211
BM_BiquadFilterFloatOptimized/1/3/4            2304 ns         2297 ns       304380
BM_BiquadFilterFloatOptimized/1/3/5            2305 ns         2297 ns       304978
BM_BiquadFilterFloatOptimized/1/3/6            2291 ns         2283 ns       306726
BM_BiquadFilterFloatOptimized/1/3/7            2305 ns         2297 ns       304572
BM_BiquadFilterFloatOptimized/1/3/8            2332 ns         2325 ns       303627
BM_BiquadFilterFloatOptimized/1/3/9            2313 ns         2305 ns       303341
BM_BiquadFilterFloatOptimized/1/3/10           2317 ns         2310 ns       303791
BM_BiquadFilterFloatOptimized/1/3/11           2325 ns         2317 ns       302256
BM_BiquadFilterFloatOptimized/1/3/12           2828 ns         2819 ns       248120
BM_BiquadFilterFloatOptimized/1/3/13           2831 ns         2821 ns       248747
BM_BiquadFilterFloatOptimized/1/3/14           2849 ns         2840 ns       247287
BM_BiquadFilterFloatOptimized/1/3/15           2829 ns         2821 ns       247794
BM_BiquadFilterFloatOptimized/1/3/16           2830 ns         2821 ns       248154
BM_BiquadFilterFloatOptimized/1/3/17           2833 ns         2823 ns       249270
BM_BiquadFilterFloatOptimized/1/3/18           2853 ns         2843 ns       247248
BM_BiquadFilterFloatOptimized/1/3/19           2839 ns         2830 ns       248141
BM_BiquadFilterFloatOptimized/1/3/20           2830 ns         2820 ns       248693
BM_BiquadFilterFloatOptimized/1/3/21           2832 ns         2823 ns       247991
BM_BiquadFilterFloatOptimized/1/3/22           2850 ns         2841 ns       246602
BM_BiquadFilterFloatOptimized/1/3/23           2828 ns         2819 ns       248859
BM_BiquadFilterFloatOptimized/1/3/24           2835 ns         2826 ns       247837
BM_BiquadFilterFloatOptimized/1/3/25           2837 ns         2828 ns       246757
BM_BiquadFilterFloatOptimized/1/3/26           2860 ns         2851 ns       245739
BM_BiquadFilterFloatOptimized/1/3/27           2830 ns         2821 ns       248257
BM_BiquadFilterFloatOptimized/1/3/28           2826 ns         2817 ns       249220
BM_BiquadFilterFloatOptimized/1/3/29           2820 ns         2811 ns       248787
BM_BiquadFilterFloatOptimized/1/3/30           2853 ns         2844 ns       245666
BM_BiquadFilterFloatOptimized/1/3/31           2829 ns         2821 ns       248423
BM_BiquadFilterFloatOptimized/1/4/1             650 ns          648 ns      1080678
BM_BiquadFilterFloatOptimized/1/4/2             808 ns          806 ns       868623
BM_BiquadFilterFloatOptimized/1/4/3             810 ns          807 ns       867092
BM_BiquadFilterFloatOptimized/1/4/4             844 ns          842 ns       831679
BM_BiquadFilterFloatOptimized/1/4/5             843 ns          841 ns       830964
BM_BiquadFilterFloatOptimized/1/4/6             845 ns          842 ns       831367
BM_BiquadFilterFloatOptimized/1/4/7             845 ns          842 ns       830992
BM_BiquadFilterFloatOptimized/1/4/8            2193 ns         2186 ns       320186
BM_BiquadFilterFloatOptimized/1/4/9            2194 ns         2187 ns       320621
BM_BiquadFilterFloatOptimized/1/4/10           2194 ns         2187 ns       320577
BM_BiquadFilterFloatOptimized/1/4/11           2192 ns         2184 ns       320221
BM_BiquadFilterFloatOptimized/1/4/12           2471 ns         2462 ns       286817
BM_BiquadFilterFloatOptimized/1/4/13           2450 ns         2442 ns       283590
BM_BiquadFilterFloatOptimized/1/4/14           2359 ns         2351 ns       297716
BM_BiquadFilterFloatOptimized/1/4/15           2472 ns         2464 ns       286187
BM_BiquadFilterFloatOptimized/1/4/16           2450 ns         2442 ns       291296
BM_BiquadFilterFloatOptimized/1/4/17           2460 ns         2453 ns       287031
BM_BiquadFilterFloatOptimized/1/4/18           2358 ns         2350 ns       297376
BM_BiquadFilterFloatOptimized/1/4/19           2469 ns         2462 ns       286443
BM_BiquadFilterFloatOptimized/1/4/20           2470 ns         2462 ns       286582
BM_BiquadFilterFloatOptimized/1/4/21           2420 ns         2412 ns       288366
BM_BiquadFilterFloatOptimized/1/4/22           2359 ns         2351 ns       297356
BM_BiquadFilterFloatOptimized/1/4/23           2448 ns         2440 ns       286534
BM_BiquadFilterFloatOptimized/1/4/24           2458 ns         2450 ns       286994
BM_BiquadFilterFloatOptimized/1/4/25           2420 ns         2412 ns       287406
BM_BiquadFilterFloatOptimized/1/4/26           2363 ns         2354 ns       298029
BM_BiquadFilterFloatOptimized/1/4/27           2421 ns         2414 ns       288812
BM_BiquadFilterFloatOptimized/1/4/28           2449 ns         2442 ns       286253
BM_BiquadFilterFloatOptimized/1/4/29           2448 ns         2441 ns       287234
BM_BiquadFilterFloatOptimized/1/4/30           2361 ns         2354 ns       297402
BM_BiquadFilterFloatOptimized/1/4/31           2465 ns         2457 ns       287662
BM_BiquadFilterFloatNonOptimized/0/1/31        2261 ns         2257 ns       310189
BM_BiquadFilterFloatNonOptimized/0/2/31        4525 ns         4510 ns       155178
BM_BiquadFilterFloatNonOptimized/0/3/31        6781 ns         6760 ns       103524
BM_BiquadFilterFloatNonOptimized/0/4/31        9037 ns         9008 ns        77684
BM_BiquadFilterFloatNonOptimized/0/5/31       11298 ns        11259 ns        62169
BM_BiquadFilterFloatNonOptimized/0/6/31       13575 ns        13534 ns        51708
BM_BiquadFilterFloatNonOptimized/0/7/31       15841 ns        15796 ns        44308
BM_BiquadFilterFloatNonOptimized/0/8/31       18105 ns        18047 ns        38797
BM_BiquadFilterFloatNonOptimized/0/9/31       20335 ns        20270 ns        34532
BM_BiquadFilterFloatNonOptimized/0/10/31      22603 ns        22534 ns        31063
BM_BiquadFilterFloatNonOptimized/0/11/31      24875 ns        24798 ns        28225
BM_BiquadFilterFloatNonOptimized/0/12/31      27122 ns        27038 ns        25889
BM_BiquadFilterFloatNonOptimized/0/13/31      29416 ns        29326 ns        23869
BM_BiquadFilterFloatNonOptimized/0/14/31      31703 ns        31602 ns        22151
BM_BiquadFilterFloatNonOptimized/0/15/31      33945 ns        33849 ns        20678
BM_BiquadFilterFloatNonOptimized/0/16/31      36232 ns        36123 ns        19379
BM_BiquadFilterFloatNonOptimized/0/17/31      38577 ns        38464 ns        18205
BM_BiquadFilterFloatNonOptimized/0/18/31      41054 ns        40924 ns        17106
BM_BiquadFilterFloatNonOptimized/0/19/31      43278 ns        43133 ns        16228
BM_BiquadFilterFloatNonOptimized/0/20/31      45549 ns        45414 ns        15411
BM_BiquadFilterFloatNonOptimized/0/21/31      48024 ns        47867 ns        14625
BM_BiquadFilterFloatNonOptimized/0/22/31      50268 ns        50109 ns        13965
BM_BiquadFilterFloatNonOptimized/0/23/31      52268 ns        52090 ns        13440
BM_BiquadFilterFloatNonOptimized/0/24/31      54528 ns        54350 ns        12883
BM_BiquadFilterDoubleOptimized/0/1/31          2264 ns         2258 ns       309965
BM_BiquadFilterDoubleOptimized/0/2/31          2503 ns         2495 ns       270274
BM_BiquadFilterDoubleOptimized/0/3/31          2787 ns         2778 ns       251685
BM_BiquadFilterDoubleOptimized/0/4/31          3175 ns         3165 ns       221339
BM_BiquadFilterDoubleNonOptimized/0/1/31       2264 ns         2257 ns       310154
BM_BiquadFilterDoubleNonOptimized/0/2/31       4523 ns         4508 ns       155292
BM_BiquadFilterDoubleNonOptimized/0/3/31       6778 ns         6756 ns       103599
BM_BiquadFilterDoubleNonOptimized/0/4/31       9063 ns         9033 ns        77461

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
