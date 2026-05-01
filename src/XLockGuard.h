#ifndef __XLOCK_GUARD_H__
#define __XLOCK_GUARD_H__

#include "XMutex.h"

class XLockGuard
{
public:
  explicit XLockGuard(const XMutex &mutex, TickType_t wait = portMAX_DELAY)
      : _mutex(mutex), _locked(false)
  {
    _locked = _mutex.lock(wait);
  }

  XLockGuard(const XLockGuard &) = delete;
  XLockGuard &operator=(const XLockGuard &) = delete;

  XLockGuard(XLockGuard &&other)
      : _mutex(other._mutex), _locked(other._locked)
  {
    other._locked = false;
  }

  ~XLockGuard()
  {
    if (_locked)
    {
      _mutex.unlock();
    }
  }

  bool locked() const
  {
    return _locked;
  }

private:
  const XMutex &_mutex;
  bool _locked;
};

#endif // __XLOCK_GUARD_H__
