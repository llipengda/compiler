#include "grammar/grammar.hpp"
#include "semantic/sema.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

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

using grammar_types = ::testing::Types<grammar::LL1, grammar::SLR<>, grammar::LR1>;

std::string trim_zero(std::string s) {
    if (const auto pos = s.find('.'); pos == std::string::npos) {
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

template <typename Grammar>
class sema_test_base : public ::testing::Test {
protected:
    semantic::sema<Grammar> parser;
    explicit sema_test_base(const std::vector<semantic::sema_production>& prods) : parser(prods), lex({}, static_cast<token_type>(0)) {
        parser.build();

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
            {"=", token_type::ASSIGN, "="},
            {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
            {"[0-9]+", token_type::INTNUM, "INTNUM"},
            {"[0-9]+\\.[0-9]*", token_type::REALNUM, "REALNUM"},
            {"[ \\t\\n]+", token_type::WHITESPACE, "WHITESPACE"}};

        lex = lexer::lexer(keywords, token_type::WHITESPACE);
    }
    lexer::lexer lex;
    std::pair<std::vector<std::string>, std::vector<std::string>> run(const std::vector<lexer::token>& tokens) {
        parser.parse(tokens);
        auto tree = std::static_pointer_cast<semantic::sema_tree>(parser.get_tree());
        auto env = tree->calc();
        std::vector<std::string> res;
        env.table.for_each_current([&](auto&& k, auto&& v) {
            res.push_back(k + ": " + trim_zero(v.at("value")));
        });
        std::ranges::sort(res);
        return {res, env.errors};
    }
    void expect_semantics(const std::string& input, const std::vector<std::string>& expected, const std::vector<std::string>& expected_errors = {}) {
        const auto tokens = lex.parse(input);
        utils::print(tokens);
        auto [res, errors] = run(tokens);
        if (errors.empty()) {
            ASSERT_EQ(res, expected);
        }
        ASSERT_EQ(errors, expected_errors);
    }
};

static const std::unordered_set<std::string> terminals{
    "int", "real", "if", "then", "else", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "=", "ID", "INTNUM", "REALNUM"};

std::vector<semantic::sema_production> build_grammar() {
    grammar::set_epsilon_str("E");
    grammar::set_end_mark_str("$");
    grammar::set_terminal_rule([](const std::string& str) {
        return terminals.count(str);
    });
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
        {"assgstmt", "ID", "=", "arithexpr", ";", ACT(GET(ID); GET(arithexpr); auto table_ID = env.table.lookup(ID.lexval); if (table_ID == nullptr) { env.
                error(ID.lexval + " is not defined"); return; }(*table_ID)["value"] = arithexpr.syn["val"];)},
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
    return prods;
}

// int/real 声明与赋值

template <typename Grammar>
class sema_test_decl : public sema_test_base<Grammar> {
public:
    sema_test_decl() : sema_test_base<Grammar>(build_grammar()) {}
};

TYPED_TEST_SUITE(sema_test_decl, grammar_types);

TYPED_TEST(sema_test_decl, int_decl_and_assign) {
    this->expect_semantics("int ID = 1 ; { ID = 2 ; }", {"ID: 2"});
}

TYPED_TEST(sema_test_decl, real_decl_and_assign) {
    this->expect_semantics("real ID = 1.5 ; { ID = 2.5 ; }", {"ID: 2.5"});
}

TYPED_TEST(sema_test_decl, undeclared_variable_error) {
    this->expect_semantics("{ ID = 1 ; }", {}, {"ID is not defined"});
}

TYPED_TEST(sema_test_decl, division_by_zero) {
    this->expect_semantics("int ID = 1 ; { ID = 1 / 0 ; }", {"ID: 1"}, {"line 1,division by zero"});
}

TYPED_TEST(sema_test_decl, multi_var_decl_and_assign) {
    this->expect_semantics("int a = 1 ; int b = 2 ; { a = b + 3 ; }", {"a: 5", "b: 2"});
}

TYPED_TEST(sema_test_decl, undeclared_var_in_block) {
    this->expect_semantics("int a = 1 ; { a = 2 ; b = a ; }", {"a: 1"}, {"b is not defined"});
}

TYPED_TEST(sema_test_decl, multi_var_scope_and_expr) {
    this->expect_semantics("int a = 1 ; int b = 2 ; { a = a + b ; }", {"a: 3", "b: 2"});
}

// if/else/while

template <typename Grammar>
class sema_test_ctrl : public sema_test_base<Grammar> {
public:
    sema_test_ctrl() : sema_test_base<Grammar>(build_grammar()) {}
};

TYPED_TEST_SUITE(sema_test_ctrl, grammar_types);

TYPED_TEST(sema_test_ctrl, if_true_branch) {
    this->expect_semantics("int ID = 1 ; {if ( 1 < 2 ) then { ID = 3 ; } else { ID = 4 ; }}", {"ID: 3"});
}

TYPED_TEST(sema_test_ctrl, if_false_branch) {
    this->expect_semantics("int ID = 1 ; {if ( 2 < 1 ) then { ID = 3 ; } else { ID = 4 ; }}", {"ID: 4"});
}

// 表达式求值

template <typename Grammar>
class sema_test_expr : public sema_test_base<Grammar> {
public:
    sema_test_expr() : sema_test_base<Grammar>(build_grammar()) {}
};

TYPED_TEST_SUITE(sema_test_expr, grammar_types);

TYPED_TEST(sema_test_expr, arith_expr) {
    this->expect_semantics("int ID = 1 ; { ID = 2 + 3 * 4 ; }", {"ID: 14"});
}

TYPED_TEST(sema_test_expr, paren_expr) {
    this->expect_semantics("int ID = 1 ; { ID = ( 2 + 3 ) * 4 ; }", {"ID: 20"});
}

TYPED_TEST(sema_test_expr, bool_expr) {
    this->expect_semantics("int ID = 1 ; { if ( 2 == 2 ) then { ID = 5 ; } else { ID = 6 ; } }", {"ID: 5"});
}

TYPED_TEST(sema_test_expr, multi_var_arith_expr) {
    this->expect_semantics("int a = 1 ; int b = 2 ; { a = a + b * 3 ; }", {"a: 7", "b: 2"});
}

TYPED_TEST(sema_test_expr, multi_var_paren_expr) {
    this->expect_semantics("int a = 1 ; int b = 2 ; { a = ( a + b ) * 2 ; }", {"a: 6", "b: 2"});
}
