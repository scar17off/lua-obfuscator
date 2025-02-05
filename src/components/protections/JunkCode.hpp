#pragma once
#include <string>
#include <random>

class JunkCode {
private:
    static std::string generateRandomString(int length);
    static std::mt19937& getGenerator();

public:
    static std::string generate(int count);
}; 