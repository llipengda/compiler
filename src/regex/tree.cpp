#include "regex/tree.hpp"
#include "regex/exception.hpp"

#include <stack>
#include <utility>

namespace regex::tree {

char_node::char_node(token::token_type ch, const std::size_t number)
    : value(std::move(ch)), number(number) {
    type = type::ch;
    nullable = false;
    firstpos.insert(number);
    lastpos.insert(number);
}

concat_node::concat_node(const node_ptr_t& left, const node_ptr_t& right)
    : left(left), right(right) {
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

star_node::star_node(const node_ptr_t& child)
    : child(child) {
    type = type::star;
    nullable = true;
    firstpos = child->firstpos;
    lastpos = child->lastpos;
}

plus_node::plus_node(const node_ptr_t& child)
    : child(child) {
    type = type::plus;
    nullable = child->nullable;
    firstpos = child->firstpos;
    lastpos = child->lastpos;
}

alt_node::alt_node(const node_ptr_t& left, const node_ptr_t& right)
    : left(left), right(right) {
    type = type::alt;
    nullable = left->nullable || right->nullable;
    firstpos = left->firstpos;
    firstpos.insert(right->firstpos.begin(), right->firstpos.end());
    lastpos = left->lastpos;
    lastpos.insert(right->lastpos.begin(), right->lastpos.end());
}

regex_tree::regex_tree(regex_node& root)
    : root(&root) {}

regex_tree::regex_tree(const std::string& s) {
    if (s.empty()) {
        return;
    }

    const auto ss = token::split(s);
#ifdef DEBUG
    ::regex::token::print(ss);
#endif
    const auto postfix = token::to_postfix(ss);
#ifdef DEBUG
    ::regex::token::print(postfix);
#endif

    std::stack<regex_node::node_ptr_t> st;

    size_t i = 1;

    using token::op;

    for (const auto& ch : postfix) {
        if (token::is(ch, op::star)) {
            if (st.empty()) {
                throw regex::invalid_regex_exception("'*' operator with empty stack");
            }
            auto operand = st.top();
            st.pop();
            st.push(std::make_shared<star_node>(operand));
        } else if (token::is(ch, op::plus)) {
            if (st.empty()) {
                throw regex::invalid_regex_exception("'+' operator with empty stack");
            }
            auto operand = st.top();
            st.pop();
            st.push(std::make_shared<plus_node>(operand));
        } else if (token::is(ch, op::concat)) {
            if (st.size() < 2) {
                throw regex::invalid_regex_exception("'·' operator with fewer than 2 operands");
            }
            auto right = st.top();
            st.pop();
            auto left = st.top();
            st.pop();
            st.push(std::make_shared<concat_node>(left, right));
        } else if (token::is(ch, op::alt)) {
            if (st.size() < 2) {
                throw regex::invalid_regex_exception("'|' operator with fewer than 2 operands");
            }
            auto right = st.top();
            st.pop();
            auto left = st.top();
            st.pop();
            st.push(std::make_shared<alt_node>(left, right));
        } else if (token::is_nonop(ch)) {
            token_map[ch].insert(i);
            st.push(std::make_shared<char_node>(ch, i++));
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
            for (const auto& concat_node = node.as<tree::concat_node>(); auto idx : concat_node.left->lastpos) {
                followpos[idx].insert(concat_node.right->firstpos.begin(), concat_node.right->firstpos.end());
            }
        } else if (node.type == regex_node::type::star) {
            for (auto& star_node = node.as<tree::star_node>(); auto idx : star_node.lastpos) {
                followpos[idx].insert(star_node.firstpos.begin(), star_node.firstpos.end());
            }
        } else if (node.type == regex_node::type::plus) {
            for (auto& plus_node = node.as<tree::plus_node>(); auto idx : plus_node.lastpos) {
                followpos[idx].insert(plus_node.firstpos.begin(), plus_node.firstpos.end());
            }
        }
    });

    token_map = disjoint_token_sets(token_map);
}

void regex_tree::visit(const std::function<void(regex_node&)>& func) const {
    visit(root, func);
}

void regex_tree::print() const {
    std::cout << "Regex Tree" << std::endl;
    print(root);
}

void regex_tree::visit(const regex_node::node_ptr_t& node, const std::function<void(regex_node&)>& func) {
    if (node == nullptr) {
        return;
    }

    func(*node.get());

    if (node->type == regex_node::type::concat) {
        const auto& concat_node = node->as<tree::concat_node>();
        visit(concat_node.left, func);
        visit(concat_node.right, func);
    } else if (node->type == regex_node::type::alt) {
        const auto& alt_node = node->as<tree::alt_node>();
        visit(alt_node.left, func);
        visit(alt_node.right, func);
    } else if (node->type == regex_node::type::star) {
        const auto& star_node = node->as<tree::star_node>();
        visit(star_node.child, func);
    } else if (node->type == regex_node::type::plus) {
        const auto& plus_node = node->as<tree::plus_node>();
        visit(plus_node.child, func);
    }
}

void regex_tree::print(const regex_node::node_ptr_t& node, int indent) {
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

std::size_t regex_tree::unordered_set_hash::operator()(const std::unordered_set<std::size_t>& s) const {
    std::size_t h = 0;
    for (const auto v : s) {
        h ^= std::hash<std::size_t>{}(v);
    }
    return h;
}

regex_tree::token_map_t regex_tree::disjoint_token_sets(const token_map_t& original) {
    std::unordered_map<char, std::unordered_set<size_t>> char_to_positions;

    for (const auto& [token, positions] : original) {
        if (token::is(token, token::symbol::end_mark)) {
            continue;
        }
        if (auto cs = token::to_char_set(token); !cs.is_negative) {
            for (char ch : cs.chars) {
                char_to_positions[ch].insert(positions.begin(), positions.end());
            }
        } else {
            for (int c = -128; c < 128; ++c) {
                if (!cs.chars.contains(static_cast<char>(c))) {
                    char_to_positions[static_cast<char>(c)].insert(positions.begin(), positions.end());
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

} // namespace regex::tree
