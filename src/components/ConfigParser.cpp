#include "ConfigParser.hpp"
#include "Logger.hpp"
#include <fstream>
#include <algorithm>

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

bool ConfigParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    sections.clear();

    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            sections[currentSection][key] = value;
        }
    }

    return true;
}

std::string ConfigParser::getValue(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    auto sectionIt = sections.find(section);
    if (sectionIt != sections.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigParser::getIntValue(const std::string& section, const std::string& key, int defaultValue) const {
    std::string value = getValue(section, key);
    if (value.empty()) return defaultValue;
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigParser::getBoolValue(const std::string& section, const std::string& key, bool defaultValue) const {
    std::string value = getValue(section, key);
    if (value.empty()) return defaultValue;
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }
    
    return defaultValue;
} 