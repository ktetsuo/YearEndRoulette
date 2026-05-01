#ifndef __DUTY_H__
#define __DUTY_H__

#include <type_traits>
#include <algorithm>

template <typename T>
class SignedDuty
{
  const T _maxValue;
  T _currentValue;

public:
  SignedDuty(T maxValue, T initialValue);
  template <typename PercentType>
  void setPercent(PercentType percent);
  template <typename PercentType>
  PercentType percent() const;
  void setValue(T value);
  T currentValue() const;
  T maxValue() const;
};

template <typename T>
SignedDuty<T>::SignedDuty(T maxValue, T initialValue)
    : _maxValue(maxValue), _currentValue(std::max(std::min(initialValue, maxValue), -maxValue))
{
  static_assert(std::is_signed<T>::value, "T must be a signed integral type");
}

template <typename T>
template <typename PercentType>
void SignedDuty<T>::setPercent(PercentType percent)
{
  static_assert(std::is_floating_point<PercentType>::value, "PercentType must be a floating-point type");
  _currentValue = std::max(std::min(static_cast<T>(_maxValue * percent / 100), _maxValue), -_maxValue);
}

template <typename T>
template <typename PercentType>
PercentType SignedDuty<T>::percent() const
{
  static_assert(std::is_floating_point<PercentType>::value, "PercentType must be a floating-point type");
  return static_cast<PercentType>(_currentValue) / _maxValue * 100;
}

template <typename T>
void SignedDuty<T>::setValue(T value)
{
  _currentValue = std::max(std::min(value, _maxValue), -_maxValue);
}

template <typename T>
T SignedDuty<T>::currentValue() const
{
  return _currentValue;
}

template <typename T>
T SignedDuty<T>::maxValue() const
{
  return _maxValue;
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
class UnsignedDuty
{
private:
  const T _maxValue;
  T _currentValue;

public:
  UnsignedDuty(T maxValue, T initialValue);

  template <typename PercentType>
  void setPercent(PercentType percent);

  template <typename PercentType>
  PercentType percent() const;

  void setValue(T value);
  T currentValue() const;
  T maxValue() const;
};

template <typename T>
UnsignedDuty<T>::UnsignedDuty(T maxValue, T initialValue)
    : _maxValue(maxValue), _currentValue(std::min(initialValue, maxValue))
{
  static_assert(std::is_unsigned<T>::value, "T must be an unsigned integral type");
}

template <typename T>
template <typename PercentType>
void UnsignedDuty<T>::setPercent(PercentType percent)
{
  static_assert(std::is_floating_point<PercentType>::value, "PercentType must be a floating-point type");
  _currentValue = static_cast<T>(std::min(_maxValue * percent / 100, static_cast<PercentType>(_maxValue)));
}

template <typename T>
template <typename PercentType>
PercentType UnsignedDuty<T>::percent() const
{
  static_assert(std::is_floating_point<PercentType>::value, "PercentType must be a floating-point type");
  return static_cast<PercentType>(_currentValue) / _maxValue * 100;
}

template <typename T>
void UnsignedDuty<T>::setValue(T value)
{
  _currentValue = std::min(value, _maxValue);
}

template <typename T>
T UnsignedDuty<T>::currentValue() const
{
  return _currentValue;
}

template <typename T>
T UnsignedDuty<T>::maxValue() const
{
  return _maxValue;
}

#endif // __DUTY_H__
