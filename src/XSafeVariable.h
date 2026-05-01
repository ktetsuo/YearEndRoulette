#ifndef __XSAFE_VARIABLE_H__
#define __XSAFE_VARIABLE_H__

#include "TinyOptional.h"
#include "XLockGuard.h"
#include "XMutex.h"

template <typename T>
class XSafeVariable
{
public:
  // ロックを保持しながら値への直接アクセスを提供するRAIIガード
  class LockedRef
  {
  public:
    LockedRef(XMutex &mutex, T &value)
        : _guard(mutex), _value(value)
    {
    }

    LockedRef(const LockedRef &) = delete;
    LockedRef &operator=(const LockedRef &) = delete;

    LockedRef(LockedRef &&other)
        : _guard(std::move(other._guard)), _value(other._value)
    {
    }

    bool locked() const { return _guard.locked(); }

    T *operator->() { return &_value; }
    const T *operator->() const { return &_value; }
    T &operator*() { return _value; }
    const T &operator*() const { return _value; }

  private:
    XLockGuard _guard;
    T &_value;
  };

  XSafeVariable()
  {
  }

  XSafeVariable(const XSafeVariable &) = delete;
  XSafeVariable &operator=(const XSafeVariable &) = delete;
  XSafeVariable(XSafeVariable &&) = delete;
  XSafeVariable &operator=(XSafeVariable &&) = delete;

  bool begin()
  {
    return _mutex.begin();
  }

  XSafeVariable &operator=(const T &value)
  {
    trySet(value);
    return *this;
  }

  operator T() const
  {
    TinyOptional<T> value = tryGet();
    return value.valueOr(T());
  }

  // 値を設定する関数。値を安全に更新できるが、更新に失敗する可能性がある（ロックの取得に失敗した場合など）。
  bool trySet(const T &value)
  {
    XLockGuard lock(_mutex);
    if (!lock.locked())
    {
      return false;
    }
    _value = value;
    return true;
  }

  // 値を更新する関数。funcは引数に値への参照を取る関数オブジェクトで、値を更新する処理を実装する。
  template <typename Func>
  bool update(Func func)
  {
    XLockGuard lock(_mutex);
    if (!lock.locked())
    {
      return false;
    }
    func(_value);
    return true;
  }

  // 値を取得する関数。値を安全に取得できるが、取得に失敗する可能性がある（ロックの取得に失敗した場合など）。取得に失敗した場合はhasValue() == falseのTinyOptionalを返す。
  TinyOptional<T> tryGet() const
  {
    XLockGuard lock(_mutex);
    if (!lock.locked())
    {
      return TinyOptional<T>();
    }
    return TinyOptional<T>(_value);
  }

  // ミューテックスをロックしたまま値への参照を返す。スコープを抜けると自動解放される。
  LockedRef lock()
  {
    return LockedRef(_mutex, _value);
  }

private:
  T _value{};
  XMutex _mutex;
};

#endif // __XSAFE_VARIABLE_H__
