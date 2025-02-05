#include "ProgressBar.hpp"
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

void ProgressBar::render() const {
    if (!Logger::isEnabled() || total == 0) return;

    float percentage = static_cast<float>(current) / total;
    size_t filled = static_cast<size_t>(width * percentage);
    
    std::stringstream progressBar;
    if (!prefix.empty()) {
        progressBar << " " << prefix << " ";
    }
    
    progressBar << "[";
    for (size_t i = 0; i < width; ++i) {
        if (i < filled) {
            progressBar << "=";
        } else if (i == filled) {
            progressBar << ">";
        } else {
            progressBar << " ";
        }
    }
    progressBar << "]";
    
    if (showPercentage) {
        progressBar << " " << std::fixed << std::setprecision(1) << (percentage * 100.0) << "%";
    }
    
    if (!suffix.empty()) {
        progressBar << " " << suffix;
    }

    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        
        SHORT savedY = csbi.dwCursorPosition.Y;
        if (lastLinePos == -1) {
            lastLinePos = savedY;
        }
        
        COORD savePos = {0, lastLinePos};
        SetConsoleCursorPosition(hConsole, savePos);
        
        DWORD written;
        FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, savePos, &written);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, csbi.dwSize.X, savePos, &written);
        
        SetConsoleCursorPosition(hConsole, savePos);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::cout << "[PROGRESS]";
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);
        std::cout << progressBar.str() << std::flush;
    #else
        std::cout << "\r\033[K" << "\033[36;1m[PROGRESS]\033[0m" << progressBar.str() << std::flush;
    #endif
}

void ProgressBar::update(size_t newCurrent) {
    if (!Logger::isEnabled() || total == 0) return;
    
    current = newCurrent;
    if (current > total) current = total;
    render();
}

void ProgressBar::finish(const std::string& message) {
    current = total;
    render();
    std::cout << std::endl;
    
    #ifdef _WIN32
    lastLinePos = -1;
    #endif
    
    if (!message.empty()) {
        Logger::info(message);
    }
} 