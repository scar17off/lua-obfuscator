#pragma once
#include "components/ConfigParser.hpp"
#include <string>
#include <vector>
#include <random>

class LuaObfuscator {
private:
    std::string sourceCode;
    std::random_device rd;
    std::mt19937 gen;
    std::vector<uint8_t> key;
    size_t ARRAY_CHUNK_SIZE;
    ConfigParser config;

    void generateEncryptionKey();
    std::string generateRandomString(int length);

public:
    LuaObfuscator();
    bool loadConfig(const std::string& filename);
    bool loadFile(const std::string& filename);
    bool saveToFile(const std::string& filename);
    void obfuscate(bool useStrings, bool useJunk, bool useVM);
    void setArrayChunkSize(size_t size);
    bool getConfigBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    int getConfigInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    std::string getConfigString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
}; 