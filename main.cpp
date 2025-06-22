// #define USE_STD_REGEX
// #define SEMA_PROD_USE_INITIALIZER_LIST

#include "grammar/production.hpp"
#include "lexer/lexer.hpp"
#include "semantic/sema.hpp"

#include <fstream>
#include <memory>

enum class token_type {
    UNKNOWN = 0,
    INT,
    REAL,
    IF,
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
    "int", "real", "if", "else", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "=", "ID", "INTNUM", "REALNUM"};

std::vector<semantic::sema_production> build_grammar() {
    grammar::set_epsilon_str("E");
    grammar::set_end_mark_str("$");
    grammar::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });


    auto convert_type = [](auto& env, const std::string& reg,
                           const std::string& from_type, const std::string& to_type) {
        if (from_type == to_type) return reg;

        std::string conv_reg = "%" + env.temp();
        if (from_type == "int" && to_type == "real") {
            env.emit("  " + conv_reg + " = sitofp i32 " + reg + " to double");
        } else if (from_type == "real" && to_type == "int") {
            env.emit("  " + conv_reg + " = fptosi double " + reg + " to i32");
        }
        return conv_reg;
    };

    auto convert_operands = [&convert_type](auto& env, std::string& left_reg, std::string& right_reg,
                                const std::string& left_type, const std::string& right_type) {
        if (left_type == "real" || right_type == "real") {
            if (left_type == "int") {
                left_reg = convert_type(env, left_reg, "int", "real");
            }
            if (right_type == "int") {
                right_reg = convert_type(env, right_reg, "int", "real");
            }
            return "real";
        }
        return "int";
    };

    const std::vector<semantic::sema_production> prods{
        {"program", ACT(
            env.emit("; ModuleID = 'main'");
            env.emit("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"");
            env.emit("target triple = \"x86_64-unknown-linux-gnu\"");
            env.emit("");
            env.emit("define i32 @main() {");
            env.emit("entry:");
        ), "decls", "compoundstmt", ACT(
            env.emit("  ret i32 0");
            env.emit("}");
        )},

        {"decls", "decl", ";", "decls"},
        {"decls", "E"},

        {"decl", "int", "ID", "=", "INTNUM",
         ACT(GET(ID); GET(INTNUM);
             env.table.insert(ID.lexval, {{"type", "int"}});
             std::string var_name = "%" + ID.lexval;
             env.emit("  " + var_name + " = alloca i32, align 4");
             env.emit("  store i32 " + INTNUM.lexval + ", i32* " + var_name + ", align 4");
         )},

        {"decl", "real", "ID", "=", "REALNUM",
         ACT(GET(ID); GET(REALNUM);
             env.table.insert(ID.lexval, {{"type", "real"}});
             std::string var_name = "%" + ID.lexval;
             env.emit("  " + var_name + " = alloca double, align 8");
             env.emit("  store double " + REALNUM.lexval + ", double* " + var_name + ", align 8");
         )},

        {"stmt", "ifstmt"},
        {"stmt", "assgstmt"},
        {"stmt", "compoundstmt"},

        {"compoundstmt", "{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)},
        {"stmts", "stmt", "stmts"},
        {"stmts", "E"},

        {"ifstmt", "if", "(", "boolexpr", ")",
         ACT(GET(boolexpr); GET(stmt); GETI(stmt, 1);
             std::string cond_label = env.label();
             std::string then_label = env.label();
             std::string else_label = env.label();
             std::string end_label = env.label();

             stmt.inh["else"] = else_label;
             stmt.inh["end"] = end_label;
             stmt_1.inh["end"] = end_label;

             env.emit("  br i1 " + boolexpr.syn["reg"] + ", label %" + then_label + ", label %" + else_label);
             env.emit(then_label + ":");
         ),
         "stmt",
         ACT(GET(stmt);
             std::string else_label = stmt.inh["else"];
             std::string end_label = stmt.inh["end"];
             env.emit("  br label %" + end_label);
             env.emit(else_label + ":");
         ),
         "else",
         "stmt",
         ACT(GETI(stmt, 1);
             std::string end_label = stmt_1.inh["end"];
             env.emit("  br label %" + end_label);
             env.emit(end_label + ":");
         )},

        {"assgstmt", "ID", "=", "arithexpr", ";",
         ACT(GET(ID); GET(arithexpr);
             auto table_ID = env.table.lookup(ID.lexval);
             if (table_ID == nullptr) {
                 env.error(ID.lexval + " is not defined");
                 return;
             }
             std::string var_name = "%" + ID.lexval;
             std::string var_type = table_ID->at("type");
             std::string expr_reg = arithexpr.syn["reg"];
             std::string expr_type = arithexpr.syn["type"];

             expr_reg = convert_type(env, expr_reg, expr_type, var_type);

             if (var_type == "int") {
                 env.emit("  store i32 " + expr_reg + ", i32* " + var_name + ", align 4");
             } else {
                 env.emit("  store double " + expr_reg + ", double* " + var_name + ", align 8");
             }
         )},

        {"boolexpr", "arithexpr", "boolop", "arithexpr",
         ACT(GET(boolexpr); GET(boolop); GET(arithexpr); GETI(arithexpr, 1);
             std::string result_reg = "%" + env.temp();
             std::string lhs_reg = arithexpr.syn["reg"];
             std::string rhs_reg = arithexpr_1.syn["reg"];
             std::string lhs_type = arithexpr.syn["type"];
             std::string rhs_type = arithexpr_1.syn["type"];
             std::string op = boolop.syn["op"];

             std::string result_type = convert_operands(env, lhs_reg, rhs_reg, lhs_type, rhs_type);

             std::string cmp_op;
             if (op == "<") cmp_op = "lt";
             else if (op == ">") cmp_op = "gt";
             else if (op == "<=") cmp_op = "le";
             else if (op == ">=") cmp_op = "ge";
             else if (op == "==") cmp_op = "eq";

             if (result_type == "int") {
                 env.emit("  " + result_reg + " = icmp " + cmp_op + " i32 " + lhs_reg + ", " + rhs_reg);
             } else {
                 env.emit("  " + result_reg + " = fcmp o" + cmp_op + " double " + lhs_reg + ", " + rhs_reg);
             }
             boolexpr.syn["reg"] = result_reg;
         )},

        {"boolop", "<", ACT(GET(boolop); boolop.syn["op"] = "<";)},
        {"boolop", ">", ACT(GET(boolop); boolop.syn["op"] = ">";)},
        {"boolop", "<=", ACT(GET(boolop); boolop.syn["op"] = "<=";)},
        {"boolop", ">=", ACT(GET(boolop); boolop.syn["op"] = ">=";)},
        {"boolop", "==", ACT(GET(boolop); boolop.syn["op"] = "==";)},

        {"arithexpr", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr);
             arithexprprime.inh["reg"] = multexpr.syn["reg"];
             arithexprprime.inh["type"] = multexpr.syn["type"];
         ),
         "arithexprprime",
         ACT(GET(arithexpr); GET(arithexprprime);
             arithexpr.syn["reg"] = arithexprprime.syn["reg"];
             arithexpr.syn["type"] = arithexprprime.syn["type"];
         )},

        {"arithexprprime", "+", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = arithexprprime.inh["reg"];
             std::string right_reg = multexpr.syn["reg"];
             std::string left_type = arithexprprime.inh["type"];
             std::string right_type = multexpr.syn["type"];

             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             if (result_type == "real") {
                 env.emit("  " + result_reg + " = fadd double " + left_reg + ", " + right_reg);
             } else {
                 env.emit("  " + result_reg + " = add nsw i32 " + left_reg + ", " + right_reg);
             }

             arithexprprime_1.inh["reg"] = result_reg;
             arithexprprime_1.inh["type"] = result_type;
         ),
         "arithexprprime",
         ACT(GET(arithexprprime); GETI(arithexprprime, 1);
             arithexprprime.syn["reg"] = arithexprprime_1.syn["reg"];
             arithexprprime.syn["type"] = arithexprprime_1.syn["type"];
         )},

        {"arithexprprime", "-", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = arithexprprime.inh["reg"];
             std::string right_reg = multexpr.syn["reg"];
             std::string left_type = arithexprprime.inh["type"];
             std::string right_type = multexpr.syn["type"];

             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             if (result_type == "real") {
                 env.emit("  " + result_reg + " = fsub double " + left_reg + ", " + right_reg);
             } else {
                 env.emit("  " + result_reg + " = sub nsw i32 " + left_reg + ", " + right_reg);
             }
             arithexprprime_1.inh["reg"] = result_reg;
             arithexprprime_1.inh["type"] = result_type;
         ),
         "arithexprprime",
         ACT(GET(arithexprprime); GETI(arithexprprime, 1);
             arithexprprime.syn["reg"] = arithexprprime_1.syn["reg"];
             arithexprprime.syn["type"] = arithexprprime_1.syn["type"];
         )},

        {"arithexprprime", "E",
         ACT(GET(arithexprprime);
             arithexprprime.syn["reg"] = arithexprprime.inh["reg"];
             arithexprprime.syn["type"] = arithexprprime.inh["type"];
         )},

        {"multexpr", "simpleexpr",
         ACT(GET(multexprprime); GET(simpleexpr);
             multexprprime.inh["reg"] = simpleexpr.syn["reg"];
             multexprprime.inh["type"] = simpleexpr.syn["type"];
         ),
         "multexprprime",
         ACT(GET(multexpr); GET(multexprprime);
             multexpr.syn["reg"] = multexprprime.syn["reg"];
             multexpr.syn["type"] = multexprprime.syn["type"];
         )},

        {"multexprprime", "*", "simpleexpr",
         ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = simpleexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = simpleexpr.syn["type"];

             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             if (result_type == "real") {
                 env.emit("  " + result_reg + " = fmul double " + left_reg + ", " + right_reg);
             } else {
                 env.emit("  " + result_reg + " = mul nsw i32 " + left_reg + ", " + right_reg);
             }
             multexprprime_1.inh["reg"] = result_reg;
             multexprprime_1.inh["type"] = result_type;
         ),
         "multexprprime",
         ACT(GET(multexprprime); GETI(multexprprime, 1);
             multexprprime.syn["reg"] = multexprprime_1.syn["reg"];
             multexprprime.syn["type"] = multexprprime_1.syn["type"];
         )},

        {"multexprprime", "/", "simpleexpr",
         ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = simpleexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = simpleexpr.syn["type"];

             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             if (result_type == "real") {
                 env.emit("  " + result_reg + " = fdiv double " + left_reg + ", " + right_reg);
             } else {
                 env.emit("  " + result_reg + " = sdiv i32 " + left_reg + ", " + right_reg);
             }
             multexprprime_1.inh["reg"] = result_reg;
             multexprprime_1.inh["type"] = result_type;
         ),
         "multexprprime",
         ACT(GET(multexprprime); GETI(multexprprime, 1);
             multexprprime.syn["reg"] = multexprprime_1.syn["reg"];
             multexprprime.syn["type"] = multexprprime_1.syn["type"];
         )},

        {"multexprprime", "E",
         ACT(GET(multexprprime);
             multexprprime.syn["reg"] = multexprprime.inh["reg"];
             multexprprime.syn["type"] = multexprprime.inh["type"];
         )},

        {"simpleexpr", "ID",
         ACT(GET(simpleexpr); GET(ID);
             auto table_entry = env.table.lookup(ID.lexval);
             if (table_entry == nullptr) {
                 env.error(ID.lexval + " is not defined");
                 return;
             }
             std::string result_reg = "%" + env.temp();
             std::string var_name = "%" + ID.lexval;
             std::string var_type = table_entry->at("type");

             if (var_type == "int") {
                 env.emit("  " + result_reg + " = load i32, i32* " + var_name + ", align 4");
             } else {
                 env.emit("  " + result_reg + " = load double, double* " + var_name + ", align 8");
             }
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = var_type;
         )},

        {"simpleexpr", "INTNUM",
         ACT(GET(simpleexpr); GET(INTNUM);
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = add i32 0, " + INTNUM.lexval);
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "int";
         )},

        {"simpleexpr", "REALNUM",
         ACT(GET(simpleexpr); GET(REALNUM);
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = fadd double 0.0, " + REALNUM.lexval);
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "real";
         )},

        {"simpleexpr", "(", "arithexpr", ")",
         ACT(GET(simpleexpr); GET(arithexpr);
             simpleexpr.syn["reg"] = arithexpr.syn["reg"];
             simpleexpr.syn["type"] = arithexpr.syn["type"];
         )},
    };

    return prods;
};

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

