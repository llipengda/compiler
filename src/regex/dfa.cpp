#include "regex/dfa.hpp"

#include <iostream>
#include <ranges>
#include <unordered_map>

namespace regex::dfa {

dfa::dfa(const tree::regex_tree& tree) {
    init(tree);
}

dfa::dfa(const std::string& regex) {
    const tree::regex_tree tree(regex);
#ifdef DEBUG
    tree.print();
#endif
    init(tree);
#ifdef DEBUG
    this->print();
#endif
}

void dfa::add_transition(const state_t from, const token_t& token, const state_t to) {
    transitions[from][token] = to;
}

bool dfa::match(const std::string& str) const {
    state_t current_state = 1;
    for (const auto& ch : str) {
        bool find = false;
        if (!transitions.contains(current_state)) {
            return false;
        }
        if (transitions.at(current_state).contains(ch)) {
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
    return accept_states.contains(current_state);
}

std::size_t dfa::match_max(const std::string& str) const {
    state_t current_state = 1;
    std::size_t last_accept_pos = 0;

    for (size_t i = 0; i < str.size(); ++i) {
        char ch = str[i];
        bool find = false;

        if (!transitions.contains(current_state)) {
            break;
        }

        if (transitions.at(current_state).contains(ch)) {
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

        if (accept_states.contains(current_state)) {
            last_accept_pos = i + 1;
        }
    }

    return last_accept_pos;
}

const dfa::dfa_state_t& dfa::get_transitions() const {
    return transitions;
}

void dfa::print() const {
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

void dfa::init(const tree::regex_tree& tree) {
    if (!tree.root) {
        accept_states.insert(1);
        return;
    }


    std::unordered_set<d_state_t, d_state_t_hash> d_states;
    size_t cur = 1;
    d_states.insert({std::unordered_set(tree.root->firstpos.begin(), tree.root->firstpos.end()), cur++});
    std::unordered_set<d_state_t, d_state_t_hash> unmarked_d_states = d_states;

    std::unordered_set<token_t, token_t_hash> all_tokens;
    for (const auto& key : tree.token_map | std::views::keys) {
        all_tokens.insert(key);
    }

    auto& token_map = tree.token_map;
    auto& followpos = tree.followpos;

    while (!unmarked_d_states.empty()) {
        auto [states, id] = *unmarked_d_states.begin();
        unmarked_d_states.erase(unmarked_d_states.begin());

        for (const auto& token : all_tokens) {
            if (token::is(token, token::symbol::end_mark)) {
                continue;
            }

            d_state_t u;
            for (auto i : token_map.at(token)) {
                if (states.contains(i)) {
                    u.states.insert(followpos.at(i).begin(), followpos.at(i).end());
                }
            }

            if (u.states.empty()) {
                continue;
            }

            if (auto it = d_states.find(u); it == d_states.end()) {
                u.id = cur++;
                d_states.insert(u);
                unmarked_d_states.insert(u);
            } else {
                u.id = it->id;
            }

            if (u.states.contains(*token_map.at(token::symbol::end_mark).begin())) {
                accept_states.insert(u.id);
            }

            add_transition(id, token, u.id);
        }
    }
}

} // namespace regex::dfa
