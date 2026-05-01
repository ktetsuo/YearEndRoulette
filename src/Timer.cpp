#include "Timer.h"

////////////////////////////////////////////////////////////////////////////////
MilliSecTimer::MilliSecTimer()
    : _t0(0)
{
}
void MilliSecTimer::reset()
{
  _t0 = millis();
}
unsigned long MilliSecTimer::elapsedTime() const
{
  return millis() - _t0;
}

/// @brief タイマーはリセットし、前回からの経過時間を返す
/// @return 経過時間
unsigned long MilliSecTimer::lap()
{
  const unsigned long t = millis();
  const unsigned long elapsed = t - _t0;
  _t0 = t;
  return elapsed;
}

////////////////////////////////////////////////////////////////////////////////
MilliSecTimeoutTimer::MilliSecTimeoutTimer(unsigned long timeout)
    : _timeout(timeout)
{
}
void MilliSecTimeoutTimer::reset()
{
  _timer.reset();
}
bool MilliSecTimeoutTimer::isTimeout() const
{
  return _timeout <= _timer.elapsedTime();
}

////////////////////////////////////////////////////////////////////////////////
MilliSecIntervalTimer::MilliSecIntervalTimer(unsigned long timeout, void (*handler)(void *), void *param)
    : _timeoutTimer(timeout), _handler(handler), _param(param)
{
}
void MilliSecIntervalTimer::reset()
{
}
void MilliSecIntervalTimer::update()
{
  if (_timeoutTimer.isTimeout())
  {
    _timeoutTimer.reset();
    if (_handler)
    {
      _handler(_param);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
MilliSecLapTimer::MilliSecLapTimer()
    : _lapTime(0)
{
}

void MilliSecLapTimer::reset()
{
  _timer.reset();
}

unsigned long MilliSecLapTimer::lap()
{
  _lapTime = _timer.lap();
  return _lapTime;
}

unsigned long MilliSecLapTimer::lapTime() const
{
  return _lapTime;
}

////////////////////////////////////////////////////////////////////////////////
MicroSecTimer::MicroSecTimer()
    : _t0(0)
{
}
void MicroSecTimer::reset()
{
  _t0 = micros();
}
unsigned long MicroSecTimer::elapsedTime() const
{
  return micros() - _t0;
}

/// @brief タイマーはリセットし、前回からの経過時間を返す
/// @return 経過時間
unsigned long MicroSecTimer::lap()
{
  const unsigned long t = micros();
  const unsigned long elapsed = t - _t0;
  _t0 = t;
  return elapsed;
}

////////////////////////////////////////////////////////////////////////////////
MicroSecTimeoutTimer::MicroSecTimeoutTimer(unsigned long timeout)
    : _timeout(timeout)
{
}
void MicroSecTimeoutTimer::reset()
{
  _timer.reset();
}
bool MicroSecTimeoutTimer::isTimeout() const
{
  return _timeout <= _timer.elapsedTime();
}

////////////////////////////////////////////////////////////////////////////////
MicroSecIntervalTimer::MicroSecIntervalTimer(unsigned long timeout, void (*handler)(void *), void *param)
    : _timeoutTimer(timeout), _handler(handler), _param(param)
{
}
void MicroSecIntervalTimer::reset()
{
}
void MicroSecIntervalTimer::update()
{
  if (_timeoutTimer.isTimeout())
  {
    _timeoutTimer.reset();
    if (_handler)
    {
      _handler(_param);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
MicroSecLapTimer::MicroSecLapTimer()
    : _lapTime(0)
{
}

void MicroSecLapTimer::reset()
{
  _timer.reset();
}

unsigned long MicroSecLapTimer::lap()
{
  _lapTime = _timer.lap();
  return _lapTime;
}

unsigned long MicroSecLapTimer::lapTime() const
{
  return _lapTime;
}
