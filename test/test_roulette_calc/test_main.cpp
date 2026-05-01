#include <unity.h>
#include "RouletteCalc.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================
// +方向 / targetAngle=0 (0度停止)
// ============================================================

// 通常の回転中 (3.758回転 → 次の4回転目=0度)
void test_forward_normal()
{
    TEST_ASSERT_EQUAL_INT32(144000, nextRevolutionPos(135300, true));
}

// ちょうど整数回転の境界 → その位置を返す
void test_forward_at_boundary()
{
    TEST_ASSERT_EQUAL_INT32(36000, nextRevolutionPos(36000, true));
}

// 境界を1だけ超えた → 次の回転へ
void test_forward_just_past_boundary()
{
    TEST_ASSERT_EQUAL_INT32(72000, nextRevolutionPos(36001, true));
}

// 原点
void test_forward_zero()
{
    TEST_ASSERT_EQUAL_INT32(0, nextRevolutionPos(0, true));
}

// 負の位置, +方向 → 0方向へ
void test_forward_negative_half()
{
    TEST_ASSERT_EQUAL_INT32(0, nextRevolutionPos(-18000, true));
}

// 負の整数回転境界, +方向 → その位置を返す
void test_forward_negative_at_boundary()
{
    TEST_ASSERT_EQUAL_INT32(-36000, nextRevolutionPos(-36000, true));
}

// ============================================================
// -方向 / targetAngle=0
// ============================================================

// 通常の回転中, -方向 (3.758回転 → 直前の3回転目)
void test_backward_normal()
{
    TEST_ASSERT_EQUAL_INT32(108000, nextRevolutionPos(135300, false));
}

// ちょうど整数回転の境界, -方向 → その位置を返す
void test_backward_at_boundary()
{
    TEST_ASSERT_EQUAL_INT32(36000, nextRevolutionPos(36000, false));
}

// 境界を1だけ手前, -方向 → 直前の回転へ
void test_backward_just_before_boundary()
{
    TEST_ASSERT_EQUAL_INT32(0, nextRevolutionPos(35999, false));
}

// 正の半回転, -方向 → 0
void test_backward_half()
{
    TEST_ASSERT_EQUAL_INT32(0, nextRevolutionPos(18000, false));
}

// 負の半回転, -方向
void test_backward_negative_half()
{
    TEST_ASSERT_EQUAL_INT32(-36000, nextRevolutionPos(-18000, false));
}

// ============================================================
// targetAngle あり (絶対角度指定)
// ============================================================

// +方向, 90度停止: 現在35000 → 次の45000 (1回転+90度)
void test_forward_target90()
{
    TEST_ASSERT_EQUAL_INT32(45000, nextRevolutionPos(35000, true, 9000));
}

// +方向, 90度停止: ちょうど45000 → その位置を返す
void test_forward_target90_at_boundary()
{
    TEST_ASSERT_EQUAL_INT32(45000, nextRevolutionPos(45000, true, 9000));
}

// +方向, 90度停止: 45001 → 次の81000 (2回転+90度)
void test_forward_target90_past_boundary()
{
    TEST_ASSERT_EQUAL_INT32(81000, nextRevolutionPos(45001, true, 9000));
}

// -方向, 90度停止: 現在47000 → 直前の45000
void test_backward_target90()
{
    TEST_ASSERT_EQUAL_INT32(45000, nextRevolutionPos(47000, false, 9000));
}

// ============================================================
// 線形加速で指定角度を進むときの終速
// ============================================================

void test_calc_final_speed_half_revolution()
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, calcFinalSpeedRpm(120.0f, 18000.0f, 0.5f));
}

void test_calc_final_speed_one_revolution()
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 120.0f, calcFinalSpeedRpm(120.0f, 36000.0f, 0.5f));
}

void test_calc_final_speed_one_and_half_revolution()
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 240.0f, calcFinalSpeedRpm(120.0f, 54000.0f, 0.5f));
}

void test_calc_final_speed_invalid_time_returns_initial_speed()
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 120.0f, calcFinalSpeedRpm(120.0f, 54000.0f, 0.0f));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_forward_normal);
    RUN_TEST(test_forward_at_boundary);
    RUN_TEST(test_forward_just_past_boundary);
    RUN_TEST(test_forward_zero);
    RUN_TEST(test_forward_negative_half);
    RUN_TEST(test_forward_negative_at_boundary);
    RUN_TEST(test_backward_normal);
    RUN_TEST(test_backward_at_boundary);
    RUN_TEST(test_backward_just_before_boundary);
    RUN_TEST(test_backward_half);
    RUN_TEST(test_backward_negative_half);
    RUN_TEST(test_forward_target90);
    RUN_TEST(test_forward_target90_at_boundary);
    RUN_TEST(test_forward_target90_past_boundary);
    RUN_TEST(test_backward_target90);
    RUN_TEST(test_calc_final_speed_half_revolution);
    RUN_TEST(test_calc_final_speed_one_revolution);
    RUN_TEST(test_calc_final_speed_one_and_half_revolution);
    RUN_TEST(test_calc_final_speed_invalid_time_returns_initial_speed);
    return UNITY_END();
}
