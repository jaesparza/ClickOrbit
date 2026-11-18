#include "unity.h"
#include "jiggler_schedule.h"
void setUp(void) {}
void tearDown(void) {}
static void test_five_minutes_and_repeat(void)
{
    jiggler_schedule_t s = {0};
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, 100 + JIGGLER_INTERVAL_US - 1));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 1, 100 + JIGGLER_INTERVAL_US));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, 100 + JIGGLER_INTERVAL_US));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 1, 100 + 2 * JIGGLER_INTERVAL_US));
}
static void test_disable_and_reenable_reset_timer(void)
{
    jiggler_schedule_t s = {0};
    TEST_ASSERT_FALSE(jiggler_due(&s, false, 0, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 1, 100));
    TEST_ASSERT_FALSE(jiggler_due(&s, false, 2, JIGGLER_INTERVAL_US));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 3, JIGGLER_INTERVAL_US));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 3, 2 * JIGGLER_INTERVAL_US - 1));
    TEST_ASSERT_TRUE(jiggler_due(&s, true, 3, 2 * JIGGLER_INTERVAL_US));
    TEST_ASSERT_FALSE(jiggler_due(&s, true, 5, 3 * JIGGLER_INTERVAL_US));
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_five_minutes_and_repeat);
    RUN_TEST(test_disable_and_reenable_reset_timer);
    return UNITY_END();
}
