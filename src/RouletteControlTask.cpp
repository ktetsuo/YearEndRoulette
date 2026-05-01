#include "RouletteControlTask.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "DigitalOut.h"
#include "NullStream.h"
#include "DigitalIn.h"
#include "DigitalInWatcher.h"
#include <Wire.h>
#include <unit_rolleri2c.hpp>
#include <unit_roller485.hpp>
#include "RouletteCalc.h"
#include "XSafeVariable.h"
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// 内部定義
////////////////////////////////////////////////////////////////////////////////
namespace
{
  ////////////////////////////////////////////////////////////////////////////////
  // 型定義
  ////////////////////////////////////////////////////////////////////////////////
  using RouletteControlTask::ControlMode;
  using RouletteControlTask::ControlState;
  struct SpeedPidParams
  {
    float kp;
    float ki;
    float kd;
  };
  struct SpeedPidState
  {
    SpeedPidParams params;
    bool pending;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // 定数
  ////////////////////////////////////////////////////////////////////////////////
  static constexpr unsigned long CONTROL_PERIOD_MS = 5;                           // 制御周期 10ms
  static constexpr float CONTROL_PERIOD_SEC = (float)CONTROL_PERIOD_MS / 1000.0f; // 制御周期[秒]
  static constexpr uint8_t ROLLER_I2C_ADDR = 0x64;
  static constexpr uint8_t I2C_SDA_PIN = 2;
  static constexpr uint8_t I2C_SCL_PIN = 1;
  static constexpr uint32_t I2C_FREQ = 100000;
  static constexpr int32_t ROULETTE_SPEED_RPM = 180;              // ルーレットの回転速度[rpm]
  static constexpr int32_t ROULETTE_ACCELERATION_RPM_PER_S = 120; // ルーレットの加速率[rpm/s]
  static constexpr SpeedPidParams DEFAULT_SPEED_PID_PARAMS = {
      10.0f, // P
      0.0f,  // I
      0.5f,  // D
  };
  static constexpr SpeedPidState DEFAULT_SPEED_PID_STATE = {
      DEFAULT_SPEED_PID_PARAMS,
      false,
  };

  ////////////////////////////////////////////////////////////////////////////////
  // ハードウェアオブジェクト
  ////////////////////////////////////////////////////////////////////////////////
  DigitalOut _led(LED_BUILTIN);
  ToggleOut _toggleled(_led, false);
  NullStream _nullStream;
  Stream *_serial = &_nullStream;

  UnitRollerI2C _roller;
  // トリガーセンサー1
  DigitalIn _triggerSensor1(5, INPUT_PULLUP);
  DigitalInWatcher _triggerSensor1Watcher(_triggerSensor1, false);
  // トリガーセンサー2
  DigitalIn _triggerSensor2(6, INPUT_PULLUP);
  DigitalInWatcher _triggerSensor2Watcher(_triggerSensor2, false);
  // スタートスイッチ
  DigitalIn _startSwitch(41, INPUT_PULLUP);
  DigitalInWatcher _startSwitchWatcher(_startSwitch, false);

  ////////////////////////////////////////////////////////////////////////////////
  // 変数
  ////////////////////////////////////////////////////////////////////////////////
  volatile ControlState _controlState = ControlState::IDLE;
  int32_t _targetAngle = 0; // 目標絶対角度[0.01deg] (0~ROULETTE_ONE_REVOLUTION-1)

  XSafeVariable<SpeedPidState> _speedPidState;
  volatile bool _serialOutputEnabled = true;
  volatile float _vinV = 0.0f;           // 電源電圧[V]
  volatile float _targetSpeedRpm = 0;    // 速度指令値[rpm]
  volatile float _speedRpm = 0.0f;       // 速度フィードバック値[rpm]
  volatile float _targetCurrentA = 0.0f; // 電流指令値[A]
  volatile float _currentA = 0.0f;       // 電流フィードバック値[A]
  volatile float _posRev = 0.0f;         // 位置フィードバック値[rev]

  volatile ControlMode _lastControlMode = ControlMode::NONE;
  volatile ControlMode _controlMode = ControlMode::ROULETTE_SPEED;

  ////////////////////////////////////////////////////////////////////////////////
  // 関数
  ////////////////////////////////////////////////////////////////////////////////

  /// @brief ログ出力ヘルパー内部：ベースケース（終了）
  inline size_t buildLog(char *buf, size_t offset, size_t maxLen)
  {
    return offset;
  }

