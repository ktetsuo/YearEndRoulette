#ifndef __DIGITALOUT_H__
#define __DIGITALOUT_H__

#include <Arduino.h>

class IDigitalOut
{
public:
  virtual void turnOn() = 0;
  virtual void turnOff() = 0;
};

////////////////////////////////////////////////////////////////////////////////
class DigitalOut : public IDigitalOut
{
  const uint8_t _pin;
public:
  DigitalOut(uint8_t pin);
  virtual ~DigitalOut();
  void setup();
  virtual void turnOn() override;
  virtual void turnOff() override;
};

////////////////////////////////////////////////////////////////////////////////
class ToggleOut
{
  IDigitalOut &_out;
  bool _isOn;
  const bool _initialOn;

public:
  ToggleOut(IDigitalOut &out, bool initialOn);
  void reset();
  void set(bool isOn);
  bool toggle();

private:
  void update();
};

#endif // __DIGITALOUT_H__
