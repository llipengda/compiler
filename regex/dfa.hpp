#pragma once

#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP

#include "token.hpp"
#include "tree.hpp"

#include <cstddef>
#include <unordered_set>

namespace regex::dfa {
class dfa {
public:
    using state_t = std::size_t;
    using token_t = regex::token::token_type;
    using token_t_hash = regex::token::token_type_hash;
    using transition_t = std::unordered_map<token_t, state_t, token_t_hash>;
    using dfa_state_t = std::unordered_map<state_t, transition_t>;

    dfa() = delete;

    explicit dfa(const regex::tree::regex_tree& tree) {
        init(tree);
    }

    explicit dfa(const std::string& regex) {
        tree::regex_tree tree(regex);
#ifdef SHOW_DEBUG
        tree.print();
#endif
        init(tree);
#ifdef SHOW_DEBUG
        this->print();
#endif
    }

    void add_transition(state_t from, const token_t& token, state_t to) {
        transitions[from][token] = to;
    }

    bool match(const std::string& str) const {
        state_t current_state = 1;
        for (const auto& ch : str) {
            bool find = false;
            if (!transitions.count(current_state)) {
                return false;
            }
            if (transitions.at(current_state).count(ch)) {
                current_state = transitions.at(current_state).at(ch);
                continue;
            }
            for (const auto& [token, to] : transitions.at(current_state)) {
                if (token::match(ch, token)) {
                    current_state = to;
                    find = true;
                    break;
                }
            }
            if (!find) {
                return false;
            }
        }
        return accept_states.count(current_state) > 0;
    }

    std::size_t match_max(const std::string& str) const {
        state_t current_state = 1;
        std::size_t last_accept_pos = 0;

        for (size_t i = 0; i < str.size(); ++i) {
            char ch = str[i];
            bool find = false;

            if (!transitions.count(current_state)) {
                break;
            }

            if (transitions.at(current_state).count(ch)) {
                current_state = transitions.at(current_state).at(ch);
                find = true;
            } else {
                for (const auto& [token, to] : transitions.at(current_state)) {
                    if (token::match(ch, token)) {
                        current_state = to;
                        find = true;
                        break;
                    }
                }
            }

            if (!find) {
                break;
            }

            if (accept_states.count(current_state)) {
                last_accept_pos = i + 1;
            }
        }

        return last_accept_pos;
    }

    const dfa_state_t& get_transitions() const {
        return transitions;
    }

    void print() const {
        std::cout << "DFA" << std::endl;
        for (const auto& [from, trans] : transitions) {
            for (const auto& [token, to] : trans) {
                std::cout << "  Transition: " << from << " -- " << token << " --> " << to << std::endl;
            }
        }
        std::cout << "  Accept states: ";
        for (const auto& state : accept_states) {
            std::cout << state << " ";
        }
        std::cout << std::endl;
    }

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

    void init(const tree::regex_tree& tree) {
        std::unordered_set<d_state_t, d_state_t_hash> d_states;
        size_t cur = 1;
        d_states.insert({std::unordered_set<state_t>(tree.root->firstpos.begin(), tree.root->firstpos.end()), cur++});

        std::unordered_set<d_state_t, d_state_t_hash> unmarked_d_states = d_states;

        std::unordered_set<token_t, token_t_hash> all_tokens;
        for (const auto& token : tree.token_map) {
            all_tokens.insert(token.first);
        }

        auto& token_map = tree.token_map;
        auto& followpos = tree.followpos;

        while (!unmarked_d_states.empty()) {
            auto d_state = *unmarked_d_states.begin();
            unmarked_d_states.erase(unmarked_d_states.begin());

            for (const auto& token : all_tokens) {
                if (token::is(token, token::symbol::end_mark)) {
                    continue;
                }

                d_state_t u;
                for (auto i : token_map.at(token)) {
                    if (d_state.states.count(i)) {
                        u.states.insert(followpos.at(i).begin(), followpos.at(i).end());
                    }
                }

                if (u.states.empty()) {
                    continue;
                }

                auto it = d_states.find(u);
                if (it == d_states.end()) {
                    u.id = cur++;
                    d_states.insert(u);
                    unmarked_d_states.insert(u);
                } else {
                    u.id = it->id;
                }

                if (u.states.count(*token_map.at(token::symbol::end_mark).begin())) {
                    accept_states.insert(u.id);
                }

                add_transition(d_state.id, token, u.id);
            }
        }
    }
};
} // namespace regex::dfa

#endif // REGEX_DFA_HPP