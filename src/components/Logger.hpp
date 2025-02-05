#pragma once
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

class Logger {
private:
    static bool enabled;
    static bool useColors;
    static bool debugMode;

    #ifdef _WIN32
    static HANDLE hConsole;
    static WORD defaultAttributes;

    static void setWindowsColor(int color);
    static void resetWindowsColor();
    #endif

    static constexpr const char* RESET   = "\033[0m";
    static constexpr const char* RED     = "\033[31m";
    static constexpr const char* GREEN   = "\033[32m";
    static constexpr const char* YELLOW  = "\033[33m";
    static constexpr const char* BLUE    = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN    = "\033[36m";
    static constexpr const char* WHITE   = "\033[37m";
    static constexpr const char* BOLD    = "\033[1m";

    static void setColor(const std::string& level);
    static void resetColor();

public:
    static void init();
    static void setEnabled(bool value);
    static void setDebug(bool debug);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void debug(const std::string& message);
    static void success(const std::string& message);
    
    static bool isEnabled() { return enabled; }
}; 