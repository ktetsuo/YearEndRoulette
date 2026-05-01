#include "DigitalOut.h"
#include <Arduino.h>

DigitalOut::DigitalOut(uint8_t pin)
:_pin(pin)
{
}
DigitalOut::~DigitalOut()
{
}
void DigitalOut::setup()
{
  pinMode(_pin, OUTPUT);
}
void DigitalOut::turnOn()
{
  digitalWrite(_pin, HIGH);
}
void DigitalOut::turnOff()
{
  digitalWrite(_pin, LOW);
}

////////////////////////////////////////////////////////////////////////////////
ToggleOut::ToggleOut(IDigitalOut &out, bool initialOn)
    : _out(out), _isOn(initialOn), _initialOn(initialOn)
{
}
void ToggleOut::reset()
{
  _isOn = _initialOn;
  update();
}
void ToggleOut::set(bool isOn)
{
  _isOn = isOn;
  update();
}
bool ToggleOut::toggle()
{
  _isOn = !_isOn;
  update();
  return _isOn;
}
void ToggleOut::update()
{
  if (_isOn)
  {
    _out.turnOn();
  }
  else
  {
    _out.turnOff();
  }
}
