#ifndef __XRECURSIVE_MUTEX_H__
#define __XRECURSIVE_MUTEX_H__

#include "XFreeRTOS.h"

class XRecursiveMutex
{
public:
  XRecursiveMutex() : _mutex(nullptr)
  {
  }

  XRecursiveMutex(const XRecursiveMutex &) = delete;
  XRecursiveMutex &operator=(const XRecursiveMutex &) = delete;
  XRecursiveMutex(XRecursiveMutex &&) = delete;
  XRecursiveMutex &operator=(XRecursiveMutex &&) = delete;

  ~XRecursiveMutex()
  {
    if (_mutex)
    {
      vSemaphoreDelete(_mutex);
      _mutex = nullptr;
    }
  }

  bool begin()
  {
    if (_mutex)
    {
      return false;
    }

    _mutex = xSemaphoreCreateRecursiveMutex();
    return _mutex != nullptr;
  }

  bool lock(TickType_t wait = portMAX_DELAY) const
  {
    if (!_mutex)
    {
      return false;
    }
    return xSemaphoreTakeRecursive(_mutex, wait) == pdTRUE;
  }

  bool unlock() const
  {
    if (!_mutex)
    {
      return false;
    }
    return xSemaphoreGiveRecursive(_mutex) == pdTRUE;
  }

  SemaphoreHandle_t native() const
  {
    return _mutex;
  }

private:
  SemaphoreHandle_t _mutex;
};

#endif // __XRECURSIVE_MUTEX_H__
