#pragma once
#include <string>
#include <vector>
#include <random>
#include <set>
#include "../../components/ConfigParser.hpp"

class ControlFlow {
private:
    static std::mt19937 generator;
    static std::set<int> validStates;
    
    static int generateRandomState();
    static std::vector<int> generateStates(const ConfigParser& config);
    static std::string generateJumpTable(const std::vector<int>& states);
    static std::mt19937& getGenerator();
    static std::string generateDispatcher();
    static std::string generateStateHandler(int state, const std::string& code, const ConfigParser& config);
    static std::string wrapInTryCatch(const std::string& code);
    static std::string generateFakeStates(const ConfigParser& config);
    static std::string generateCoroutineWrapper();
    static std::string generateVM();

public:
    static std::string scramble(const std::string& code, const ConfigParser& config);
}; 