#pragma once
#ifndef SEMANTIC_SEMA_TREE_HPP
#define SEMANTIC_SEMA_TREE_HPP

#include "grammar/tree.hpp"
#include "sema_production.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

namespace semantic {
struct sema_tree_node : public grammar::tree_node {
    using action_t = sema_production::action;
    using symbol_t = sema_symbol;

    action_t action = nullptr;

    bool is_action() const;
    bool is_symbol() const;

    explicit sema_tree_node(const symbol_t& symbol);
    explicit sema_tree_node(const action_t& action);
    explicit sema_tree_node(const sema_production::rhs_value_t& value);
};

class sema_tree : public grammar::tree {
    using production = grammar::production::production;

private:
    std::unordered_map<production, sema_production> prod_map;

public:
    explicit sema_tree(const std::vector<sema_production>& productions);

    virtual void add(const production& prod) override;
    virtual void add_r(const production& prod) override;
    virtual void print_node(const std::shared_ptr<grammar::tree_node>& node, const int depth) const override;

    sema_env calc();

private:
    void calc_node(std::shared_ptr<grammar::tree_node> node, sema_env& env);
};
} // namespace semantic

#endif