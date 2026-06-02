#include "Utilities.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace Fractal {

std::string readFile(const std::filesystem::path &path) {
    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) {
        std::cerr << "readFile: cannot open " << path << '\n';
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string readLine(const std::filesystem::path &path, uint32_t lineIndex) {
    if (lineIndex == 0) return {};

    std::ifstream file{path};
    if (!file.is_open()) return {};

    std::string line;
    for (uint32_t current = 0; current < lineIndex; ++current) {
        if (!std::getline(file, line)) return {};
    }
    return line;
}

void writeFile(const std::string &source, const std::filesystem::path &path) {
    std::ofstream file{path};
    if (!file.is_open()) {
        std::cerr << "writeFile: cannot open " << path << " for writing\n";
        return;
    }
    file << source;
}

bool isDigit(char character) {
    return character >= '0' && character <= '9';
}

bool isLetter(char character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || character == '_';
}

bool isAlphanumeric(char character) {
    return isDigit(character) || isLetter(character);
}

} // namespace Fractal
