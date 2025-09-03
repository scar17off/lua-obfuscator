#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <random>
#include <algorithm>
#include <cstring>
#include <bitset>
#include "LuaObfuscator.hpp"
#include "components/Logger.hpp"
#include <chrono>

void printUsage() {
    std::cout << "Lua Code Obfuscator\n"
              << "Usage: obfuscator <command> [options]\n\n"
              << "Commands:\n"
              << "  obfuscate <input_file> <output_file> [options]\n"
              << "Options:\n"
              << "  --strings      Encrypt strings\n"
              << "  --no-strings   Disable string encryption\n"
              << "  --junk        Add junk code\n"
              << "  --no-junk     Disable junk code\n"
              << "  --vm          Apply VM wrapper\n"
              << "  --no-vm       Disable VM wrapper\n"
              << "  --all         Apply all obfuscation techniques\n"
              << "  --chunk-size  Configure array chunk size\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];
    if (command != "obfuscate") {
        std::cout << "Unknown command: " << command << "\n";
        return 1;
    }

    std::string inputFile = argv[2];
    std::string outputFile = argv[3];
    bool useStrings = false;
    bool useJunk = false;
    bool useVM = false;

    Logger::debug("Parsing command line arguments...");
    for (int i = 4; i < argc; i++) {
        std::string flag = argv[i];
        Logger::debug("Processing flag: " + flag);
        
        if (flag == "--all") {
            useStrings = useJunk = useVM = true;
            Logger::debug("Enabled all features");
        } else if (flag == "--strings") {
            useStrings = true;
            Logger::debug("Enabled string encryption");
        } else if (flag == "--junk") {
            useJunk = true;
            Logger::debug("Enabled junk code generation");
        } else if (flag == "--vm") {
            useVM = true;
            Logger::debug("Enabled VM protection");
        } else {
            Logger::warning("Unknown flag: " + flag);
        }
    }

    Logger::debug("Features enabled - Strings: " + std::string(useStrings ? "yes" : "no") + 
                 ", Junk: " + std::string(useJunk ? "yes" : "no") + 
                 ", VM: " + std::string(useVM ? "yes" : "no"));

    LuaObfuscator obfuscator;
    
    if (!obfuscator.loadConfig("config.ini")) {
        return 1;
    }

    if (!obfuscator.loadFile(inputFile)) {
        Logger::error("Failed to load input file: " + inputFile);
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    obfuscator.obfuscate(useStrings, useJunk, useVM);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (!obfuscator.saveToFile(outputFile)) {
        Logger::error("Failed to save output file: " + outputFile);
        return 1;
    }

    Logger::info("Obfuscation completed successfully in " + std::to_string(duration.count()) + "ms!");
    return 0;
}