#include "grammar/production.hpp"

namespace grammar::production {

std::string symbol::epsilon_str = "ε";
std::string symbol::end_mark_str = "$";
std::function<bool(const std::string&)> symbol::terminal_rule = [](const std::string& str) {
    return !std::isupper(str[0]);
};
symbol symbol::epsilon = symbol(epsilon_str);
symbol symbol::end_mark = symbol(end_mark_str);

symbol::symbol() {
    type = type::epsilon;
    name = std::string(epsilon_str);
    lexval = std::string(epsilon_str);
}

symbol::symbol(const std::string& str) {
    const auto trimed = trim(str);

    if (str == epsilon_str) {
        type = type::epsilon;
    } else if (trimed == end_mark_str) {
        type = type::end_mark;
    } else if (terminal_rule(trimed)) {
        type = type::terminal;
    } else {
        type = type::non_terminal;
    }

    name = trimed;
    lexval = trimed;
}

symbol::symbol(const lexer::token& token) : symbol(std::string(token)) {
    update(token);
}

void symbol::update(const lexer::token& token) {
    this->lexval = token.value;
    this->line = token.line;
    this->column = token.column;
}

void symbol::update(const symbol& other) {
    this->lexval = other.lexval;
    this->line = other.line;
    this->column = other.column;
}

void symbol::update_pos(const symbol& other) {
    this->line = other.line;
    this->column = other.column;
}

std::string symbol::to_string() const {
    return name;
}

bool symbol::is_terminal() const {
    return type == type::terminal || type == type::epsilon;
}

bool symbol::is_non_terminal() const {
    return type == type::non_terminal;
}

bool symbol::is_epsilon() const {
    return type == type::epsilon;
}

bool symbol::is_end_mark() const {
    return type == type::end_mark;
}

bool symbol::operator==(const symbol& other) const {
    return type == other.type && name == other.name;
}

bool symbol::operator<(const symbol& other) const {
    if (type != other.type) {
        return static_cast<int>(type) < static_cast<int>(other.type);
    }
    return name < other.name;
}

bool symbol::operator!=(const symbol& other) const {
    return !(*this == other);
}

std::string symbol::trim(const std::string& str) {
    std::size_t start = str.find_first_not_of(" \t\n\r");
    std::size_t end = str.find_last_not_of(" \t\n\r");
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    } else {
        return str.substr(start, end - start + 1);
    }
}

std::ostream& operator<<(std::ostream& os, const symbol& sym) {
    os << sym.name;
    return os;
}

production::production(const std::string& str) {
    std::size_t pos = str.find("->");
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid production format: " + str);
    }

    lhs = symbol(str.substr(0, pos));
    std::string rhs_str = str.substr(pos + 2);

    std::size_t start = 0;
    std::size_t end = 0;
    while ((end = rhs_str.find(' ', start)) != std::string::npos) {
        auto s = rhs_str.substr(start, end - start);
        if (s.empty()) {
            start = end + 1;
            continue;
        }
        rhs.emplace_back(s);
        start = end + 1;
    }
    if (auto remain = rhs_str.substr(start); !remain.empty()) {
        rhs.emplace_back(remain);
    }
}

std::vector<production> production::parse(const std::string& str) {
    std::vector<production> productions;
    std::size_t pos = 0;
    while (pos < str.size()) {
        auto end_pos = str.find('\n', pos);
        if (end_pos == std::string::npos) {
            end_pos = str.size();
        }
        auto prod_str = str.substr(pos, end_pos - pos);
        // process | to multiple productions
        auto pipe_pos = prod_str.find('|');
        std::string lhs_str;
        while (pipe_pos != std::string::npos) {
            productions.emplace_back(lhs_str + prod_str.substr(0, pipe_pos));
            if (lhs_str.empty()) {
                lhs_str = productions.rbegin()->lhs.name + " -> ";
            }
            prod_str = prod_str.substr(pipe_pos + 1);
            pipe_pos = prod_str.find('|');
        }
        if (!prod_str.empty()) {
            productions.emplace_back(lhs_str + prod_str);
        }
        pos = end_pos + 1;
    }
    return productions;
}

