#include "unity.h"
#include "keyboard_text.h"
void setUp(void) {}
void tearDown(void) {}

static void test_direction_words(void)
{
    TEST_ASSERT_EQUAL_STRING("Hello", keyboard_text_for_direction(1));
    TEST_ASSERT_EQUAL_STRING("World", keyboard_text_for_direction(-1));
    TEST_ASSERT_NULL(keyboard_text_for_direction(0));
}

static void test_shift_and_key_usages(void)
{
    uint8_t mod, key;
    TEST_ASSERT_TRUE(keyboard_letter('H', &mod, &key));
    TEST_ASSERT_EQUAL_UINT8(2, mod);
    TEST_ASSERT_EQUAL_UINT8(0x0b, key);
    TEST_ASSERT_TRUE(keyboard_letter('l', &mod, &key));
    TEST_ASSERT_EQUAL_UINT8(0, mod);
    TEST_ASSERT_EQUAL_UINT8(0x0f, key);
    TEST_ASSERT_FALSE(keyboard_letter('\n', &mod, &key));
    TEST_ASSERT_FALSE(keyboard_letter('1', &mod, &key));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_direction_words);
    RUN_TEST(test_shift_and_key_usages);
    return UNITY_END();
}
