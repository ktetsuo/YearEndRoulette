#ifndef __VALUECOMMAND_H__
#define __VALUECOMMAND_H__

#include "IConsoleCommand.h"
#include "ParseValue.h"
#include <Arduino.h>
#include <cstring>
#include <optional>

template <typename T>
class ValueCommand : public IConsoleCommand
{
  const char *const _valueName;
  const size_t _nameLen;
  T(*_getfunc)
  ();                  // getterの関数ポインタ
  void (*_setfunc)(T); // setterの関数ポインタ
public:
  constexpr ValueCommand(const char *valueName, T (*getfunc)(), void (*setfunc)(T))
      : _valueName(valueName), _getfunc(getfunc), _setfunc(setfunc), _nameLen(calcNameLen(valueName))
  {
  }
  virtual bool run(const ArrayString<CMD_MAX_LEN> &cmdline, Print &printer) const override
  {
    // 長さチェック("valueName="より短いなら不一致)
    // if (cmdline.length() < _nameLen + 1) と同義
    if (cmdline.length() <= _nameLen)
    {
      return false;
    }
    // "valueName="で始まるか
    const char *s = cmdline.c_str();
    if (s[_nameLen] != '=')
    {
      return false;
    }
    if (std::strstr(s, _valueName) != s)
    {
      return false;
    }
    // 現在の値を表示
    const T currentValue = getValue(printer);
    printer.printf("NOW %s : %s\r\n", _valueName, String(currentValue).c_str());
    // パラメータをパース
    const char *valuestr = &s[_nameLen + 1]; // '='の次の文字を指す
    if (*valuestr == '\0')
    {
      return true; // 取得して終わり
    }
    std::optional<T> value = parseValue<T>(valuestr);
    if (value)
    {
      // パース成功
      printer.printf("SET %s : %s\r\n", _valueName, String(*value).c_str());
      setValue(*value, printer);
    }
    else
    {
      // パース失敗
      printer.printf("%s : parse failed\r\n", _valueName);
    }
    // 現在の値を表示
    const T newValue = getValue(printer);
    printer.printf("NOW %s : %s\r\n", _valueName, String(newValue).c_str());

    return true;
  }
  virtual void info(Print &printer) const override
  {
    // 現在の値を表示
    const T newValue = getValue(printer);
    printer.printf("%s : %s\r\n", _valueName, String(newValue).c_str());
  }

private:
  static constexpr size_t calcNameLen(const char *name)
  {
    size_t len = 0;
    while (*name++ != '\0')
    {
      len++;
    }
    return len;
  }
  T getValue(Print &printer) const
  {
    if (_getfunc == nullptr)
    {
      printer.println("WARNING: getfunc is null");
      return 0;
    }
    return _getfunc();
  }
  void setValue(T value, Print &printer) const
  {
    if (_setfunc == nullptr)
    {
      printer.println("WARNING: setfunc is null");
      return;
    }
    _setfunc(value);
  }
};

#endif // __VALUECOMMAND_H__
