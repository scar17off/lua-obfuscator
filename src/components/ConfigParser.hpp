#pragma once
#include <string>
#include <map>
#include "Logger.hpp"

class ConfigParser {
private:
    std::map<std::string, std::map<std::string, std::string>> sections;
    static std::string trim(const std::string& str);

public:
    bool load(const std::string& filename);
    std::string getValue(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getIntValue(const std::string& section, const std::string& key, int defaultValue = 0) const;
    bool getBoolValue(const std::string& section, const std::string& key, bool defaultValue = false) const;
}; 