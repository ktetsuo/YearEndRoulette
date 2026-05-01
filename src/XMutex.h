#ifndef __XMUTEX_H__
#define __XMUTEX_H__

#include "XFreeRTOS.h"

class XMutex
{
public:
  XMutex() : _mutex(nullptr)
  {
  }

  XMutex(const XMutex &) = delete;
  XMutex &operator=(const XMutex &) = delete;
  XMutex(XMutex &&) = delete;
  XMutex &operator=(XMutex &&) = delete;

  ~XMutex()
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

    _mutex = xSemaphoreCreateMutex();
    return _mutex != nullptr;
  }

  bool lock(TickType_t wait = portMAX_DELAY) const
  {
    if (!_mutex)
    {
      return false;
    }
    return xSemaphoreTake(_mutex, wait) == pdTRUE;
  }

  bool unlock() const
  {
    if (!_mutex)
    {
      return false;
    }
    return xSemaphoreGive(_mutex) == pdTRUE;
  }

  SemaphoreHandle_t native() const
  {
    return _mutex;
  }

private:
  SemaphoreHandle_t _mutex;
};

#endif // __XMUTEX_H__
