#ifndef __XQUEUE_H__
#define __XQUEUE_H__

#include "XFreeRTOS.h"

template <typename T, UBaseType_t Length>
class XQueue
{
public:
  XQueue() : _queue(nullptr)
  {
  }

  XQueue(const XQueue &) = delete;
  XQueue &operator=(const XQueue &) = delete;
  XQueue(XQueue &&) = delete;
  XQueue &operator=(XQueue &&) = delete;

  ~XQueue()
  {
    if (_queue)
    {
      vQueueDelete(_queue);
      _queue = nullptr;
    }
  }

  bool begin()
  {
    if (_queue)
    {
      return false;
    }

    _queue = xQueueCreate(Length, sizeof(T));
    return _queue != nullptr;
  }

  bool send(const T &value, TickType_t wait = 0)
  {
    if (!_queue)
    {
      return false;
    }
    return xQueueSend(_queue, &value, wait) == pdTRUE;
  }

  bool receive(T &value, TickType_t wait = 0)
  {
    if (!_queue)
    {
      return false;
    }
    return xQueueReceive(_queue, &value, wait) == pdTRUE;
  }

  bool peek(T &value, TickType_t wait = 0) const
  {
    if (!_queue)
    {
      return false;
    }
    return xQueuePeek(_queue, &value, wait) == pdTRUE;
  }

  bool reset()
  {
    if (!_queue)
    {
      return false;
    }
    return xQueueReset(_queue) == pdPASS;
  }

  UBaseType_t messagesWaiting() const
  {
    if (!_queue)
    {
      return 0;
    }
    return uxQueueMessagesWaiting(_queue);
  }

  UBaseType_t spacesAvailable() const
  {
    if (!_queue)
    {
      return 0;
    }
    return uxQueueSpacesAvailable(_queue);
  }

  QueueHandle_t native() const
  {
    return _queue;
  }

private:
  QueueHandle_t _queue;
};

#endif // __XQUEUE_H__
