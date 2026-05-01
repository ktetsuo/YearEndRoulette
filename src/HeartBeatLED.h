#ifndef __HEARTBEATLED_H__
#define __HEARTBEATLED_H__

#include "DigitalOut.h"
#include "Timer.h"

class HeartBeatLED
{
  ToggleOut _toggleOut;
  MilliSecIntervalTimer _intervalTimer;

public:
  explicit HeartBeatLED(IDigitalOut &out, unsigned long intervalms);
  void setup();
  void loop();

private:
  static void intervalHandler(void *param);
};

#endif // __HEARTBEATLED_H__
