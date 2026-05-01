#include "HeartBeatLED.h"

HeartBeatLED::HeartBeatLED(IDigitalOut &out, unsigned long intervalms)
    : _toggleOut(out, false), _intervalTimer(intervalms, intervalHandler, this)
{
}

void HeartBeatLED::intervalHandler(void *param)
{
  if (param)
  {
    HeartBeatLED *self = reinterpret_cast<HeartBeatLED *>(param);
    self->_toggleOut.toggle();
  }
}
void HeartBeatLED::setup()
{
  _toggleOut.reset();
}
void HeartBeatLED::loop()
{
  _intervalTimer.update();
}
