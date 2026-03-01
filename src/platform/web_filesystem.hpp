#pragma once

#include <string>
#include <vector>

namespace platform {

class WebFileSystem {
public:
    static std::vector<uint8_t> readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    static bool fileExists(const std::string& path);
};

} // namespace platform