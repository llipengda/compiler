// #define USE_STD_REGEX
// #define SEMA_PROD_USE_INITIALIZER_LIST

#include "grammar/production.hpp"
#include "lexer/lexer.hpp"
#include "semantic/sema.hpp"
#include "utils.hpp"

#include <fstream>
#include <memory>
#include <ranges>
#include <sstream>

enum class token_type {
    UNKNOWN = 0,
    INT,
    DOUBLE,
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
    DOUBLENUM,
    WHITESPACE,    BITAND,    // &
    BITOR,     // |
    BITXOR,    // ^
    LOGAND,    // &&
    LOGOR,     // ||
    COMMA,     // ,
    STRING,    // "string"
    COMMENT
};

const lexer::lexer::input_keywords_t<token_type> keywords = {
    {"int", token_type::INT, "int"},
    {"double", token_type::DOUBLE, "double"},
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
    {"&&", token_type::LOGAND, "&&"},
    {"\\|\\|", token_type::LOGOR, "||"},    {"&", token_type::BITAND, "&"},
    {"\\|", token_type::BITOR, "|"},
    {"^", token_type::BITXOR, "^"},
    {",", token_type::COMMA, ","},
    {"\"([^\"\\\\]|\\\\.)*\"", token_type::STRING, "STRING"},
    {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
    {"[0-9]+", token_type::INTNUM, "INTNUM"},
    {"[0-9]+\\.[0-9]*", token_type::DOUBLENUM, "DOUBLENUM"},
    {"[ \t\n]+", token_type::WHITESPACE, "WHITESPACE"},
    {"(//[^\n]*)|(/\\*([^*]|\\*+[^*/])*\\*+/)", token_type::COMMENT, "COMMENT"}
};

const std::unordered_set<std::string> terminals{
    "int", "double", "if", "else", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "=", "ID", "INTNUM", "DOUBLENUM",
    "&", "|", "^", "&&", "||", ",", "STRING"};

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
        if (from_type == "int" && to_type == "double") {
            env.emit("  " + conv_reg + " = sitofp i32 " + reg + " to double");
        } else if (from_type == "double" && to_type == "int") {
            env.emit("  " + conv_reg + " = fptosi double " + reg + " to i32");
        }
        return conv_reg;
    };    
      auto convert_operands = [&convert_type](auto& env, 
                                std::string& left_reg, std::string& right_reg,
                                const std::string& left_type, const std::string& right_type) {        
        if (left_type == "double" || right_type == "double") {
            if (left_type == "int") {
                left_reg = convert_type(env, left_reg, "int", "double");
            }
            if (right_type == "int") {
                right_reg = convert_type(env, right_reg, "int", "double");
            }
            return "double";
        }
        return "int";
    };

    auto convert_to_i1 = [](auto& env, const std::string& reg) {
        std::string i1_reg = "%" + env.temp();
        env.emit("  " + i1_reg + " = trunc i32 " + reg + " to i1");
        return i1_reg;
    };

    auto emit_alloca = [](auto& env, const std::string& var_name, const std::string& var_type) {
        if (var_type == "int") {
            env.emit("  " + var_name + " = alloca i32, align 4");
        } else {
            env.emit("  " + var_name + " = alloca double, align 8");
        }
    };

    auto emit_store = [](auto& env, const std::string& value_reg, const std::string& var_name, const std::string& var_type) {
        if (var_type == "int") {
            env.emit("  store i32 " + value_reg + ", i32* " + var_name + ", align 4");
        } else {
            env.emit("  store double " + value_reg + ", double* " + var_name + ", align 8");
        }
    };    
    
    auto emit_load = [](auto& env, const std::string& result_reg, const std::string& var_name, const std::string& var_type) {
        if (var_type == "int") {
            env.emit("  " + result_reg + " = load i32, i32* " + var_name + ", align 4");
        } else {
            env.emit("  " + result_reg + " = load double, double* " + var_name + ", align 8");
        }
    };    
    
    auto emit_add = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
        if (result_type == "double") {
            env.emit("  " + result_reg + " = fadd double " + left_reg + ", " + right_reg);
        } else {
            env.emit("  " + result_reg + " = add nsw i32 " + left_reg + ", " + right_reg);
        }
    };    
    
    auto emit_sub = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
        if (result_type == "double") {
            env.emit("  " + result_reg + " = fsub double " + left_reg + ", " + right_reg);
        } else {
            env.emit("  " + result_reg + " = sub nsw i32 " + left_reg + ", " + right_reg);
        }
    };    
    
    auto emit_mul = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
        if (result_type == "double") {
            env.emit("  " + result_reg + " = fmul double " + left_reg + ", " + right_reg);
        } else {
            env.emit("  " + result_reg + " = mul nsw i32 " + left_reg + ", " + right_reg);
        }
    };    
    
    auto emit_div = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
        if (result_type == "double") {
            env.emit("  " + result_reg + " = fdiv double " + left_reg + ", " + right_reg);
        } else {
            env.emit("  " + result_reg + " = sdiv i32 " + left_reg + ", " + right_reg);
        }
    };

    auto emit_bitand = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg) {
        env.emit("  " + result_reg + " = and i32 " + left_reg + ", " + right_reg);
    };

    auto emit_bitor = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg) {
        env.emit("  " + result_reg + " = or i32 " + left_reg + ", " + right_reg);
    };

    auto emit_bitxor = [](auto& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg) {
        env.emit("  " + result_reg + " = xor i32 " + left_reg + ", " + right_reg);
    };

    const std::vector<semantic::sema_production> prods{          {"program", ACT(
            env.emit("; ModuleID = 'main'");
            env.emit("");
            env.emit("declare i32 @printf(i8*, ...)");
            env.emit("");
            env.emit("define i32 @main() {");
            env.emit("entry:");
        ), "stmts", ACT(
            env.emit("  ret i32 0");
            env.emit("}");
        )},

        {"declstmt", "decl", ";"},        {"type", "int", ACT(GET(type); type.syn["type"] = "int";)},
        {"type", "double", ACT(GET(type); type.syn["type"] = "double";)},          {"decl", "type", "ID", "=", "expr",
         ACT(GET(type); GET(ID); GET(expr);
             std::string var_type = type.syn["type"];
             // 使用唯一的变量名（在符号表中存储原名，在LLVM IR中使用唯一名）
             std::string unique_var_name = "%" + ID.lexval + "_" + env.temp();
             env.table.insert(ID.lexval, {{"type", var_type}, {"llvm_name", unique_var_name}});
             std::string expr_reg = expr.syn["reg"];
             std::string expr_type = expr.syn["type"];

             // 类型转换
             expr_reg = convert_type(env, expr_reg, expr_type, var_type);

             emit_alloca(env, unique_var_name, var_type);
             emit_store(env, expr_reg, unique_var_name, var_type);
         )},

        {"decl", "type", "ID",
         ACT(GET(type); GET(ID);
             std::string var_type = type.syn["type"];
             // 使用唯一的变量名
             std::string unique_var_name = "%" + ID.lexval + "_" + env.temp();
             env.table.insert(ID.lexval, {{"type", var_type}, {"llvm_name", unique_var_name}});

             // 只分配内存，不进行初始化
             emit_alloca(env, unique_var_name, var_type);
         )},        {"stmt", "ifstmt"},
        {"stmt", "assgstmt"},
        {"stmt", "compoundstmt"},
        {"stmt", "declstmt"},
        {"stmt", "callstmt"},

        {"compoundstmt", "{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)},
        {"stmts", "stmt", "stmts"},
        {"stmts", "E"},        {"ifstmt", "if", "(", "expr", ")",
         ACT(GET(expr); GET(stmt); GETI(stmt, 1);
             std::string cond_label = env.label();
             std::string then_label = env.label();
             std::string else_label = env.label();
             std::string end_label = env.label();

             stmt.inh["else"] = else_label;
             stmt.inh["end"] = end_label;
             stmt_1.inh["end"] = end_label;

             auto cond = convert_to_i1(env, expr.syn["reg"]);

             env.emit("  br i1 " + cond + ", label %" + then_label + ", label %" + else_label);
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
         )},{"assgstmt", "ID", "=", "expr", ";",
         ACT(GET(ID); GET(expr);
             auto table_ID = env.table.lookup(ID.lexval);
             if (table_ID == nullptr) {
                 env.error(ID.lexval + " is not defined");
                 return;
             }             
             std::string var_name = table_ID->at("llvm_name");
             std::string var_type = table_ID->at("type");
             std::string expr_reg = expr.syn["reg"];
             std::string expr_type = expr.syn["type"];

             expr_reg = convert_type(env, expr_reg, expr_type, var_type);

             emit_store(env, expr_reg, var_name, var_type);
         )},// 表达式层次（按优先级从低到高）
        // expr -> logorexpr
        {"expr", "logorexpr",
         ACT(GET(expr); GET(logorexpr);
             expr.syn["reg"] = logorexpr.syn["reg"];
             expr.syn["type"] = logorexpr.syn["type"];
         )},        // logorexpr -> logandexpr ( "||" logandexpr )*
        {"logorexpr", "logandexpr", 
         ACT(GET(logandexpr); GET(logorprime);
             logorprime.inh["reg"] = logandexpr.syn["reg"];
             logorprime.inh["type"] = logandexpr.syn["type"];
         ),
         "logorprime",
         ACT(GET(logorexpr); GET(logorprime);
             logorexpr.syn["reg"] = logorprime.syn["reg"];
             logorexpr.syn["type"] = logorprime.syn["type"];
         )},            {"logorprime", "||", "logandexpr", 
         ACT(GET(logorprime); GET(logandexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = logorprime.inh["reg"];
             std::string right_reg = logandexpr.syn["reg"];
             
             // 将操作数转换为i1类型
             std::string left_i1 = convert_to_i1(env, left_reg);
             std::string right_i1 = convert_to_i1(env, right_reg);
             
             // 逻辑或运算
             env.emit("  " + result_reg + " = or i1 " + left_i1 + ", " + right_i1);
             
             // 将结果扩展为i32
             std::string final_reg = "%" + env.temp();
             env.emit("  " + final_reg + " = zext i1 " + result_reg + " to i32");
             
             // 为下一个logorprime设置继承属性
             GETI(logorprime, 1);
             logorprime_1.inh["reg"] = final_reg;
             logorprime_1.inh["type"] = "int";
         ),
         "logorprime",
         ACT(GET(logorprime); GETI(logorprime, 1);
             logorprime.syn["reg"] = logorprime_1.syn["reg"];
             logorprime.syn["type"] = logorprime_1.syn["type"];
         )},

        {"logorprime", "E",
         ACT(GET(logorprime);
             logorprime.syn["reg"] = logorprime.inh["reg"];
             logorprime.syn["type"] = logorprime.inh["type"];
         )},        // logandexpr -> bitorexpr ( "&&" bitorexpr )*
        {"logandexpr", "bitorexpr", 
         ACT(GET(bitorexpr); GET(logandprime);
             logandprime.inh["reg"] = bitorexpr.syn["reg"];
             logandprime.inh["type"] = bitorexpr.syn["type"];
         ),
         "logandprime",
         ACT(GET(logandexpr); GET(logandprime);
             logandexpr.syn["reg"] = logandprime.syn["reg"];
             logandexpr.syn["type"] = logandprime.syn["type"];
         )},        {"logandprime", "&&", "bitorexpr", 
         ACT(GET(logandprime); GET(bitorexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = logandprime.inh["reg"];
             std::string right_reg = bitorexpr.syn["reg"];
             
             // 将操作数转换为i1类型
             std::string left_i1 = convert_to_i1(env, left_reg);
             std::string right_i1 = convert_to_i1(env, right_reg);
             
             // 逻辑与运算
             env.emit("  " + result_reg + " = and i1 " + left_i1 + ", " + right_i1);
             
             // 将结果扩展为i32
             std::string final_reg = "%" + env.temp();
             env.emit("  " + final_reg + " = zext i1 " + result_reg + " to i32");
             
             // 为下一个logandprime设置继承属性
             GETI(logandprime, 1);
             logandprime_1.inh["reg"] = final_reg;
             logandprime_1.inh["type"] = "int";
         ),
         "logandprime",
         ACT(GET(logandprime); GETI(logandprime, 1);
             logandprime.syn["reg"] = logandprime_1.syn["reg"];
             logandprime.syn["type"] = logandprime_1.syn["type"];
         )},

        {"logandprime", "E",
         ACT(GET(logandprime);
             logandprime.syn["reg"] = logandprime.inh["reg"];
             logandprime.syn["type"] = logandprime.inh["type"];
         )},        // bitorexpr -> bitxorexpr ( "|" bitxorexpr )*
        {"bitorexpr", "bitxorexpr", 
         ACT(GET(bitxorexpr); GET(bitorprime);
             bitorprime.inh["reg"] = bitxorexpr.syn["reg"];
             bitorprime.inh["type"] = bitxorexpr.syn["type"];
         ),
         "bitorprime",
         ACT(GET(bitorexpr); GET(bitorprime);
             bitorexpr.syn["reg"] = bitorprime.syn["reg"];
             bitorexpr.syn["type"] = bitorprime.syn["type"];
         )},        {"bitorprime", "|", "bitxorexpr", 
         ACT(GET(bitorprime); GET(bitxorexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitorprime.inh["reg"];
             std::string right_reg = bitxorexpr.syn["reg"];
             
             // 转换为int类型进行位运算
             left_reg = convert_type(env, left_reg, bitorprime.inh["type"], "int");
             right_reg = convert_type(env, right_reg, bitxorexpr.syn["type"], "int");
             
             emit_bitor(env, result_reg, left_reg, right_reg);
             
             // 为下一个bitorprime设置继承属性
             GETI(bitorprime, 1);
             bitorprime_1.inh["reg"] = result_reg;
             bitorprime_1.inh["type"] = "int";
         ),
         "bitorprime",
         ACT(GET(bitorprime); GETI(bitorprime, 1);
             bitorprime.syn["reg"] = bitorprime_1.syn["reg"];
             bitorprime.syn["type"] = bitorprime_1.syn["type"];
         )},

        {"bitorprime", "E",
         ACT(GET(bitorprime);
             bitorprime.syn["reg"] = bitorprime.inh["reg"];
             bitorprime.syn["type"] = bitorprime.inh["type"];
         )},        // bitxorexpr -> bitandexpr ( "^" bitandexpr )*
        {"bitxorexpr", "bitandexpr", 
         ACT(GET(bitandexpr); GET(bitxorprime);
             bitxorprime.inh["reg"] = bitandexpr.syn["reg"];
             bitxorprime.inh["type"] = bitandexpr.syn["type"];
         ),
         "bitxorprime",
         ACT(GET(bitxorexpr); GET(bitxorprime);
             bitxorexpr.syn["reg"] = bitxorprime.syn["reg"];
             bitxorexpr.syn["type"] = bitxorprime.syn["type"];
         )},        {"bitxorprime", "^", "bitandexpr", 
         ACT(GET(bitxorprime); GET(bitandexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitxorprime.inh["reg"];
             std::string right_reg = bitandexpr.syn["reg"];
             
             // 转换为int类型进行位运算
             left_reg = convert_type(env, left_reg, bitxorprime.inh["type"], "int");
             right_reg = convert_type(env, right_reg, bitandexpr.syn["type"], "int");
             
             emit_bitxor(env, result_reg, left_reg, right_reg);
             
             // 为下一个bitxorprime设置继承属性
             GETI(bitxorprime, 1);
             bitxorprime_1.inh["reg"] = result_reg;
             bitxorprime_1.inh["type"] = "int";
         ),
         "bitxorprime",
         ACT(GET(bitxorprime); GETI(bitxorprime, 1);
             bitxorprime.syn["reg"] = bitxorprime_1.syn["reg"];
             bitxorprime.syn["type"] = bitxorprime_1.syn["type"];
         )},

        {"bitxorprime", "E",
         ACT(GET(bitxorprime);
             bitxorprime.syn["reg"] = bitxorprime.inh["reg"];
             bitxorprime.syn["type"] = bitxorprime.inh["type"];
         )},        // bitandexpr -> relexpr ( "&" relexpr )*
        {"bitandexpr", "relexpr", 
         ACT(GET(relexpr); GET(bitandprime);
             bitandprime.inh["reg"] = relexpr.syn["reg"];
             bitandprime.inh["type"] = relexpr.syn["type"];
         ),
         "bitandprime",
         ACT(GET(bitandexpr); GET(bitandprime);
             bitandexpr.syn["reg"] = bitandprime.syn["reg"];
             bitandexpr.syn["type"] = bitandprime.syn["type"];
         )},        {"bitandprime", "&", "relexpr", 
         ACT(GET(bitandprime); GET(relexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitandprime.inh["reg"];
             std::string right_reg = relexpr.syn["reg"];
             
             // 转换为int类型进行位运算
             left_reg = convert_type(env, left_reg, bitandprime.inh["type"], "int");
             right_reg = convert_type(env, right_reg, relexpr.syn["type"], "int");
             
             emit_bitand(env, result_reg, left_reg, right_reg);
             
             // 为下一个bitandprime设置继承属性
             GETI(bitandprime, 1);
             bitandprime_1.inh["reg"] = result_reg;
             bitandprime_1.inh["type"] = "int";
         ),
         "bitandprime",
         ACT(GET(bitandprime); GETI(bitandprime, 1);
             bitandprime.syn["reg"] = bitandprime_1.syn["reg"];
             bitandprime.syn["type"] = bitandprime_1.syn["type"];
         )},

        {"bitandprime", "E",
         ACT(GET(bitandprime);
             bitandprime.syn["reg"] = bitandprime.inh["reg"];
             bitandprime.syn["type"] = bitandprime.inh["type"];
         )},        // relexpr -> arithexpr ( relop arithexpr )?
        {"relexpr", "arithexpr", 
         ACT(GET(arithexpr); GET(relprime);
             relprime.inh["reg"] = arithexpr.syn["reg"];
             relprime.inh["type"] = arithexpr.syn["type"];
         ), 
         "relprime",
         ACT(GET(relexpr); GET(relprime);
             relexpr.syn["reg"] = relprime.syn["reg"];
             relexpr.syn["type"] = relprime.syn["type"];
         )},

        {"relprime", "relop", "arithexpr",
         ACT(GET(relprime); GET(relop); GET(arithexpr);
             std::string result_reg = "%" + env.temp();
             std::string lhs_reg = relprime.inh["reg"];
             std::string rhs_reg = arithexpr.syn["reg"];
             std::string lhs_type = relprime.inh["type"];
             std::string rhs_type = arithexpr.syn["type"];
             std::string op = relop.syn["op"];

             std::string result_type = convert_operands(env, lhs_reg, rhs_reg, lhs_type, rhs_type);

             std::string cmp_op;
             if (op == "<") cmp_op = "lt";
             else if (op == ">") cmp_op = "gt";
             else if (op == "<=") cmp_op = "le";
             else if (op == ">=") cmp_op = "ge";
             else if (op == "==") cmp_op = "eq";             if (result_type == "int") {
                 if (cmp_op != "eq") {
                     cmp_op = "s" + cmp_op;
                 }
                 env.emit("  " + result_reg + " = icmp " + cmp_op + " i32 " + lhs_reg + ", " + rhs_reg);
             } else {
                 env.emit("  " + result_reg + " = fcmp o" + cmp_op + " double " + lhs_reg + ", " + rhs_reg);
             }
             
             // 将i1类型扩展为i32类型
             std::string final_reg = "%" + env.temp();
             env.emit("  " + final_reg + " = zext i1 " + result_reg + " to i32");
             
             relprime.syn["reg"] = final_reg;
             relprime.syn["type"] = "int";
         )},

        {"relprime", "E",
         ACT(GET(relprime);
             relprime.syn["reg"] = relprime.inh["reg"];
             relprime.syn["type"] = relprime.inh["type"];
         )},

        {"relop", "<", ACT(GET(relop); relop.syn["op"] = "<";)},
        {"relop", ">", ACT(GET(relop); relop.syn["op"] = ">";)},
        {"relop", "<=", ACT(GET(relop); relop.syn["op"] = "<=";)},
        {"relop", ">=", ACT(GET(relop); relop.syn["op"] = ">=";)},
        {"relop", "==", ACT(GET(relop); relop.syn["op"] = "==";)},

        {"arithexpr", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr);
             arithexprprime.inh["reg"] = multexpr.syn["reg"];
             arithexprprime.inh["type"] = multexpr.syn["type"];
         ),
         "arithexprprime",
         ACT(GET(arithexpr); GET(arithexprprime);
             arithexpr.syn["reg"] = arithexprprime.syn["reg"];
             arithexpr.syn["type"] = arithexprprime.syn["type"];
         )},        {"arithexprprime", "+", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = arithexprprime.inh["reg"];
             std::string right_reg = multexpr.syn["reg"];
             std::string left_type = arithexprprime.inh["type"];
             std::string right_type = multexpr.syn["type"];
             
             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_add(env, result_reg, left_reg, right_reg, result_type);

             // 为下一个arithexprprime设置继承属性
             GETI(arithexprprime, 1);
             arithexprprime_1.inh["reg"] = result_reg;
             arithexprprime_1.inh["type"] = result_type;
         ),
         "arithexprprime",
         ACT(GET(arithexprprime); GETI(arithexprprime, 1);
             arithexprprime.syn["reg"] = arithexprprime_1.syn["reg"];
             arithexprprime.syn["type"] = arithexprprime_1.syn["type"];
         )},        {"arithexprprime", "-", "multexpr",
         ACT(GET(arithexprprime); GET(multexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = arithexprprime.inh["reg"];
             std::string right_reg = multexpr.syn["reg"];
             std::string left_type = arithexprprime.inh["type"];
             std::string right_type = multexpr.syn["type"];
             
             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_sub(env, result_reg, left_reg, right_reg, result_type);
             
             // 为下一个arithexprprime设置继承属性
             GETI(arithexprprime, 1);
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
         )},        {"multexprprime", "*", "simpleexpr",
         ACT(GET(multexprprime); GET(simpleexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = simpleexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = simpleexpr.syn["type"];
             
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_mul(env, result_reg, left_reg, right_reg, result_type);
             
             // 为下一个multexprprime设置继承属性
             GETI(multexprprime, 1);
             multexprprime_1.inh["reg"] = result_reg;
             multexprprime_1.inh["type"] = result_type;
         ),
         "multexprprime",
         ACT(GET(multexprprime); GETI(multexprprime, 1);
             multexprprime.syn["reg"] = multexprprime_1.syn["reg"];
             multexprprime.syn["type"] = multexprprime_1.syn["type"];
         )},        {"multexprprime", "/", "simpleexpr",
         ACT(GET(multexprprime); GET(simpleexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = simpleexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = simpleexpr.syn["type"];
             
             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_div(env, result_reg, left_reg, right_reg, result_type);
             
             // 为下一个multexprprime设置继承属性
             GETI(multexprprime, 1);
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
         )},        {"simpleexpr", "ID",
         ACT(GET(simpleexpr); GET(ID);
             auto table_entry = env.table.lookup(ID.lexval);
             if (table_entry == nullptr) {
                 env.error(ID.lexval + " is not defined");
                 return;
             }
             std::string result_reg = "%" + env.temp();
             std::string var_name = table_entry->at("llvm_name");
             std::string var_type = table_entry->at("type");

             emit_load(env, result_reg, var_name, var_type);
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

        {"simpleexpr", "DOUBLENUM",
         ACT(GET(simpleexpr); GET(DOUBLENUM);
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = fadd double 0.0, " + DOUBLENUM.lexval);
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "double";
         )},        {"simpleexpr", "(", "expr", ")",
         ACT(GET(simpleexpr); GET(expr);
             simpleexpr.syn["reg"] = expr.syn["reg"];
             simpleexpr.syn["type"] = expr.syn["type"];
         )},        // 函数调用语句
        {"callstmt", "ID", "(", "arglist", ")", ";",
         ACT(GET(ID); GET(arglist);
             if (ID.lexval == "printf") {
                 std::string format_str = arglist.syn["format_str"];
                 if (!format_str.empty()) {
                     // 处理格式字符串
                     std::string clean_str = format_str.substr(1, format_str.length() - 2);
                     
                     // 处理转义序列
                     size_t pos = 0;
                     while ((pos = clean_str.find("\\n", pos)) != std::string::npos) {
                         clean_str.replace(pos, 2, "\\0A");
                         pos += 3;
                     }
                     
                     // 创建全局字符串常量
                     std::string str_label = ".str" + env.temp();
                     std::string str_size = std::to_string(clean_str.length() + 1);
                     
                     // 输出全局字符串声明作为注释（实际应该在程序开头）
                     env.emit("  ; Global: @" + str_label + " = private unnamed_addr constant [" + 
                             str_size + " x i8] c\"" + clean_str + "\\00\", align 1");
                     
                     // 构建printf调用参数
                     std::string printf_args = "i8* getelementptr inbounds ([" + str_size + " x i8], [" + 
                                              str_size + " x i8]* @" + str_label + ", i64 0, i64 0)";
                     
                     // 添加额外参数
                     if (!arglist.syn["arg_count"].empty() && arglist.syn["arg_count"] != "0") {
                         std::string arg_regs = arglist.syn["arg_regs"];
                         std::string arg_types = arglist.syn["arg_types"];
                         
                         // 解析参数
                         std::vector<std::string> regs, types;
                         std::stringstream reg_ss(arg_regs), type_ss(arg_types);
                         std::string item;
                         
                         while (std::getline(reg_ss, item, ',')) {
                             regs.push_back(item);
                         }
                         while (std::getline(type_ss, item, ',')) {
                             types.push_back(item);
                         }
                         
                         // 添加参数到printf调用
                         for (size_t i = 0; i < regs.size() && i < types.size(); ++i) {
                             if (types[i] == "int") {
                                 printf_args += ", i32 " + regs[i];
                             } else if (types[i] == "double") {
                                 printf_args += ", double " + regs[i];
                             }
                         }
                     }
                     
                     // 生成printf调用
                     std::string result_reg = "%" + env.temp();
                     env.emit("  " + result_reg + " = call i32 (i8*, ...) @printf(" + printf_args + ")");
                 }
             } else {
                 env.error("Unsupported function: " + ID.lexval);
             }
         )},// 参数列表处理 - 简化版本
        {"arglist", "STRING",
         ACT(GET(arglist); GET(STRING);
             arglist.syn["format_str"] = STRING.lexval;
             arglist.syn["arg_count"] = "0";
         )},
         
        {"arglist", "STRING", ",", "exprlist",
         ACT(GET(arglist); GET(STRING); GET(exprlist);
             arglist.syn["format_str"] = STRING.lexval;
             arglist.syn["arg_count"] = exprlist.syn["count"];
             arglist.syn["arg_regs"] = exprlist.syn["regs"];
             arglist.syn["arg_types"] = exprlist.syn["types"];
         )},
         
        // 表达式列表
        {"exprlist", "expr",
         ACT(GET(exprlist); GET(expr);
             exprlist.syn["count"] = "1";
             exprlist.syn["regs"] = expr.syn["reg"];
             exprlist.syn["types"] = expr.syn["type"];
         )},
         
        {"exprlist", "expr", ",", "exprlist",
         ACT(GET(exprlist); GET(expr); GETI(exprlist, 1);
             int count = std::stoi(exprlist_1.syn["count"]) + 1;
             exprlist.syn["count"] = std::to_string(count);
             exprlist.syn["regs"] = expr.syn["reg"] + "," + exprlist_1.syn["regs"];
             exprlist.syn["types"] = expr.syn["type"] + "," + exprlist_1.syn["types"];
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

    std::vector<lexer::token> tokens;
    auto filtered = lex.parse(input) | std::views::filter([](const lexer::token& t) {
                                                 return t.type != static_cast<int>(token_type::WHITESPACE) && t.type != static_cast<int>(token_type::COMMENT);
                                             });
    for (auto& token : filtered) {
        tokens.emplace_back(token);
    }

    using gram_t = grammar::LR1;
    auto gram = semantic::sema<gram_t>(build_grammar()
        ,out
        );
    gram.init_error_handlers([](gram_t::action_table_t& action_table, gram_t::goto_table_t&, std::vector<gram_t::error_handle_fn>& error_handlers) {
    });
    gram.build();
    gram.parse(tokens);

    const auto tree = std::static_pointer_cast<semantic::sema_tree>(gram.get_tree());
    
    const auto env = tree->calc();
    // tree->print();

    for (const auto& error : env.errors) {
        std::cout << "error message:" << error << '\n';
    }
}
