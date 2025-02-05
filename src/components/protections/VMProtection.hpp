#pragma once
#include <string>
#include <vector>
#include "../../components/ConfigParser.hpp"

class VMProtection {
private:
    static std::string generateVM();
    static std::string encryptCode(const std::string& code, const std::vector<uint8_t>& key, size_t chunkSize);

public:
    static std::string wrapCode(const std::string& code, const std::vector<uint8_t>& key, size_t chunkSize, const ConfigParser& config);
}; 