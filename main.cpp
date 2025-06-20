#define USE_STD_REGEX
#define SEMA_PROD_USE_INITIALIZER_LIST

#include "grammar/production.hpp"
#include "lexer/lexer.hpp"
#include "semantic/sema_grammar.hpp"
#include <memory>

enum class token_type {
    UNKNOWN = 0,
    INT,
    REAL,
    IF,
    THEN,
    ELSE,
    LPAR,
    RPAR,
    SEMI,
    LBRACE,
    RBRACE,
    PLUS,
    MINUS,
    MULT,
    DIV,
    LT,
    LE,
    GT,
    GE,
    EQ,
    ASSIGN,
    ID,
    INTNUM,
    REALNUM,
    WHITESPACE
};

const lexer::lexer::input_keywords_t<token_type> keywords = {
    {"int", token_type::INT, "int"},
    {"real", token_type::REAL, "real"},
    {"if", token_type::IF, "if"},
    {"then", token_type::THEN, "then"},
    {"else", token_type::ELSE, "else"},
    {"\\(", token_type::LPAR, "("},
    {"\\)", token_type::RPAR, ")"},
    {";", token_type::SEMI, ";"},
    {"\\{", token_type::LBRACE, "{"},
    {"\\}", token_type::RBRACE, "}"},
    {"\\+", token_type::PLUS, "+"},
    {"-", token_type::MINUS, "-"},
    {"\\*", token_type::MULT, "*"},
    {"/", token_type::DIV, "/"},
    {"<", token_type::LT, "<"},
    {"<=", token_type::LE, "<="},
    {">", token_type::GT, ">"},
    {">=", token_type::GE, ">="},
    {"==", token_type::EQ, "=="},
    {"=", token_type::ASSIGN, " ="},
    {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
    {"[0-9]+", token_type::INTNUM, "INTNUM"},
    {"[0-9]+\\.[0-9]*", token_type::REALNUM, "REALNUM"},
    {"[ \t\n]+", token_type::WHITESPACE, "WHITESPACE"}};

const std::unordered_set<std::string> terminals{
    "int", "real", "if", "then", "else", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "=", "ID", "INTNUM", "REALNUM"};
std::vector<semantic::sema_production> build_grammar() {
    grammar::set_epsilon_str("E");
    grammar::set_end_mark_str("$");
    grammar::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });
