#pragma once
#include <cmath>
#include <cstdint>

static constexpr int32_t ROULETTE_ONE_REVOLUTION = 36000; // 1回転 = 36000 [0.01deg]

/// @brief 初速v0から一定加速度でt秒後にtheta進むときの終速を返す
/// @param initialSpeedRpm 初速 [rpm]
/// @param travelAngle 移動角度 [0.01deg]
/// @param travelTimeSec 経過時間 [s]
/// @param oneRevolution 1回転あたりの単位数 (デフォルト: 36000)
/// @return 終速 [rpm]
inline float calcFinalSpeedRpm(float initialSpeedRpm, float travelAngle, float travelTimeSec,
                               float oneRevolution = (float)ROULETTE_ONE_REVOLUTION)
{
    if ((travelTimeSec <= 0.0f) || (oneRevolution <= 0.0f)) {
        return initialSpeedRpm;
    }

    const float travelRevolutions = travelAngle / oneRevolution;
    const float requiredAverageSpeedRpm = travelRevolutions * 60.0f / travelTimeSec;
    return (2.0f * requiredAverageSpeedRpm) - initialSpeedRpm;
}

/// @brief 現在位置から指定方向に最も近い (targetAngle + n×oneRevolution) の絶対位置を返す
/// @param currentPos    現在位置 [0.01deg]
/// @param forward       true = +方向, false = -方向
/// @param targetAngle   1回転内の目標絶対角度 [0.01deg]  (0~oneRevolution-1, 例: 9000 = 90度)
/// @param oneRevolution 1回転あたりの単位数 (デフォルト: 36000)
/// @return 目標位置 [0.01deg]
inline int32_t nextRevolutionPos(int32_t currentPos, bool forward, int32_t targetAngle = 0,
                                  int32_t oneRevolution = ROULETTE_ONE_REVOLUTION)
{
    const float shifted = (float)(currentPos - targetAngle) / (float)oneRevolution;
    const float n = forward ? std::ceilf(shifted) : std::floorf(shifted);
    return (int32_t)(n * (float)oneRevolution) + targetAngle;
}
