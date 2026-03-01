#include "platform/web_filesystem.hpp"
#include <fstream>
#include <sys/stat.h>
#include <emscripten.h>

namespace platform {

std::vector<uint8_t> WebFileSystem::readFile(const std::string& path) {
    // Check if file exists
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        throw std::runtime_error("File not found: " + path);
    }

    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
}

bool WebFileSystem::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool WebFileSystem::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

} // namespace platform