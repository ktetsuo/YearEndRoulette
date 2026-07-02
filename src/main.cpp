#include <Arduino.h>
#include <M5GFX.h>
#include "RouletteControlTask.h"
#include "XSafeStream.h"
#include "SerialConsole.h"
#include "ValueCommand.h"
#include "RotationBuffer.h"
#include <BleSerial.h>
#include <array>
#include <cstdio>

namespace
{
  BleSerial _bleSerial;

  m5gfx::M5GFX _display;
  m5gfx::M5Canvas _canvas(&_display);
  RotationBuffer<unsigned long, 10> _intervalMsBuffer;

  XSafeStream<128> _safeSerial;
  ValueCommand<float> _cmdSpeedPidKp("spdkp", RouletteControlTask::getSpeedPidKp, RouletteControlTask::setSpeedPidKp);
  ValueCommand<float> _cmdSpeedPidKi("spdki", RouletteControlTask::getSpeedPidKi, RouletteControlTask::setSpeedPidKi);
  ValueCommand<float> _cmdSpeedPidKd("spdkd", RouletteControlTask::getSpeedPidKd, RouletteControlTask::setSpeedPidKd);
  ValueCommand<float> _cmdPosPidKp("poskp", RouletteControlTask::getPosPidKp, RouletteControlTask::setPosPidKp);
  ValueCommand<float> _cmdPosPidKi("poski", RouletteControlTask::getPosPidKi, RouletteControlTask::setPosPidKi);
  ValueCommand<float> _cmdPosPidKd("poskd", RouletteControlTask::getPosPidKd, RouletteControlTask::setPosPidKd);
  ValueCommand<bool> _cmdControlLog("log", RouletteControlTask::isSerialOutputEnabled, RouletteControlTask::setSerialOutputEnabled);
  ValueCommand<float> _cmdSpeedRpm("spd", RouletteControlTask::getTargetSpeedRpm, RouletteControlTask::setTargetSpeedRpm);
  ValueCommand<float> _cmdTargetCurrentA("cur", RouletteControlTask::getTargetCurrentA, RouletteControlTask::setTargetCurrentA);
  ValueCommand<float> _cmdTargetAngleRev("ang", RouletteControlTask::getTargetAngleRev, RouletteControlTask::setTargetAngleRev);
  ValueCommand<int> _cmdTargetNumber("num", RouletteControlTask::getTargetNumber, RouletteControlTask::setTargetNumber);
  class ControlModeCommand : public IConsoleCommand
  {
  public:
    virtual bool run(const ArrayString<CMD_MAX_LEN> &cmdline, Print &printer) const override
    {
      if (cmdline == "mode=")
      {
        printer.printf("Current Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::getControlMode()));
        return true;
      }
      if (cmdline == "mode=none")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::NONE);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::NONE));
        return true;
      }
      else if (cmdline == "mode=rs")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::ROULETTE_SPEED);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::ROULETTE_SPEED));
        return true;
      }
      else if (cmdline == "mode=rc")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::ROULETTE_CURRENT);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::ROULETTE_CURRENT));
        return true;
      }
      else if (cmdline == "mode=dp")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::DIRECT_POSITION);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::DIRECT_POSITION));
        return true;
      }
      else if (cmdline == "mode=ds")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::DIRECT_SPEED);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::DIRECT_SPEED));
        return true;
      }
      else if (cmdline == "mode=dc")
      {
        RouletteControlTask::setControlMode(RouletteControlTask::ControlMode::DIRECT_CURRENT);
        printer.printf("Set Control Mode: %s\r\n", RouletteControlTask::controlModeToString(RouletteControlTask::ControlMode::DIRECT_CURRENT));
        return true;
      }
      return false;
    }
    virtual void info(Print &printer) const override
    {
      const RouletteControlTask::ControlMode mode = RouletteControlTask::getControlMode();
      printer.printf("Current Control Mode: %s\r\n", RouletteControlTask::controlModeToString(mode));
    }
  };
  ControlModeCommand _cmdControlMode;
  const std::array<const IConsoleCommand *, 8> _commands = {
      &_cmdSpeedPidKp,
      &_cmdSpeedPidKi,
      &_cmdSpeedPidKd,
      &_cmdControlLog,
      &_cmdSpeedRpm,
      &_cmdTargetCurrentA,
      &_cmdTargetAngleRev,
      &_cmdControlMode,
  };
  SerialConsole _console(_safeSerial, _commands);

  const char* controlStateToString(RouletteControlTask::ControlState state)
  {
    switch (state)
    {
      case RouletteControlTask::ControlState::IDLE:
        return "IDLE";
      case RouletteControlTask::ControlState::ACCELERATING:
        return "ACCEL";
      case RouletteControlTask::ControlState::WAITING_TRIGGER1:
        return "WAIT1";
      case RouletteControlTask::ControlState::TARGETING1:
        return "TARGET1";
      case RouletteControlTask::ControlState::TARGETING2:
        return "TARGET2";
      case RouletteControlTask::ControlState::DECELERATING:
        return "DECEL";
      default:
        return "UNKNOWN";
    }
  }
}

