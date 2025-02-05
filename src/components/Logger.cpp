#include "Logger.hpp"
#include <iostream>

bool Logger::enabled = true;
bool Logger::debugMode = false;
bool Logger::useColors = false;

#ifdef _WIN32
HANDLE Logger::hConsole = nullptr;
WORD Logger::defaultAttributes = 0;

void Logger::setWindowsColor(int color) {
    if (!useColors) return;
    SetConsoleTextAttribute(hConsole, color);
}

void Logger::resetWindowsColor() {
    if (!useColors) return;
    SetConsoleTextAttribute(hConsole, defaultAttributes);
}
#endif

void Logger::setColor(const std::string& level) {
    #ifdef _WIN32
        if (level == "INFO" || level == "SUCCESS")
            setWindowsColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (level == "WARN")
            setWindowsColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (level == "ERROR")
            setWindowsColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        else if (level == "DEBUG")
            setWindowsColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    #else
        if (level == "INFO" || level == "SUCCESS")
            std::cout << GREEN << BOLD;
        else if (level == "WARN")
            std::cout << YELLOW << BOLD;
        else if (level == "ERROR")
            std::cout << RED << BOLD;
        else if (level == "DEBUG")
            std::cout << CYAN << BOLD;
    #endif
}

void Logger::resetColor() {
    #ifdef _WIN32
        resetWindowsColor();
    #else
        std::cout << RESET;
    #endif
}

void Logger::init() {
    #ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    defaultAttributes = consoleInfo.wAttributes;
    useColors = true;
    #else
    useColors = true;
    #endif
    enabled = true;
    debugMode = false;
}

void Logger::setEnabled(bool value) { 
    enabled = value; 
}

void Logger::setDebug(bool debug) { 
    debugMode = debug; 
}

void Logger::info(const std::string& message) {
    if (enabled) {
        setColor("INFO");
        std::cout << "[INFO]";
        resetColor();
        std::cout << " " << message << std::endl;
    }
}

void Logger::warning(const std::string& message) {
    if (enabled) {
        setColor("WARN");
        std::cout << "[WARN]";
        resetColor();
        std::cout << " " << message << std::endl;
    }
}

void Logger::error(const std::string& message) {
    if (enabled) {
        setColor("ERROR");
        std::cerr << "[ERROR]";
        resetColor();
        std::cerr << " " << message << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    if (enabled && debugMode) {
        setColor("DEBUG");
        std::cout << "[DEBUG]";
        resetColor();
        std::cout << " " << message << std::endl;
    }
}

void Logger::success(const std::string& message) {
    if (enabled) {
        setColor("SUCCESS");
        std::cout << "[SUCCESS]";
        resetColor();
        std::cout << " " << message << std::endl;
    }
} 