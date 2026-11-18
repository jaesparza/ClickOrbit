#include "unity.h"
#include "lock_combination.h"
void setUp(void) {}
void tearDown(void) {}
static bool turn(lock_combination_t *s, int dir, int count)
{
    bool unlocked = false;
    for (int i = 0; i < count; ++i) unlocked = lock_combination_step(s, dir);
    return unlocked;
}
static void test_exact_sequence(void)
{
    lock_combination_t s = {0};
    TEST_ASSERT_FALSE(turn(&s, -1, 5));
    TEST_ASSERT_FALSE(turn(&s, 1, 4));
    TEST_ASSERT_TRUE(turn(&s, 1, 1));
}
static void test_wrong_direction_resets(void)
{
    lock_combination_t s = {0};
    TEST_ASSERT_FALSE(turn(&s, 1, 5));
    TEST_ASSERT_EQUAL_UINT8(0, s.progress);
    TEST_ASSERT_FALSE(turn(&s, -1, 4));
    TEST_ASSERT_FALSE(turn(&s, 1, 5));
    TEST_ASSERT_EQUAL_UINT8(0, s.progress);
    TEST_ASSERT_FALSE(turn(&s, -1, 6));
    TEST_ASSERT_FALSE(turn(&s, 1, 5));
    TEST_ASSERT_EQUAL_UINT8(0, s.progress);
    TEST_ASSERT_FALSE(turn(&s, -1, 5));
    TEST_ASSERT_TRUE(turn(&s, 1, 5));
}
static void test_zero_input_and_reset(void)
{
    lock_combination_t s = {0};
    TEST_ASSERT_FALSE(turn(&s, -1, 5));
    TEST_ASSERT_FALSE(lock_combination_step(&s, 0));
    TEST_ASSERT_TRUE(turn(&s, 1, 5));
    s = (lock_combination_t){0};
    TEST_ASSERT_FALSE(turn(&s, 1, 5));
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_exact_sequence);
    RUN_TEST(test_wrong_direction_resets);
    RUN_TEST(test_zero_input_and_reset);
    return UNITY_END();
}
