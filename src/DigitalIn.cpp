#include "DigitalIn.h"

DigitalIn::DigitalIn(uint8_t pin, int pinMode)
    : _pin(pin), _pinMode(pinMode)
{
}

void DigitalIn::setup()
{
  pinMode(_pin, _pinMode);
}

bool DigitalIn::isOn()
{
  return digitalRead(_pin);
}
