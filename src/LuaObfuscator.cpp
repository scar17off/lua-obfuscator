#include "LuaObfuscator.hpp"
#include "components/protections/StringEncryption.hpp"
#include "components/protections/VMProtection.hpp"
#include "components/protections/JunkCode.hpp"
#include "components/protections/Compression.hpp"
#include <fstream>
#include <sstream>
#include "components/Logger.hpp"

LuaObfuscator::LuaObfuscator() : gen(rd()), ARRAY_CHUNK_SIZE(20) {
    generateEncryptionKey();
}

void LuaObfuscator::generateEncryptionKey() {
    key.resize(16);
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 16; ++i) {
        key[i] = dis(gen);
    }
}

std::string LuaObfuscator::generateRandomString(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

bool LuaObfuscator::loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    sourceCode = buffer.str();
    return true;
}

bool LuaObfuscator::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << sourceCode;
    return true;
}

void LuaObfuscator::obfuscate(bool useStrings, bool useJunk, bool useVM) {
    try {
        // Override configuration if --all flag is used
        if (useStrings) {
            Logger::debug("Overriding config: Enabling string encryption");
        } else {
            useStrings = config.getBoolValue("Strings", "enabled", false);
        }

        if (useJunk) {
            Logger::debug("Overriding config: Enabling junk code generation");
        } else {
            useJunk = config.getBoolValue("Junk", "enabled", false);
        }

        if (useVM) {
            Logger::debug("Overriding config: Enabling VM protection");
        } else {
            useVM = config.getBoolValue("VM", "enabled", false);
        }

        if (useStrings) {
            size_t chunkSize = config.getIntValue("Encryption", "chunk_size", 20);
            StringEncryption::processString(sourceCode, key, chunkSize);
        }

        if (useJunk) {
            Logger::debug("Adding junk code...");
            int junkCount = config.getIntValue("Junk", "junk_count", 3);
            std::string junkCode = JunkCode::generate(junkCount);
            size_t insertPos = sourceCode.find("\n");
            if (insertPos != std::string::npos) {
                sourceCode.insert(insertPos + 1, junkCode);
            }
        }

        if (useVM) {
            Logger::debug("Applying VM protection...");
            size_t codeChunkSize = config.getIntValue("VM", "code_chunk_size", 100);
            sourceCode = VMProtection::wrapCode(sourceCode, key, codeChunkSize, config);
        }

    } catch (const std::exception& e) {
        Logger::error("Obfuscation failed: " + std::string(e.what()));
        throw;
    }
}

bool LuaObfuscator::loadConfig(const std::string& filename) {
    Logger::init();
    Logger::info("Loading configuration from: " + filename);
    
    if (!config.load(filename)) {
        Logger::error("Failed to load config file");
        return false;
    }

    ARRAY_CHUNK_SIZE = config.getIntValue("Encryption", "chunk_size", 20);
    key.resize(config.getIntValue("Strings", "key_size", 16));
    generateEncryptionKey();
    
    Logger::setEnabled(config.getBoolValue("Logging", "enabled", true));
    Logger::setDebug(config.getBoolValue("Logging", "debug", false));
    
    Logger::info("Configuration loaded successfully");
    return true;
}

void LuaObfuscator::setArrayChunkSize(size_t size) {
    ARRAY_CHUNK_SIZE = size;
}

bool LuaObfuscator::getConfigBool(const std::string& section, const std::string& key, bool defaultValue) const {
    return config.getBoolValue(section, key, defaultValue);
}

int LuaObfuscator::getConfigInt(const std::string& section, const std::string& key, int defaultValue) const {
    return config.getIntValue(section, key, defaultValue);
}

std::string LuaObfuscator::getConfigString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    return config.getValue(section, key, defaultValue);
}