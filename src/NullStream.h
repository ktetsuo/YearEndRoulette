#ifndef __NULL_STREAM_H__
#define __NULL_STREAM_H__

#include <Arduino.h>

// 何も出力しないStreamクラス（Null Objectパターン）
class NullStream : public Stream
{
public:
  size_t write(uint8_t) override
  {
    return 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override
  {
    return size;
  }

  int available() override
  {
    return 0;
  }

  int read() override
  {
    return -1;
  }

  int peek() override
  {
    return -1;
  }

  void flush() override
  {
    // No-op
  }
};

#endif // __NULL_STREAM_H__
