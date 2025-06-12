#pragma once

#ifndef REGEX_TREE_HPP
#define REGEX_TREE_HPP

#include "exception.hpp"
#include "token.hpp"

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace regex::tree {

class regex_node {
public:
    enum class type {
        ch,
        concat,
        alt,
        star,
        plus,
    };

    using node_ptr_t = std::shared_ptr<regex_node>;

    type type;

    bool nullable;
    std::unordered_set<std::size_t> firstpos;
    std::unordered_set<std::size_t> lastpos;

    virtual ~regex_node() = default;

    template <class T>
    T& as() {
        return static_cast<T&>(*this);
    }
};

class char_node final : public regex_node {
public:
    token::token_type value;
    std::size_t number;

    explicit char_node(const token::token_type& ch, const std::size_t number) : value(ch), number(number) {
        type = type::ch;
        nullable = false;
        firstpos.insert(number);
        lastpos.insert(number);
    }
};

class concat_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit concat_node(const node_ptr_t& left, const node_ptr_t& right) : left(left), right(right) {
        type = type::concat;
        nullable = left->nullable && right->nullable;
        firstpos = left->firstpos;
        if (left->nullable) {
            firstpos.insert(right->firstpos.begin(), right->firstpos.end());
        }
        lastpos = right->lastpos;
        if (right->nullable) {
            lastpos.insert(left->lastpos.begin(), left->lastpos.end());
        }
    }
};