  /// @brief ログ出力ヘルパー内部：バッファに再帰的に組み立て
  template <typename T, typename... Args>
  inline size_t buildLog(char *buf, size_t offset, size_t maxLen, const char *fmt, T value, Args... args)
  {
    offset += snprintf(buf + offset, maxLen - offset, fmt, value);
    return buildLog(buf, offset, maxLen, args...);
  }

  /// @brief ログ出力ヘルパー（フォーマット文字列と値をペアで指定、1回のwrite呼び出しで出力）
  /// @details logPrintf(_serial, "%lu,", stepCount, "%lu,", t0, "%.2f\r\n", value);
  template <typename T, typename... Args>
  void logPrintf(Stream *serial, const char *fmt, T value, Args... args)
  {
    char buf[512];
    size_t len = buildLog(buf, 0, sizeof(buf), fmt, value, args...);
    serial->write((const uint8_t *)buf, len);
  }

  /// @brief Rollerから電源電圧[V]を取得するヘルパー関数
  float getRollerVinV()
  {
    const int32_t vin = _roller.getVin(); // 電源電圧[10mV]
    return (float)vin / 100.0f;
  }

  /// @brief Rollerに電流指示値[A]を設定するヘルパー関数
  void setRollerTargetCurrentA(float targetCurrentA)
  {
    const int32_t current = static_cast<int32_t>(std::roundf(targetCurrentA * 100000.0f)); // 電流指示値[0.01m
    _roller.setCurrent(current);
  }

  /// @brief Rollerから電流フィードバック値[A]を取得するヘルパー関数
  float getRollerCurrentA()
  {
    const int32_t current = _roller.getCurrentReadback(); // 電流フィードバック値[0.01mA]
    return (float)current / 100000.0f;
  }

  /// @brief Rollerに速度指示値[rpm]を設定するヘルパー関数
  void setRollerTargetSpeedRpm(float targetSpeedRpm)
  {
    const int32_t speed = static_cast<int32_t>(std::roundf(targetSpeedRpm * 100.0f)); // 速度指示値[0.01rpm]
    _roller.setSpeed(speed);
  }

  /// @brief Rollerから速度フィードバック値[rpm]を取得するヘルパー関数
  float getRollerSpeedRpm()
  {
    const int32_t speed = _roller.getSpeedReadback(); // 速度フィードバック値[0.01rpm]
    return (float)speed / 100.0f;
  }

  /// @brief Rollerに位置指示値[rev]を設定するヘルパー関数
  void setRollerTargetPosRev(float targetPosRev)
  {
    const int32_t pos = static_cast<int32_t>(std::roundf(targetPosRev * (float)ROULETTE_ONE_REVOLUTION)); // 位置指示値[0.01deg]
    _roller.setPos(pos);
  }

  /// @brief Rollerから位置フィードバック値[rev]を取得するヘルパー関数
  float getRollerPosRev()
  {
    const int32_t pos = _roller.getPosReadback(); // 位置フィードバック値[0.01deg]
    return (float)pos / (float)ROULETTE_ONE_REVOLUTION;
  }

  /// @brief Speed PIDのパラメータ更新が保留されている場合にローラーに適用する
  void applyPendingSpeedPidToRoller()
  {
    SpeedPidParams params = DEFAULT_SPEED_PID_PARAMS;
    {
      auto locked = _speedPidState.lock();
      if (!locked->pending)
      {
        return;
      }
      params = locked->params;
      locked->pending = false;
    }

    const float kp = params.kp;
    const float ki = params.ki;
    const float kd = params.kd;

    const float kpNonNegative = kp < 0.0f ? 0.0f : kp;
    const float kiNonNegative = ki < 0.0f ? 0.0f : ki;
    const float kdNonNegative = kd < 0.0f ? 0.0f : kd;

    static constexpr float SPEED_PID_P_SCALE = 100000.0f;
    static constexpr float SPEED_PID_I_SCALE = 10000000.0f;
    static constexpr float SPEED_PID_D_SCALE = 100000.0f;
    const uint32_t p = static_cast<uint32_t>(std::roundf(kpNonNegative * SPEED_PID_P_SCALE));
    const uint32_t i = static_cast<uint32_t>(std::roundf(kiNonNegative * SPEED_PID_I_SCALE));
    const uint32_t d = static_cast<uint32_t>(std::roundf(kdNonNegative * SPEED_PID_D_SCALE));

    _roller.setSpeedPID(p, i, d);
  }

