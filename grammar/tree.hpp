#pragma once

#ifndef GRAMMAR_TREE_HPP
#define GRAMMAR_TREE_HPP

#include "production.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace grammar {
struct tree_node {
    production::symbol symbol;
    std::vector<std::shared_ptr<tree_node>> children;
    std::shared_ptr<tree_node> parent = nullptr;
};

class tree {
    std::shared_ptr<tree_node> root = nullptr;
    std::shared_ptr<tree_node> next = nullptr;

public:
    void add(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>();
            root->symbol = prod.lhs;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>();
                node->symbol = rhs;
                node->parent = root;
                root->children.push_back(node);
                if (!next && rhs.is_non_terminal()) {
                    next = node;
                }
            }
            return;
        }
        assert(next != nullptr);
        assert(next->children.empty());
        assert(next->symbol == prod.lhs);
        bool found = false;
        auto new_next = std::make_shared<tree_node>();
        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>();
            node->symbol = rhs;
            node->parent = next;
            next->children.push_back(node);
            if (!found && rhs.is_non_terminal()) {
                new_next = node;
                found = true;
            }
        }
        if (found) {
            next = new_next;
        } else {
            next = next->parent;
            while (next) {
                for (const auto& child : next->children) {
                    if (child->symbol.is_non_terminal() && child->children.empty()) {
                        next = child;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                next = next->parent;
            }
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    void print_node(const std::shared_ptr<tree_node>& node, int depth) const {
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        std::cout << node->symbol << "\n";
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_TREE_HPP