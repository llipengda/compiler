#pragma once
#ifndef REGEX_REGEX_HPP
#define REGEX_REGEX_HPP

#include "dfa.hpp"

namespace regex {
class regex {
public:
    regex() = delete;
    explicit regex(const std::string& regex) : dfa_(regex) {}

    bool match(const std::string& str) const {
        return dfa_.match(str);
    }

    std::size_t match_max(const std::string& str) const {
        return dfa_.match_max(str);
    }

private:
    dfa::dfa dfa_;
};
}; // namespace regex

#endif // REGEX_REGEX_HPP