#pragma once
#ifndef REGEX_EXCEPTION_HPP
#define REGEX_EXCEPTION_HPP

#include <exception>
#include <string>

namespace regex {
class unknown_character_exception final : public std::exception {
public:
    explicit unknown_character_exception(const std::string& ch)
        : character(ch), message("Unknown character: ") {
        message += character;
    }

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string character;
    std::string message;
};

class invalid_regex_exception final : public std::exception {
public:
    explicit invalid_regex_exception(const std::string& mes)
        : message("Invalid regex: ") {
        message += mes;
    }

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};
}; // namespace regex

#endif // REGEX_EXCEPTION_HPP