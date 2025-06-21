#include "semantic/sema_tree.hpp"
#include <iostream>

namespace semantic {

bool sema_tree_node::is_action() const {
    return action != nullptr;
}

bool sema_tree_node::is_symbol() const {
    return symbol != nullptr;
}

sema_tree_node::sema_tree_node(const symbol_t& symbol) : grammar::tree_node(std::make_shared<symbol_t>(symbol)) {}

sema_tree_node::sema_tree_node(const action_t& action) : grammar::tree_node(nullptr), action(action) {}

sema_tree_node::sema_tree_node(const sema_production::rhs_value_t& value) : grammar::tree_node(nullptr) {
    if (value.is_symbol) {
        symbol = std::make_shared<symbol_t>(value.get_symbol());
    } else if (value.is_action) {
        action = value.get_action();
    }
}

sema_tree::sema_tree(const std::vector<sema_production>& productions) {
    for (const auto& prod : productions) {
        prod_map[production(prod)] = prod;
    }
}

void sema_tree::add(const production& prod) {
    const auto& sema_prod = prod_map[prod];
    if (!root) {
        root = std::make_shared<sema_tree_node>(sema_prod.lhs);
        std::vector<std::shared_ptr<grammar::production::symbol>> tmp;
        for (const auto& rhs : sema_prod.rhs) {
            auto node = std::make_shared<sema_tree_node>(rhs);
            if (rhs.is_symbol && rhs.get_symbol().is_terminal()) {
                tmp.emplace_back(node->symbol);
            }
            node->parent = root;
            root->children.push_back(node);
            if (!next && rhs.is_symbol && rhs.get_symbol().is_non_terminal()) {
                next = node;
            }
        }
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
            to_replace.push_back(*it);
        }
        for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
            const auto& sym = (*it)->symbol;
            if (sym && sym->is_non_terminal()) {
                next_r = *it;
                break;
            }
        }
        return;
    }
    assert(next != nullptr);
    assert(next->children.empty());
    assert(*next->symbol == sema_prod.lhs);
    bool found = false;
    std::shared_ptr<sema_tree_node> new_next = nullptr;
    std::vector<std::shared_ptr<grammar::production::symbol>> tmp;
    for (const auto& rhs : sema_prod.rhs) {
        auto node = std::make_shared<sema_tree_node>(rhs);
        if (rhs.is_symbol && rhs.get_symbol().is_terminal()) {
            tmp.emplace_back(node->symbol);
        }
        node->parent = next;
        next->children.push_back(node);
        if (!found && rhs.is_symbol && rhs.get_symbol().is_non_terminal()) {
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
                if (child->symbol && child->symbol->is_non_terminal() && child->children.empty()) {
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

void sema_tree::add_r(const production& prod) {
    const auto& sema_prod = prod_map[prod];
    if (!root) {
        root = std::make_shared<sema_tree_node>(sema_prod.lhs);
        for (const auto& rhs : sema_prod.rhs) {
            auto node = std::make_shared<sema_tree_node>(rhs);
            node->parent = root;
            root->children.push_back(node);
        }
        for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
            auto snode = std::static_pointer_cast<sema_tree_node>(*it);
            if (snode->is_symbol() && snode->symbol->is_non_terminal()) {
                next_r = snode;
                break;
            }
        }
        return;
    }

    assert(next_r != nullptr);
    assert(next_r->children.empty());
    assert(*next_r->symbol == sema_prod.lhs);

    for (const auto& rhs : sema_prod.rhs) {
        auto node = std::make_shared<sema_tree_node>(rhs);
        node->parent = next_r;
        next_r->children.push_back(node);
    }

    bool found = false;

    for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
        auto snode = std::static_pointer_cast<sema_tree_node>(*it);
        if (snode->is_symbol() && snode->symbol->is_non_terminal()) {
            next_r = snode;
            found = true;
            break;
        }
    }

    if (!found) {
        next_r = next_r->parent;
        while (next_r) {
            for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                auto snode = std::static_pointer_cast<sema_tree_node>(*it);
                if (snode->is_symbol()
                    && snode->symbol->is_non_terminal()
                    && snode->children.empty()) {
                    next_r = snode;
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

void sema_tree::print_node(const std::shared_ptr<grammar::tree_node>& node, const int depth) const {
    auto snode = std::static_pointer_cast<sema_tree_node>(node);
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
    if (snode->is_symbol()) {
        auto ssym = std::static_pointer_cast<sema_symbol>(snode->symbol);
        std::cout << *ssym << '\n';
    } else {
        std::cout << "[action]\n";
    }
    for (const auto& child : node->children) {
        print_node(child, depth + 1);
    }
}

sema_env sema_tree::calc() {
    sema_env env;
    calc_node(root, env);
    return env;
}

void sema_tree::calc_node(std::shared_ptr<grammar::tree_node> node, sema_env& env) {
    auto snode = std::static_pointer_cast<sema_tree_node>(node);

    if (snode->is_action()) {
        snode->action(env);
        return;
    }
    if (snode->children.empty()) {
        return;
    }
    env.enter_symbol_scope();
    env.add_symbol(std::static_pointer_cast<sema_symbol>(snode->symbol));
    for (auto child : node->children) {
        auto schild = std::static_pointer_cast<sema_tree_node>(child);
        if (schild->is_symbol()) {
            env.add_symbol(std::static_pointer_cast<sema_symbol>(schild->symbol));
        }
    }
    for (auto child : node->children) {
        calc_node(child, env);
    }
    env.exit_symbol_scope();
}

} // namespace semantic