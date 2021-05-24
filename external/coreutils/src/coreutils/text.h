#pragma once
#include "split.h"

#include "log.h"

#include <string>
#include <vector>

namespace utils {

inline static void makeLower(std::string& s)
{
    for (auto& c : s)
        c = (char)tolower(c);
}

inline static std::string toLower(const std::string& s)
{
    std::string s2 = s;
    makeLower(s2);
    return s2;
}

inline std::string rstrip(const std::string& x, char c = ' ')
{
    auto l = x.length();
    if (c == 10) {
        while (l > 1 && (x[l - 1] == 10 || x[l - 1] == 13))
            l--;
    } else {
        while (l > 1 && x[l - 1] == c)
            l--;
    }
    if (l == x.length()) return x;
    return x.substr(0, l);
}

inline std::string lstrip(const std::string& x, char c = ' ')
{
    size_t l = 0;
    while (x[l] && x[l] == c)
        l++;
    if (l == 0) return x;
    return x.substr(l);
}

inline std::string lrstrip(const std::string& x, char c = ' ')
{
    size_t l = 0;
    while (x[l] && x[l] == c)
        l++;
    size_t r = l;
    while (x[r] && x[r] != c)
        r++;
    return x.substr(l, r - l);
}

template <typename String>
inline std::vector<String> text_wrap(
    const String& t, int width, int initialWidth = -1)
{
    std::vector<String> lines;
    std::vector<String> origLines = split(t, String(1, '\n'));

    for (auto const& text : origLines) {
        // Find space from right
        int w = width;
        size_t start = 0;
        size_t end = w;
        int subseqWidth = w;
        if (initialWidth >= 0) w = initialWidth;
  //      LOGI("'%s'", text);
        while (true) {
            if (end > text.length()) {
                lines.push_back(text.substr(start));
 //               LOGI("Trailing '%s'", lines.back());
                break;
            }
            auto pos = text.rfind(' ', end);
            if (pos != std::string::npos && pos > start) {
                lines.push_back(text.substr(start, pos - start));
                start = pos + 1;
            } else {
                lines.push_back(text.substr(start, w));
                start += w;
            }
//            LOGI("Pushed '%s'", lines.back());
            w = subseqWidth;
            end = start + w;
        }
    }
    return lines;
}

inline std::string text_center(std::string const& text, int width)
{
    int prefix = (width - (int)text.length()) / 2;

    if (prefix <= 0) return text;
    return std::string(prefix, ' ') + text;
}

inline bool endsWith(const std::string& name, const std::string& ext)
{
    auto pos = name.rfind(ext);
    return (pos != std::string::npos && pos == name.length() - ext.length());
}

inline bool startsWith(
    const std::string_view& name, const std::string_view& pref)
{
    if (pref.empty()) return true;
    return name.find(pref) == 0;
}

} // namespace utils
