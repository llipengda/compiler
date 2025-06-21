#pragma once

#ifndef REGEX_TREE_HPP
#define REGEX_TREE_HPP

#include "token.hpp"

#include <cstddef>
#include <functional>
#include <memory>
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
    T& as();
};

class char_node final : public regex_node {
public:
    token::token_type value;
    std::size_t number;

    explicit char_node(token::token_type ch, std::size_t number);
};

class concat_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit concat_node(const node_ptr_t& left, const node_ptr_t& right);
};

class star_node final : public regex_node {
public:
    node_ptr_t child;
    explicit star_node(const node_ptr_t& child);
};

class plus_node final : public regex_node {
public:
    node_ptr_t child;
    explicit plus_node(const node_ptr_t& child);
};

class alt_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit alt_node(const node_ptr_t& left, const node_ptr_t& right);
};

class regex_tree {
public:
    regex_node::node_ptr_t root;

    using token_map_t = std::unordered_map<token::token_type, std::unordered_set<std::size_t>, token::token_type_hash>;

    token_map_t token_map;

    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> followpos;

    explicit regex_tree(regex_node& root);
    explicit regex_tree(const std::string& s);

    void visit(const std::function<void(regex_node&)>& func) const;
    void print() const;

private:
    static void visit(const regex_node::node_ptr_t& node, const std::function<void(regex_node&)>& func);
    static void print(const regex_node::node_ptr_t& node, int indent = 0);

    struct unordered_set_hash {
        std::size_t operator()(const std::unordered_set<std::size_t>& s) const;
    };

    static token_map_t disjoint_token_sets(const token_map_t& original);
};

} // namespace regex::tree

#pragma region tpp

namespace regex::tree {

template <class T>
inline T& regex_node::as() {
    return static_cast<T&>(*this);
}

} // namespace regex::tree

#pragma endregion

#endif // REGEX_TREE_HPP
