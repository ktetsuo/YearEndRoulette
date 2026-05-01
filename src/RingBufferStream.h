#ifndef __RINGBUFFERSTREAM_H__
#define __RINGBUFFERSTREAM_H__

#include <Arduino.h>
#include "RingBuffer.h"

template <size_t N>
class RingBufferStream : public Stream
{
  RingBuffer<uint8_t, N> _ringBuffer;

public:
  virtual int available() override;
  virtual int read() override;
  virtual int peek() override;
  virtual size_t write(uint8_t) override;
  virtual size_t write(const uint8_t *buffer, size_t size) override;
  virtual int availableForWrite() override;
};

template <size_t N>
int RingBufferStream<N>::available()
{
  return static_cast<int>(_ringBuffer.length());
}

template <size_t N>
int RingBufferStream<N>::read()
{
  if (_ringBuffer.isEmpty())
  {
    return -1;
  }
  return _ringBuffer.pop();
}

template <size_t N>
int RingBufferStream<N>::peek()
{
  if (_ringBuffer.isEmpty())
  {
    return -1;
  }
  return _ringBuffer.peek();
}

template <size_t N>
size_t RingBufferStream<N>::write(uint8_t byte)
{
  if (_ringBuffer.push(byte))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

template <size_t N>
size_t RingBufferStream<N>::write(const uint8_t *buffer, size_t size)
{
  return _ringBuffer.write(buffer, size);
}

template <size_t N>
int RingBufferStream<N>::availableForWrite()
{
  return static_cast<int>(_ringBuffer.capacity() - _ringBuffer.length());
}

#endif // __RINGBUFFERSTREAM_H__
