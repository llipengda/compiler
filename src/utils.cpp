#include "utils.hpp"

namespace utils {

std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

#if __cplusplus >= 201703L
std::string_view trim(std::string_view str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}
#endif

std::vector<std::string> split_trim(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t pos = 0, last_pos = 0;

    while ((pos = s.find(delimiter, last_pos)) != std::string::npos) {
        if (pos > last_pos) {
            auto sub = trim(s.substr(last_pos, pos - last_pos));
            if (!sub.empty()) {
                result.emplace_back(sub);
            }
        }
        last_pos = pos + delimiter.length();
    }

    if (last_pos < s.size()) {
        auto sub = trim(s.substr(last_pos, pos - last_pos));
        if (!sub.empty()) {
            result.emplace_back(sub);
        }
    }

    return result;
}

#if __cplusplus >= 201703L
std::vector<std::string_view> split_trim(std::string_view s, std::string_view delimiter) {
    std::vector<std::string_view> result;
    size_t pos = 0;

    while (true) {
        size_t next = s.find(delimiter, pos);
        if (next == std::string_view::npos) {
            if (pos < s.size()) {
                auto sub = trim(s.substr(pos));
                if (!sub.empty()) {
                    result.emplace_back(sub);
                }
            }
            break;
        }
        if (next > pos) {
            auto sub = trim(s.substr(pos, next - pos));
            if (!sub.empty()) {
                result.emplace_back(sub);
            }
        }
        pos = next + delimiter.size();
    }

    return result;
}
#endif

std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}

#if __cplusplus >= 201703L
std::string join(const std::vector<std::string_view>& strings, const std::string_view& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}
#endif

bool starts_with(const std::string& str, const std::string& prefix) {
#if __cplusplus >= 202002L
    return str.starts_with(prefix);
#else
    return str.find(prefix) == 0;
#endif
}

} // namespace utils
