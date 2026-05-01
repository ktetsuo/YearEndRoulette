#ifndef __ROTATIONBUFFER_H__
#define __ROTATIONBUFFER_H__

#include <cstddef>
#include <iterator>
#include <optional>
#include <algorithm>

/// @brief 古いデータが削除されるリングバッファ
/// @tparam T データ型
/// @tparam N データ数
/// @tparam INITIAL_VALUE 初期値（初期値を指定すると最初に埋められます）
template <typename T, size_t N, const T *INITIAL_VALUE = nullptr>
class RotationBuffer
{
private:
  T _buf[N + 1];
  size_t _pos = 0;
  size_t _totalCount = 0;

  template <size_t BufSize>
  class Iterator
  {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;

    Iterator(pointer buf, size_t pos, size_t index) : _buf(buf), _pos(pos), _index(index) {}

    reference operator*() const { return _buf[(_pos + _index) % BufSize]; }
    pointer operator->() { return &_buf[(_pos + _index) % BufSize]; }

    Iterator &operator++()
    {
      _index = (_index + 1) % BufSize;
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    Iterator &operator--()
    {
      _index = (_index - 1 + BufSize) % BufSize;
      return *this;
    }

    Iterator operator--(int)
    {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b)
    {
      return a._buf == b._buf && a._index == b._index && a._pos == b._pos;
    }
    friend bool operator!=(const Iterator &a, const Iterator &b)
    {
      return !(a == b);
    }

  private:
    pointer _buf;
    size_t _pos, _index;
  };

public:
  using iterator = Iterator<N + 1>;
  using const_iterator = Iterator<N + 1>;
  using reverse_iterator = std::reverse_iterator<Iterator<N + 1>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<N + 1>>;

  RotationBuffer()
  {
    if (INITIAL_VALUE)
    {
      std::fill_n(_buf, N + 1, *INITIAL_VALUE);
    }
  }

  iterator begin() { return iterator(_buf, _pos, 1); }
  iterator end() { return iterator(_buf, _pos, 0); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_iterator begin() const { return const_iterator(_buf, _pos, 1); }
  const_iterator end() const { return const_iterator(_buf, _pos, 0); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator crend() const { return rend(); }

  T &operator[](size_t index)
  {
    return _buf[(_pos + index) % (N + 1)];
  }

  const T &operator[](size_t index) const
  {
    return _buf[(_pos + index) % (N + 1)];
  }

  void clear()
  {
    static constexpr T DEFAULT_INITIAL_VALUE = {};
    const T initialValue = INITIAL_VALUE ? *INITIAL_VALUE : DEFAULT_INITIAL_VALUE;
    std::fill_n(_buf, N + 1, initialValue);
    _totalCount = 0;
  }

  size_t capacity() const
  {
    return N;
  }

  void add(T val)
  {
    _buf[_pos] = val;
    _pos = (_pos + 1) % (N + 1);
    _totalCount++;
  }

  size_t totalCount() const
  {
    return _totalCount;
  }

  bool isFilled() const
  {
    return N <= _totalCount;
  }
};

#endif // __ROTATIONBUFFER_H__
