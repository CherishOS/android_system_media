#include <audio_utils/hal_smoothness.h>
#include <errno.h>
#include <float.h>
#include <gtest/gtest.h>
#include <limits.h>

#include <memory>

struct TestDeleter {
  void operator()(hal_smoothness *p) { hal_smoothness_free(&p); }
};

struct custom_private_data {
  bool ran_callback;

  struct hal_smoothness_metrics metrics;
};

void custom_flush(struct hal_smoothness_metrics *metrics, void *private_data) {
  struct custom_private_data *data = (custom_private_data *)private_data;

  data->ran_callback = true;

  memcpy(&data->metrics, metrics, sizeof(struct hal_smoothness_metrics));
}

class HalSmoothnessTest : public ::testing::Test {
 protected:
  void CommonSmoothnessInit(unsigned int num_writes_to_log) {
    struct hal_smoothness *smoothness_init;
    data.ran_callback = false;
    int result =
        hal_smoothness_initialize(&smoothness_init, HAL_SMOOTHNESS_VERSION_1,
                                  num_writes_to_log, custom_flush, &data);
    ASSERT_EQ(result, 0);
    smoothness = std::unique_ptr<hal_smoothness, TestDeleter>{smoothness_init};
  }

  std::unique_ptr<hal_smoothness, TestDeleter> smoothness;
  struct custom_private_data data;
};

// Test that the callback runs after the total write count is equal to
// "num_writes_to_log".
TEST_F(HalSmoothnessTest, callback_should_run) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 1));
  // Since "num_writes_to_log" is set to 1, after this write, the callback should
  // run.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 200);

  EXPECT_EQ(data.ran_callback, true);
}

// Test that the callback should not run if the total write count is less than
// "num_writes_to_log".
TEST_F(HalSmoothnessTest, callback_should_not_run) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 2));

  // Since "num_writes_to_log" is set to 2, after this write, the callback should
  // NOT run.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 200);
  EXPECT_EQ(data.ran_callback, false);

  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 200);
  EXPECT_EQ(data.ran_callback, true);
}

// Test that metric values in "struct hal_smoothness_metrics" that is passed
// into the callback are correct.
TEST_F(HalSmoothnessTest, verify_metrics) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 6));

  // Simulate how these increment methods would be called during a real runtime.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 200);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 201);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost= */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 202);
  smoothness->increment_overrun(smoothness.get(), /* frames_lost */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 203);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 204);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 205);

  EXPECT_EQ(data.metrics.underrun_count, 2U);
  EXPECT_EQ(data.metrics.overrun_count, 1U);
  EXPECT_EQ(data.metrics.total_writes, 6U);
  EXPECT_EQ(data.metrics.total_frames_written, 3300U);
  EXPECT_EQ(data.metrics.total_frames_lost, 2700U);
  EXPECT_EQ(data.metrics.timestamp, 205U);
}

// Test that metric values in "struct hal_smoothness_metrics" are reset after it
// has met "num_writes_to_log".
TEST_F(HalSmoothnessTest, verify_metrics_reset) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 6));

  // Simulate how these increment methods would be called during a real runtime.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 200);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 201);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost= */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 202);
  smoothness->increment_overrun(smoothness.get(), /* frames_lost */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 203);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost */ 900);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 100,
                                     /* timestamp */ 204);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 1000,
                                     /* timestamp */ 205);

  // At this point, metrics values should be reset. We will write 6 more times
  // to trigger the callback again.
  for (int i = 0; i < 6; i++) {
    // last timestamp will be 211 because 206 + 5.
    smoothness->increment_total_writes(smoothness.get(),
                                       /* frames_written= */ 1000,
                                       /* timestamp */ 206 + i);
  }

  EXPECT_EQ(data.metrics.underrun_count, 0U);
  EXPECT_EQ(data.metrics.overrun_count, 0U);
  EXPECT_EQ(data.metrics.total_writes, 6U);
  EXPECT_EQ(data.metrics.total_frames_written, 6000U);
  EXPECT_EQ(data.metrics.total_frames_lost, 0U);
  EXPECT_EQ(data.metrics.timestamp, 211U);
}