#ifdef SEMA_PROD_USE_INITIALIZER_LIST
    const std::vector<semantic::sema_production> prods{
        {"program", {"decls", "compoundstmt"}},
        {"decls", {"decl", ";", "decls"}},
        {"decls", {"E"}},
        {"decl", {"int", "ID", "=", "INTNUM", ACT(GET(ID); GET(INTNUM); env.table.insert(ID.lexval, {{"type", "int"}, {"value", INTNUM.lexval}});)}},
        {"decl", {"real", "ID", "=", "REALNUM", ACT(GET(ID); GET(REALNUM); env.table.insert(ID.lexval, {{"type", "real"}, {"value", REALNUM.lexval}});)}},
        {"stmt", {"ifstmt"}},
        {"stmt", {"assgstmt"}},
        {"stmt", {"compoundstmt"}},
        {"compoundstmt", {"{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)}},
        {"stmts", {"stmt", "stmts"}},
        {"stmts", {"E"}},
        {"ifstmt", {"if", "(", "boolexpr", ")", "then", ACT(env.table.enter_scope_copy();), "stmt", ACT(GET(stmt); env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt.inh[key] = value.at("value"); }); env.table.exit_scope();), "else", ACT(env.table.enter_scope_copy();), "stmt", ACT(GETI(stmt, 1); env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt_1.inh[key] = value.at("value"); }); env.table.exit_scope();), ACT(GETI(stmt, 1); GET(stmt); GET(boolexpr); if (boolexpr.syn["val"] == "true") {
                for (auto& [key, value] : stmt.inh) {
                    (*env.table.lookup(key))["value"] = value;
                } } else {
                for (auto& [key, value] : stmt_1.inh) {
                    (*env.table.lookup(key))["value"] = value;
                } })}},
        {"assgstmt", {"ID", "=", "arithexpr", ";", ACT(GET(ID); GET(arithexpr); auto table_ID = env.table.lookup(ID.lexval); if (table_ID == nullptr) { env.error(ID.lexval + " is not defined"); return; }(*table_ID)["value"] = arithexpr.syn["val"];)}},
        {"boolexpr", {"arithexpr", "boolop", "arithexpr", ACT(GET(boolexpr); GET(boolop); GET(arithexpr); GETI(arithexpr, 1); auto lhs = std::stod(arithexpr.syn["val"]); auto rhs = std::stod(arithexpr_1.syn["val"]); if (boolop.syn["op"] == "<") { boolexpr.syn["val"] = lhs < rhs ? "true" : "false"; } if (boolop.syn["op"] == ">") { boolexpr.syn["val"] = lhs > rhs ? "true" : "false"; } if (boolop.syn["op"] == "==") { boolexpr.syn["val"] = lhs == rhs ? "true" : "false"; } if (boolop.syn["op"] == "<=") { boolexpr.syn["val"] = lhs <= rhs ? "true" : "false"; } if (boolop.syn["op"] == ">=") { boolexpr.syn["val"] = lhs >= rhs ? "true" : "false"; })}},
        {"boolop", {"<", ACT(GET(boolop); boolop.syn["op"] = "<";)}},
        {"boolop", {">", ACT(GET(boolop); boolop.syn["op"] = ">";)}},
        {"boolop", {"<=", ACT(GET(boolop); boolop.syn["op"] = "<=";)}},
        {"boolop", {">=", ACT(GET(boolop); boolop.syn["op"] = ">=";)}},
        {"boolop", {"==", ACT(GET(boolop); boolop.syn["op"] = "==";)}},
        {"arithexpr", {"multexpr", ACT(GET(arithexprprime); GET(multexpr); arithexprprime.inh["val"] = multexpr.syn["val"]; arithexprprime.inh["type"] = multexpr.syn["type"];), "arithexprprime", ACT(GET(arithexpr); GET(arithexprprime); arithexpr.syn["val"] = arithexprprime.syn["val"]; arithexpr.syn["type"] = arithexprprime.syn["type"];)}},
        {"arithexprprime", {"+", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(multexpr.syn["val"]) + std::stod(arithexprprime.inh["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)}},
        {"arithexprprime", {"-", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(arithexprprime.inh["val"]) - std::stod(multexpr.syn["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)}},
        {"arithexprprime", {"E", ACT(GET(arithexprprime); arithexprprime.syn["val"] = arithexprprime.inh["val"]; arithexprprime.syn["type"] = arithexprprime.inh["type"];)}},
        {"multexpr", {"simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); multexprprime.inh["val"] = simpleexpr.syn["val"]; multexprprime.inh["type"] = simpleexpr.syn["type"];), "multexprprime", ACT(GET(multexpr); GET(multexprprime); multexpr.syn["val"] = multexprprime.syn["val"]; multexpr.syn["type"] = multexprprime.syn["type"];)}},
        {"multexprprime", {"*", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) * std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)}},
        {"multexprprime", {"/", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); if (std::stod(simpleexpr.syn["val"]) == 0) {
                env.error("line " + std::to_string(simpleexpr.line) + ",division by zero");
                return; } multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) / std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)}},
        {"multexprprime", {"E", ACT(GET(multexprprime); multexprprime.syn["val"] = multexprprime.inh["val"]; multexprprime.syn["type"] = multexprprime.inh["type"];)}},
        {"simpleexpr", {"ID", ACT(GET(simpleexpr); GET(ID); simpleexpr.syn["val"] = env.table.lookup(ID.lexval)->at("value"); simpleexpr.syn["type"] = env.table.lookup(ID.lexval)->at("type");)}},
        {"simpleexpr", {"INTNUM", ACT(GET(simpleexpr); GET(INTNUM); simpleexpr.syn["val"] = INTNUM.lexval; simpleexpr.syn["type"] = "int"; simpleexpr.update_pos(INTNUM);)}},
        {"simpleexpr", {"REALNUM", ACT(GET(simpleexpr); GET(REALNUM); simpleexpr.syn["val"] = REALNUM.lexval; simpleexpr.syn["type"] = "real";)}},
        {"simpleexpr", {"(", "arithexpr", ")", ACT(GET(simpleexpr); GET(arithexpr); simpleexpr.syn["val"] = arithexpr.syn["val"]; simpleexpr.syn["type"] = arithexpr.syn["type"];)}},
    };
#else
    const std::vector<semantic::sema_production> prods{
        {"program", "decls", "compoundstmt"},
        {"decls", "decl", ";", "decls"},
        {"decls", "E"},
        {"decl", "int", "ID", "=", "INTNUM",
         ACT(GET(ID); GET(INTNUM);
             env.table.insert(ID.lexval, {{"type", "int"}, {"value", INTNUM.lexval}});)},
        {"decl", "real", "ID", "=", "REALNUM",
         ACT(GET(ID); GET(REALNUM);
             env.table.insert(ID.lexval, {{"type", "real"}, {"value", REALNUM.lexval}});)},
        {"stmt", "ifstmt"},
        {"stmt", "assgstmt"},
        {"stmt", "compoundstmt"},
        {"compoundstmt", "{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)},
        {"stmts", "stmt", "stmts"},
        {"stmts", "E"},
        {"ifstmt", "if", "(", "boolexpr", ")", "then",
         ACT(env.table.enter_scope_copy();),
         "stmt",
         ACT(GET(stmt);
             env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt.inh[key] = value.at("value"); });
             env.table.exit_scope();),
         "else",
         ACT(env.table.enter_scope_copy();),
         "stmt",
         ACT(GETI(stmt, 1);
             env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt_1.inh[key] = value.at("value"); });
             env.table.exit_scope();),
         ACT(GETI(stmt, 1); GET(stmt); GET(boolexpr); if (boolexpr.syn["val"] == "true") {
                for (auto& [key, value] : stmt.inh) {
                    (*env.table.lookup(key))["value"] = value;
                } } else {
                for (auto& [key, value] : stmt_1.inh) {
                    (*env.table.lookup(key))["value"] = value;
                } })},
        {"assgstmt", "ID", "=", "arithexpr", ";", ACT(GET(ID); GET(arithexpr); auto table_ID = env.table.lookup(ID.lexval); if (table_ID == nullptr) { env.error(ID.lexval + " is not defined"); return; }(*table_ID)["value"] = arithexpr.syn["val"];)},
        {"boolexpr", "arithexpr", "boolop", "arithexpr", ACT(GET(boolexpr); GET(boolop); GET(arithexpr); GETI(arithexpr, 1); auto lhs = std::stod(arithexpr.syn["val"]); auto rhs = std::stod(arithexpr_1.syn["val"]); if (boolop.syn["op"] == "<") { boolexpr.syn["val"] = lhs < rhs ? "true" : "false"; } if (boolop.syn["op"] == ">") { boolexpr.syn["val"] = lhs > rhs ? "true" : "false"; } if (boolop.syn["op"] == "==") { boolexpr.syn["val"] = lhs == rhs ? "true" : "false"; } if (boolop.syn["op"] == "<=") { boolexpr.syn["val"] = lhs <= rhs ? "true" : "false"; } if (boolop.syn["op"] == ">=") { boolexpr.syn["val"] = lhs >= rhs ? "true" : "false"; })},
        {"boolop", "<", ACT(GET(boolop); boolop.syn["op"] = "<";)},
        {"boolop", ">", ACT(GET(boolop); boolop.syn["op"] = ">";)},
        {"boolop", "<=", ACT(GET(boolop); boolop.syn["op"] = "<=";)},
        {"boolop", ">=", ACT(GET(boolop); boolop.syn["op"] = ">=";)},
        {"boolop", "==", ACT(GET(boolop); boolop.syn["op"] = "==";)},
        {"arithexpr", "multexpr", ACT(GET(arithexprprime); GET(multexpr); arithexprprime.inh["val"] = multexpr.syn["val"]; arithexprprime.inh["type"] = multexpr.syn["type"];), "arithexprprime", ACT(GET(arithexpr); GET(arithexprprime); arithexpr.syn["val"] = arithexprprime.syn["val"]; arithexpr.syn["type"] = arithexprprime.syn["type"];)},
        {"arithexprprime", "+", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(multexpr.syn["val"]) + std::stod(arithexprprime.inh["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)},
        {"arithexprprime", "-", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(arithexprprime.inh["val"]) - std::stod(multexpr.syn["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)},
        {"arithexprprime", "E", ACT(GET(arithexprprime); arithexprprime.syn["val"] = arithexprprime.inh["val"]; arithexprprime.syn["type"] = arithexprprime.inh["type"];)},
        {"multexpr", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); multexprprime.inh["val"] = simpleexpr.syn["val"]; multexprprime.inh["type"] = simpleexpr.syn["type"];), "multexprprime", ACT(GET(multexpr); GET(multexprprime); multexpr.syn["val"] = multexprprime.syn["val"]; multexpr.syn["type"] = multexprprime.syn["type"];)},
        {"multexprprime", "*", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) * std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)},
        {"multexprprime", "/", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); if (std::stod(simpleexpr.syn["val"]) == 0) {
                env.error("line " + std::to_string(simpleexpr.line) + ",division by zero");
                return; } multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) / std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)},
        {"multexprprime", "E", ACT(GET(multexprprime); multexprprime.syn["val"] = multexprprime.inh["val"]; multexprprime.syn["type"] = multexprprime.inh["type"];)},
        {"simpleexpr", "ID", ACT(GET(simpleexpr); GET(ID); simpleexpr.syn["val"] = env.table.lookup(ID.lexval)->at("value"); simpleexpr.syn["type"] = env.table.lookup(ID.lexval)->at("type");)},
        {"simpleexpr", "INTNUM", ACT(GET(simpleexpr); GET(INTNUM); simpleexpr.syn["val"] = INTNUM.lexval; simpleexpr.syn["type"] = "int"; simpleexpr.update_pos(INTNUM);)},
        {"simpleexpr", "REALNUM", ACT(GET(simpleexpr); GET(REALNUM); simpleexpr.syn["val"] = REALNUM.lexval; simpleexpr.syn["type"] = "real";)},
        {"simpleexpr", "(", "arithexpr", ")", ACT(GET(simpleexpr); GET(arithexpr); simpleexpr.syn["val"] = arithexpr.syn["val"]; simpleexpr.syn["type"] = arithexpr.syn["type"];)},
    };
