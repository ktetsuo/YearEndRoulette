#ifndef __ROULETTE_CONTROL_TASK_H__
#define __ROULETTE_CONTROL_TASK_H__

#include <Arduino.h>

namespace RouletteControlTask
{
  /// @brief 制御モードを表す列挙型
  enum class ControlMode
  {
    NONE,             // 制御なし
    ROULETTE_SPEED,   // 速度によるルーレット制御モード
    ROULETTE_CURRENT, // 電流によるルーレット制御モード
    DIRECT_SPEED,     // 速度を直接指定モード
    DIRECT_CURRENT,   // 電流を直接指定モード
    DIRECT_POSITION,  // 位置を直接指定モード
  };
  /// @brief 制御モードを文字列に変換する
  const char* controlModeToString(ControlMode mode);

  /// @brief 制御状態を表す列挙型
  enum class ControlState
  {
    IDLE,             // 待機状態
    ACCELERATING,     // 加速中
    WAITING_TRIGGER1, // トリガーセンサー1待ち
    WAITING_TRIGGER2, // トリガーセンサー2待ち
    TARGETING,        // 目標位置に向けて制御中
    DECELERATING,     // 減速中
  };

  void start(Stream &serial);

  /// @brief 停止目標の絶対角度を設定する
  /// @param targetAngle 1回転内の目標絶対角度 [0.01deg] (0~35999, 例: 9000 = 90度)
  void setTargetAngle(int32_t targetAngle);

  /// @brief 速度PIDのPゲインを設定する
  void setSpeedPid(float kp, float ki, float kd);

  /// @brief 速度PIDのPゲインを設定する
  void setSpeedPidKp(float kp);
  /// @brief 速度PIDのIゲインを設定する
  void setSpeedPidKi(float ki);
  /// @brief 速度PIDのDゲインを設定する
  void setSpeedPidKd(float kd);

  /// @brief 速度PIDのPゲインを取得する
  void getSpeedPid(float &kp, float &ki, float &kd);

  /// @brief 速度PIDのPゲインを取得する
  float getSpeedPidKp();
  /// @brief 速度PIDのIゲインを取得する
  float getSpeedPidKi();
  /// @brief 速度PIDのDゲインを取得する
  float getSpeedPidKd();

  /// @brief RouletteControlTaskの定期シリアル出力を有効/無効にする
  void setSerialOutputEnabled(bool enabled);

  /// @brief RouletteControlTaskの定期シリアル出力が有効か取得する
  bool isSerialOutputEnabled();

  /// @brief 制御状態を取得する
  ControlState getControlState();
  /// @brief 電源電圧を取得する [V]
  float getVinV();

  /// @brief 速度指示を設定する [rpm]
  void setTargetSpeedRpm(float rpm);
  /// @brief 速度指示を取得する [rpm]
  float getTargetSpeedRpm();
  /// @brief 現在の速度を取得する [rpm]
  float getSpeedRpm();
  /// @brief 現在の電流を取得する [A]
  float getCurrentA();
  /// @brief 現在の位置を取得する [rev]
  float getPosRev();
  /// @brief 電流指示を設定する [A]
  void setTargetCurrentA(float current);
  /// @brief 電流指示を取得する [A]
  float getTargetCurrentA();

  /// @brief 制御モードを設定する
  void setControlMode(ControlMode mode);
  /// @brief 制御モードを取得する
  ControlMode getControlMode();
}

#endif // __ROULETTE_CONTROL_TASK_H__