// Test that metric values in "struct hal_smoothness_metrics" that is passed
// into the callback are correct.
TEST_F(HalSmoothnessTest, smoothness_value_10ish) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 5));

  // Simulate how these increment methods would be called during a real runtime.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 200);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 201);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 202);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 203);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost */ 1);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 7999,
                                     /* timestamp */ 204);

  // -ln(1/40000)
  EXPECT_FLOAT_EQ(data.metrics.smoothness_value, 10.596635);
}

TEST_F(HalSmoothnessTest, smoothness_value_6ish) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 5));

  // Simulate how these increment methods would be called during a real runtime.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 200);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 201);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 202);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 203);
  smoothness->increment_underrun(smoothness.get(), /* frames_lost */ 100);
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 7900,
                                     /* timestamp */ 204);

  // -ln(1/400)
  EXPECT_FLOAT_EQ(data.metrics.smoothness_value, 5.9914646);
}

TEST_F(HalSmoothnessTest, log_zero_smoothness_value) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 1));

  // Simulate how these increment methods would be called during a real runtime.
  smoothness->increment_total_writes(smoothness.get(),
                                     /* frames_written= */ 8000,
                                     /* timestamp */ 200);

  // -ln(0). This should return DBL_MAX
  EXPECT_FLOAT_EQ(data.metrics.smoothness_value, DBL_MAX);
}

TEST(hal_smoothness, init_fail_with_zero_num_writes_to_log) {
  struct hal_smoothness *smoothness;
  struct custom_private_data data;
  int result =
      hal_smoothness_initialize(&smoothness, HAL_SMOOTHNESS_VERSION_1,
                                /* num_writes_to_log= */ 0, custom_flush, &data);
  EXPECT_EQ(result, -EINVAL);
}

TEST(hal_smoothness, init_pass_with_null_private_data) {
  struct hal_smoothness *smoothness_init;
  int result =
      hal_smoothness_initialize(&smoothness_init, HAL_SMOOTHNESS_VERSION_1,
                                /* num_writes_to_log= */ 6, custom_flush, NULL);
  ASSERT_EQ(result, 0);
  auto smoothness =
      std::unique_ptr<hal_smoothness, TestDeleter>{smoothness_init};
}

TEST(hal_smoothness, hal_smoothness_free) {
  struct hal_smoothness *smoothness;
  struct custom_private_data data;
  int result =
      hal_smoothness_initialize(&smoothness, HAL_SMOOTHNESS_VERSION_1,
                                /* num_writes_to_log= */ 6, custom_flush, &data);
  ASSERT_EQ(result, 0);

  hal_smoothness_free(&smoothness);
  EXPECT_EQ(smoothness, nullptr);
}

TEST(hal_smoothness, hal_smoothness_free_pass_in_null) {
  struct hal_smoothness *smoothness;

  hal_smoothness_free(&smoothness);
  EXPECT_EQ(smoothness, nullptr);
}

// Excluded testing overflow for values that only increment by 1 (ie.
// underrun_count, overrun_count, total_writes).
TEST_F(HalSmoothnessTest, underrun_overflow) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 1));

  ASSERT_EQ(smoothness->increment_underrun(smoothness.get(),
                                           /* frames_lost= */ UINT_MAX),
            0);
  ASSERT_EQ(
      smoothness->increment_underrun(smoothness.get(), /* frames_lost= */ 1),
      -EOVERFLOW);
}

TEST_F(HalSmoothnessTest, overrun_overflow) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 1));

  ASSERT_EQ(smoothness->increment_overrun(smoothness.get(),
                                          /* frames_lost= */ UINT_MAX),
            0);
  ASSERT_EQ(
      smoothness->increment_overrun(smoothness.get(), /* frames_lost= */ 1),
      -EOVERFLOW);
}

TEST_F(HalSmoothnessTest, overflow_total_writes) {
  ASSERT_NO_FATAL_FAILURE(
      HalSmoothnessTest::CommonSmoothnessInit(/* num_writes_to_log= */ 2));

  ASSERT_EQ(smoothness->increment_total_writes(smoothness.get(),
                                               /* frames_written= */ UINT_MAX,
                                               /* timestamp */ 200),
            0);
  ASSERT_EQ(smoothness->increment_total_writes(smoothness.get(),
                                               /* frames_written= */ 1,
                                               /* timestamp */ 200),
            -EOVERFLOW);
}
