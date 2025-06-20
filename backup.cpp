#include "utils.hpp"
#include <iostream>
#include <string_view>
#include <vector>

const auto semantic_grammar = R"(
program -> decls compoundstmt

decls -> decl ; decls | E

decl -> int ID = INTNUM    { ID.addr = var(ID.lexval, "int"); emit(ID.addr, "=", INTNUM.lexval); }
      | real ID = REALNUM  { ID.addr = var(ID.lexval, "real"); emit(ID.addr, "=", REALNUM.lexval); }

stmt -> ifstmt | assgstmt | compoundstmt

compoundstmt -> \{        { enter_block(); } 
                stmts \}  { exit_block(); }

stmts -> stmt stmts | E

ifstmt -> if (             { L1 = label(); L2 = label(); L3 = label(); boolexpr.inh.true = L1; boolexpr.inh.false = L2; }
          boolexpr ) then  { emit("label", L1); } 
          stmt else        { emit("goto", L3); emit("label", L2); }
          stmt             { emit("label", L3); }  

assgstmt -> ID = arithexpr ;  { if (lookup(ID).type != arithexpr.type) 
                                  error(arithexpr.type, " can not be assigned to ", lookup(ID).type); 
                                emit(ID.addr, "=", arithexpr.addr); }

boolexpr -> arithexpr boolop arithexpr  { if (arithexpr<1>.type != arithexpr<2>.type) 
                                            error(arithexpr<1>.type, " can not be compared to ", arithexpr<2>.type); 
                                          boolexpr.type = "bool"; boolexpr.addr = temp();
                                          emit(boolexpr.addr, "=", arithexpr<1>.addr, boolop.op, arithexpr<2>.addr);
                                          emit("if", boolexpr.addr, "goto", boolexpr.inh.true);
                                          emit("goto", boolexpr.inh.false); }

boolop -> <   { boolop.op = "<"; }
        | >   { boolop.op = ">"; }
        | <=  { boolop.op = "<="; }
        | >=  { boolop.op = ">="; }
        | ==  { boolop.op = "=="; }

arithexpr -> multexpr        { arithexprprime.inh.addr = multexpr.addr; arithexprprime.inh.type = multexpr.type; } 
             arithexprprime  { arithexpr.addr = arithexprprime.addr; arithexpr.type = arithexprprime.type; }

arithexprprime -> + multexpr      { if (multexpr.type != arithexprprime<1>.inh.type)
                                      error(multexpr.type, " can not be added to ", arithexprprime<1>.type);
                                    arithexprprime<2>.inh.addr = temp(); 
                                    arithexprprime<2>.inh.type = multexpr.type; 
                                    emit(arithexprprime<2>.inh.addr, "=", multexpr.addr, "+", arithexprprime<1>.addr); }
                  arithexprprime  { arithexprprime<1>.addr = arithexprprime<2>.addr; arithexprprime<1>.type = arithexprprime<2>.type; }
                | - multexpr      { if (multexpr.type != arithexprprime<1>.inh.type)
                                      error(multexpr.type, " can not be subtracted from ", arithexprprime<1>.type);
                                    arithexprprime<2>.inh.addr = temp(); 
                                    arithexprprime<2>.inh.type = multexpr.type; 
                                    emit(arithexprprime<2>.inh.addr, "=", arithexprprime<1>.addr, "-", multexpr.addr); }
                  arithexprprime  { arithexprprime<1>.addr = arithexprprime<2>.addr; arithexprprime<1>.type = arithexprprime<2>.type; }
                | E               { arithexprprime.addr = arithexprprime.inh.addr; arithexprprime.type = arithexprprime.inh.type; }

multexpr -> simpleexpr     { multexprprime.inh.addr = simpleexpr.addr; multexprprime.inh.type = simpleexpr.type; }
            multexprprime  { multexpr.addr = multexprprime.addr; multexpr.type = multexprprime.type; }

multexprprime -> * simpleexpr   { if (simpleexpr.type != multexprprime<1>.inh.type)
                                    error(simpleexpr.type, " can not be multiplied with ", multexprprime<1>.type);
                                 multexprprime<2>.inh.addr = temp(); 
                                 multexprprime<2>.inh.type = simpleexpr.type; 
                                 emit(multexprprime<2>.inh.addr, "=", multexprprime<1>.addr, "*", simpleexpr.addr); } 
                 multexprprime  { multexprprime<1>.addr = multexprprime<2>.addr; multexprprime<1>.type = multexprprime<2>.type; }
               | / simpleexpr   { if (simpleexpr.type != multexprprime<1>.inh.type)
                                    error(simpleexpr.type, " can not be divided by ", multexprprime<1>.type);
                                  multexprprime<2>.inh.addr = temp(); 
                                  multexprprime<2>.inh.type = simpleexpr.type; 
                                  emit(multexprprime<2>.inh.addr, "=", multexprprime<1>.addr, "/", simpleexpr.addr); }
                 multexprprime  { multexprprime<1>.addr = multexprprime<2>.addr; multexprprime<1>.type = multexprprime<2>.type; }
               | E              { multexprprime.addr = multexprprime.inh.addr; multexprprime.type = multexprprime.inh.type; }

