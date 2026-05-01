#include "IntervalWatcher.h"

IntervalWatcher::IntervalWatcher(const char *name, Print& printer)
    : _name(name), _printer(printer) ,_isFirst(true), _lastus(0)
{
}
void IntervalWatcher::reset()
{
  _isFirst = true;
  _maxmin.reset();
}
bool IntervalWatcher::update()
{
  bool updated = false;
  const unsigned long t = micros();
  if (_isFirst)
  {
    _isFirst = false;
    _maxmin.reset();
  }
  else
  {
    const unsigned long interval = t - _lastus;
    const MaxMinHolder<unsigned long>::UpdateResult result = _maxmin.update(interval);
    if (result.isUpdatedMax())
    {
      _printer.print("MaxInterval(");
      _printer.print(_name);
      _printer.print(") : ");
      _printer.println(_maxmin.max());
      updated = true;
    }
    if (result.isUpdatedMin())
    {
      _printer.print("MinInterval(");
      _printer.print(_name);
      _printer.print(") : ");
      _printer.println(_maxmin.min());
      updated = true;
    }
  }
  _lastus = t;
  return updated;
}

unsigned long IntervalWatcher::minInterval() const
{
  return _maxmin.min();
}
unsigned long IntervalWatcher::maxInterval() const
{
  return _maxmin.max();
}
