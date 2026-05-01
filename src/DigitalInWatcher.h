#ifndef __DIGITALINWATCHER_H__
#define __DIGITALINWATCHER_H__

#include "FlagWatcher.h"
class IDigitalIn;

class DigitalInWatcher
{
private:
  FlagWatcher _flagWatcher;
  IDigitalIn &_digitalIn;

public:
  DigitalInWatcher(IDigitalIn &digitalIn, bool initialOn);
  void update();
  bool isOn() const;
  bool isRisingEdge() const;
  bool isFallingEdge() const;
};

#endif // __DIGITALINWATCHER_H__