simpleexpr -> ID             { simpleexpr.addr = ID.addr; simpleexpr.type = lookup(ID).type; }
            | INTNUM         { simpleexpr.addr = temp(); simpleexpr.type = "int"; emit(simpleexpr.addr, "=", INTNUM.lexval); }
            | REALNUM        { simpleexpr.addr = temp(); simpleexpr.type = "real"; emit(simpleexpr.addr, "=", REALNUM.lexval); }
            | ( arithexpr )  { simpleexpr.addr = arithexpr.addr; simpleexpr.type = arithexpr.type; }
)";

class action {
    std::vector<std::string_view> code;

public:
    action(std::string_view code_str) {
        this->code = utils::split_trim(code_str, ";");
    }

    friend std::ostream& operator<<(std::ostream& os, const action& act) {
        os << "{";
        for (const auto& c : act.code) {
            os << c << ';';
        }
        os << "}";
        return os;
    }
};

class symbol {
    std::string_view lexval;

public:
    symbol(std::string_view lexval) : lexval(lexval) {}

    friend std::ostream& operator<<(std::ostream& os, const symbol& sym) {
        os << sym.lexval;
        return os;
    }
};

class prod_value_t {
    union {
        symbol sym;
        action act;
    };

public:
    bool is_symbol;
    bool is_action;

    prod_value_t(symbol sym) : sym(sym), is_symbol(true), is_action(false) {}
    prod_value_t(action act) : act(std::move(act)), is_symbol(false), is_action(true) {}

    friend std::ostream& operator<<(std::ostream& os, const prod_value_t& val) {
        if (val.is_symbol) {
            os << val.sym;
        } else if (val.is_action) {
            os << val.act;
        }
        return os;
    }

    prod_value_t(const prod_value_t& other) : is_symbol(other.is_symbol), is_action(other.is_action) {
        if (is_action) {
            new (&act) action(other.act);
        } else {
            new (&sym) symbol(other.sym);
        }
    }

    prod_value_t& operator=(const prod_value_t& other) {
        if (this != &other) {
            this->~prod_value_t();
            new (this) prod_value_t(other);
        }
        return *this;
    }

    ~prod_value_t() {
        if (is_action) {
            act.~action();
        } else {
            sym.~symbol();
        }
    }
};

class prod {
    std::string_view lhs;
    std::vector<prod_value_t> rhs;

public:
    prod(std::string_view lhs, std::vector<prod_value_t> rhs) : lhs(lhs), rhs(std::move(rhs)) {}

    friend std::ostream& operator<<(std::ostream& os, const prod& p) {
        os << p.lhs << " -> ";
        for (const auto& val : p.rhs) {
            os << val << ' ';
        }
        return os;
    }
};

// 优化后的解析函数
std::vector<prod_value_t> parse_rhs(std::string_view rhs_str) {
    std::vector<prod_value_t> rhs_values;
    size_t pos = 0;

    while (pos < rhs_str.size()) {
        // 跳过空白字符
        while (pos < rhs_str.size() && std::isspace(rhs_str[pos])) {
            pos++;
        }

        if (pos >= rhs_str.size()) break;

        if (rhs_str[pos] == '{') {
            // 找到 action 块的结束位置
            size_t start = pos + 1;
            size_t end = start;
            int brace_count = 1;

            while (end < rhs_str.size() && brace_count > 0) {
                if (rhs_str[end] == '{') {
                    brace_count++;
                } else if (rhs_str[end] == '}') {
                    brace_count--;
                }
                if (brace_count > 0) {
                    end++;
                }
            }

            if (brace_count == 0) {
                // 提取 {} 内的完整内容，不需要重新合并
                std::string_view action_content = rhs_str.substr(start, end - start);
                rhs_values.emplace_back(action(action_content));
                pos = end + 1; // 跳过 '}'
            } else {
                // 未匹配的大括号，处理错误
                break;
            }
        } else {
            // 解析 symbol
            size_t start = pos;

            // 处理转义字符
            if (pos + 1 < rhs_str.size() && rhs_str[pos] == '\\' && (rhs_str[pos + 1] == '{' || rhs_str[pos + 1] == '}')) {
                pos += 2;
            } else {
                // 找到下一个空白字符或特殊字符
                while (pos < rhs_str.size() && !std::isspace(rhs_str[pos]) && rhs_str[pos] != '{' && rhs_str[pos] != '}') {
                    pos++;
                }
            }

            std::string_view token = rhs_str.substr(start, pos - start);

            // 处理转义
            if (token == "\\{") {
                // 创建临时字符串处理转义，或者使用其他方式
                static std::string escaped_brace = "{";
                rhs_values.emplace_back(symbol(escaped_brace));
            } else if (token == "\\}") {
                static std::string escaped_brace = "}";
                rhs_values.emplace_back(symbol(escaped_brace));
            } else if (!token.empty()) {
                rhs_values.emplace_back(symbol(token));
            }
        }
    }

    return rhs_values;
}

int main() {
    auto res = utils::split_trim(std::string_view{semantic_grammar}, "\n\n");
    std::vector<prod> prods;

    for (const auto& prod_str : res) {
        auto parts = utils::split_trim(prod_str, " -> ");
        if (parts.size() < 2) continue;

        auto lhs = parts[0];
        auto& rhs = parts[1];

        for (auto r : utils::split_trim(rhs, "|")) {
            if (r.empty()) {
                continue;
            }

            // 使用优化后的解析函数
            auto rhs_values = parse_rhs(r);
            if (!rhs_values.empty()) {
                prods.emplace_back(lhs, std::move(rhs_values));
            }
        }
    }

    for (const auto& prod : prods) {
        std::cout << prod << std::endl;
    }
}