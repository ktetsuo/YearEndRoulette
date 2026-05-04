#include "UnitRollerWrapper.h"

namespace
{
  static constexpr float SPEED_PID_P_SCALE = 100000.0f;
  static constexpr float SPEED_PID_I_SCALE = 10000000.0f;
  static constexpr float SPEED_PID_D_SCALE = 100000.0f;
  static constexpr float POSITION_PID_P_SCALE = 100000.0f;
  static constexpr float POSITION_PID_I_SCALE = 10000000.0f;
  static constexpr float POSITION_PID_D_SCALE = 100000.0f;
}

bool UnitRollerWrapper::begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
    if (_initialized)
    {
        return true;
    }
    _initialized = _roller.begin(wire, addr, sda, scl, speed);
    return _initialized;
}

void UnitRollerWrapper::setMode(roller_mode_t mode)
{
    if (!_initialized)
    {
        return;
    }
    _roller.setMode(mode);
}

void UnitRollerWrapper::setOutput(uint8_t en)
{
    if (!_initialized)
    {
        return;
    }
    _roller.setOutput(en);
}

///////////////////////////////////////////////////////////////////////////////
// 読み取り値の取得
///////////////////////////////////////////////////////////////////////////////
float UnitRollerWrapper::getVinV()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t vinReadback = _roller.getVin(); // 電源電圧フィードバック値[10mV]
    return (float)vinReadback / 100.0f;
}

float UnitRollerWrapper::getCurrentA()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t currentReadback = _roller.getCurrentReadback(); // 電流フィードバック値[0.00001A]
    return (float)currentReadback / 100000.0f;
}

float UnitRollerWrapper::getSpeedRpm()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t speedReadback = _roller.getSpeedReadback(); // 速度フィードバック値[0.01rpm]
    return (float)speedReadback / 100.0f;
}

float UnitRollerWrapper::getPosRev()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t pos = _roller.getPosReadback(); // 位置フィードバック値[0.01deg]
    return (float)pos / 36000.0f;
}

///////////////////////////////////////////////////////////////////////////////
// 電流制御モード
///////////////////////////////////////////////////////////////////////////////
void UnitRollerWrapper::setTargetCurrentA(float currentA)
{
    if (!_initialized)
    {
        return;
    }
    const int32_t current = static_cast<int32_t>(currentA * 100000.0f); // 電流指示値[0.00001A]
    _roller.setCurrent(current);
}

float UnitRollerWrapper::getTargetCurrentA()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t targetCurrent = _roller.getCurrent(); // 電流指示値[0.00001A]
    return (float)targetCurrent / 100000.0f;
}

///////////////////////////////////////////////////////////////////////////////
// 速度制御モード
///////////////////////////////////////////////////////////////////////////////
void UnitRollerWrapper::setTargetSpeedRpm(float speedRpm)
{
    if (!_initialized)
    {
        return;
    }
    const int32_t speed = static_cast<int32_t>(speedRpm * 100.0f);  // 速度指示値[0.01rpm]
    _roller.setSpeed(speed);
}

float UnitRollerWrapper::getTargetSpeedRpm()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t targetSpeed = _roller.getSpeed(); // 速度指示値[0.01rpm]
    return (float)targetSpeed / 100.0f;
}
void UnitRollerWrapper::setSpeedPID(float kp, float ki, float kd)
{
  if(!_initialized)
  {
    return;
  }
  const float kpNonNegative = kp < 0.0f ? 0.0f : kp;
  const float kiNonNegative = ki < 0.0f ? 0.0f : ki;
  const float kdNonNegative = kd < 0.0f ? 0.0f : kd;
  const uint32_t kpScaled = static_cast<uint32_t>(std::roundf(kpNonNegative * SPEED_PID_P_SCALE));
  const uint32_t kiScaled = static_cast<uint32_t>(std::roundf(kiNonNegative * SPEED_PID_I_SCALE));
  const uint32_t kdScaled = static_cast<uint32_t>(std::roundf(kdNonNegative * SPEED_PID_D_SCALE));
  _roller.setSpeedPID(kpScaled, kiScaled, kdScaled);
}

