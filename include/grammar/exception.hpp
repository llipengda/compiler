#pragma once
#ifndef GRAMMAR_EXCEPTION_HPP
#define GRAMMAR_EXCEPTION_HPP

#include "production.hpp"
#include <exception>
#include <string>

namespace grammar::exception {

class ambiguous_grammar_exception final : public std::exception {
public:
    explicit ambiguous_grammar_exception(const std::vector<grammar::production::production>& prods);
    const char* what() const noexcept override;

private:
    std::string msg = "Ambiguous grammar: \n";
};

class grammar_error final : public std::exception {
public:
    explicit grammar_error(const std::string& message);
    const char* what() const noexcept override;

private:
    std::string msg;
};

} // namespace grammar::exception

#endif // GRAMMAR_EXCEPTION_HPP