  // ルーレット速度制御モードでの制御ステップ
  void controlStepRouletteSpeedMode(unsigned long t0, unsigned long dt, unsigned long stepCount)
  {
    static int32_t _lastPos = 0;
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_SPEED);
      applyPendingSpeedPidToRoller();
      _roller.setSpeed(0);
      _roller.setOutput(1);
      _lastPos = _roller.getPosReadback();
    }
    const int32_t pos = _roller.getPosReadback();         // 位置フィードバック値[0.01deg]
    const int32_t speed = _roller.getSpeedReadback();     // 速度フィードバック値[0.01rpm]
    const int32_t current = _roller.getCurrentReadback(); // 電流フィードバック値[0.01mA]
    const int32_t vin = _roller.getVin();                 // 電源電圧[10mV]

    const int32_t posDiff = pos - _lastPos;
    _posRev = (float)pos / (float)ROULETTE_ONE_REVOLUTION;                                                                   // 位置フィードバック値[rev]
    _speedRpm = (float)speed / 100.0f;                                                                                       // 速度フィードバック値[rpm]
    const float speedCalcRpm = (float)posDiff / (float)ROULETTE_ONE_REVOLUTION * 60.0f * 1000.0f / (float)CONTROL_PERIOD_MS; // (位置フィードバック値から計算)
    _currentA = (float)current / 100000.0f;                                                                                  // 電流フィードバック値[A]
    _vinV = (float)vin / 100.0f;
    const bool serialOutputEnabled = _serialOutputEnabled;

    _triggerSensor1Watcher.update();
    _triggerSensor2Watcher.update();
    _startSwitchWatcher.update();

    applyPendingSpeedPidToRoller();

    static float targetPosRev = 0;         // 目標位置[rev]
    static unsigned long trigger1Time = 0; // トリガーセンサー1が反応した時間[us]
    static unsigned long trigger2Time = 0; // トリガーセンサー2が反応した時間[us]
    if (_controlState == ControlState::IDLE)
    {
      // 待機状態
      targetPosRev = 0;
      trigger1Time = 0;
      trigger2Time = 0;
      _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
      if (_startSwitchWatcher.isFallingEdge())
      {
        // スタートスイッチが押されたら加速開始
        _controlState = ControlState::ACCELERATING;
      }
    }
    if (_controlState == ControlState::ACCELERATING)
    {
      // 加速中
      if (_targetSpeedRpm < ROULETTE_SPEED_RPM)
      {
        // 目標速度に向けて徐々に加速する
        _targetSpeedRpm = _targetSpeedRpm + ((float)ROULETTE_ACCELERATION_RPM_PER_S * CONTROL_PERIOD_SEC);
        _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
      }
      else
      {
        // 目標速度に達したら、次の状態へ移行
        _targetSpeedRpm = ROULETTE_SPEED_RPM;
        _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
        _controlState = ControlState::WAITING_TRIGGER1;
      }
    }
    if (_controlState == ControlState::WAITING_TRIGGER1)
    {
      // トリガーセンサー1待ち
      // とりあえずすぐに次の状態へ移行。実際にはトリガーセンサー1の立ち下がりを待つ
      trigger1Time = t0;
      _controlState = ControlState::WAITING_TRIGGER2;
    }
    if (_controlState == ControlState::WAITING_TRIGGER2)
    {
      // トリガーセンサー2待ち
      if (_triggerSensor2Watcher.isRisingEdge())
      {
        trigger2Time = t0;
        _controlState = ControlState::TARGETING;
        // 現在の回転方向に最も近い(offset + n×360度)の位置に目標位置を設定
        //        targetPos = nextRevolutionPos(pos, speed >= 0, _targetAngle);
        targetPosRev = _posRev + 1.5f; // とりあえず1回転分先を目標位置にする
      }
    }
    if (_controlState == ControlState::TARGETING)
    {
      const uint32_t targetTime = trigger2Time + 300000; // トリガーセンサー2から0.3秒後に停止することを目標とする
      // 目標位置に向けて制御中
      if (t0 >= targetTime)
      {
        // 目標時間に達したら減速開始
        _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
        _controlState = ControlState::DECELERATING;
      }
      else
      {
        const float remainingSec = (float)(targetTime - t0) / 1000000.0f; // 目標時間までの残り時間[秒]
        const float remainingRev = targetPosRev - _posRev;                // 目標位置までの残り位置[rev]
        // 現在の速度と位置から、終速を計算
        const float finalSpeedRpm = calcFinalSpeedRpm(_speedRpm, remainingRev * 360.0f, remainingSec, 360.0f);
        if (serialOutputEnabled)
        {
          _serial->printf("T,%.2f,%.2f,%.2f,%.2f\r\n",
                          remainingSec, remainingRev, _speedRpm, finalSpeedRpm);
        }
        // 目標時間までの残り時間を考慮して、目標速度を更新
        float diffRpm = (CONTROL_PERIOD_SEC / remainingSec) * (finalSpeedRpm - _targetSpeedRpm);            // 目標時間までの残り時間を考慮した加速度
        diffRpm = std::max(diffRpm, -((float)ROULETTE_ACCELERATION_RPM_PER_S * CONTROL_PERIOD_SEC * 4.0f)); // 加速度の上限を適用
        diffRpm = std::min(diffRpm, ((float)ROULETTE_ACCELERATION_RPM_PER_S * CONTROL_PERIOD_SEC * 4.0f));  // 加速度の下限を適用
        float rpm = _targetSpeedRpm + diffRpm;
        rpm = std::max(rpm, ROULETTE_SPEED_RPM * 0.25f); // 目標速度が最低速度の1/4を下回らないようにする
        rpm = std::min(rpm, ROULETTE_SPEED_RPM * 2.0f);  // 目標速度が最大速度の2倍を超えないようにする
        if (rpm < 0)
        {
          rpm = 0;
        }
        _targetSpeedRpm = rpm;
        _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
      }
    }
    if (_controlState == ControlState::DECELERATING)
    {
      // 減速中
      // 0rpmに向けて徐々に減速する
      float rpm = rpm - ((float)ROULETTE_ACCELERATION_RPM_PER_S * CONTROL_PERIOD_SEC);
      if (rpm < 0)
      {
        rpm = 0;
      }
      _targetSpeedRpm = rpm;
      _roller.setSpeed(static_cast<int32_t>(_targetSpeedRpm * 100));
      if (_targetSpeedRpm <= 0)
      {
        _controlState = ControlState::IDLE;
      }
    }

