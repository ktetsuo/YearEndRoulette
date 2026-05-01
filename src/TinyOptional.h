#ifndef __TINY_OPTIONAL_H__
#define __TINY_OPTIONAL_H__

#include <new>
#include <type_traits>
#include <utility>

template <typename T>
class TinyOptional
{
public:
  TinyOptional() : _hasValue(false)
  {
  }

  TinyOptional(const T &value) : _hasValue(false)
  {
    emplace(value);
  }

  TinyOptional(T &&value) : _hasValue(false)
  {
    emplace(static_cast<T &&>(value));
  }

  TinyOptional(const TinyOptional &other) : _hasValue(false)
  {
    if (other._hasValue)
    {
      emplace(other.value());
    }
  }

  TinyOptional(TinyOptional &&other) : _hasValue(false)
  {
    if (other._hasValue)
    {
      emplace(static_cast<T &&>(other.value()));
      other.reset();
    }
  }

  TinyOptional &operator=(const TinyOptional &other)
  {
    if (this == &other)
    {
      return *this;
    }

    if (!other._hasValue)
    {
      reset();
      return *this;
    }

    if (_hasValue)
    {
      value() = other.value();
    }
    else
    {
      emplace(other.value());
    }

    return *this;
  }

  TinyOptional &operator=(TinyOptional &&other)
  {
    if (this == &other)
    {
      return *this;
    }

    if (!other._hasValue)
    {
      reset();
      return *this;
    }

    if (_hasValue)
    {
      value() = static_cast<T &&>(other.value());
    }
    else
    {
      emplace(static_cast<T &&>(other.value()));
    }
    other.reset();

    return *this;
  }

  TinyOptional &operator=(const T &value)
  {
    if (_hasValue)
    {
      this->value() = value;
    }
    else
    {
      emplace(value);
    }
    return *this;
  }

  TinyOptional &operator=(T &&value)
  {
    if (_hasValue)
    {
      this->value() = static_cast<T &&>(value);
    }
    else
    {
      emplace(static_cast<T &&>(value));
    }
    return *this;
  }

  ~TinyOptional()
  {
    reset();
  }

  bool hasValue() const
  {
    return _hasValue;
  }

  explicit operator bool() const
  {
    return _hasValue;
  }

  T &value()
  {
    return *reinterpret_cast<T *>(&_storage);
  }

  const T &value() const
  {
    return *reinterpret_cast<const T *>(&_storage);
  }

  T valueOr(const T &fallback) const
  {
    if (_hasValue)
    {
      return value();
    }
    return fallback;
  }

  template <typename... Args>
  void emplace(Args &&...args)
  {
    reset();
    new (&_storage) T(std::forward<Args>(args)...);
    _hasValue = true;
  }

  void reset()
  {
    if (_hasValue)
    {
      value().~T();
      _hasValue = false;
    }
  }

private:
  typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type _storage;
  bool _hasValue;
};

#endif // __TINY_OPTIONAL_H__
