#pragma once
#ifndef REGEX_REGEX_HPP
#define REGEX_REGEX_HPP

#include "dfa.hpp"

namespace regex {

class regex {
public:
    regex() = delete;
    explicit regex(const std::string& regex);

    bool match(const std::string& str) const;
    std::size_t match_max(const std::string& str) const;

private:
    dfa::dfa dfa_;
};

} // namespace regex

#endif // REGEX_REGEX_HPP
