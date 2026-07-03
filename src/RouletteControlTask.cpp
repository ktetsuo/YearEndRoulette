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
#include "UnitRollerWrapper.h"
#include "RouletteCalc.h"
#include "XSafeVariable.h"
#include "PID.h"
#include "M5_EXTIO2.h"
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
  struct RollerPidParams
  {
    float kp;
    float ki;
    float kd;
  };
  struct RollerPidState
  {
    RollerPidParams params;
    bool pending;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // 定数
  ////////////////////////////////////////////////////////////////////////////////
  static constexpr unsigned long CONTROL_PERIOD_MS = 5;                           // 制御周期[ms]
  static constexpr float CONTROL_PERIOD_SEC = (float)CONTROL_PERIOD_MS / 1000.0f; // 制御周期[秒]
  static constexpr uint8_t ROLLER_I2C_ADDR = 0x64;
  static constexpr uint8_t I2C_SDA_PIN = 2;
  static constexpr uint8_t I2C_SCL_PIN = 1;
  static constexpr uint32_t I2C_FREQ = 100000;
  static constexpr int32_t ROULETTE_SPEED_RPM = 180;              // ルーレットの回転速度[rpm]
  static constexpr int32_t ROULETTE_ACCELERATION_RPM_PER_S = 120; // ルーレットの加速率[rpm/s]

  // Rollerの速度制御のPIDの初期ゲイン
  static constexpr RollerPidParams DEFAULT_SPEED_PID_PARAMS = {
      10.0f, // P
      0.0f,  // I
      0.2f,  // D
  };
  static constexpr RollerPidState DEFAULT_SPEED_PID_STATE = {
      DEFAULT_SPEED_PID_PARAMS,
      false,
  };

  // Rollerの位置制御のPIDの初期ゲイン
  static constexpr RollerPidParams DEFAULT_POS_PID_PARAMS = {
      10.0f, // P
      0.0f,  // I
      0.2f,  // D
  };
  static constexpr RollerPidState DEFAULT_POS_PID_STATE = {
      DEFAULT_POS_PID_PARAMS,
      false,
  };

  ////////////////////////////////////////////////////////////////////////////////
  // ハードウェアオブジェクト
  ////////////////////////////////////////////////////////////////////////////////
  DigitalOut _led(LED_BUILTIN);
  ToggleOut _toggleled(_led, false);
  NullStream _nullStream;
  Stream *_serial = &_nullStream;

  UnitRollerWrapper _roller;
  M5_EXTIO2 _extio;
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

  XSafeVariable<RollerPidState> _rollerSpeedPidState; // 速度PIDの状態
  XSafeVariable<RollerPidState> _rollerPosPidState;   // 位置PIDの状態
  volatile bool _serialOutputEnabled = true; // シリアル出力有効フラグ
  volatile unsigned long _loadTimeUs = 0; // 制御ループの処理時間[us]
  volatile float _vinV = 0.0f;            // 電源電圧[V]
  volatile float _targetSpeedRpm = 0;     // 速度指令値[rpm]
  volatile float _speedRpm = 0.0f;        // 速度フィードバック値[rpm]
  volatile float _targetCurrentA = 0.0f;  // 電流指令値[A]
  volatile float _currentA = 0.0f;        // 電流フィードバック値[A]
  volatile float _posRev = 0.0f;          // 位置フィードバック値[rev]
  volatile float _targetPosRev = 0.0f;    // ターゲット位置[rev]
  volatile float _targetAngleRev = 0.0f;  // ターゲット角度[rev]
  volatile int _targetNumber = 0;         // 目標の数字（1～8）
  volatile float _zeroPosRev = 0.0f;      // 位置のゼロ点（8と1の間）[rev]
  volatile unsigned long _targettingMsec = 275; // ターゲット位置に向けて制御する時間[ms]
  volatile float _targettingSec = _targettingMsec / 1000.0f; // ターゲット位置に向けて制御する時間[秒]

  volatile ControlMode _lastControlMode = ControlMode::NONE;
  volatile ControlMode _controlMode = ControlMode::ROULETTE_CURRENT;

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

  /// @brief Speed PIDのパラメータ更新が保留されている場合にローラーに適用する
  void applyPendingSpeedPidToRoller()
  {
    RollerPidParams params = DEFAULT_SPEED_PID_PARAMS;
    {
      auto locked = _rollerSpeedPidState.lock();
      if (!locked->pending)
      {
        return;
      }
      params = locked->params;
      locked->pending = false;
    }
    _roller.setSpeedPID(params.kp, params.ki, params.kd);
  }

  /// @brief Position PIDのパラメータ更新が保留されている場合にローラーに適用する
  void applyPendingPosPidToRoller()
  {
    RollerPidParams params = DEFAULT_POS_PID_PARAMS;
    {
      auto locked = _rollerPosPidState.lock();
      if (!locked->pending)      {
        return;
      }
      params = locked->params;
      locked->pending = false;
    }
    _roller.setPosPID(params.kp, params.ki, params.kd);
  }

  /// @brief 現在の位置に最も近いターゲット位置を計算する
  /// @param posRev 現在の位置[rev]
  /// @param targetAngleRev ターゲット角度[rev]
  /// @return 目標位置[rev]
  /// @details ターゲット位置は、posRevに最も近い(targetAngleRev + n)の形になる
  float calcNearestTargetPosRev(float posRev, float targetAngleRev)
  {
    float targetFrac = targetAngleRev - std::floor(targetAngleRev); // ターゲット角度の小数部分
    float posFrac = posRev - std::floor(posRev);                    // 現在位置の小数部分
    float diff = targetFrac - posFrac;                              // ターゲット角度と現在位置の小数部分の差
    if (diff > 0.5f)
    {
      diff -= 1.0f; // ターゲット角度の方が現在位置より大きい場合、差が0.5を超えるときは逆回りの方が近い
    }
    else if (diff < -0.5f)
    {
      diff += 1.0f; // ターゲット角度の方が現在位置より小さい場合、差が-0.5を下回るときは逆回りの方が近い
    }
    return posRev + diff; // 現在位置に差を加えることで、最も近いターゲット位置を計算
  }

  /// @brief 回転位置を0.0～1.0の範囲に正規化する
  float normalizeRev(float rev)
  {
    return rev - std::floor(rev);
  }

  /// @brief 数字の中心に対応する角度を計算する
  /// @param number ルーレットの数字（1～8）
  /// @return 角度[rev]
  /// @details ルーレットの数字に対応する角度は、1～8の数字に対して0.0～1.0の範囲で割り当てられる
  float numberToAngleRev(int number)
  {
    if (number < 1 || number > 8)
    {
      return 0.0f;
    }
    // 0.5を引くことで数字の中心に対応する角度を計算し、ゼロ点を加算
    const float angle = (number - 0.5f) / 8.0f + _zeroPosRev;
    // 角度を0.0～1.0の範囲に正規化
    return normalizeRev(angle);
  }

  /// @brief 制御なしモードでの制御ステップ
  void controlStepNoneMode(unsigned long t0, unsigned long stepCount)
  {
    _roller.setOutput(0);
    _extio.setAllDigitalOutputs(0xff);
    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu,", stepCount,
                "%lu,", t0,
                "%lu, ", _loadTimeUs,
                "%d,", _startSwitch.isOn() ? 1 : 0,
                "%d,", _triggerSensor1.isOn() ? 1 : 0,
                "%d,", _triggerSensor2.isOn() ? 1 : 0,
                "%.2f,", _posRev,
                "%.2f,", _speedRpm,
                "%.2f,", _currentA,
                "%.2f\r\n", _vinV);
    }
  }

  /// @brief ルーレット速度制御モードでの制御ステップ
  void controlStepRouletteSpeedMode(unsigned long t0, unsigned long stepCount)
  {
    static float _lastPosRev = 0;
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_SPEED);
      applyPendingSpeedPidToRoller();
      _roller.setTargetSpeedRpm(0.0f);
      _roller.setOutput(1);
      _lastPosRev = _posRev;
    }

    const float posDiffRev = _posRev - _lastPosRev;
    const float speedCalcRpm = posDiffRev * 60.0f / CONTROL_PERIOD_SEC; // (位置フィードバック値から計算)
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
      _roller.setTargetSpeedRpm(_targetSpeedRpm);
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
        _roller.setTargetSpeedRpm(_targetSpeedRpm);
      }
      else
      {
        // 目標速度に達したら、次の状態へ移行
        _targetSpeedRpm = ROULETTE_SPEED_RPM;
        _roller.setTargetSpeedRpm(_targetSpeedRpm);
        _controlState = ControlState::WAITING_TRIGGER1;
      }
    }
    if (_controlState == ControlState::WAITING_TRIGGER1)
    {
      // トリガーセンサー1待ち
      // とりあえずすぐに次の状態へ移行。実際にはトリガーセンサー1の立ち下がりを待つ
      trigger1Time = t0;
      _controlState = ControlState::TARGETING1;
    }
    if (_controlState == ControlState::TARGETING1)
    {
      // トリガーセンサー2待ち
      if (_triggerSensor2Watcher.isRisingEdge())
      {
        trigger2Time = t0;
        _controlState = ControlState::TARGETING2;
        // 現在の回転方向に最も近い(offset + n×360度)の位置に目標位置を設定
        //        targetPos = nextRevolutionPos(pos, speed >= 0, _targetAngle);
        targetPosRev = _posRev + 1.5f; // とりあえず1回転分先を目標位置にする
      }
    }
    if (_controlState == ControlState::TARGETING2)
    {
      const uint32_t targetTime = trigger2Time + 300000; // トリガーセンサー2から0.3秒後に停止することを目標とする
      // 目標位置に向けて制御中
      if (t0 >= targetTime)
      {
        // 目標時間に達したら減速開始
        _roller.setTargetSpeedRpm(_targetSpeedRpm);
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
        _roller.setTargetSpeedRpm(_targetSpeedRpm);
      }
    }
    if (_controlState == ControlState::DECELERATING)
    {
      // 減速中
      // 0rpmに向けて徐々に減速する
      float rpm = _targetSpeedRpm - ((float)ROULETTE_ACCELERATION_RPM_PER_S * CONTROL_PERIOD_SEC);
      if (rpm < 0)
      {
        rpm = 0;
      }
      _targetSpeedRpm = rpm;
      _roller.setTargetSpeedRpm(_targetSpeedRpm);
      if (_targetSpeedRpm <= 0)
      {
        _controlState = ControlState::IDLE;
      }
    }

    if (serialOutputEnabled)
    {
      // logPrintf(_serial,
      //           "%lu, ", dt,
      //           "%.4f,", _targetSpeedRpm,
      //           "%.4f,", _speedRpm,
      //           "%.4f\r\n", _currentA);
      logPrintf(_serial,
                "%lu,", stepCount,
                "%lu,", t0,
                "%lu, ", _loadTimeUs,
                "%d,", _startSwitch.isOn() ? 1 : 0,
                "%d,", _triggerSensor1.isOn() ? 1 : 0,
                "%d,", _triggerSensor2.isOn() ? 1 : 0,
                "%d, ", (int)_controlState,
                "%.2f,", _posRev,
                "%.2f,", _speedRpm,
                "%.2f,", speedCalcRpm,
                "%.2f,", _currentA,
                "%.2f,", _vinV,
                "%.2f,", _targetSpeedRpm,
                "%.2f\r\n", targetPosRev);
    }
    _lastPosRev = _posRev;
  }

  // @brief ルーレット電流制御モードでの制御ステップ
  void controlStepRouletteCurrentMode(unsigned long t0, unsigned long stepCount)
  {
    static float _lastPosRev = 0;
    static unsigned long _trigger1Time = 0;          // トリガーセンサー1が反応した時間[us]
    static unsigned long _trigger2Time = 0;          // トリガーセンサー2が反応した時間[us]
    static unsigned long _triggerTimeDiff = 0;       // トリガーセンサー1と2の反応時間差[us]
    static unsigned long _ledCount = 0;              // LEDの点滅用カウンタ
    static float _targettingPosRev = 0.0f;           // ターゲット位置[rev]
    static float _remainingRev = 0.0f;               // 目標位置までの残り位置[rev]
    static float _remainingSec = 0.0f;               // 目標時間までの残り時間[秒]
    static PID _speedPid(
        DEFAULT_SPEED_PID_PARAMS.kp,
        DEFAULT_SPEED_PID_PARAMS.ki,
        DEFAULT_SPEED_PID_PARAMS.kd,
        CONTROL_PERIOD_SEC,
        0.0f, // outputMin
        1.0f, // outputMax
        0.0f  // targetValue
    );
    const float currentRevPerSec = _speedRpm / 60.0f; // 現在速度[rev/s]

    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_CURRENT);
      _roller.setTargetCurrentA(0.0f);
      _roller.setOutput(1);
      _controlState = ControlState::IDLE;
      _trigger1Time = 0;
      _trigger2Time = 0;
      _triggerTimeDiff = 0;
      _ledCount = 0;
      _targettingPosRev = 0.0f;
      _remainingRev = 0.0f;
      _remainingSec = 0.0f;
      _lastPosRev = _posRev;
    }

    _triggerSensor1Watcher.update();
    _triggerSensor2Watcher.update();
    _startSwitchWatcher.update();

    if (_controlState == ControlState::IDLE)
    {
      _ledCount = 0; // LED消灯
      _trigger1Time = 0;
      _trigger2Time = 0;
      _triggerTimeDiff = 0;
      _targettingPosRev = 0.0f;
      _remainingRev = 0.0f;
      _remainingSec = 0.0f;
      if (_startSwitchWatcher.isFallingEdge() || _triggerSensor1Watcher.isFallingEdge())
      {
        // スタートスイッチが押されたかトリガーセンサー1がONになったら加速開始
        _controlState = ControlState::ACCELERATING;
        _targetCurrentA = 0.5f; // とりあえず0.5Aにする
      }
    }
    if (_controlState == ControlState::ACCELERATING)
    {
      // 加速中
      if (std::fabs(_speedRpm - ROULETTE_SPEED_RPM) < 10.0f)
      {
        // 速度が目標速度に近づいたら、次の状態へ移行
        _controlState = ControlState::WAITING_TRIGGER1;
        _speedPid.reset();
        _speedPid.targetValue(ROULETTE_SPEED_RPM);
      }
    }
    if (_controlState == ControlState::WAITING_TRIGGER1)
    {
      // トリガーセンサー1待ち
      _speedPid.update(_speedRpm);
      _targetCurrentA = _speedPid.output();
      if (_triggerSensor1Watcher.isRisingEdge())
      {
        // トリガーセンサー1が反応した時間を記録
        _trigger1Time = t0;
        // 現在の速度で、トリガーセンサー1から一定時間後に到達する予想位置を求める
        const float estimatedReachPosRev = _posRev + currentRevPerSec * _targettingSec;
        // ターゲット位置を、目標の数字に対応する角度に最も近い位置に設定
        _targettingPosRev = calcNearestTargetPosRev(estimatedReachPosRev, numberToAngleRev(_targetNumber));
        // 次の状態へ移行
        _controlState = ControlState::TARGETING1;
      }
    }
    if (_controlState == ControlState::TARGETING1 || _controlState == ControlState::TARGETING2)
    {
      // ターゲット位置に向けて制御中
      _ledCount++;  // LED点滅用カウンタを更新
      _remainingRev = _targettingPosRev - _posRev;                                 // 目標位置までの残り位置[rev]
      _remainingSec = _targettingSec - (float)(t0 - _trigger1Time) / 1000000.0f; // 目標時間までの残り時間[秒]
      const float kRpmPerSecA = 1000.0f;                                           // 電流指令値1Aあたりの加速度[rpm/s/A]

      // --- 必要な加速度と電流指令値の計算例 ---
      if (_remainingSec > 0.01f)
      {
        // 等加速度運動の公式: x = v0 * t + 0.5 * a * t^2 から a を求める
        float requiredAcc = 2.0f * (_remainingRev - currentRevPerSec * _remainingSec) / (_remainingSec * _remainingSec); // [rev/s^2]
        // 加速度[rev/s^2]→[rpm/s^2]に変換
        float requiredAccRpm = requiredAcc * 60.0f;
        // 電流指令値に変換（加速度→電流の比例定数で割る）
        float requiredCurrent = requiredAccRpm / kRpmPerSecA; // [A]
        // 安全のためクリップ
        requiredCurrent = std::max(requiredCurrent, -1.0f);
        requiredCurrent = std::min(requiredCurrent, 1.0f);
        _targetCurrentA = requiredCurrent;
      }

      if (_controlState == ControlState::TARGETING1)
      {
        // トリガーセンサー2待ち
        if (_triggerSensor2Watcher.isFallingEdge())
        {
          _trigger2Time = t0;
          _triggerTimeDiff = _trigger2Time - _trigger1Time;
          _controlState = ControlState::TARGETING2;
        }
      }
      if (t0 - _trigger1Time >= _targettingMsec * 1000)
      {
        // トリガーセンサー1から一定時間経過後に減速開始
        _controlState = ControlState::DECELERATING;
      }
    }
    if (_controlState == ControlState::DECELERATING)
    {
      // 減速中
      _ledCount++;  // LED点滅用カウンタを更新
      _targetCurrentA = 0.0f;
      if (std::fabs(_speedRpm) <= 1.0f)
      {
        _controlState = ControlState::IDLE;
      }
    }
    // 電流指令値をローラーに設定
    _roller.setTargetCurrentA(_targetCurrentA);

    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "@%lu, ", _loadTimeUs,
                "%d, ", (int)_controlState,
                "%.4f,", _targetCurrentA,
                "%.4f,", _currentA,
                "%.4f,", _speedRpm,
                "%.4f,", _posRev,
                "%.4f,", _targettingPosRev,
                "%.4f,", _remainingRev,
                "%.4f,", _remainingSec,
                "%.4f\r\n", _vinV);
    }
    // LED表示
    _extio.setAllDigitalOutputs(~(_ledCount & 0x000000ffUL));

    _lastPosRev = _posRev;
  }

  // @brief 電流制御モードでの制御ステップ
  void controlStepDirectCurrentMode(unsigned long t0, unsigned long stepCount)
  {
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_CURRENT);
      _roller.setTargetCurrentA(0.0f);
      _roller.setOutput(1);
    }
    _roller.setTargetCurrentA(_targetCurrentA);
    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu, ", _loadTimeUs,
                "%.4f,", _targetCurrentA,
                "%.4f,", _currentA,
                "%.4f,", _speedRpm,
                "%.4f,", _posRev,
                "%.4f\r\n", _vinV);
    }
  }

  // @brief 速度制御モードでの制御ステップ
  void controlStepDirectSpeedMode(unsigned long t0, unsigned long stepCount)
  {
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_SPEED);
      _roller.setTargetSpeedRpm(0.0f);
      _roller.setOutput(1);
    }
    _roller.setTargetSpeedRpm(_targetSpeedRpm);
    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu, ", _loadTimeUs,
                "%.4f,", _targetSpeedRpm,
                "%.4f,", _currentA,
                "%.4f,", _speedRpm,
                "%.4f,", _posRev,
                "%.4f\r\n", _vinV);
    }
  }

  // @brief 位置制御モードでの制御ステップ
  void controlStepDirectPositionMode(unsigned long t0, unsigned long stepCount)
  {
    if (stepCount == 0)
    {
      // モード変更直後の初期化処理
      _roller.setOutput(0);
      _roller.setMode(ROLLER_MODE_POSITION);
      _roller.setTargetPosRev(_posRev);
      _roller.setOutput(1);
    }
    applyPendingPosPidToRoller();
    _targetPosRev = calcNearestTargetPosRev(_posRev, _targetAngleRev);
    _roller.setTargetPosRev(_targetPosRev);
    if (_serialOutputEnabled)
    {
      logPrintf(_serial,
                "%lu, ", _loadTimeUs,
                "%.4f,", _targetAngleRev,
                "%.4f,", _targetPosRev,
                "%.4f,", _posRev,
                "%.4f,", _currentA,
                "%.4f,", _speedRpm,
                "%.4f\r\n", _vinV);
    }
  }

  // @brief タスクで一定周期ごとに実行される関数
  void controlStep(void *pvParameters)
  {
    static unsigned long _stepCount = 0;
    const unsigned long t0 = micros();
    // 制御モードが変わっていたら更新
    if (_controlMode != _lastControlMode)
    {
      _serial->printf("Control mode changed: %s -> %s\r\n", controlModeToString(_lastControlMode), controlModeToString(_controlMode));
      _lastControlMode = _controlMode;
      _stepCount = 0;
    }
    // 各種フィードバック値の取得
    _vinV = _roller.getVinV();
    _currentA = _roller.getCurrentA();
    _speedRpm = _roller.getSpeedRpm();
    _posRev = _roller.getPosRev();
    // 制御モードに応じた制御ステップを実行
    switch (_controlMode)
    {
    case ControlMode::NONE:
      controlStepNoneMode(t0, _stepCount);
      break;
    case ControlMode::ROULETTE_SPEED:
      controlStepRouletteSpeedMode(t0, _stepCount);
      break;
    case ControlMode::ROULETTE_CURRENT:
      controlStepRouletteCurrentMode(t0, _stepCount);
      break;
    case ControlMode::DIRECT_SPEED:
      controlStepDirectSpeedMode(t0, _stepCount);
      break;
    case ControlMode::DIRECT_CURRENT:
      controlStepDirectCurrentMode(t0, _stepCount);
      break;
    case ControlMode::DIRECT_POSITION:
      controlStepDirectPositionMode(t0, _stepCount);
      break;
    default:
      break;
    }

    _stepCount++;
    const unsigned long t1 = micros();
    _loadTimeUs = t1 - t0;
  }

  // @brief タスクで実行される関数
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
    auto locked = _rollerSpeedPidState.lock();
    locked->params = RollerPidParams{kp, ki, kd};
    locked->pending = true;
  }

  void setSpeedPidKp(float kp)
  {
    auto locked = _rollerSpeedPidState.lock();
    locked->params.kp = kp;
    locked->pending = true;
  }

  void setSpeedPidKi(float ki)
  {
    auto locked = _rollerSpeedPidState.lock();
    locked->params.ki = ki;
    locked->pending = true;
  }

  void setSpeedPidKd(float kd)
  {
    auto locked = _rollerSpeedPidState.lock();
    locked->params.kd = kd;
    locked->pending = true;
  }

  void getSpeedPid(float &kp, float &ki, float &kd)
  {
    const RollerPidParams params = _rollerSpeedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params;
    kp = params.kp;
    ki = params.ki;
    kd = params.kd;
  }

  float getSpeedPidKp()
  {
    return _rollerSpeedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.kp;
  }

  float getSpeedPidKi()
  {
    return _rollerSpeedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.ki;
  }

  float getSpeedPidKd()
  {
    return _rollerSpeedPidState.tryGet().valueOr(DEFAULT_SPEED_PID_STATE).params.kd;
  }

  void setPosPid(float kp, float ki, float kd)
  {
    auto locked = _rollerPosPidState.lock();
    locked->params = RollerPidParams{kp, ki, kd};
    locked->pending = true;
  }

  void setPosPidKp(float kp)
  {
    auto locked = _rollerPosPidState.lock();
    locked->params.kp = kp;
    locked->pending = true;
  }

  void setPosPidKi(float ki)
  {
    auto locked = _rollerPosPidState.lock();
    locked->params.ki = ki;
    locked->pending = true;
  }

  void setPosPidKd(float kd)
  {
    auto locked = _rollerPosPidState.lock();
    locked->params.kd = kd;
    locked->pending = true;
  }

  void getPosPid(float &kp, float &ki, float &kd)
  {
    const RollerPidParams params = _rollerPosPidState.tryGet().valueOr(DEFAULT_POS_PID_STATE).params;
    kp = params.kp;
    ki = params.ki;
    kd = params.kd;
  }

  float getPosPidKp()
  {
    return _rollerPosPidState.tryGet().valueOr(DEFAULT_POS_PID_STATE).params.kp;
  }

  float getPosPidKi()
  {
    return _rollerPosPidState.tryGet().valueOr(DEFAULT_POS_PID_STATE).params.ki;
  }

  float getPosPidKd()
  {
    return _rollerPosPidState.tryGet().valueOr(DEFAULT_POS_PID_STATE).params.kd;
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

  float getTargetPosRev()
  {
    return _targetPosRev;
  }
  // _targetPosは取得専用
  // void setTargetPosRev(float posRev)
  // {
  //   _targetPosRev = posRev;
  // }

  float getTargetAngleRev()
  {
    return _targetAngleRev;
  }

  void setTargetAngleRev(float angleRev)
  {
    _targetAngleRev = angleRev;
  }

  void setControlMode(ControlMode mode)
  {
    _controlMode = mode;
  }

  ControlMode getControlMode()
  {
    return _controlMode;
  }

  void setTargetNumber(int number)
  {
    _targetNumber = number;
  }

  int getTargetNumber()
  {
    return _targetNumber;
  }

  void resetZeroPosRev()
  {
    // 現在位置をゼロ点として設定（0.0～1.0の範囲に正規化）
    _zeroPosRev = normalizeRev(_posRev);
  }

  void setTargettingMsec(unsigned long msec)
  {
    _targettingMsec = msec;
    _targettingSec = msec / 1000.0f;
  }

  unsigned long getTargettingMsec()
  {
    return _targettingMsec;
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
    _rollerSpeedPidState.begin();
    _rollerSpeedPidState.trySet(DEFAULT_SPEED_PID_STATE);
    _rollerPosPidState.begin();
    _rollerPosPidState.trySet(DEFAULT_POS_PID_STATE);
    _roller.begin(&Wire, ROLLER_I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    _roller.setOutput(0);
    _extio.begin(&Wire, I2C_SDA_PIN, I2C_SCL_PIN, EXTIO2_DEFAULT_ADDR, false);
    _extio.setAllPinMode(DIGITAL_OUTPUT_MODE);
    _extio.setAllDigitalOutputs(0xff);
    xTaskCreate(controlTask, "Control", 8 * 1024, nullptr, 2, nullptr);
    _serial->println("RouletteControlTask started");
  }
}