    if (serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu, ", dt,
                "%.4f,", _targetSpeedRpm,
                "%.4f,", _speedRpm,
                "%.4f\r\n", _currentA);
      // logPrintf(_serial,
      //           "%lu,", _stepCount,
      //           "%lu,", t0,
      //           "%lu, ", _dt,
      //           "%d,", _startSwitch.isOn() ? 1 : 0,
      //           "%d,", _triggerSensor1.isOn() ? 1 : 0,
      //           "%d,", _triggerSensor2.isOn() ? 1 : 0,
      //           "%d, ", (int)_controlState,
      //           "%.2f,", _posRev,
      //           "%.2f,", _speedRpm,
      //           "%.2f,", speedCalcRpm,
      //           "%.2f,", _currentA,
      //           "%.2f,", _vinV,
      //           "%.2f,", _targetSpeedRpm,
      //           "%.2f\r\n", targetPosRev);
    }
    _lastPos = pos;
  }

  // ルーレット電流制御モードでの制御ステップ
  void controlStepRouletteCurrentMode(unsigned long t0, unsigned long dt, unsigned long stepCount)
  {
  }

  // 電流制御モードでの制御ステップ
  void controlStepDirectCurrentMode(unsigned long t0, unsigned long dt, unsigned long stepCount)
  {
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_CURRENT);
      _roller.setCurrent(0);
      _roller.setOutput(1);
    }
    _vinV = getRollerVinV();         // 電源電圧[V]
    _currentA = getRollerCurrentA(); // 電流フィードバック値[A]
    _speedRpm = getRollerSpeedRpm();   // 速度フィードバック値[rpm]
    setRollerTargetCurrentA(_targetCurrentA); // 電流指令値[A]をローラーに設定
    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu, ", dt,
                "%.4f,", _targetCurrentA,
                "%.4f,", _currentA,
                "%.4f,", _speedRpm,
                "%.4f\r\n", _vinV);
    }
  }

  // 速度制御モードでの制御ステップ
  void controlStepDirectSpeedMode(unsigned long t0, unsigned long dt, unsigned long stepCount)
  {
  }

  // 位置制御モードでの制御ステップ
  void controlStepDirectPositionMode(unsigned long t0, unsigned long _dt, unsigned long stepCount)
  {
  }

  // タスクで一定周期ごとに実行される関数
  void controlStep(void *pvParameters)
  {
    static unsigned long _stepCount = 0;
    static unsigned long _dt = 0;
    const unsigned long t0 = micros();
    // 制御モードが変わっていたら更新
    if (_controlMode != _lastControlMode)
    {
      _serial->printf("Control mode changed: %s -> %s\r\n", controlModeToString(_lastControlMode), controlModeToString(_controlMode));
      _lastControlMode = _controlMode;
      _stepCount = 0;
    }
    // 制御モードに応じた制御ステップを実行
    switch (_controlMode)
    {
    case ControlMode::NONE:
      // 制御なし
      _roller.setOutput(0);
      break;
    case ControlMode::ROULETTE_SPEED:
      controlStepRouletteSpeedMode(t0, _dt, _stepCount);
      break;
    case ControlMode::ROULETTE_CURRENT:
      controlStepRouletteCurrentMode(t0, _dt, _stepCount);
      break;
    case ControlMode::DIRECT_SPEED:
      controlStepDirectSpeedMode(t0, _dt, _stepCount);
      break;
    case ControlMode::DIRECT_CURRENT:
      controlStepDirectCurrentMode(t0, _dt, _stepCount);
      break;
    case ControlMode::DIRECT_POSITION:
      controlStepDirectPositionMode(t0, _dt, _stepCount);
      break;
    default:
      break;
    }

    _stepCount++;
    const unsigned long t1 = micros();
    _dt = t1 - t0;
  }

  // タスクで実行される関数
  void controlTask(void *pvParameters)
  {
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true)
    {
      controlStep(pvParameters);
      vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
  }

}

