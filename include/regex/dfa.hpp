#pragma once

#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP

#include "token.hpp"
#include "tree.hpp"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>


namespace regex::dfa {

class dfa {
public:
    using state_t = std::size_t;
    using token_t = token::token_type;
    using token_t_hash = token::token_type_hash;
    using transition_t = std::unordered_map<token_t, state_t, token_t_hash>;
    using dfa_state_t = std::unordered_map<state_t, transition_t>;

    dfa() = delete;

    explicit dfa(const tree::regex_tree& tree);
    explicit dfa(const std::string& regex);

    void add_transition(state_t from, const token_t& token, state_t to);
    bool match(const std::string& str) const;
    std::size_t match_max(const std::string& str) const;

    const dfa_state_t& get_transitions() const;
    void print() const;

private:
    dfa_state_t transitions;
    std::unordered_set<state_t> accept_states;

    struct d_state_t {
        std::unordered_set<state_t> states;
        state_t id;
        bool operator==(const d_state_t& other) const {
            return states == other.states;
        }
    };

    struct d_state_t_hash {
        std::size_t operator()(const d_state_t& s) const {
            std::size_t h = 0;
            for (auto v : s.states) {
                h ^= std::hash<state_t>{}(v);
            }
            return h;
        }
    };

    void init(const tree::regex_tree& tree);
};

} // namespace regex::dfa

#endif // REGEX_DFA_HPP
