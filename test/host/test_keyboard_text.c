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
    // Each pair occupies the same physical key on the US keyboard layout.
    const char plain[] = "abcdefghijklmnopqrstuvwxyz1234567890-=[]\\;'\x60,./";
    const char shifted[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+{}|:\"~<>?";
    const uint8_t usages[] = {
        4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
        30,31,32,33,34,35,36,37,38,39,45,46,47,48,49,51,52,53,54,55,56
    };
    uint8_t mod, key;
    TEST_ASSERT_EQUAL_UINT(sizeof(usages), sizeof(plain) - 1);
    TEST_ASSERT_EQUAL_UINT(sizeof(plain), sizeof(shifted));
    for (size_t i = 0; i < sizeof(usages); ++i) {
        TEST_ASSERT_TRUE(keyboard_ascii(shifted[i], &mod, &key));
        TEST_ASSERT_EQUAL_UINT8(2, mod);
        TEST_ASSERT_EQUAL_UINT8(usages[i], key);
        TEST_ASSERT_TRUE(keyboard_ascii(plain[i], &mod, &key));
        TEST_ASSERT_EQUAL_UINT8(0, mod);
        TEST_ASSERT_EQUAL_UINT8(usages[i], key);
    }
}

static void test_whitespace_and_invalid_inputs(void)
{
    uint8_t mod, key;
    TEST_ASSERT_TRUE(keyboard_ascii(' ', &mod, &key));
    TEST_ASSERT_EQUAL_UINT8(0x2c, key);
    TEST_ASSERT_TRUE(keyboard_ascii('\n', &mod, &key));
    TEST_ASSERT_EQUAL_UINT8(0x28, key);
    TEST_ASSERT_TRUE(keyboard_ascii('\t', &mod, &key));
    TEST_ASSERT_EQUAL_UINT8(0x2b, key);
    TEST_ASSERT_FALSE(keyboard_ascii('\0', &mod, &key));
    TEST_ASSERT_FALSE(keyboard_ascii('a', NULL, &key));
    TEST_ASSERT_FALSE(keyboard_ascii('a', &mod, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_direction_words);
    RUN_TEST(test_shift_and_key_usages);
    RUN_TEST(test_whitespace_and_invalid_inputs);
    return UNITY_END();
}
