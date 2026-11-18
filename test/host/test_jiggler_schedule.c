#include "unity.h"
#include "jiggler_schedule.h"
void setUp(void) {}
void tearDown(void) {}
static void test_five_minutes_and_repeat(void)
{
    jiggler_schedule_t s = {0};
    int64_t interval_us = JIGGLER_DEFAULT_INTERVAL_MS * 1000;
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100 + interval_us - 1));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100 + interval_us));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100 + interval_us));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100 + 2 * interval_us));
}
static void test_disable_and_reenable_reset_timer(void)
{
    jiggler_schedule_t s = {0};
    int64_t interval_us = JIGGLER_DEFAULT_INTERVAL_MS * 1000;
    TEST_ASSERT_FALSE(jiggler_due(&s, false, 0, JIGGLER_DEFAULT_INTERVAL_MS, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_DEFAULT_INTERVAL_MS, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, false, 2, JIGGLER_DEFAULT_INTERVAL_MS, interval_us));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 3, JIGGLER_DEFAULT_INTERVAL_MS, interval_us));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 3, JIGGLER_DEFAULT_INTERVAL_MS, 2 * interval_us - 1));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 3, JIGGLER_DEFAULT_INTERVAL_MS, 2 * interval_us));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 5, JIGGLER_DEFAULT_INTERVAL_MS, 3 * interval_us));
}
static void test_two_second_test_mode_and_interval_change(void)
{
    jiggler_schedule_t s = {0};
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_TEST_INTERVAL_MS, 10));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, JIGGLER_TEST_INTERVAL_MS, 2000009));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 1, JIGGLER_TEST_INTERVAL_MS, 2000010));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 2, 60000, 2000010));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 2, 60000, 62000009));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 2, 60000, 62000010));
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_five_minutes_and_repeat);
    RUN_TEST(test_disable_and_reenable_reset_timer);
    RUN_TEST(test_two_second_test_mode_and_interval_change);
    return UNITY_END();
}
