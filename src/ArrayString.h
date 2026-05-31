#ifndef __ARRAYSTRING_H__
#define __ARRAYSTRING_H__

#include <array>
#include <cstring>

template <std::size_t N>
class ArrayString
{
  std::array<char, N + 1> _array;
  std::size_t _len;

public:
  constexpr std::size_t capacity() const
  {
    return N;
  }
  constexpr std::size_t length() const
  {
    return _len;
  }
  constexpr const char *c_str() const
  {
    return _array.data();
  }
  constexpr bool isEmpty() const
  {
    return _len == 0;
  }
  constexpr bool isFull() const
  {
    return _len >= N;
  }
  bool operator==(const char *s) const
  {
    return std::strcmp(_array.data(), s) == 0;
  }
  bool operator!=(const char *s) const
  {
    return !(*this == s);
  }
  void clear()
  {
    _array[0] = '\0';
    _len = 0;
  }
  bool append(char c)
  {
    if (isFull())
    {
      return false;
    }
    _array[_len] = c;
    _len++;
    _array[_len] = '\0';
    return true;
  }
  bool backspace()
  {
    if (isEmpty())
    {
      return false;
    }
    _len--;
    _array[_len] = '\0';
    return true;
  }
  std::size_t append(const char *s, std::size_t len)
  {
    std::size_t i = 0;
    while (i < len)
    {
      if (_len + i >= N)
      {
        break;
      }
      if (s[i] == '\0')
      {
        break;
      }
      _array[_len + i] = s[i];
      i++;
    }
    _len += i;
    _array[_len] = '\0';
    return i;
  }
  bool startsWith(const char *prefix) const
  {
    std::size_t prefixLen = std::strlen(prefix);
    if (prefixLen > _len)
    {
      return false;
    }
    return std::strncmp(_array.data(), prefix, prefixLen) == 0;
  }
  bool endsWith(const char *suffix) const
  {
    std::size_t suffixLen = std::strlen(suffix);
    if (suffixLen > _len)
    {
      return false;
    }
    return std::strncmp(_array.data() + _len - suffixLen, suffix, suffixLen) == 0;
  }
};

#endif // __ARRAYSTRING_H__