////////////////////////////////////////////////////////////////////////////////
// 公開関数
////////////////////////////////////////////////////////////////////////////////
namespace RouletteControlTask
{
  void setSpeedPid(float kp, float ki, float kd)
  {
    auto locked = _speedPidState.lock();
    locked->params = SpeedPidParams{kp, ki, kd};
    locked->pending = true;
  }

  void setSpeedPidKp(float kp)
  {
    auto locked = _speedPidState.lock();
    locked->params.kp = kp;
    locked->pending = true;
  }

  void setSpeedPidKi(float ki)
  {
    auto locked = _speedPidState.lock();
    locked->params.ki = ki;
    locked->pending = true;
  }

  void setSpeedPidKd(float kd)
  {
    auto locked = _speedPidState.lock();
    locked->params.kd = kd;
    locked->pending = true;
  }

  void getSpeedPid(float &kp, float &ki, float &kd)
  {
    const SpeedPidParams params = _speedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params;
    kp = params.kp;
    ki = params.ki;
    kd = params.kd;
  }

  float getSpeedPidKp()
  {
    return _speedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.kp;
  }

  float getSpeedPidKi()
  {
    return _speedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.ki;
  }

  float getSpeedPidKd()
  {
    return _speedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.kd;
  }

  void setSerialOutputEnabled(bool enabled)
  {
    _serialOutputEnabled = enabled;
  }

  bool isSerialOutputEnabled()
  {
    return _serialOutputEnabled;
  }

  void setTargetSpeedRpm(float rpm)
  {
    _targetSpeedRpm = rpm;
  }

  float getTargetSpeedRpm()
  {
    return _targetSpeedRpm;
  }

  ControlState getControlState()
  {
    return _controlState;
  }

  float getSpeedRpm()
  {
    return _speedRpm;
  }

  float getCurrentA()
  {
    return _currentA;
  }

  float getPosRev()
  {
    return _posRev;
  }

  float getVinV()
  {
    return _vinV;
  }

  float getTargetCurrentA()
  {
    return _targetCurrentA;
  }

  void setTargetCurrentA(float current)
  {
    _targetCurrentA = current;
  }

  void setControlMode(ControlMode mode)
  {
    _controlMode = mode;
  }

  ControlMode getControlMode()
  {
    return _controlMode;
  }

  const char *controlModeToString(ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::NONE:
      return "NONE";
    case ControlMode::ROULETTE_SPEED:
      return "ROULET_SPD";
    case ControlMode::ROULETTE_CURRENT:
      return "ROULET_CUR";
    case ControlMode::DIRECT_SPEED:
      return "DIRECT_SPD";
    case ControlMode::DIRECT_CURRENT:
      return "DIRECT_CUR";
    case ControlMode::DIRECT_POSITION:
      return "DIRECT_POS";
    default:
      return "UNKNOWN";
    }
  }

  /// @brief タスク開始
  void start(Stream &serial)
  {
    _led.setup();
    _toggleled.reset();
    _triggerSensor1.setup();
    _triggerSensor2.setup();
    _startSwitch.setup();
    _serial = &serial;
    _speedPidState.begin();
    _speedPidState.trySet(DEFAULT_SPEED_PID_STATE);
    _roller.begin(&Wire, ROLLER_I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    _roller.setOutput(0);
    xTaskCreate(controlTask, "Control", 8 * 1024, nullptr, 2, nullptr);
    _serial->println("RouletteControlTask started");
  }
}
