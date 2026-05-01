#ifndef __SERIALCONSOLE_H__
#define __SERIALCONSOLE_H__

#include <Arduino.h>
#include "IConsoleCommand.h"
#include <array>

template <std::size_t CMD_NUM>
class SerialConsole
{
    Stream &_serial;
    const std::array<const IConsoleCommand *, CMD_NUM> &_commands;
    ArrayString<IConsoleCommand::CMD_MAX_LEN> _buf;
    bool _echo = true;

public:
    SerialConsole(Stream &serial, const std::array<const IConsoleCommand *, CMD_NUM> &commands)
        : _serial(serial), _commands(commands)
    {
    }
    void run()
    {
        for (;;)
        {
            int c = _serial.read();
            if (c < 0)
            {
                return;
            }
            if (_echo)
            {
                _serial.write(static_cast<uint8_t>(c));
            }
            // 文字のみ追加する
            if (isprint(c) && !isspace(c))
            {
                _buf.append(static_cast<char>(c));
            }
            // コマンドの区切り
            if (c == '\r')
            {
                if (_echo)
                {
                    _serial.println();
                }
                const bool processed = proccessCommand();
                if (!processed)
                {
                    _serial.print("Command Failed: ");
                    _serial.println(_buf.c_str());
                }
                _buf.clear();
                return;
            }
        }
    }
    void setEcho(bool on)
    {
        _echo = on;
    }

private:
    bool proccessCommand()
    {
        switch (_buf.c_str()[0])
        {
        case '?':
            infoCommand();
            return true;
        default:
            break;
        }
        for (const IConsoleCommand *cmd : _commands)
        {
            if (cmd == nullptr)
            {
                continue;
            }
            if (cmd->run(_buf, _serial))
            {
                return true;
            }
        }
        return false;
    }
    void infoCommand()
    {
        for (const IConsoleCommand *cmd : _commands)
        {
            if (cmd == nullptr)
            {
                continue;
            }
            cmd->info(_serial);
        }
    }
};

#endif // __SERIALCONSOLE_H__