class star_node final : public regex_node {
public:
    node_ptr_t child;
    explicit star_node(const node_ptr_t& child) : child(child) {
        type = type::star;
        nullable = true;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class plus_node final : public regex_node {
public:
    node_ptr_t child;
    explicit plus_node(const node_ptr_t& child) : child(child) {
        type = type::plus;
        nullable = child->nullable;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class alt_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit alt_node(const node_ptr_t& left, const node_ptr_t& right) : left(left), right(right) {
        type = type::alt;
        nullable = left->nullable || right->nullable;
        firstpos = left->firstpos;
        firstpos.insert(right->firstpos.begin(), right->firstpos.end());
        lastpos = left->lastpos;
        lastpos.insert(right->lastpos.begin(), right->lastpos.end());
    }
};

class regex_tree {
public:
    regex_node::node_ptr_t root;

    using token_map_t = std::unordered_map<token::token_type, std::unordered_set<std::size_t>, token::token_type_hash>;

    token_map_t token_map;

    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> followpos;

    explicit regex_tree(regex_node& root) : root(&root) {}

    explicit regex_tree(const std::string& s) {
        auto ss = token::split(s);
#ifdef SHOW_DEBUG
        ::regex::print(ss);
#endif
        auto postfix = to_postfix(ss);
#ifdef SHOW_DEBUG
        ::regex::print(postfix);
#endif

        std::stack<regex_node::node_ptr_t> st;

        size_t i = 1;

        using token::op;

        for (const auto& ch : postfix) {
            if (is(ch, op::star)) {
                if (st.empty()) {
                    throw regex::invalid_regex_exception("'*' operator with empty stack");
                }
                auto operand = st.top();
                st.pop();
                st.push(std::make_shared<star_node>(star_node{operand}));
            } else if (is(ch, op::plus)) {
                if (st.empty()) {
                    throw regex::invalid_regex_exception("'+' operator with empty stack");
                }
                auto operand = st.top();
                st.pop();
                st.push(std::make_shared<plus_node>(plus_node{operand}));
            } else if (is(ch, op::concat)) {
                if (st.size() < 2) {
                    throw regex::invalid_regex_exception("'·' operator with fewer than 2 operands");
                }
                auto right = st.top();
                st.pop();
                auto left = st.top();
                st.pop();
                st.push(std::make_shared<concat_node>(concat_node{left, right}));
            } else if (is(ch, op::alt)) {
                if (st.size() < 2) {
                    throw regex::invalid_regex_exception("'|' operator with fewer than 2 operands");
                }
                auto right = st.top();
                st.pop();
                auto left = st.top();
                st.pop();
                st.push(std::make_shared<alt_node>(alt_node{left, right}));
            } else if (is_nonop(ch)) {
                token_map[ch].insert(i);
                st.push(std::make_shared<char_node>(char_node{ch, i++}));
            } else {
                throw regex::invalid_regex_exception(s);
            }
        }

        if (st.size() != 1) {
            throw regex::invalid_regex_exception("leftover operands after parsing");
        }

        root = st.top();
        st.pop();

        visit([&](regex_node& node) {
            if (node.type == regex_node::type::concat) {
                auto& concat_node = node.as<tree::concat_node>();
                for (auto idx : concat_node.left->lastpos) {
                    followpos[idx].insert(concat_node.right->firstpos.begin(), concat_node.right->firstpos.end());
                }
            } else if (node.type == regex_node::type::star) {
                auto& star_node = node.as<tree::star_node>();
                for (auto idx : star_node.lastpos) {
                    followpos[idx].insert(star_node.firstpos.begin(), star_node.firstpos.end());
                }
            } else if (node.type == regex_node::type::plus) {
                auto& plus_node = node.as<tree::plus_node>();
                for (auto idx : plus_node.lastpos) {
                    followpos[idx].insert(plus_node.firstpos.begin(), plus_node.firstpos.end());
                }
            }
        });

        token_map = disjoint_token_sets(token_map);
    }

    void visit(const std::function<void(regex_node&)>& func) const {
        visit(root, func);
    }

    void print() const {
        std::cout << "Regex Tree" << std::endl;
        print(root);
    }

private:
    static void visit(const regex_node::node_ptr_t& node, const std::function<void(regex_node&)>& func) {
        if (node == nullptr) {
            return;
        }

        func(*node.get());

        if (node->type == regex_node::type::concat) {
            auto& concat_node = node->as<tree::concat_node>();
            visit(concat_node.left, func);
            visit(concat_node.right, func);
        } else if (node->type == regex_node::type::alt) {
            auto& alt_node = node->as<tree::alt_node>();
            visit(alt_node.left, func);
            visit(alt_node.right, func);
        } else if (node->type == regex_node::type::star) {
            auto& star_node = node->as<tree::star_node>();
            visit(star_node.child, func);
        } else if (node->type == regex_node::type::plus) {
            auto& plus_node = node->as<tree::plus_node>();
            visit(plus_node.child, func);
        }
    }

    static void print(const regex_node::node_ptr_t& node, const int indent = 0) {
        using token::op;

        if (node == nullptr) {
            return;
        }
        std::cout << std::string(indent, ' ');
        if (node->type == regex_node::type::ch) {
            const auto& char_node = node->as<tree::char_node>();
            std::cout << char_node.value << '(' << char_node.number << ')' << std::endl;
        } else if (node->type == regex_node::type::concat) {
            std::cout << token::op_map.at(op::concat) << std::endl;
            print(node->as<concat_node>().left, indent + 2);
            print(node->as<concat_node>().right, indent + 2);
        } else if (node->type == regex_node::type::alt) {
            std::cout << token::op_map.at(op::alt) << std::endl;
            print(node->as<alt_node>().left, indent + 2);
            print(node->as<alt_node>().right, indent + 2);
        } else if (node->type == regex_node::type::star) {
            std::cout << token::op_map.at(op::star) << std::endl;
            print(node->as<star_node>().child, indent + 2);
        } else if (node->type == regex_node::type::plus) {
            std::cout << token::op_map.at(op::plus) << std::endl;
            print(node->as<plus_node>().child, indent + 2);
        }
    }

    struct unordered_set_hash {
        std::size_t operator()(const std::unordered_set<std::size_t>& s) const {
            std::size_t h = 0;
            for (const auto v : s) {
                h ^= std::hash<std::size_t>{}(v);
            }
            return h;
        }
    };

    static token_map_t disjoint_token_sets(const token_map_t& original) {
        std::unordered_map<char, std::unordered_set<size_t>> char_to_positions;

        for (const auto& [token, positions] : original) {
            if (is(token, token::symbol::end_mark)) {
                continue;
            }
            auto cs = to_char_set(token);
            if (!cs.is_negative) {
                for (char ch : cs.chars) {
                    char_to_positions[ch].insert(positions.begin(), positions.end());
                }
            } else {
                for (int c = -128; c < 128; ++c) {
                    if (!cs.chars.count(static_cast<char>(c))) {
                        char_to_positions[c].insert(positions.begin(), positions.end());
                    }
                }
            }
        }

        std::unordered_map<std::unordered_set<size_t>, std::unordered_set<char>, unordered_set_hash> grouped;
        for (const auto& [ch, pos] : char_to_positions) {
            grouped[pos].insert(ch);
        }

        token_map_t result;

        for (const auto& [positions, chars] : grouped) {
            if (chars.size() == 1) {
                result[token::token_type{*chars.begin()}] = positions;
                continue;
            }
            token::char_set cs;
            for (char ch : chars) {
                cs.add(ch);
            }
            result[token::token_type{cs}] = positions;
        }

        result[token::token_type{token::symbol::end_mark}] = original.at(token::symbol::end_mark);

        return result;
    }
};

} // namespace regex::tree

#endif // REGEX_TREE_HPP
