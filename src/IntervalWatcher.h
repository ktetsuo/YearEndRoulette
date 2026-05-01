#ifndef __INTERVALWATCHER_H__
#define __INTERVALWATCHER_H__

#include <Arduino.h>
#include "MaxMinHolder.h"

class IntervalWatcher
{
  const char *const _name;
  Print &_printer;
  bool _isFirst;
  unsigned long _lastus;
  MaxMinHolder<unsigned long> _maxmin;

public:
  explicit IntervalWatcher(const char *name, Print &printer);
  void reset();
  bool update();
  unsigned long minInterval() const;
  unsigned long maxInterval() const;
};

#endif // __INTERVALWATCHER_H__
