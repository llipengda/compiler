#include "grammar/grammar.hpp"
#include "utils.hpp"
#include <gtest/gtest.h>

using grammar_types = ::testing::Types<grammar::LL1, grammar::SLR<>, grammar::LR1>;

template <typename Grammar>
class grammar_test_add : public ::testing::Test {
protected:
    Grammar parser;

    grammar_test_add() : parser(R"(
E -> T E'
E' -> + T E' | ε
T -> id
)") {
        parser.build();
    }

    static std::vector<lexer::token> simple_lexer(const std::string& input) {
        std::vector<lexer::token> tokens;
        for (const auto& c : utils::split_trim(input, " ")) {
            if (!c.empty()) {
                tokens.emplace_back(c);
            }
        }
        return tokens;
    }

    void expect_parse(const std::string& input, const std::vector<std::string>& expected) {
        const auto tokens = simple_lexer(input);
        parser.parse(tokens);
        const auto tree = parser.get_tree();
        std::vector<std::string> preorder;
        tree->visit([&preorder](auto&& node) {
            preorder.push_back(node->symbol->lexval);
        });

        ASSERT_EQ(preorder, expected);
    }

    void expect_parse_fail(const std::string& input) {
        testing::internal::CaptureStderr();
        const auto tokens = simple_lexer(input);
        bool e = false;
        try {
            parser.parse(tokens);
        } catch (...) {
            e = true;
        }
        const auto& err = testing::internal::GetCapturedStderr();
        ASSERT_TRUE(e || !err.empty());
    }
};

TYPED_TEST_SUITE(grammar_test_add, grammar_types);

TYPED_TEST(grammar_test_add, parses_single_id) {
    this->expect_parse("id", {"E", "T", "id", "E'", "ε"});
}

TYPED_TEST(grammar_test_add, parses_id_plus_id) {
    this->expect_parse("id + id", {"E", "T", "id", "E'", "+", "T", "id", "E'", "ε"});
}

TYPED_TEST(grammar_test_add, parses_id_plus_id_plus_id) {
    this->expect_parse("id + id + id", {"E", "T", "id", "E'", "+", "T", "id", "E'", "+", "T", "id", "E'", "ε"});
}

TYPED_TEST(grammar_test_add, fails_on_incomplete_input) {
    this->expect_parse_fail("id +");
}

TYPED_TEST(grammar_test_add, fails_on_unexpected_token) {
    this->expect_parse_fail("id * id");
}

template <typename Grammar>
class grammar_test_expr : public ::testing::Test {
protected:
    Grammar parser;

    grammar_test_expr() : parser(R"(
E  -> T E'
E' -> + T E' | - T E' | ε
T  -> F T'
T' -> * F T' | / F T' | ε
F  -> ( E ) | id
)") {
        parser.build();
    }

    static std::vector<lexer::token> simple_lexer(const std::string& input) {
        std::vector<lexer::token> tokens;
        for (const auto& c : utils::split_trim(input, " ")) {
            if (!c.empty()) {
                tokens.emplace_back(c);
            }
        }
        return tokens;
    }

    void expect_parse(const std::string& input, const std::vector<std::string>& expected) {
        const auto tokens = simple_lexer(input);
        parser.parse(tokens);
        const auto tree = parser.get_tree();
        std::vector<std::string> preorder;
        tree->visit([&preorder](auto&& node) {
            preorder.push_back(node->symbol->lexval);
        });

        ASSERT_EQ(preorder, expected);
    }

    void expect_parse_fail(const std::string& input) {
        testing::internal::CaptureStderr();
        const auto tokens = simple_lexer(input);
        bool e = false;
        try {
            parser.parse(tokens);
        } catch (...) {
            e = true;
        }
        const auto& err = testing::internal::GetCapturedStderr();
        ASSERT_TRUE(e || !err.empty());
    }
};

TYPED_TEST_SUITE(grammar_test_expr, grammar_types);

TYPED_TEST(grammar_test_expr, parses_simple_id) {
    this->expect_parse("id", {"E", "T", "F", "id", "T'", "ε", "E'", "ε"});
}

TYPED_TEST(grammar_test_expr, parses_addition_and_multiplication) {
    this->expect_parse("id + id * id", {"E", "T", "F", "id", "T'", "ε", "E'", "+", "T", "F", "id", "T'", "*", "F", "id", "T'", "ε", "E'", "ε"});
}

TYPED_TEST(grammar_test_expr, parses_expression_with_parentheses) {
    this->expect_parse("( id + id ) * id", {"E", "T", "F", "(", "E", "T", "F", "id", "T'", "ε", "E'", "+", "T", "F", "id", "T'", "ε", "E'", "ε", ")", "T'", "*", "F", "id", "T'", "ε", "E'", "ε"});
}

TYPED_TEST(grammar_test_expr, parses_expression_with_subtraction_and_division) {
    this->expect_parse("id - id / id", {"E", "T", "F", "id", "T'", "ε", "E'", "-", "T", "F", "id", "T'", "/", "F", "id", "T'", "ε", "E'", "ε"});
}

TYPED_TEST(grammar_test_expr, fails_on_missing_parenthesis) {
    this->expect_parse_fail("( id + id");
}

