#pragma once

#include <string>
#include <iostream>


namespace framework
{
    enum class LogLevel { LogDebug, LogError };

#ifdef _DEBUG
    static const LogLevel Level = LogLevel::LogDebug;
#else
    static const LogLevel Level = LogLevel::LogDebug; ;// LogLevel::LogError;
#endif
    

    inline void logg(const std::string& str, const LogLevel l = LogLevel::LogDebug)
    {
        if (!str.empty() && l == Level)
        {
            std::cout << std::endl << str;
        }
    }
}