#ifndef __XEVENT_GROUP_H__
#define __XEVENT_GROUP_H__

#include "XFreeRTOS.h"

class XEventGroup
{
public:
  XEventGroup() : _handle(nullptr)
  {
  }

  XEventGroup(const XEventGroup &) = delete;
  XEventGroup &operator=(const XEventGroup &) = delete;
  XEventGroup(XEventGroup &&) = delete;
  XEventGroup &operator=(XEventGroup &&) = delete;

  ~XEventGroup()
  {
    if (_handle)
    {
      vEventGroupDelete(_handle);
      _handle = nullptr;
    }
  }

  bool begin()
  {
    if (_handle)
    {
      return false;
    }

    _handle = xEventGroupCreate();
    return _handle != nullptr;
  }

  EventBits_t setBits(EventBits_t bits)
  {
    if (!_handle)
    {
      return 0;
    }
    return xEventGroupSetBits(_handle, bits);
  }

  EventBits_t clearBits(EventBits_t bits)
  {
    if (!_handle)
    {
      return 0;
    }
    return xEventGroupClearBits(_handle, bits);
  }

  EventBits_t getBits() const
  {
    if (!_handle)
    {
      return 0;
    }
    return xEventGroupGetBits(_handle);
  }

  EventBits_t waitBits(
      EventBits_t bitsToWaitFor,
      BaseType_t clearOnExit = pdFALSE,
      BaseType_t waitForAllBits = pdTRUE,
      TickType_t wait = portMAX_DELAY) const
  {
    if (!_handle)
    {
      return 0;
    }
    return xEventGroupWaitBits(
        _handle,
        bitsToWaitFor,
        clearOnExit,
        waitForAllBits,
        wait);
  }

  EventGroupHandle_t native() const
  {
    return _handle;
  }

private:
  EventGroupHandle_t _handle;
};

#endif // __XEVENT_GROUP_H__
