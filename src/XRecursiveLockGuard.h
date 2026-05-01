#ifndef __XRECURSIVE_LOCK_GUARD_H__
#define __XRECURSIVE_LOCK_GUARD_H__

#include "XRecursiveMutex.h"

class XRecursiveLockGuard
{
public:
  explicit XRecursiveLockGuard(const XRecursiveMutex &mutex, TickType_t wait = portMAX_DELAY)
      : _mutex(mutex), _locked(false)
  {
    _locked = _mutex.lock(wait);
  }

  XRecursiveLockGuard(const XRecursiveLockGuard &) = delete;
  XRecursiveLockGuard &operator=(const XRecursiveLockGuard &) = delete;

  ~XRecursiveLockGuard()
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
  const XRecursiveMutex &_mutex;
  bool _locked;
};

#endif // __XRECURSIVE_LOCK_GUARD_H__
