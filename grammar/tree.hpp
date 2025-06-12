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
    std::shared_ptr<tree_node> next_r = nullptr;

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
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol.is_non_terminal()) {
                    next_r = *it;
                    break;
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

    void add_r(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>();
            root->symbol = prod.lhs;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>();
                node->symbol = rhs;
                node->parent = root;
                root->children.push_back(node);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol.is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }
        
        assert(next_r != nullptr);
        assert(next_r->children.empty());
        assert(next_r->symbol == prod.lhs);
        
        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>();
            node->symbol = rhs;
            node->parent = next_r;
            next_r->children.push_back(node);
        }
        
        bool found = false;
        
        for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
            if ((*it)->symbol.is_non_terminal()) {
                next_r = *it;
                found = true;
                break;
            }
        }
        
        if (!found) {
            next_r = next_r->parent;
            while (next_r) {
                for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                    if ((*it)->symbol.is_non_terminal() && (*it)->children.empty()) {
                        next_r = *it;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                next_r = next_r->parent;
            }
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    static void print_node(const std::shared_ptr<tree_node>& node, const int depth) {
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