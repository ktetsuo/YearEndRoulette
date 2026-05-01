#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <cstddef>

template <typename T, size_t N>
class RingBuffer
{
private:
  T _buf[N];
  size_t _head;
  size_t _tail;

public:
  RingBuffer();
  void clear();
  bool push(T val);
  T pop();
  T peek() const;
  size_t write(const T *p, size_t length);
  size_t read(T *p, size_t length);
  bool isEmpty() const;
  bool isFull() const;
  constexpr size_t capacity() const;
  size_t length() const;

  class iterator;
  iterator begin();
  iterator end();

  class const_iterator;
  const_iterator cbegin() const;
  const_iterator cend() const;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, size_t N>
class RingBuffer<T, N>::iterator
{
private:
  RingBuffer<T, N> *_rb;
  size_t _index;

public:
  iterator(RingBuffer<T, N> *rb, size_t index);
  iterator &operator++();
  iterator operator++(int);
  T &operator*();
  T *operator->();
  T &operator[](size_t offset);
  bool operator==(const iterator &other) const;
  bool operator!=(const iterator &other) const;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, size_t N>
class RingBuffer<T, N>::const_iterator
{
private:
  const RingBuffer<T, N> *_rb;
  size_t _index;

public:
  const_iterator(const RingBuffer<T, N> *rb, size_t index);
  const_iterator &operator++();
  const_iterator operator++(int);
  const T &operator*() const;
  const T *operator->() const;
  const T &operator[](size_t offset) const;
  bool operator==(const const_iterator &other) const;
  bool operator!=(const const_iterator &other) const;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, size_t N>
RingBuffer<T, N>::RingBuffer() : _head(0), _tail(0) {}

template <typename T, size_t N>
void RingBuffer<T, N>::clear()
{
  _head = 0;
  _tail = 0;
}

template <typename T, size_t N>
bool RingBuffer<T, N>::push(T val)
{
  if (isFull())
  {
    return false;
  }
  _buf[_tail] = val;
  _tail = (_tail + 1) % N;
  return true;
}

template <typename T, size_t N>
T RingBuffer<T, N>::pop()
{
  if (isEmpty())
  {
    // throw std::runtime_error("RingBuffer is empty");
    return 0;
  }
  T val = _buf[_head];
  _head = (_head + 1) % N;
  return val;
}

template <typename T, size_t N>
T RingBuffer<T, N>::peek() const
{
  if (isEmpty())
  {
    // throw std::runtime_error("RingBuffer is empty");
    return 0;
  }
  return _buf[_head];
}

template <typename T, size_t N>
size_t RingBuffer<T, N>::write(const T *p, size_t length)
{
  size_t written = 0;
  while (length > 0 && !isFull())
  {
    _buf[_tail] = *p++;
    _tail = (_tail + 1) % N;
    length--;
    written++;
  }
  return written;
}

template <typename T, size_t N>
size_t RingBuffer<T, N>::read(T *p, size_t length)
{
  size_t read = 0;
  while (length > 0 && !isEmpty())
  {
    *p++ = _buf[_head];
    _head = (_head + 1) % N;
    length--;
    read++;
  }
  return read;
}

template <typename T, size_t N>
bool RingBuffer<T, N>::isEmpty() const
{
  return _head == _tail;
}

template <typename T, size_t N>
bool RingBuffer<T, N>::isFull() const
{
  return ((_tail + 1) % N) == _head;
}

template <typename T, size_t N>
constexpr size_t RingBuffer<T, N>::capacity() const
{
  return N;
}

template <typename T, size_t N>
size_t RingBuffer<T, N>::length() const
{
  if (_tail >= _head)
  {
    return _tail - _head;
  }
  else
  {
    return N - (_head - _tail);
  }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, size_t N>
RingBuffer<T, N>::iterator::iterator(RingBuffer<T, N> *rb, size_t index) : _rb(rb), _index(index) {}

template <typename T, size_t N>
typename RingBuffer<T, N>::iterator &RingBuffer<T, N>::iterator::operator++()
{
  _index = (_index + 1) % _rb->capacity();
  return *this;
}

template <typename T, size_t N>
typename RingBuffer<T, N>::iterator RingBuffer<T, N>::iterator::operator++(int)
{
  iterator temp = *this;
  ++(*this);
  return temp;
}

template <typename T, size_t N>
T &RingBuffer<T, N>::iterator::operator*()
{
  return _rb->_buf[_index];
}

template <typename T, size_t N>
T *RingBuffer<T, N>::iterator::operator->()
{
  return &_rb->_buf[_index];
}

template <typename T, size_t N>
T &RingBuffer<T, N>::iterator::operator[](size_t offset)
{
  return _rb->_buf[(_index + offset) % _rb->capacity()];
}

template <typename T, size_t N>
bool RingBuffer<T, N>::iterator::operator==(const iterator &other) const
{
  return _rb == other._rb && _index == other._index;
}

template <typename T, size_t N>
bool RingBuffer<T, N>::iterator::operator!=(const iterator &other) const
{
  return !(*this == other);
}

template <typename T, size_t N>
typename RingBuffer<T, N>::iterator RingBuffer<T, N>::begin()
{
  return iterator(this, _head);
}

template <typename T, size_t N>
typename RingBuffer<T, N>::iterator RingBuffer<T, N>::end()
{
  return iterator(this, _tail);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, size_t N>
RingBuffer<T, N>::const_iterator::const_iterator(const RingBuffer<T, N> *rb, size_t index) : _rb(rb), _index(index) {}

template <typename T, size_t N>
typename RingBuffer<T, N>::const_iterator &RingBuffer<T, N>::const_iterator::operator++()
{
  _index = (_index + 1) % _rb->capacity();
  return *this;
}

template <typename T, size_t N>
typename RingBuffer<T, N>::const_iterator RingBuffer<T, N>::const_iterator::operator++(int)
{
  const_iterator temp = *this;
  ++(*this);
  return temp;
}

template <typename T, size_t N>
const T &RingBuffer<T, N>::const_iterator::operator*() const
{
  return _rb->_buf[_index];
}

template <typename T, size_t N>
const T *RingBuffer<T, N>::const_iterator::operator->() const
{
  return &_rb->_buf[_index];
}

template <typename T, size_t N>
const T &RingBuffer<T, N>::const_iterator::operator[](size_t offset) const
{
  return _rb->_buf[(_index + offset) % _rb->capacity()];
}

template <typename T, size_t N>
bool RingBuffer<T, N>::const_iterator::operator==(const const_iterator &other) const
{
  return _rb == other._rb && _index == other._index;
}

template <typename T, size_t N>
bool RingBuffer<T, N>::const_iterator::operator!=(const const_iterator &other) const
{
  return !(*this == other);
}

template <typename T, size_t N>
typename RingBuffer<T, N>::const_iterator RingBuffer<T, N>::cbegin() const
{
  return const_iterator(this, _head);
}

template <typename T, size_t N>
typename RingBuffer<T, N>::const_iterator RingBuffer<T, N>::cend() const
{
  return const_iterator(this, _tail);
}

#endif // __RINGBUFFER_H__
