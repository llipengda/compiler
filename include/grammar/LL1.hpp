#pragma once
#ifndef GRAMMAR_LL1_HPP
#define GRAMMAR_LL1_HPP

#include "grammar_base.hpp"
#include "production.hpp"

#include <unordered_map>
#include <vector>

namespace grammar {
class LL1 : public grammar_base {
public:
    using table_t = std::unordered_map<production::symbol, std::unordered_map<production::symbol, production::production>>;

    explicit LL1(const std::vector<production::production>& productions);
    explicit LL1(const std::string& str);

    void build() override;
    void parse(const std::vector<lexer::token>& input) override;
    void print_productions() const;
    void print_parsing_table() const;
    void print_parsing_table_pretty() const;

private:
    table_t parsing_table;
    void build_parsing_table();
};
} // namespace grammar

#endif // GRAMMAR_LL1_HPP