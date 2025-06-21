#pragma once
#ifndef GRAMMAR_BASE_HPP
#define GRAMMAR_BASE_HPP

#include "lexer/lexer.hpp"
#include "production.hpp"
#include "tree.hpp"

#include <memory>
#include <unordered_set>

namespace grammar {
class grammar_base {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    virtual void parse(const std::vector<lexer::token>&) = 0;
    virtual void build() = 0;
    virtual ~grammar_base() = default;

    virtual void print_tree() const;
    std::shared_ptr<tree> get_tree() const;

protected:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;
    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    std::shared_ptr<tree> tree_ = std::make_shared<tree>();

    void calc_first();
    symbol_set& calc_first(const production::symbol& sym);
    symbol_set calc_first(const std::vector<production::symbol>& symbols);
    void calc_follow();
    bool calc_follow(std::size_t pos);
    symbol_set calc_first(const production::production& prod) const;
};
} // namespace grammar

#endif