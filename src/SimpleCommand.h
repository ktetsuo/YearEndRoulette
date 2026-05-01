#ifndef __SIMPLECOMMAND_H__
#define __SIMPLECOMMAND_H__

#include "IConsoleCommand.h"
#include "ParseValue.h"
#include <Arduino.h>
#include <cstring>
#include <optional>

class SimpleCommand : public IConsoleCommand
{
  const char *const _name;
  const size_t _nameLen;
  void (*_func)(); // 実行する関数の関数ポインタ
public:
  constexpr SimpleCommand(const char *name, void (*func)())
      : _name(name), _nameLen(calcNameLen(name)), _func(func)
  {
  }
  virtual bool run(const ArrayString<CMD_MAX_LEN> &cmdline, Print &printer) const override
  {
    // 長さチェック
    if (cmdline.length() != _nameLen)
    {
      return false;
    }
    // コマンド確認
    const char *s = cmdline.c_str();
    if (std::strncmp(s, _name, _nameLen) != 0)
    {
      return false;
    }
    // 登録された関数を実行
    if (_func)
    {
      _func();
    }
    return true;
  }
  virtual void info(Print &printer) const override
  {
    // コマンド名を出力
    printer.println(_name);
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
};

#endif // __SIMPLECOMMAND_H__
