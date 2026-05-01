#include "FlagWatcher.h"

FlagWatcher::FlagWatcher(bool initialOn)
    : _lastOn(initialOn), _isOn(initialOn)
{
}

void FlagWatcher::update(bool isOn)
{
  _lastOn = _isOn;
  _isOn = isOn;
}

bool FlagWatcher::isOn() const
{
  return _isOn;
}

bool FlagWatcher::isRisingEdge() const
{
  return _isOn && !_lastOn;
}

bool FlagWatcher::isFallingEdge() const
{
  return !_isOn && _lastOn;
}