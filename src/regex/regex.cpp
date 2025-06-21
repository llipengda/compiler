#include "regex/regex.hpp"

namespace regex {

regex::regex(const std::string& regex) : dfa_(regex) {}

bool regex::match(const std::string& str) const {
    return dfa_.match(str);
}

std::size_t regex::match_max(const std::string& str) const {
    return dfa_.match_max(str);
}

} // namespace regex
