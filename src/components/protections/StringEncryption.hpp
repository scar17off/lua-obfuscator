#pragma once
#include <string>
#include <vector>
#include "../Logger.hpp"

class StringEncryption {
private:
    static std::string encryptBytes(const std::string& input, const std::vector<uint8_t>& key, const std::string& varName, size_t chunkSize);

public:
    static std::string encrypt(const std::string& input, const std::vector<uint8_t>& key, const std::string& varName, size_t chunkSize);
    static std::string generateDecryptor(const std::vector<uint8_t>& key);
    static void processString(std::string& sourceCode, const std::vector<uint8_t>& key, size_t chunkSize);
}; 