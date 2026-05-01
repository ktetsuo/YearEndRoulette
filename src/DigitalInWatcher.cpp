#include "DigitalInWatcher.h"
#include "DigitalIn.h"

DigitalInWatcher::DigitalInWatcher(IDigitalIn &digitalIn, bool initialOn)
    : _flagWatcher(initialOn), _digitalIn(digitalIn)
{
}

void DigitalInWatcher::update()
{
  _flagWatcher.update(_digitalIn.isOn());
}

bool DigitalInWatcher::isOn() const
{
  return _flagWatcher.isOn();
}

bool DigitalInWatcher::isRisingEdge() const
{
  return _flagWatcher.isRisingEdge();
}

bool DigitalInWatcher::isFallingEdge() const
{
  return _flagWatcher.isFallingEdge();
}