void setup() {
  _display.begin();
  _display.setRotation(0);
  _display.fillScreen(BLACK);

  _canvas.setColorDepth(16);
  _canvas.createSprite(_display.width(), _display.height());
  _canvas.setTextSize(1);
  _canvas.setFont(&fonts::AsciiFont8x16);
  _canvas.setTextColor(WHITE, BLACK);

  Serial.begin(115200);
  _safeSerial.begin(&Serial, 32, "SafeSerial");
  _bleSerial.begin("YearEndRoulette");
  _safeSerial.println("Start");
  RouletteControlTask::start(_safeSerial);
}

void loop() {
  const unsigned long t0 = micros();
  _console.run();

  const float vinV = RouletteControlTask::getVinV();
  const RouletteControlTask::ControlState state = RouletteControlTask::getControlState();
  const float targetSpeedRpm = RouletteControlTask::getTargetSpeedRpm();
  const float speedRpm = RouletteControlTask::getSpeedRpm();
  const float currentA = RouletteControlTask::getCurrentA();
  const float posRev = RouletteControlTask::getPosRev();

  constexpr int lcdWidth = 128;
  constexpr int lcdHeight = 128;
  constexpr int fontWidth = 8;
  constexpr int fontHeight = 16;

  _canvas.fillScreen(BLACK);
  // 電圧を表示
  _canvas.setCursor(lcdWidth - 1 - fontWidth * 6, 0);
  _canvas.printf("%5.2fV", vinV);
  // 制御状態を表示
  _canvas.setCursor(0, 0);
  _canvas.print(controlStateToString(state));
  // 速度指示を表示
  _canvas.setCursor(0, fontHeight);
  _canvas.printf("Set:%6.2frpm", targetSpeedRpm);
  // 現在の速度を表示
  _canvas.setCursor(0, fontHeight * 2);
  _canvas.printf("Cur:%6.2frpm", speedRpm);
  // 現在の電流を表示
  _canvas.setCursor(0, fontHeight * 3);
  _canvas.printf("Cur:%6.2fA", currentA);
  // 現在の位置を表示
  _canvas.setCursor(0, fontHeight * 4);
  _canvas.printf("Pos:%6.2f", posRev);
  // 過去の制御ループの実行時間の最大値を表示
  const unsigned long intervalMsMax = *std::max_element(_intervalMsBuffer.begin(), _intervalMsBuffer.end());
  _canvas.setCursor(lcdWidth - 1 - fontWidth * 4, lcdHeight - 1 - fontHeight);
  _canvas.printf("%4lu", _intervalMsBuffer);

  _canvas.pushSprite(0, 0);

  // BLEで受信したデータをシリアルに出力してエコーする
  const int bleAvailable = _bleSerial.available();
  if (bleAvailable > 0)  {
    uint8_t buf[bleAvailable + 1];
    _bleSerial.readBytes(buf, bleAvailable);
    _bleSerial.write(buf, bleAvailable);
    _safeSerial.write(buf, bleAvailable);
  }

  _intervalMsBuffer.add(millis() - t0);
}
