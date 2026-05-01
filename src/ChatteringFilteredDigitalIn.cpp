#include "ChatteringFilteredDigitalIn.h"

ChatteringFilteredDigitalIn::ChatteringFilteredDigitalIn(IDigitalIn &digitalIn, unsigned long msec, bool initialOn)
    : _digitalIn(digitalIn), _chatteringFilter(msec, initialOn)
{
}

void ChatteringFilteredDigitalIn::update()
{
  _chatteringFilter.update(_digitalIn.isOn());
}

bool ChatteringFilteredDigitalIn::isOn()
{
  return _chatteringFilter.isOn();
}
