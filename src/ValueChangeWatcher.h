#ifndef __VALUECHANGEWATCHER_H__
#define __VALUECHANGEWATCHER_H__

template <typename T>
class ValueChangeWatcher
{
  T _lastValue;
  T _currentValue;
  bool _isFirst;

public:
  ValueChangeWatcher();
  void update(T value);
  T value() const;
  T lastValue() const;
  bool isChanged() const;
};
template <typename T>
ValueChangeWatcher<T>::ValueChangeWatcher()
    : _lastValue(), _currentValue(), _isFirst(true)
{
}

template <typename T>
void ValueChangeWatcher<T>::update(T value)
{
  _lastValue = _currentValue;
  _currentValue = value;
  _isFirst = false;
}

template <typename T>
T ValueChangeWatcher<T>::value() const
{
  return _currentValue;
}

template <typename T>
T ValueChangeWatcher<T>::lastValue() const
{
  return _lastValue;
}

template <typename T>
bool ValueChangeWatcher<T>::isChanged() const
{
  return !_isFirst && _lastValue != _currentValue;
}

#endif // __VALUECHANGEWATCHER_H__
