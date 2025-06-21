#pragma once
#ifndef GRAMMAR_TREE_HPP
#define GRAMMAR_TREE_HPP

#include "production.hpp"
#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace grammar {

struct tree_node {
    std::shared_ptr<production::symbol> symbol;
    std::vector<std::shared_ptr<tree_node>> children;
    std::shared_ptr<tree_node> parent = nullptr;

    virtual ~tree_node() = default;
    explicit tree_node(const production::symbol& sym);
    explicit tree_node(std::shared_ptr<production::symbol> sym);
};

class tree {
protected:
    std::shared_ptr<tree_node> root = nullptr;
    std::shared_ptr<tree_node> next = nullptr;
    std::shared_ptr<tree_node> next_r = nullptr;

    std::vector<std::shared_ptr<production::symbol>> to_replace;
    std::size_t replace_r_idx = 0;

public:
    tree() = default;
    virtual ~tree() = default;

    virtual void add(const production::production& prod);
    virtual void add_r(const production::production& prod);
    void update(const production::symbol& sym);
    void update_r(const production::symbol& sym);
    void print() const;
    virtual void print_node(const std::shared_ptr<tree_node>& node, int depth) const;
    void visit(const std::function<void(std::shared_ptr<tree_node>)>& func) const;

private:
    static void visit(const std::shared_ptr<tree_node>& node, const std::function<void(std::shared_ptr<tree_node>)>& func);
};

} // namespace grammar

#endif // GRAMMAR_TREE_HPP