void UnitRollerWrapper::getSpeedPID(float &kp, float &ki, float &kd)
{
  if(!_initialized)
  {
    return;
  }
  uint32_t kpScaled, kiScaled, kdScaled;
  _roller.getSpeedPID(&kpScaled, &kiScaled, &kdScaled);
  kp = static_cast<float>(kpScaled) / SPEED_PID_P_SCALE;
  ki = static_cast<float>(kiScaled) / SPEED_PID_I_SCALE;
  kd = static_cast<float>(kdScaled) / SPEED_PID_D_SCALE;
}

float UnitRollerWrapper::getSpeedMaxCurrentA()
{
  if(!_initialized)
  {
    return 0.0f;
  }
  const int32_t speedMaxCurrent = _roller.getSpeedMaxCurrent(); // 速度制御モードの最大電流指示値[0.00001A]
  return (float)speedMaxCurrent / 100000.0f;
}

void UnitRollerWrapper::setSpeedMaxCurrentA(float speedMaxCurrentA)
{
  if(!_initialized)
  {
    return;
  }
  const int32_t speedMaxCurrent = static_cast<int32_t>(speedMaxCurrentA * 100000.0f); // 速度制御モードの最大電流指示値[0.00001A]
  _roller.setSpeedMaxCurrent(speedMaxCurrent);
}

///////////////////////////////////////////////////////////////////////////////
// 位置制御モード
///////////////////////////////////////////////////////////////////////////////
void UnitRollerWrapper::setTargetPosRev(float posRev)
{
    if (!_initialized)
    {
        return;
    }
    const int32_t pos = static_cast<int32_t>(posRev * 36000.0f); // 位置指示値[0.01deg]
    _roller.setPos(pos);
}

float UnitRollerWrapper::getTargetPosRev()
{
    if (!_initialized)
    {
        return 0.0f;
    }
    const int32_t pos = _roller.getPos(); // 位置指示値[0.01deg]
    return (float)pos / 36000.0f;
}

void UnitRollerWrapper::setPosPID(float kp, float ki, float kd)
{
  if(!_initialized)
  {
    return;
  }
  const uint32_t kpScaled = static_cast<uint32_t>(std::roundf(kp * POSITION_PID_P_SCALE));
  const uint32_t kiScaled = static_cast<uint32_t>(std::roundf(ki * POSITION_PID_I_SCALE));
  const uint32_t kdScaled = static_cast<uint32_t>(std::roundf(kd * POSITION_PID_D_SCALE));
  _roller.setPosPID(kpScaled, kiScaled, kdScaled);
}

void UnitRollerWrapper::getPosPID(float &kp, float &ki, float &kd)
{
  if(!_initialized)
  {
    return;
  }
  uint32_t kpScaled, kiScaled, kdScaled;
  _roller.getPosPID(&kpScaled, &kiScaled, &kdScaled);
  kp = static_cast<float>(kpScaled) / POSITION_PID_P_SCALE;
  ki = static_cast<float>(kiScaled) / POSITION_PID_I_SCALE;
  kd = static_cast<float>(kdScaled) / POSITION_PID_D_SCALE;
}

float UnitRollerWrapper::getPosMaxCurrentA()
{
  if(!_initialized)
  {
    return 0.0f;
  }
  const int32_t posMaxCurrent = _roller.getPosMaxCurrent(); // 位置制御モードの最大電流指示値[0.00001A]
  return (float)posMaxCurrent / 100000.0f;
}

void UnitRollerWrapper::setPosMaxCurrentA(float posMaxCurrentA)
{
  if(!_initialized)
  {
    return;
  }
  const int32_t posMaxCurrent = static_cast<int32_t>(posMaxCurrentA * 100000.0f); // 位置制御モードの最大電流指示値[0.00001A]
  _roller.setPosMaxCurrent(posMaxCurrent);
}
