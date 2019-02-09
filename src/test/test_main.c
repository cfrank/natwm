#include <stdio.h>

#include <unity.h>

void test_Example_should_Run(void)
{
        TEST_ASSERT_EQUAL_INT(1, 1);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_Example_should_Run);
        return UNITY_END();
}