#endif
    return prods;
};

std::string trim_zero(std::string s) {
    auto pos = s.find('.');
    if (pos == std::string::npos) {
        return s;
    }
    while (!s.empty() && s.back() == '0') {
        s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
        s.pop_back();
    }
    return s;
}

int main() {
    const auto lex = lexer::lexer(keywords, token_type::WHITESPACE);

    std::string input;
    std::string tmp;
    while (std::getline(std::cin, tmp)) {
        input += tmp;
        input += '\n';
    }

    const auto tokens = lex.parse(input);
    using gram_t = grammar::SLR<>;
    auto gram = semantic::sema<gram_t>(build_grammar());
    gram.init_error_handlers([](gram_t::action_table_t& action_table, gram_t::goto_table_t& goto_table, std::vector<gram_t::error_handle_fn>& error_handlers) {
        const auto realnum = grammar::production::symbol{"REALNUM"};
        for (auto& [state, row] : action_table) {
            if (!row.count(realnum)) {
                row[realnum] = grammar::action::error(0);
            }
        }

        error_handlers.emplace_back([](std::stack<grammar::LR_stack_t>& stack, std::vector<lexer::token>& in, std::size_t& pos) {
            std::cout << "error message:line " << in[pos].line << ",realnum can not be translated into int type" << std::endl;
            in[pos].type = static_cast<int>(token_type::INTNUM);
        });
    });
    gram.build();
    gram.parse(tokens);

    auto tree = std::static_pointer_cast<semantic::sema_tree>(gram.get_tree());

    // tree->print();
    auto env = tree->calc();
    // tree->print();

    for (const auto& error : env.errors) {
        std::cout << "error message:" << error << '\n';
    }

    if (!env.errors.empty()) {
        return 0;
    }

    std::vector<std::string> res;
    env.table.for_each_current([&](auto&& k, auto&& v) {
        res.push_back(k + ": " + trim_zero(v.at("value")));
    });

    std::sort(res.begin(), res.end());
    for (const auto& r : res) {
        std::cout << r << '\n';
    }
}