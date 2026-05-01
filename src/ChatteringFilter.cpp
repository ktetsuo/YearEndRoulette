#include "ChatteringFilter.h"

ChatteringFilter::ChatteringFilter(unsigned long msec, bool initialOn)
    : _timeoutTimer(msec), _isOn(initialOn), _lastInputState(initialOn), _firstUpdate(true)
{
}

void ChatteringFilter::update(bool isOn)
{
  if (_firstUpdate)
  {
    _timeoutTimer.reset();
    _lastInputState = isOn;
    _isOn = isOn;
    _firstUpdate = false;
    return;
  }
  if (isOn != _lastInputState)
  {
    _timeoutTimer.reset();
    _lastInputState = isOn;
    return;
  }
  if (_timeoutTimer.isTimeout())
  {
    _isOn = _lastInputState;
  }
}

bool ChatteringFilter::isOn() const
{
  return _isOn;
}
