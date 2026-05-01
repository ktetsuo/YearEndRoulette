#ifndef __PARSEVALUE_H__
#define __PARSEVALUE_H__

#include <cstdlib>
#include <limits>
#include <optional>
#include <cstring>

template <typename T>
std::optional<T> parseValue(const char *s);

template <typename T>
std::optional<T> parseIntegerValue(const char *s)
{
    char *endptr;
    long long int value = std::strtoll(s, &endptr, 10);
    if (*endptr == '\0' && value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max())
    {
        return static_cast<T>(value);
    }
    else
    {
        return std::nullopt;
    }
}

template <>
std::optional<int> parseValue<int>(const char *s)
{
    return parseIntegerValue<int>(s);
}

template <>
std::optional<long> parseValue<long>(const char *s)
{
    return parseIntegerValue<long>(s);
}

template <>
std::optional<short> parseValue<short>(const char *s)
{
    return parseIntegerValue<short>(s);
}

template <typename T>
std::optional<T> parseUnsignedValue(const char *s)
{
    char *endptr;
    unsigned long long int value = std::strtoull(s, &endptr, 10);
    if (*endptr == '\0' && value <= std::numeric_limits<T>::max())
    {
        return static_cast<T>(value);
    }
    else
    {
        return std::nullopt;
    }
}

template <>
std::optional<unsigned long> parseValue<unsigned long>(const char *s)
{
    return parseUnsignedValue<unsigned long>(s);
}

template <>
std::optional<unsigned int> parseValue<unsigned int>(const char *s)
{
    return parseUnsignedValue<unsigned int>(s);
}

template <>
std::optional<unsigned short> parseValue<unsigned short>(const char *s)
{
    return parseUnsignedValue<unsigned short>(s);
}

template <>
std::optional<float> parseValue<float>(const char *s)
{
    char *endptr;
    float value = std::strtof(s, &endptr);
    if (*endptr == '\0')
    {
        return value;
    }
    else
    {
        return std::nullopt;
    }
}

template <>
std::optional<double> parseValue<double>(const char *s)
{
    char *endptr;
    double value = std::strtod(s, &endptr);
    if (*endptr == '\0')
    {
        return value;
    }
    else
    {
        return std::nullopt;
    }
}

template <>
std::optional<bool> parseValue<bool>(const char *s)
{
    if (s == nullptr)
    {
        return std::nullopt;
    }

    if (std::strcmp(s, "1") == 0 || std::strcmp(s, "true") == 0 || std::strcmp(s, "on") == 0)
    {
        return true;
    }

    if (std::strcmp(s, "0") == 0 || std::strcmp(s, "false") == 0 || std::strcmp(s, "off") == 0)
    {
        return false;
    }

    return std::nullopt;
}

#endif // __PARSEVALUE_H__
