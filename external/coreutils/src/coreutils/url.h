#pragma once

#include <coreutils/log.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace utils {
inline std::string url_encode(const std::string& s,
                              const std::string& chars = "!*'();:@&=+$,/?#[]")
{
    std::vector<char> target(s.length() * 3 + 1);
    char* ptr = target.data();
    for (char c : s) {
        if (chars.find(c) != std::string::npos || c == '%') {
            sprintf(ptr, "%%%02x", c);
            ptr += 3;
        } else {
            *ptr++ = c;
        }
    }
    *ptr = 0;
    return std::string(target.data());
}

inline std::string url_decode(const std::string& s)
{
    std::vector<char> target(s.length() + 1);
    char* ptr = target.data();
    for (size_t i = 0; i < s.length(); i++) {
        auto c = s[i];
        if (c == '%' && i + 2 < s.length()) {
            char* eptr = nullptr;
            auto n = std::strtol(s.substr(i + 1, 2).c_str(), &eptr, 16);
            if (*eptr == 0) {
                *ptr++ = static_cast<char>(n);
                i += 2;
                continue;
            }
        }
        *ptr++ = c;
    }
    *ptr = 0;
    return std::string(target.data());
}

} // namespace utils
