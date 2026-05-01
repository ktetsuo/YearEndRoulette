#ifndef __CHATTERINGFILTEREDDIGITALIN_H__
#define __CHATTERINGFILTEREDDIGITALIN_H__

#include "DigitalIn.h"
#include "ChatteringFilter.h"

class ChatteringFilteredDigitalIn : public IDigitalIn
{
  IDigitalIn &_digitalIn;
  ChatteringFilter _chatteringFilter;

public:
  ChatteringFilteredDigitalIn(IDigitalIn &digitalIn, unsigned long msec, bool initialOn);
  void update();
  virtual bool isOn() override;
};

#endif // __CHATTERINGFILTEREDDIGITALIN_H__
