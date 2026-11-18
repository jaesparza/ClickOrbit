#include "unity.h"
#include "pomodoro.h"
void setUp(void) {}
void tearDown(void) {}

static void test_dial_pause_resume(void)
{
    pomodoro_t p;
    pomo_init(&p);
    for (int i = 0; i < 70; i++) pomo_adjust(&p, 1);
    TEST_ASSERT_EQUAL_INT(60, p.minutes);
    for (int i = 0; i < 70; i++) pomo_adjust(&p, -1);
    TEST_ASSERT_EQUAL_INT(1, p.minutes);
    pomo_toggle(&p, 100);
    pomo_adjust(&p, 1);
    pomo_toggle(&p, 1150);
    TEST_ASSERT_EQUAL_INT(58950, pomo_remaining(&p, 9000));
    pomo_toggle(&p, 11000);
    TEST_ASSERT_FALSE(pomo_finish(&p, 69949));
    TEST_ASSERT_TRUE(pomo_finish(&p, 69950));
    TEST_ASSERT_FALSE(pomo_finish(&p, 99999));
    TEST_ASSERT_EQUAL_INT(POMO_BREAK, p.phase);
}
static void test_four_round_cycle_and_restart(void)
{
    pomodoro_t p;
    pomo_init(&p);
    pomo_adjust(&p, -1);
    for (int i = 1; i <= 4; i++) {
        pomo_toggle(&p, 0);
        pomo_toggle(&p, 1000);
        pomo_restart(&p);
        TEST_ASSERT_EQUAL_INT(24 * 60000, pomo_remaining(&p, 1500));
        pomo_toggle(&p, 2000);
        pomo_toggle(&p, p.deadline_ms);
        TEST_ASSERT_EQUAL_INT(POMO_READY, p.mode);
        TEST_ASSERT_EQUAL_INT(i == 4 ? 15 : 5, p.minutes);
        pomo_toggle(&p, 10000000);
        TEST_ASSERT_TRUE(pomo_finish(&p, p.deadline_ms + 100));
        TEST_ASSERT_EQUAL_INT(POMO_FOCUS, p.phase);
        TEST_ASSERT_EQUAL_INT(24, p.minutes);
    }
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dial_pause_resume);
    RUN_TEST(test_four_round_cycle_and_restart);
    return UNITY_END();
}
