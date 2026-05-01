#include "ChatteringFilteredDigitalInWatcher.h"

ChatteringFilteredDigitalInWatcher::ChatteringFilteredDigitalInWatcher(IDigitalIn &digitalIn, unsigned long msec, bool initialOn)
    : _filteredIn(digitalIn, msec, initialOn), _watcher(_filteredIn, initialOn)
{
}

void ChatteringFilteredDigitalInWatcher::update()
{
  _filteredIn.update();
  _watcher.update();
}

bool ChatteringFilteredDigitalInWatcher::isOn() const
{
  return _watcher.isOn();
}

bool ChatteringFilteredDigitalInWatcher::isRisingEdge() const
{
  return _watcher.isRisingEdge();
}

bool ChatteringFilteredDigitalInWatcher::isFallingEdge() const
{
  return _watcher.isFallingEdge();
}