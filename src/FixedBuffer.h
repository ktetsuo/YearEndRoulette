#ifndef __FIXEDBUFFER_H__
#define __FIXEDBUFFER_H__

#include <array>

template <typename T, std::size_t N>
class FixedBuffer
{
  std::array<T, N> _buf;
  size_t _len = 0;

public:
  constexpr const T *data() const
  {
    return _buf.data();
  }
  constexpr size_t size() const
  {
    return N;
  }
  const std::array<T, N> &array() const
  {
    return _buf;
  }
  size_t length() const
  {
    return _len;
  }
  bool isEmpty() const
  {
    return _len == 0;
  }
  bool isFull() const
  {
    return _len >= _buf.size();
  }
  void clear()
  {
    _len = 0;
  }
  bool add(T val)
  {
    if (isFull())
    {
      return false;
    }
    _buf[_len] = val;
    _len++;
    return true;
  }
  size_t append(const T *data, size_t len)
  {
    size_t appended = 0;
    while (0 < len--)
    {
      if (!add(*(data + appended)))
      {
        break;
      }
      appended++;
    }
    return appended;
  }
};

#endif // __FIXEDBUFFER_H__
