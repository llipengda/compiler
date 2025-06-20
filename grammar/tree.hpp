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
    std::shared_ptr<production::symbol> symbol;
    std::vector<std::shared_ptr<tree_node>> children;
    std::shared_ptr<tree_node> parent = nullptr;

    virtual ~tree_node() = default;

    explicit tree_node(const production::symbol& sym) : symbol(std::make_shared<production::symbol>(sym)) {}

    explicit tree_node(std::shared_ptr<production::symbol> sym) : symbol(sym) {}
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

    virtual void add(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>(prod.lhs);
            std::vector<std::shared_ptr<production::symbol>> tmp;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>(rhs);
                if (rhs.is_terminal()) {
                    tmp.emplace_back(node->symbol);
                }
                node->parent = root;
                root->children.push_back(node);
                if (!next && rhs.is_non_terminal()) {
                    next = node;
                }
            }
            for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
                to_replace.push_back(*it);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol->is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }
        assert(next != nullptr);
        assert(next->children.empty());
        assert(*next->symbol == prod.lhs);
        bool found = false;
        std::shared_ptr<tree_node> new_next = nullptr;
        std::vector<std::shared_ptr<production::symbol>> tmp;
        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>(rhs);
            if (rhs.is_terminal()) {
                tmp.emplace_back(node->symbol);
            }
            node->parent = next;
            next->children.push_back(node);
            if (!found && rhs.is_non_terminal()) {
                new_next = node;
                found = true;
            }
        }
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
            to_replace.push_back(*it);
        }

        if (found) {
            next = new_next;
        } else {
            next = next->parent;
            while (next) {
                for (const auto& child : next->children) {
                    if (child->symbol->is_non_terminal() && child->children.empty()) {
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

    virtual void add_r(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>(prod.lhs);
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>(rhs);
                node->parent = root;
                root->children.push_back(node);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol->is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }

        assert(next_r != nullptr);
        assert(next_r->children.empty());
        assert(*next_r->symbol == prod.lhs);

        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>(rhs);
            node->parent = next_r;
            next_r->children.push_back(node);
        }

        bool found = false;

        for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
            if ((*it)->symbol->is_non_terminal()) {
                next_r = *it;
                found = true;
                break;
            }
        }

        if (!found) {
            next_r = next_r->parent;
            while (next_r) {
                for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                    if ((*it)->symbol->is_non_terminal() && (*it)->children.empty()) {
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

    void update(const production::symbol& sym) {
        if (to_replace.empty()) {
            return;
        }
        auto back = to_replace.back();
        if (sym == *back) {
            back->update(sym);
            to_replace.pop_back();
        }
    }

    void update_r(const production::symbol& sym) {
        if (to_replace.empty()) {
            visit([&](std::shared_ptr<tree_node> node) {
                if (node->symbol && node->symbol->is_terminal() && !node->symbol->is_epsilon()) {
                    to_replace.emplace_back(node->symbol);
                }
            });
        }
        if (replace_r_idx >= to_replace.size()) {
            return;
        }
        auto ori = to_replace[replace_r_idx];
        if (sym == *ori) {
            ori->update(sym);
            replace_r_idx++;
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    virtual void print_node(const std::shared_ptr<tree_node>& node, const int depth) const {
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        if (node->symbol) {
            std::cout << *node->symbol << "\n";
        }
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }

    void visit(const std::function<void(std::shared_ptr<tree_node>)>& func) {
        visit(root, func);
    }

private:
    void visit(std::shared_ptr<tree_node> node, const std::function<void(std::shared_ptr<tree_node>)>& func) {
        if (!node) {
            return;
        }
        func(node);
        if (node->children.empty()) {
            return;
        }
        for (auto child : node->children) {
            visit(child, func);
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_TREE_HPP