TYPED_TEST(grammar_test_expr, fails_on_illegal_operator) {
    this->expect_parse_fail("id ^ id");
}

TYPED_TEST(grammar_test_expr, fails_on_unexpected_token) {
    this->expect_parse_fail("id id");
}

const std::unordered_set<std::string> terminals = {
    "{", "}", "(", ")", "if", "then", "else", "while", "=", ";", "<", ">", "<=", ">=", "==", "+", "-", "*", "/", "ID", "NUM"};

std::string get_gram() {
    grammar::set_epsilon_str("E");
    grammar::set_terminal_rule([&](auto&& str) {
        return terminals.contains(str);
    });
    return R"(
program -> compoundstmt
stmt ->  ifstmt  |  whilestmt  |  assgstmt  |  compoundstmt
compoundstmt ->  { stmts }
stmts ->  stmt stmts   |   E
ifstmt ->  if ( boolexpr ) then stmt else stmt
whilestmt ->  while ( boolexpr ) stmt
assgstmt ->  ID = arithexpr ;
boolexpr  ->  arithexpr boolop arithexpr
boolop ->   <  |  >  |  <=  |  >=  | ==
arithexpr  ->  multexpr arithexprprime
arithexprprime ->  + multexpr arithexprprime  |  - multexpr arithexprprime  |   E
multexpr ->  simpleexpr  multexprprime
multexprprime ->  * simpleexpr multexprprime  |  / simpleexpr multexprprime  |   E
simpleexpr ->  ID  |  NUM  |  ( arithexpr )
)";
}

template <typename Grammar>
class grammar_test_program : public ::testing::Test {
protected:
    Grammar parser;

    grammar_test_program() : parser(get_gram()) {
        parser.build();
    }

    static std::vector<lexer::token> simple_lexer(const std::string& input) {
        std::vector<lexer::token> tokens;
        for (const auto& c : utils::split_trim(input, " ")) {
            if (!c.empty()) {
                tokens.emplace_back(c);
            }
        }
        return tokens;
    }

    void expect_parse(const std::string& input, const std::vector<std::string>& expected) {
        const auto tokens = simple_lexer(input);
        parser.parse(tokens);
        const auto tree = parser.get_tree();
        std::vector<std::string> preorder;
        tree->visit([&preorder](auto&& node) {
            preorder.push_back(node->symbol->lexval);
        });

        ASSERT_EQ(preorder, expected);
    }

    void expect_parse_fail(const std::string& input) {
        testing::internal::CaptureStderr();
        const auto tokens = simple_lexer(input);
        bool e = false;
        try {
            parser.parse(tokens);
        } catch (...) {
            e = true;
        }
        const auto& err = testing::internal::GetCapturedStderr();
        ASSERT_TRUE(e || !err.empty());
    }
};

TYPED_TEST_SUITE(grammar_test_program, grammar_types);

TYPED_TEST(grammar_test_program, parses_simple_assignment) {
    this->expect_parse("{ ID = NUM ; }", {"program", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, parses_expression_with_precedence) {
    this->expect_parse("{ ID = NUM + ID * NUM ; }", {"program", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "+", "multexpr", "simpleexpr", "ID", "multexprprime", "*", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, parses_parenthesized_expression) {
    this->expect_parse("{ ID = ( NUM + ID ) ; }", {"program", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "(", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "+", "multexpr", "simpleexpr", "ID", "multexprprime", "E", "arithexprprime", "E", ")", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, parses_if_statement) {
    this->expect_parse("{ if ( ID < NUM ) then { ID = NUM ; } else { ID = NUM ; } }", {"program", "compoundstmt", "{", "stmts", "stmt", "ifstmt", "if", "(", "boolexpr", "arithexpr", "multexpr", "simpleexpr", "ID", "multexprprime", "E", "arithexprprime", "E", "boolop", "<", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ")", "then", "stmt", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}", "else", "stmt", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, parses_while_statement) {
    this->expect_parse("{ while ( ID == ID ) { ID = NUM ; } }", {"program", "compoundstmt", "{", "stmts", "stmt", "whilestmt", "while", "(", "boolexpr", "arithexpr", "multexpr", "simpleexpr", "ID", "multexprprime", "E", "arithexprprime", "E", "boolop", "==", "arithexpr", "multexpr", "simpleexpr", "ID", "multexprprime", "E", "arithexprprime", "E", ")", "stmt", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, parses_nested_blocks) {
    this->expect_parse("{ ID = NUM ; { ID = NUM ; } ID = ID ; }", {"program", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "stmt", "compoundstmt", "{", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "NUM", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}", "stmts", "stmt", "assgstmt", "ID", "=", "arithexpr", "multexpr", "simpleexpr", "ID", "multexprprime", "E", "arithexprprime", "E", ";", "stmts", "E", "}"});
}

TYPED_TEST(grammar_test_program, fails_missing_semicolon) {
    this->expect_parse_fail("{ ID = NUM }"); // 缺少 ;
}

TYPED_TEST(grammar_test_program, fails_missing_closing_brace) {
    this->expect_parse_fail("{ ID = NUM ; "); // 缺少 }
}

TYPED_TEST(grammar_test_program, fails_invalid_token) {
    this->expect_parse_fail("{ ID := NUM ; }"); // := 非法
}
