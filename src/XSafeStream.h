#ifndef __XSAFE_STREAM_H__
#define __XSAFE_STREAM_H__

#include "XFreeRTOS.h"

template <size_t ChunkSize = 128>
class XSafeStream : public Stream
{
  static_assert(ChunkSize > 0, "ChunkSize must be greater than 0");

public:
  XSafeStream()
      : _stream(nullptr), _queue(nullptr), _streamMutex(nullptr), _writerTask(nullptr), _running(false)
  {
  }

  ~XSafeStream()
  {
    end();
  }

  bool begin(Stream *stream, size_t queueSize = 32, const char *taskName = "XSafeStream")
  {
    if (!stream || queueSize == 0)
    {
      return false;
    }

    if (_queue || _streamMutex || _writerTask)
    {
      return false;
    }

    _stream = stream;
    _running = true;

    _queue = xQueueCreate(queueSize, sizeof(TxChunk));
    if (!_queue)
    {
      _stream = nullptr;
      _running = false;
      return false;
    }

    _streamMutex = xSemaphoreCreateMutex();
    if (!_streamMutex)
    {
      vQueueDelete(_queue);
      _stream = nullptr;
      _queue = nullptr;
      _running = false;
      return false;
    }

    if (xTaskCreate(taskWriter, taskName ? taskName : "XSafeStream", 3072, this, tskIDLE_PRIORITY + 1, &_writerTask) != pdPASS)
    {
      vSemaphoreDelete(_streamMutex);
      _streamMutex = nullptr;
      vQueueDelete(_queue);
      _stream = nullptr;
      _queue = nullptr;
      _running = false;
      return false;
    }

    return true;
  }

  void end()
  {
    _running = false;

    if (_writerTask)
    {
      for (int i = 0; i < 200 && _writerTask; ++i)
      {
        vTaskDelay(1);
      }

      if (_writerTask)
      {
        vTaskDelete(_writerTask);
        _writerTask = nullptr;
      }
    }

    if (_streamMutex)
    {
      vSemaphoreDelete(_streamMutex);
      _streamMutex = nullptr;
    }

    if (_queue)
    {
      vQueueDelete(_queue);
      _queue = nullptr;
    }

    _stream = nullptr;
  }

  virtual size_t write(uint8_t c) override
  {
    return write(&c, 1);
  }

  virtual size_t write(const uint8_t *buffer, size_t size) override
  {
    if (!buffer || size == 0)
    {
      return 0;
    }

    size_t totalEnqueued = 0;
    size_t offset = 0;
    while (offset < size)
    {
      const size_t chunkSize = min(static_cast<size_t>(ChunkSize), size - offset);
      size_t enqueued = enqueue(buffer + offset, chunkSize);
      if (enqueued == 0)
      {
        break;
      }
      offset += enqueued;
      totalEnqueued += enqueued;
    }
    return totalEnqueued;
  }

  virtual int available() override
  {
    if (!_stream || !_streamMutex)
    {
      return 0;
    }
    xSemaphoreTake(_streamMutex, portMAX_DELAY);
    const int v = _stream->available();
    xSemaphoreGive(_streamMutex);
    return v;
  }

  virtual int read() override
  {
    if (!_stream || !_streamMutex)
    {
      return -1;
    }
    xSemaphoreTake(_streamMutex, portMAX_DELAY);
    const int v = _stream->read();
    xSemaphoreGive(_streamMutex);
    return v;
  }

  virtual int peek() override
  {
    if (!_stream || !_streamMutex)
    {
      return -1;
    }
    xSemaphoreTake(_streamMutex, portMAX_DELAY);
    const int v = _stream->peek();
    xSemaphoreGive(_streamMutex);
    return v;
  }

  virtual void flush() override
  {
    if (!_stream || !_streamMutex)
    {
      return;
    }

    while (uxQueueMessagesWaiting(_queue) > 0)
    {
      vTaskDelay(1);
    }

    xSemaphoreTake(_streamMutex, portMAX_DELAY);
    _stream->flush();
    xSemaphoreGive(_streamMutex);
  }

private:
  struct TxChunk
  {
    uint8_t data[ChunkSize];
    size_t len;
  };

  Stream *_stream;
  QueueHandle_t _queue;
  SemaphoreHandle_t _streamMutex;
  TaskHandle_t _writerTask = nullptr;
  volatile bool _running;

  size_t enqueue(const uint8_t *data, size_t len)
  {
    if (!_queue || !data || len == 0)
    {
      return 0;
    }

    TxChunk chunk = {};
    chunk.len = len;
    memcpy(chunk.data, data, len);

    if (xQueueSend(_queue, &chunk, 0) != pdPASS)
    {
      return 0;
    }

    return len;
  }

  static void taskWriter(void *pv)
  {
    XSafeStream *self = (XSafeStream *)pv;
    TxChunk chunk = {};
    while (self->_running || uxQueueMessagesWaiting(self->_queue) > 0)
    {
      if (xQueueReceive(self->_queue, &chunk, pdMS_TO_TICKS(10)) == pdTRUE)
      {
        if (chunk.len > 0)
        {
          if (self->_streamMutex)
          {
            xSemaphoreTake(self->_streamMutex, portMAX_DELAY);
          }
          self->_stream->write(chunk.data, chunk.len);
          if (self->_streamMutex)
          {
            xSemaphoreGive(self->_streamMutex);
          }
        }
      }
    }

    self->_writerTask = nullptr;
    vTaskDelete(nullptr);
  }
};

#endif // __XSAFE_STREAM_H__