int main() {
    const auto lex = lexer::lexer(keywords, token_type::WHITESPACE);

    std::string input;
    std::string tmp;
    while (std::getline(std::cin, tmp)) {
        input += tmp;
        input += '\n';
    }

    std::ofstream out("out.ll");

    const auto tokens = lex.parse(input);
    using gram_t = grammar::SLR<>;
    auto gram = semantic::sema<gram_t>(build_grammar()
        ,out
        );
    gram.init_error_handlers([](gram_t::action_table_t& action_table, gram_t::goto_table_t&,
                                std::vector<gram_t::error_handle_fn>& error_handlers) {
        const auto realnum = grammar::production::symbol{"REALNUM"};
        for (auto& row : action_table | std::views::values) {
            if (!row.contains(realnum)) {
                row[realnum] = grammar::action::error(0);
            }
        }

        error_handlers.emplace_back(
            [](std::stack<grammar::LR_stack_t>&, std::vector<lexer::token>& in, const std::size_t& pos) {
                std::cout << "error message:line " << in[pos].line << ",realnum can not be translated into int type" << std::endl;
                in[pos].type = static_cast<int>(token_type::INTNUM);
            });
    });
    gram.build();
    gram.parse(tokens);

    const auto tree = std::static_pointer_cast<semantic::sema_tree>(gram.get_tree());

    const auto env = tree->calc();

    for (const auto& error : env.errors) {
        std::cout << "error message:" << error << '\n';
    }
}
