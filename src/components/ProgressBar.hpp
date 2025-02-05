#pragma once
#include <string>
#include <iostream>
#include "Logger.hpp"

class ProgressBar {
private:
    size_t total;
    size_t current;
    size_t width;
    std::string prefix;
    std::string suffix;
    bool showPercentage;
    
    #ifdef _WIN32
    mutable SHORT lastLinePos;
    #endif
    
    void render() const;

public:
    ProgressBar(size_t total, size_t width = 50, const std::string& prefix = "", bool showPercentage = true)
        : total(total)
        , current(0)
        , width(width)
        , prefix(prefix)
        , showPercentage(showPercentage)
        , suffix("")
        #ifdef _WIN32
        , lastLinePos(-1)
        #endif
    {}
    
    void update(size_t current);
    void setPrefix(const std::string& prefix) { this->prefix = prefix; }
    void setSuffix(const std::string& suffix) { this->suffix = suffix; }
    void finish(const std::string& message = "");
}; 