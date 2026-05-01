#ifndef __FLAGWATCHER_H__
#define __FLAGWATCHER_H__

class FlagWatcher
{
  bool _lastOn;
  bool _isOn;

public:
  explicit FlagWatcher(bool initialOn);
  void update(bool isOn);
  bool isOn() const;
  bool isRisingEdge() const;
  bool isFallingEdge() const;
};

#endif // __FLAGWATCHER_H__