std::string production::to_string() const {
    std::string result = lhs.name + " -> ";
    for (const auto& sym : rhs) {
        result += sym.name + " ";
    }
    return result;
}

bool production::operator==(const production& other) const {
    return lhs == other.lhs && rhs == other.rhs;
}

std::ostream& operator<<(std::ostream& os, const production& prod) {
    os << prod.to_string();
    return os;
}

LR_production::LR_production(const std::string& str) : production(str) {
    dot_pos = 0;
}

LR_production::LR_production(const production& prod) : production(prod) {
    dot_pos = 0;
    if (rhs.size() == 1 && rhs[0].is_epsilon()) {
        rhs.clear();
    }
}

bool LR_production::is_start() const {
    return dot_pos == 0;
}

bool LR_production::is_end() const {
    return dot_pos == rhs.size();
}

bool LR_production::is_rhs_empty() const {
    return rhs.empty();
}

LR_production LR_production::next() const {
    LR_production next = *this;
    if (dot_pos < rhs.size()) {
        ++next.dot_pos;
    }
    return next;
}

const symbol& LR_production::symbol_after_dot() const {
    if (dot_pos < rhs.size()) {
        return rhs[dot_pos];
    }
    throw std::out_of_range("No symbol after dot");
}

std::string LR_production::to_string() const {
    std::string result = lhs.name + " -> ";
    for (std::size_t i = 0; i < rhs.size(); ++i) {
        if (i == dot_pos) {
            result += "· ";
        }
        result += rhs[i].name + " ";
    }
    if (dot_pos == rhs.size()) {
        result += "·";
    }
    return result;
}

bool LR_production::operator==(const LR_production& other) const {
    return production::operator==(other) && dot_pos == other.dot_pos;
}

bool LR_production::operator==(const production& other) const {
    if (lhs != other.lhs) {
        return false;
    }
    if (rhs == other.rhs) {
        return true;
    }
    if (rhs.empty() && other.rhs.size() == 1 && other.rhs[0].is_epsilon()) {
        return true;
    }
    return false;
}

LR1_production::LR1_production(const production& prod) {
    throw std::invalid_argument("LR1_production requires a lookahead symbol");
}

LR1_production::LR1_production(const production& prod, symbol&& lookahead)
    : LR_production(prod), lookahead(std::move(lookahead)) {}

LR1_production::LR1_production(const production& prod, const symbol& lookahead)
    : LR_production(prod), lookahead(lookahead) {}

void LR1_production::set_lookahead(const symbol& lookahead) {
    this->lookahead = lookahead;
}

LR1_production LR1_production::next() const {
    LR1_production next = *this;
    if (dot_pos < rhs.size()) {
        ++next.dot_pos;
    }
    return next;
}

std::string LR1_production::to_string() const {
    return LR_production::to_string() + ", " + lookahead.name;
}

bool LR1_production::operator==(const LR1_production& other) const {
    return LR_production::operator==(other) && lookahead == other.lookahead;
}

bool LR1_production::operator==(const production& other) const {
    return LR_production::operator==(other);
}

} // namespace grammar::production

namespace grammar {

void set_epsilon_str(const std::string& str) {
    using namespace grammar::production;
    symbol::epsilon_str = str;
    symbol::epsilon = symbol(symbol::epsilon_str);
}

void set_end_mark_str(const std::string& str) {
    using namespace grammar::production;
    symbol::end_mark_str = str;
    symbol::end_mark = symbol(symbol::end_mark_str);
}

void set_terminal_rule(const std::function<bool(const std::string&)>& rule) {
    using namespace grammar::production;
    symbol::terminal_rule = rule;
}

void set_terminal_rule(std::function<bool(const std::string&)>&& rule) {
    using namespace grammar::production;
    symbol::terminal_rule = std::move(rule);
}

} // namespace grammar