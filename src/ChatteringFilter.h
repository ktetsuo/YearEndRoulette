#ifndef __CHATTERINGFILTER_H__
#define __CHATTERINGFILTER_H__

#include "Timer.h"

class ChatteringFilter
{
  MilliSecTimeoutTimer _timeoutTimer;
  bool _isOn;
  bool _lastInputState;
  bool _firstUpdate;

public:
  explicit ChatteringFilter(unsigned long msec, bool initialOn);
  void update(bool isOn);
  bool isOn() const;
};
#endif // __CHATTERINGFILTER_H__
