#ifndef __ICONSOLECOMMAND_H__
#define __ICONSOLECOMMAND_H__

#include "ArrayString.h"
#include "Arduino.h"

class IConsoleCommand
{
public:
  static constexpr std::size_t CMD_MAX_LEN = 64;
  virtual bool run(const ArrayString<CMD_MAX_LEN> &cmdline, Print &printer) const = 0;
  virtual void info(Print &printer) const = 0;
};

#endif // __ICONSOLECOMMAND_H__
