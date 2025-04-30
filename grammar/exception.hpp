#pragma once
#ifndef GRAMMAR_EXCEPTION_HPP
#define GRAMMAR_EXCEPTION_HPP

#include "production.hpp"
#include <exception>
#include <string>

namespace grammar::exception {
class ambiguous_grammar_exception : public std::exception {
public:
    ambiguous_grammar_exception(const std::vector<grammar::production::production>& prods) {
        for (const auto& prod : prods) {
            msg += prod.to_string() + "\n";
        }
    }

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg = "Ambiguous grammar: \n";
};

class grammar_error : public std::exception {
public:
    grammar_error(const std::string& message) : msg(message) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};
} // namespace grammar::exception

#endif // GRAMMAR_EXCEPTION_HPP