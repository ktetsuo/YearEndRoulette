#include "PID.h"
#include <algorithm>

PID::PID(float kP, float kI, float kD, float dt, float outputMin, float outputMax, float targetValue)
    : _kP(kP), _kI(kI), _kD(kD), _dt(dt),
      _outputMin(outputMin), _outputMax(outputMax),
      _targetValue(targetValue),
      _prevError(0.0f), _prevProp(0), _output(0.0f), _integral(0.0f)
{
}

float PID::kP() const
{
  return _kP;
}
void PID::kP(float k)
{
  _kP = k;
}

float PID::kI() const
{
  return _kI;
}
void PID::kI(float k)
{
  _kI = k;
}

float PID::kD() const
{
  return _kD;
}
void PID::kD(float k)
{
  _kD = k;
}

void PID::targetValue(float target)
{
  _targetValue = target;
}

float PID::targetValue() const
{
  return _targetValue;
}

// 実際の出力値（クランプ後）を返す
float PID::output() const
{
  return std::clamp(_output, _outputMin, _outputMax);
}

// PID制御ループを1ステップ実行する（アンチワインドアップ対応）
void PID::update(float currentValue)
{
  const float error = _targetValue - currentValue;
  const float prop = (error - _prevError) / _dt;
  const float deriv = (prop - _prevProp) / _dt;

  // 積分値を仮更新
  float next_integral = _integral + error * _dt;

  // クランプ前の出力値を計算
  const float next_output = _output + _kP * prop + _kI * next_integral + _kD * deriv;
  float clamped_output = std::clamp(next_output, _outputMin, _outputMax);

  // アンチワインドアップ：出力がクランプされていなければ積分値を更新
  if (next_output == clamped_output)
  {
    _integral = next_integral;
  }
  // クランプされている場合は_integralを更新しない

  _output = next_output; // クランプ前の値を保持
  _prevError = error;
  _prevProp = prop;
}

void PID::reset()
{
  _prevError = 0.0f;
  _prevProp = 0.0f;
  _output = 0.0f;
  _integral = 0.0f;
}
