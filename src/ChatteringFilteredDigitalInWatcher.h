#ifndef __CHATTERINGFILTEREDDIGITALINWATCHER_H__
#define __CHATTERINGFILTEREDDIGITALINWATCHER_H__

#include "DigitalInWatcher.h"
#include "ChatteringFilteredDigitalIn.h"

class ChatteringFilteredDigitalInWatcher
{
private:
  DigitalInWatcher _watcher;
  ChatteringFilteredDigitalIn _filteredIn;

public:
  explicit ChatteringFilteredDigitalInWatcher(IDigitalIn &digitalIn, unsigned long msec, bool initialOn);
  void update();
  bool isOn() const;
  bool isRisingEdge() const;
  bool isFallingEdge() const;
};

#endif // __CHATTERINGFILTEREDDIGITALINWATCHER_H__
