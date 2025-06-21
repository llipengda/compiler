#include "grammar/LL1.hpp"
#include "grammar/exception.hpp"

#include <algorithm>
#include <iomanip>
#include <ranges>
#include <stack>

namespace grammar {

LL1::LL1(const std::vector<production::production>& productions) {
    this->productions = productions;

    for (std::size_t i = 0; i < productions.size(); ++i) {
        const auto& prod = productions[i];
        symbol_map[prod.lhs].push_back(i);
    }
}

LL1::LL1(const std::string& str) {
    productions = production::production::parse(str);
    for (std::size_t i = 0; i < productions.size(); ++i) {
        const auto& prod = productions[i];
        symbol_map[prod.lhs].push_back(i);
    }
}

void LL1::build() {
    calc_first();
    calc_follow();
    build_parsing_table();
}

void LL1::parse(const std::vector<lexer::token>& input) {
    auto in = input;
    in.emplace_back(production::symbol::end_mark_str);

    std::stack<production::symbol> stack;
    stack.push(production::symbol::end_mark);
    stack.push(productions[0].lhs);

    std::size_t pos = 0;

    while (pos < in.size() || !stack.empty()) {
        auto cur_input = production::symbol{in[pos]};
        auto& top = stack.top();

        auto get_pos = [&]() {
            return "at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column);
        };

        if (top.is_terminal() || top.is_end_mark()) {
            if (top == cur_input) {
                pos++;
                stack.pop();
                tree_->update(cur_input);
            } else {
                stack.pop();
                std::cerr << "Expect: " << top.name << " but got: " << cur_input.name << ' ' << get_pos() << '\n';
            }
        } else {
            const auto& table = parsing_table.at(top);
            if (!table.contains(cur_input)) {
                if (first.at(top).contains(production::symbol::epsilon)) {
                    stack.pop();
                    tree_->add(production::production(top.name + " -> " + production::symbol::epsilon_str));
                } else if (!follow.at(top).contains(cur_input)) {
                    pos++;
                } else {
                    throw exception::grammar_error("Unexpected token: " + cur_input.name + ' ' + get_pos());
                }
                std::cerr << "Unexpected token: " << cur_input.name << ' ' << get_pos() << '\n';
                continue;
            }
            stack.pop();
            const auto& prod = table.at(cur_input);
            tree_->add(prod);
            const auto& symbols = prod.rhs;

            if (symbols.size() == 1 && symbols[0].is_epsilon()) {
                continue;
            }

            for (const auto& symbol : std::ranges::reverse_view(symbols)) {
                stack.push(symbol);
            }
        }
    }
}

void LL1::print_first() const {
    std::cout << "FIRST sets:\n";
    for (const auto& [sym, first_set] : first) {
        if (sym.is_terminal()) {
            continue;
        }
        std::cout << "FIRST(" << sym << ") = {";
        for (auto it = first_set.begin(); it != first_set.end(); ++it) {
            std::cout << *it;
            if (std::next(it) != first_set.end()) {
                std::cout << ',';
            }
        }
        std::cout << "}\n";
    }
    std::cout << '\n';
}

void LL1::print_follow() const {
    std::cout << "FOLLOW sets:\n";
    for (const auto& [sym, follow_set] : follow) {
        std::cout << "FOLLOW(" << sym << ") = {";
        for (auto it = follow_set.begin(); it != follow_set.end(); ++it) {
            std::cout << *it;
            if (std::next(it) != follow_set.end()) {
                std::cout << ',';
            }
        }
        std::cout << "}\n";
    }
    std::cout << '\n';
}

void LL1::print_productions() const {
    std::cout << "Productions:\n";
    for (const auto& prod : productions) {
        std::cout << prod << '\n';
    }
    std::cout << '\n';
}

void LL1::print_parsing_table() const {
    std::cout << "Parsing Table:\n";
    for (const auto& [nt, table] : parsing_table) {
        for (const auto& [t, prod] : table) {
            std::cout << "M[" << nt << "," << t << "] = " << prod << '\n';
        }
    }
}

void LL1::print_parsing_table_pretty() const {
    const auto longest_prod_len =
        std::ranges::max_element(productions,
                                 [](const auto& a, const auto& b) {
                                     return a.to_string().size() < b.to_string().size();
                                 })
            ->to_string()
            .size();
    const auto longest_non_terminal_sym_len =
        std::ranges::max_element(follow,
                                 [](const auto& a, const auto& b) {
                                     return a.first.name.size() < b.first.name.size();
                                 })
            ->first.name.size();
    symbol_set terminals;
    symbol_set non_terminals;
    for (const auto& sym : first | std::views::keys) {
        if ((sym.is_terminal() || sym.is_end_mark()) && !sym.is_epsilon()) {
            terminals.insert(sym);
        }
        if (sym.is_non_terminal()) {
            non_terminals.insert(sym);
        }
    }
    auto width = longest_non_terminal_sym_len + longest_prod_len * terminals.size() + (terminals.size() + 2);
    auto line = std::string(width, '-');
    std::cout << "Parsing Table:\n"
              << line << '\n';
    std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << " " << '|';
    for (const auto& t : terminals) {
        std::cout << std::left << std::setw(longest_prod_len) << t << '|';
    }
    std::cout << '\n'
              << line << '\n';
    for (const auto& nt : non_terminals) {
        std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << nt << '|';
        for (const auto& t : terminals) {
            if (parsing_table.at(nt).contains(t)) {
                const auto& prod = parsing_table.at(nt).at(t);
                std::cout << std::left << std::setw(longest_prod_len) << prod << '|';
            } else {
                std::cout << std::left << std::setw(longest_prod_len) << " " << '|';
            }
        }
        std::cout << '\n'
                  << line << '\n';
    }
}

void LL1::build_parsing_table() {
    for (const auto& prod : productions) {
        auto first_set = calc_first(prod);
        for (const auto& sym : first_set) {
            if (sym.is_terminal() && !sym.is_epsilon()) {
                if (parsing_table[prod.lhs].contains(sym)) {
                    throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                }
                parsing_table[prod.lhs][sym] = prod;
            }
        }
        if (first_set.contains(production::symbol::epsilon)) {
            for (auto follow_set = follow[prod.lhs]; const auto& sym : follow_set) {
                if (sym.is_terminal() || sym.is_end_mark()) {
                    if (parsing_table[prod.lhs].contains(sym)) {
                        throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                    }
                    parsing_table[prod.lhs][sym] = prod;
                }
            }
        }
    }
}

} // namespace grammar