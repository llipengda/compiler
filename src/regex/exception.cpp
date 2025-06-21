#include "regex/exception.hpp"

namespace regex {

unknown_character_exception::unknown_character_exception(const std::string& ch)
    : character(ch), message("Unknown character: " + ch) {}

const char* unknown_character_exception::what() const noexcept {
    return message.c_str();
}

invalid_regex_exception::invalid_regex_exception(const std::string& mes)
    : message("Invalid regex: " + mes) {}

const char* invalid_regex_exception::what() const noexcept {
    return message.c_str();
}

} // namespace regex