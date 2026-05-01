#ifndef __DIGITALIN_H__
#define __DIGITALIN_H__

#include <Arduino.h>

class IDigitalIn
{
public:
  virtual bool isOn() = 0;
};

////////////////////////////////////////////////////////////////////////////////
class DigitalIn : public IDigitalIn
{
  const uint8_t _pin;
  const int _pinMode;

public:
  DigitalIn(uint8_t pin, int pinMode);
  void setup();
  virtual bool isOn() override;
};

#endif // __DIGITALIN_H__
