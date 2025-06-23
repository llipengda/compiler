#include "build_grammar.hpp"
#include "semantic/sema_production.hpp"
#include "build_lexer.hpp"
#include "helper.hpp"
#include <sstream>

std::vector<semantic::sema_production> build_grammar() {
    grammar::set_epsilon_str("E");
    grammar::set_end_mark_str("$");
    grammar::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });

    const std::vector<semantic::sema_production> prods{
        // 程序入口
        {"program", "int", "ID", "(", ")",
         ACT(
             env.emit("; ModuleID = 'main'");
             env.emit("");

             for (const auto& [content, var_name] : strings) {
                 size_t str_len = content.length() + 1; // +1 for null terminator
                 env.emit("@" + var_name + " = private unnamed_addr constant ["
                          + std::to_string(str_len) + " x i8] c" + to_llvmstr(content) + ", align 1");
             }

             env.emit("");
             env.emit("declare i32 @printf(i8*, ...)");
             env.emit("declare i32 @scanf(i8*, ...)");
             env.emit("");
             env.emit("define i32 @main() {");
             env.emit("entry:");
         ),
         "compoundstmt",
         ACT(
             env.emit("  ret i32 0");
             env.emit("}");
         )
        },

        // 声明语句
        {"declstmt", "decl", ";"},

        // 类型定义
        {"type", "int",
         ACT(
             GET(type);
             type.syn["type"] = "int";
         )
        },
        {"type", "double",
         ACT(
             GET(type);
             type.syn["type"] = "double";
         )
        },
        {"type", "long",
         ACT(
             GET(type);
             type.syn["type"] = "long";
         )
        },

        // 带初始化的声明
        {"decl", "type", "ID", "=", "expr",
         ACT(
             GET(type);
             GET(ID);
             GET(expr);
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
         )
        },

        // 不带初始化的声明
        {"decl", "type", "ID",
         ACT(
             GET(type);
             GET(ID);
             std::string var_type = type.syn["type"];
             // 使用唯一的变量名
             std::string unique_var_name = "%" + ID.lexval + "_" + env.temp();
             env.table.insert(ID.lexval, {{"type", var_type}, {"llvm_name", unique_var_name}});

             // 只分配内存，不进行初始化
             emit_alloca(env, unique_var_name, var_type);
         )
        },

        // 语句类型
        {"stmt", "ifstmt"},
        {"stmt", "forstmt"},
        {"stmt", "whilestmt"},
        {"stmt", "assgstmt"},
        {"stmt", "compoundstmt"},
        {"stmt", "declstmt"},
        {"stmt", "callstmt"},
        {"stmt", "emptystmt"},

        {"emptystmt", ";"},

        // 复合语句
        {"compoundstmt", "{",
         ACT(env.table.enter_scope();),
         "stmts",
         "}",
         ACT(env.table.exit_scope();)
        },

        {"stmts", "stmt", "stmts"},
        {"stmts", "E"},

        // 带else的if语句
        {"ifstmt", "if", "(", "expr", ")",
         ACT(
             GET(expr);
             GET(stmt);
             GETI(stmt, 1);
             std::string cond_label = env.label();
             std::string then_label = env.label();
             std::string else_label = env.label();
             std::string end_label = env.label();

             stmt.inh["else"] = else_label;
             stmt.inh["end"] = end_label;
             stmt_1.inh["end"] = end_label;

             auto cond = convert_to_i1(env, expr.syn["reg"], expr.syn["type"]);

             env.emit("  br i1 " + cond + ", label %" + then_label + ", label %" + else_label);
             env.emit(then_label + ":");
         ),
         "stmt",
         ACT(
             GET(stmt);
             std::string else_label = stmt.inh["else"];
             std::string end_label = stmt.inh["end"];
             env.emit("  br label %" + end_label);
             env.emit(else_label + ":");
         ),
         "else",
         "stmt",
         ACT(
             GETI(stmt, 1);
             std::string end_label = stmt_1.inh["end"];
             env.emit("  br label %" + end_label);
             env.emit(end_label + ":");
         )
        },

        // 不带else的if语句
        {"ifstmt", "if", "(", "expr", ")",
         ACT(
             GET(expr);
             GET(stmt);
             std::string then_label = env.label();
             std::string end_label = env.label();

             stmt.inh["end"] = end_label;

             auto cond = convert_to_i1(env, expr.syn["reg"], expr.syn["type"]);

             env.emit("  br i1 " + cond + ", label %" + then_label + ", label %" + end_label);
             env.emit(then_label + ":");
         ),
         "stmt",
         ACT(
             GET(stmt);
             std::string end_label = stmt.inh["end"];
             env.emit("  br label %" + end_label);
             env.emit(end_label + ":");
         )
        },

        // for循环语句
        {"forstmt", "for", "(",
         ACT(
             GET(stmt);
             GET(forinit);
             GET(expr);
             std::string cond_label = env.label();
             std::string body_label = env.label();
             std::string update_label = env.label();
             std::string end_label = env.label();

             stmt.inh["update_label"] = update_label;
             stmt.inh["end_label"] = end_label;
             stmt.inh["cond_label"] = cond_label;
             stmt.inh["body_label"] = body_label;
             forinit.inh["cond_label"] = cond_label;
             expr.inh["end_label"] = end_label;
             expr.inh["body_label"] = body_label;
             expr.inh["update_label"] = update_label;

             env.table.enter_scope();
         ),
         "forinit",
         ACT(
             GET(forinit);
             auto cond_label = forinit.inh["cond_label"];

             env.emit("  br label %" + cond_label);
             env.emit(cond_label + ":");
         ),
         "expr", ";",
         ACT(
             GET(expr);
             auto body_label = expr.inh["body_label"];
             auto end_label = expr.inh["end_label"];
             auto update_label = expr.inh["update_label"];

             // test expr
             auto cond = convert_to_i1(env, expr.syn["reg"], expr.syn["type"]);
             env.emit("  br i1 " + cond + ", label %" + body_label + ", label %" + end_label);

             env.emit(update_label + ":");
         ),
         "forupdate", ")",
         ACT(
             GET(stmt);
             auto cond_label = stmt.inh["cond_label"];
             auto body_label = stmt.inh["body_label"];

             env.emit("  br label %" + cond_label);

             env.emit(body_label + ":");
         ),
         "stmt",
         ACT(
             GET(stmt);
             std::string update_label = stmt.inh["update_label"];
             std::string end_label = stmt.inh["end_label"];

             // 跳转到更新部分
             env.emit("  br label %" + update_label);
             env.emit(end_label + ":");

             env.table.exit_scope();
         )
        },

        {"forinit", ";"},
        {"forinit", "decl", ";"},
        {"forinit", "assgstmt"},

        // for语句更新部分
        {"forupdate", "E"}, // 空更新
        {"forupdate", "ID", "=", "expr",
         ACT(
             GET(ID);
             GET(expr);
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
         )
        },

        // while循环语句
        {"whilestmt", "while",
         ACT(
             GET(whilestmt);
             auto cond_label = env.label();
             auto body_label = env.label();
             auto end_label = env.label();

             whilestmt.syn["cond_label"] = cond_label;
             whilestmt.syn["body_label"] = body_label;
             whilestmt.syn["end_label"] = end_label;

             env.emit("  br label %" + cond_label);
             env.emit(cond_label + ":");
         ),
         "(", "expr", ")",
         ACT(
             GET(expr);
             GET(whilestmt);
             auto body_label = whilestmt.syn["body_label"];
             auto end_label = whilestmt.syn["end_label"];

             auto cond = convert_to_i1(env, expr.syn["reg"], expr.syn["type"]);
             env.emit("  br i1 " + cond + ", label %" + body_label + ", label %" + end_label);

             env.emit(body_label + ":");
         ),
         "stmt",
         ACT(
             GET(whilestmt);
             std::string cond_label = whilestmt.syn["cond_label"];
             std::string end_label = whilestmt.syn["end_label"];

             // 循环体执行完后跳回条件检查
             env.emit("  br label %" + cond_label);
             env.emit(end_label + ":");
         )
        },

        // 赋值语句
        {"assgstmt", "ID", "=", "expr", ";",
         ACT(
             GET(ID);
             GET(expr);
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
         )
        },

        // 表达式层次（按优先级从低到高）
        // expr -> logorexpr
        {"expr", "logorexpr",
         ACT(
             GET(expr);
             GET(logorexpr);
             expr.syn["reg"] = logorexpr.syn["reg"];
             expr.syn["type"] = logorexpr.syn["type"];
         )
        },

        // logorexpr -> logandexpr ( "||" logandexpr )*
        {"logorexpr", "logandexpr",
         ACT(
             GET(logandexpr);
             GET(logorprime);
             logorprime.inh["reg"] = logandexpr.syn["reg"];
             logorprime.inh["type"] = logandexpr.syn["type"];
         ),
         "logorprime",
         ACT(
             GET(logorexpr);
             GET(logorprime);
             logorexpr.syn["reg"] = logorprime.syn["reg"];
             logorexpr.syn["type"] = logorprime.syn["type"];
         )
        },

        {"logorprime", "||", "logandexpr",
         ACT(
             GET(logorprime);
             GET(logandexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = logorprime.inh["reg"];
             std::string right_reg = logandexpr.syn["reg"];
             std::string left_type = logorprime.inh["type"];
             std::string right_type = logandexpr.syn["type"];

             // 将操作数转换为i1类型
             std::string left_i1 = convert_to_i1(env, left_reg, left_type);
             std::string right_i1 = convert_to_i1(env, right_reg, right_type);

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
         ACT(
             GET(logorprime);
             GETI(logorprime, 1);
             logorprime.syn["reg"] = logorprime_1.syn["reg"];
             logorprime.syn["type"] = logorprime_1.syn["type"];
         )
        },

        {"logorprime", "E",
         ACT(
             GET(logorprime);
             logorprime.syn["reg"] = logorprime.inh["reg"];
             logorprime.syn["type"] = logorprime.inh["type"];
         )
        },

        // logandexpr -> bitorexpr ( "&&" bitorexpr )*
        {"logandexpr", "bitorexpr",
         ACT(
             GET(bitorexpr);
             GET(logandprime);
             logandprime.inh["reg"] = bitorexpr.syn["reg"];
             logandprime.inh["type"] = bitorexpr.syn["type"];
         ),
         "logandprime",
         ACT(
             GET(logandexpr);
             GET(logandprime);
             logandexpr.syn["reg"] = logandprime.syn["reg"];
             logandexpr.syn["type"] = logandprime.syn["type"];
         )
        },

        {"logandprime", "&&", "bitorexpr",
         ACT(
             GET(logandprime);
             GET(bitorexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = logandprime.inh["reg"];
             std::string right_reg = bitorexpr.syn["reg"];
             std::string left_type = logandprime.inh["type"];
             std::string right_type = bitorexpr.syn["type"];

             // 将操作数转换为i1类型
             std::string left_i1 = convert_to_i1(env, left_reg, left_type);
             std::string right_i1 = convert_to_i1(env, right_reg, right_type);

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
         ACT(
             GET(logandprime);
             GETI(logandprime, 1);
             logandprime.syn["reg"] = logandprime_1.syn["reg"];
             logandprime.syn["type"] = logandprime_1.syn["type"];
         )
        },

        {"logandprime", "E",
         ACT(
             GET(logandprime);
             logandprime.syn["reg"] = logandprime.inh["reg"];
             logandprime.syn["type"] = logandprime.inh["type"];
         )
        },

        // bitorexpr -> bitxorexpr ( "|" bitxorexpr )*
        {"bitorexpr", "bitxorexpr",
         ACT(
             GET(bitxorexpr);
             GET(bitorprime);
             bitorprime.inh["reg"] = bitxorexpr.syn["reg"];
             bitorprime.inh["type"] = bitxorexpr.syn["type"];
         ),
         "bitorprime",
         ACT(
             GET(bitorexpr);
             GET(bitorprime);
             bitorexpr.syn["reg"] = bitorprime.syn["reg"];
             bitorexpr.syn["type"] = bitorprime.syn["type"];
         )
        },

        {"bitorprime", "|", "bitxorexpr",
         ACT(
             GET(bitorprime);
             GET(bitxorexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitorprime.inh["reg"];
             std::string right_reg = bitxorexpr.syn["reg"];
             std::string left_type = bitorprime.inh["type"];
             std::string right_type = bitxorexpr.syn["type"];

             // 转换为相同的整数类型进行位运算
             std::string result_type = (left_type == "long" || right_type == "long") ? "long" : "int";
             left_reg = convert_type(env, left_reg, left_type, result_type);
             right_reg = convert_type(env, right_reg, right_type, result_type);

             emit_bitor(env, result_reg, left_reg, right_reg, result_type);

             GETI(bitorprime, 1);
             bitorprime_1.inh["reg"] = result_reg;
             bitorprime_1.inh["type"] = result_type;
         ),
         "bitorprime",
         ACT(
             GET(bitorprime);
             GETI(bitorprime, 1);
             bitorprime.syn["reg"] = bitorprime_1.syn["reg"];
             bitorprime.syn["type"] = bitorprime_1.syn["type"];
         )
        },

        {"bitorprime", "E",
         ACT(
             GET(bitorprime);
             bitorprime.syn["reg"] = bitorprime.inh["reg"];
             bitorprime.syn["type"] = bitorprime.inh["type"];
         )
        },

        // bitxorexpr -> bitandexpr ( "^" bitandexpr )*
        {"bitxorexpr", "bitandexpr",
         ACT(
             GET(bitandexpr);
             GET(bitxorprime);
             bitxorprime.inh["reg"] = bitandexpr.syn["reg"];
             bitxorprime.inh["type"] = bitandexpr.syn["type"];
         ),
         "bitxorprime",
         ACT(
             GET(bitxorexpr);
             GET(bitxorprime);
             bitxorexpr.syn["reg"] = bitxorprime.syn["reg"];
             bitxorexpr.syn["type"] = bitxorprime.syn["type"];
         )
        },

        {"bitxorprime", "^", "bitandexpr",
         ACT(
             GET(bitxorprime);
             GET(bitandexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitxorprime.inh["reg"];
             std::string right_reg = bitandexpr.syn["reg"];
             std::string left_type = bitxorprime.inh["type"];
             std::string right_type = bitandexpr.syn["type"];

             // 转换为相同的整数类型进行位运算
             std::string result_type = (left_type == "long" || right_type == "long") ? "long" : "int";
             left_reg = convert_type(env, left_reg, left_type, result_type);
             right_reg = convert_type(env, right_reg, right_type, result_type);

             emit_bitxor(env, result_reg, left_reg, right_reg, result_type);

             // 为下一个bitxorprime设置继承属性
             GETI(bitxorprime, 1);
             bitxorprime_1.inh["reg"] = result_reg;
             bitxorprime_1.inh["type"] = result_type;
         ),
         "bitxorprime",
         ACT(
             GET(bitxorprime);
             GETI(bitxorprime, 1);
             bitxorprime.syn["reg"] = bitxorprime_1.syn["reg"];
             bitxorprime.syn["type"] = bitxorprime_1.syn["type"];
         )
        },

        {"bitxorprime", "E",
         ACT(
             GET(bitxorprime);
             bitxorprime.syn["reg"] = bitxorprime.inh["reg"];
             bitxorprime.syn["type"] = bitxorprime.inh["type"];
         )
        },

        // bitandexpr -> relexpr ( "&" relexpr )*
        {"bitandexpr", "relexpr",
         ACT(
             GET(relexpr);
             GET(bitandprime);
             bitandprime.inh["reg"] = relexpr.syn["reg"];
             bitandprime.inh["type"] = relexpr.syn["type"];
         ),
         "bitandprime",
         ACT(
             GET(bitandexpr);
             GET(bitandprime);
             bitandexpr.syn["reg"] = bitandprime.syn["reg"];
             bitandexpr.syn["type"] = bitandprime.syn["type"];
         )
        },

        {"bitandprime", "&", "relexpr",
         ACT(
             GET(bitandprime);
             GET(relexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = bitandprime.inh["reg"];
             std::string right_reg = relexpr.syn["reg"];
             std::string left_type = bitandprime.inh["type"];
             std::string right_type = relexpr.syn["type"];

             // 转换为相同的整数类型进行位运算
             std::string result_type = (left_type == "long" || right_type == "long") ? "long" : "int";
             left_reg = convert_type(env, left_reg, left_type, result_type);
             right_reg = convert_type(env, right_reg, right_type, result_type);

             emit_bitand(env, result_reg, left_reg, right_reg, result_type);

             // 为下一个bitandprime设置继承属性
             GETI(bitandprime, 1);
             bitandprime_1.inh["reg"] = result_reg;
             bitandprime_1.inh["type"] = result_type;
         ),
         "bitandprime",
         ACT(
             GET(bitandprime);
             GETI(bitandprime, 1);
             bitandprime.syn["reg"] = bitandprime_1.syn["reg"];
             bitandprime.syn["type"] = bitandprime_1.syn["type"];
         )
        },

        {"bitandprime", "E",
         ACT(
             GET(bitandprime);
             bitandprime.syn["reg"] = bitandprime.inh["reg"];
             bitandprime.syn["type"] = bitandprime.inh["type"];
         )
        },

        // relexpr -> arithexpr ( relop arithexpr )?
        {"relexpr", "arithexpr",
         ACT(
             GET(arithexpr);
             GET(relprime);
             relprime.inh["reg"] = arithexpr.syn["reg"];
             relprime.inh["type"] = arithexpr.syn["type"];
         ),
         "relprime",
         ACT(
             GET(relexpr);
             GET(relprime);
             relexpr.syn["reg"] = relprime.syn["reg"];
             relexpr.syn["type"] = relprime.syn["type"];
         )
        },

        {"relprime", "relop", "arithexpr",
         ACT(
             GET(relprime);
             GET(relop);
             GET(arithexpr);
             std::string result_reg = "%" + env.temp();
             std::string lhs_reg = relprime.inh["reg"];
             std::string rhs_reg = arithexpr.syn["reg"];
             std::string lhs_type = relprime.inh["type"];
             std::string rhs_type = arithexpr.syn["type"];
             std::string op = relop.syn["op"];

             std::string result_type = convert_operands(env, lhs_reg, rhs_reg, lhs_type, rhs_type);
             std::string cmp_op;
             if (op == "<")
                 cmp_op = "lt";
             else if (op == ">")
                 cmp_op = "gt";
             else if (op == "<=")
                 cmp_op = "le";
             else if (op == ">=")
                 cmp_op = "ge";
             else if (op == "==")
                 cmp_op = "eq";
             else if (op == "!=")
                 cmp_op = "ne";

             if (result_type == "int") {
                 if (cmp_op != "eq" && cmp_op != "ne") {
                     cmp_op = "s" + cmp_op;
                 }
                 env.emit("  " + result_reg + " = icmp " + cmp_op + " i32 " + lhs_reg + ", " + rhs_reg);
             } else if (result_type == "long") {
                 if (cmp_op != "eq" && cmp_op != "ne") {
                     cmp_op = "s" + cmp_op;
                 }
                 env.emit("  " + result_reg + " = icmp " + cmp_op + " i64 " + lhs_reg + ", " + rhs_reg);
             } else {
                 env.emit("  " + result_reg + " = fcmp o" + cmp_op + " double " + lhs_reg + ", " + rhs_reg);
             }

             // 将i1类型扩展为i32类型
             std::string final_reg = "%" + env.temp();
             env.emit("  " + final_reg + " = zext i1 " + result_reg + " to i32");

             relprime.syn["reg"] = final_reg;
             relprime.syn["type"] = "int";
         )
        },

        {"relprime", "E",
         ACT(
             GET(relprime);
             relprime.syn["reg"] = relprime.inh["reg"];
             relprime.syn["type"] = relprime.inh["type"];
         )
        },

        {"relop", "<", ACT(GET(relop); relop.syn["op"] = "<";)},
        {"relop", ">", ACT(GET(relop); relop.syn["op"] = ">";)},
        {"relop", "<=", ACT(GET(relop); relop.syn["op"] = "<=";)},
        {"relop", ">=", ACT(GET(relop); relop.syn["op"] = ">=";)},
        {"relop", "==", ACT(GET(relop); relop.syn["op"] = "==";)},
        {"relop", "!=", ACT(GET(relop); relop.syn["op"] = "!=";)},

        // 算术表达式规则
        {"arithexpr", "multexpr",
         ACT(
             GET(arithexprprime);
             GET(multexpr);
             arithexprprime.inh["reg"] = multexpr.syn["reg"];
             arithexprprime.inh["type"] = multexpr.syn["type"];
         ),
         "arithexprprime",
         ACT(
             GET(arithexpr);
             GET(arithexprprime);
             arithexpr.syn["reg"] = arithexprprime.syn["reg"];
             arithexpr.syn["type"] = arithexprprime.syn["type"];
         )
        },

        // 算术表达式递归 - 加法
        {"arithexprprime", "+", "multexpr",
         ACT(
             GET(arithexprprime);
             GET(multexpr);
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
         ACT(
             GET(arithexprprime);
             GETI(arithexprprime, 1);
             arithexprprime.syn["reg"] = arithexprprime_1.syn["reg"];
             arithexprprime.syn["type"] = arithexprprime_1.syn["type"];
         )
        },

        // 算术表达式递归 - 减法
        {"arithexprprime", "-", "multexpr",
         ACT(
             GET(arithexprprime);
             GET(multexpr);
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
         ACT(
             GET(arithexprprime);
             GETI(arithexprprime, 1);
             arithexprprime.syn["reg"] = arithexprprime_1.syn["reg"];
             arithexprprime.syn["type"] = arithexprprime_1.syn["type"];
         )
        },

        // 算术表达式递归 - 空产生式
        {"arithexprprime", "E",
         ACT(
             GET(arithexprprime);
             arithexprprime.syn["reg"] = arithexprprime.inh["reg"];
             arithexprprime.syn["type"] = arithexprprime.inh["type"];
         )
        },

        // 乘法表达式
        {"multexpr", "unaryexpr",
         ACT(
             GET(multexprprime);
             GET(unaryexpr);
             multexprprime.inh["reg"] = unaryexpr.syn["reg"];
             multexprprime.inh["type"] = unaryexpr.syn["type"];
         ),
         "multexprprime",
         ACT(
             GET(multexpr);
             GET(multexprprime);
             multexpr.syn["reg"] = multexprprime.syn["reg"];
             multexpr.syn["type"] = multexprprime.syn["type"];
         )
        },

        // 乘法表达式递归 - 乘法
        {"multexprprime", "*", "unaryexpr",
         ACT(
             GET(multexprprime);
             GET(unaryexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = unaryexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = unaryexpr.syn["type"];

             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_mul(env, result_reg, left_reg, right_reg, result_type);

             // 为下一个multexprprime设置继承属性
             GETI(multexprprime, 1);
             multexprprime_1.inh["reg"] = result_reg;
             multexprprime_1.inh["type"] = result_type;
         ),
         "multexprprime",
         ACT(
             GET(multexprprime);
             GETI(multexprprime, 1);
             multexprprime.syn["reg"] = multexprprime_1.syn["reg"];
             multexprprime.syn["type"] = multexprprime_1.syn["type"];
         )
        },

        // 乘法表达式递归 - 除法
        {"multexprprime", "/", "unaryexpr",
         ACT(
             GET(multexprprime);
             GET(unaryexpr);
             std::string result_reg = "%" + env.temp();
             std::string left_reg = multexprprime.inh["reg"];
             std::string right_reg = unaryexpr.syn["reg"];
             std::string left_type = multexprprime.inh["type"];
             std::string right_type = unaryexpr.syn["type"];

             // 使用通用操作数转换函数
             std::string result_type = convert_operands(env, left_reg, right_reg, left_type, right_type);

             emit_div(env, result_reg, left_reg, right_reg, result_type);

             // 为下一个multexprprime设置继承属性
             GETI(multexprprime, 1);
             multexprprime_1.inh["reg"] = result_reg;
             multexprprime_1.inh["type"] = result_type;
         ),
         "multexprprime",
         ACT(
             GET(multexprprime);
             GETI(multexprprime, 1);
             multexprprime.syn["reg"] = multexprprime_1.syn["reg"];
             multexprprime.syn["type"] = multexprprime_1.syn["type"];
         )
        },

        // 乘法表达式递归 - 空产生式
        {"multexprprime", "E",
         ACT(
             GET(multexprprime);
             multexprprime.syn["reg"] = multexprprime.inh["reg"];
             multexprprime.syn["type"] = multexprprime.inh["type"];
         )
        },

        // 一元表达式 - 简单表达式
        {"unaryexpr", "simpleexpr",
         ACT(
             GET(unaryexpr);
             GET(simpleexpr);
             unaryexpr.syn["reg"] = simpleexpr.syn["reg"];
             unaryexpr.syn["type"] = simpleexpr.syn["type"];
         )
        },

        // 一元负号运算符
        {"unaryexpr", "-", "unaryexpr",
         ACT(
             GET(unaryexpr);
             GETI(unaryexpr, 1);
             std::string operand_reg = unaryexpr_1.syn["reg"];
             std::string operand_type = unaryexpr_1.syn["type"];
             std::string result_reg = "%" + env.temp();

             // 一元负号：根据类型生成相应的LLVM IR
             if (operand_type == "int") {
                 env.emit("  " + result_reg + " = sub nsw i32 0, " + operand_reg);
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "int";
             } else if (operand_type == "long") {
                 env.emit("  " + result_reg + " = sub nsw i64 0, " + operand_reg);
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "long";
             } else if (operand_type == "double") {
                 env.emit("  " + result_reg + " = fsub double 0.0, " + operand_reg);
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "double";
             } else {
                 env.error("Unary minus (-) can only be applied to numeric types");
                 return;
             }
         )
        },

        // 逻辑取反运算符
        {"unaryexpr", "!", "unaryexpr",
         ACT(
             GET(unaryexpr);
             GETI(unaryexpr, 1);
             std::string operand_reg = unaryexpr_1.syn["reg"];
             std::string operand_type = unaryexpr_1.syn["type"];
             std::string result_reg = "%" + env.temp();

             // 逻辑取反：先转换为i1，然后取反，再扩展为i32
             std::string i1_reg = convert_to_i1(env, operand_reg, operand_type);
             std::string not_reg = "%" + env.temp();
             env.emit("  " + not_reg + " = xor i1 " + i1_reg + ", true");
             env.emit("  " + result_reg + " = zext i1 " + not_reg + " to i32");

             unaryexpr.syn["reg"] = result_reg;
             unaryexpr.syn["type"] = "int";
         )
        },

        // 按位取反运算符
        {"unaryexpr", "~", "unaryexpr",
         ACT(
             GET(unaryexpr);
             GETI(unaryexpr, 1);
             std::string operand_reg = unaryexpr_1.syn["reg"];
             std::string operand_type = unaryexpr_1.syn["type"];
             std::string result_reg = "%" + env.temp();

             // 按位取反，支持整数类型
             if (operand_type == "int") {
                 env.emit("  " + result_reg + " = xor i32 " + operand_reg + ", -1");
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "int";
             } else if (operand_type == "long") {
                 env.emit("  " + result_reg + " = xor i64 " + operand_reg + ", -1");
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "long";
             } else {
                 env.error("Bitwise NOT (~) can only be applied to integer types");
                 return;
             }
         )
        },

        // 取地址运算符
        {"unaryexpr", "&", "simpleexpr",
         ACT(
             GET(unaryexpr);
             GET(simpleexpr);

             // 取地址操作符：只能应用于变量（左值）
             // 检查simpleexpr是否为变量
             if (simpleexpr.syn["is_var"] == "true") {
                 std::string var_name = simpleexpr.syn["var_name"];
                 std::string var_type = simpleexpr.syn["var_type"];
                 std::string result_reg = "%" + env.temp();

                 // 返回变量的地址（alloca指令返回的指针）
                 // 根据变量类型使用正确的指针类型，转换为i64
                 if (var_type == "int") {
                     env.emit("  " + result_reg + " = ptrtoint i32* " + var_name + " to i64");
                 } else if (var_type == "double") {
                     env.emit("  " + result_reg + " = ptrtoint double* " + var_name + " to i64");
                 } else if (var_type == "long") {
                     env.emit("  " + result_reg + " = ptrtoint i64* " + var_name + " to i64");
                 }

                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "long"; // 地址表示为64位整数
             } else {
                 env.error("Address-of operator (&) can only be applied to variables");
                 std::string result_reg = "%" + env.temp();
                 env.emit("  " + result_reg + " = add i64 0, 0  ; error placeholder");
                 unaryexpr.syn["reg"] = result_reg;
                 unaryexpr.syn["type"] = "long";
             }
         )
        },

        // 简单表达式 - 标识符
        {"simpleexpr", "ID",
         ACT(
             GET(simpleexpr);
             GET(ID);
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

             // 添加变量信息，用于取地址操作
             simpleexpr.syn["is_var"] = "true";
             simpleexpr.syn["var_name"] = var_name;
             simpleexpr.syn["var_type"] = var_type; // 添加变量类型信息
         )
        },

        // 简单表达式 - 整数字面量
        {"simpleexpr", "INTNUM",
         ACT(
             GET(simpleexpr);
             GET(INTNUM);
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = add i32 0, " + INTNUM.lexval);
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "int";
         )
        },

        // 简单表达式 - 浮点数字面量
        {"simpleexpr", "DOUBLENUM",
         ACT(
             GET(simpleexpr);
             GET(DOUBLENUM);
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = fadd double 0.0, " + DOUBLENUM.lexval);
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "double";
         )
        },

        // 简单表达式 - 字符串字面量
        {"simpleexpr", "STRING",
         ACT(
             GET(simpleexpr);
             GET(STRING);

             // 处理字符串字面量
             std::string content = process_string_literal(STRING.lexval);
             std::string str_label = strings[content];
             std::string str_size = std::to_string(content.length() + 1);

             // 创建指向字符串的指针
             std::string result_reg = "%" + env.temp();
             env.emit("  " + result_reg + " = getelementptr inbounds [" + str_size + " x i8], [" + str_size + " x i8]* @" + str_label + ", i64 0, i64 0");
             simpleexpr.syn["reg"] = result_reg;
             simpleexpr.syn["type"] = "string";
         )
        },

        // 简单表达式 - 括号表达式
        {"simpleexpr", "(", "expr", ")",
         ACT(
             GET(simpleexpr);
             GET(expr);
             simpleexpr.syn["reg"] = expr.syn["reg"];
             simpleexpr.syn["type"] = expr.syn["type"];
         )
        },

        // 函数调用语句
        {"callstmt", "ID", "(", "arglist", ")", ";",
         ACT(
             GET(ID);
             GET(arglist);

             if (ID.lexval == "printf") {
                 // 获取参数列表信息
                 int arg_count = std::stoi(arglist.syn["count"]);
                 std::string arg_regs = arglist.syn["regs"];
                 std::string arg_types = arglist.syn["types"];

                 if (arg_count > 0) {
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

                     // 构建printf调用参数
                     std::string printf_args;

                     // 第一个参数必须是字符串（格式字符串）
                     if (types[0] == "string") {
                         printf_args = "i8* " + regs[0];
                     } else {
                         env.error("printf first argument must be a string");
                         return;
                     }

                     // 添加其他参数
                     for (size_t i = 1; i < regs.size() && i < types.size(); ++i) {
                         if (types[i] == "int") {
                             printf_args += ", i32 " + regs[i];
                         } else if (types[i] == "double") {
                             printf_args += ", double " + regs[i];
                         } else if (types[i] == "string") {
                             printf_args += ", i8* " + regs[i];
                         } else if (types[i] == "long") {
                             printf_args += ", i64 " + regs[i];
                         }
                     }

                     // 生成printf调用
                     std::string result_reg = "%" + env.temp();
                     env.emit("  " + result_reg + " = call i32 (i8*, ...) @printf(" + printf_args + ")");
                 }
             } else if (ID.lexval == "scanf") {
                 // 获取参数列表信息
                 int arg_count = std::stoi(arglist.syn["count"]);
                 std::string arg_regs = arglist.syn["regs"];
                 std::string arg_types = arglist.syn["types"];

                 if (arg_count > 0) {
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

                     // 构建scanf调用参数
                     std::string scanf_args;

                     // 第一个参数必须是字符串（格式字符串）
                     if (types[0] == "string") {
                         scanf_args = "i8* " + regs[0];
                     } else {
                         env.error("scanf first argument must be a string");
                         return;
                     }

                     // 添加其他参数 - scanf需要变量地址
                     for (size_t i = 1; i < regs.size() && i < types.size(); ++i) {
                         if (types[i] == "long") {
                             // 对于scanf，我们需要传递地址，而不是值
                             // 这里假设传入的是地址类型（通过&运算符获得）
                             // 我们需要知道原始变量的类型来确定指针类型
                             // 由于当前实现的限制，我们假设long类型的值是int变量的地址
                             scanf_args += ", i32* ";
                             // 将i64地址转换回int指针
                             std::string ptr_reg = "%" + env.temp();
                             env.emit("  " + ptr_reg + " = inttoptr i64 " + regs[i] + " to i32*");
                             scanf_args += ptr_reg;
                         } else {
                             env.error("scanf arguments must be addresses (use & operator). Got type: " + types[i]);
                         }
                     }

                     // 生成scanf调用
                     std::string result_reg = "%" + env.temp();
                     env.emit("  " + result_reg + " = call i32 (i8*, ...) @scanf(" + scanf_args + ")");
                 }
             } else {
                 env.error("Unsupported function: " + ID.lexval);
             }
         )
        },

        // 参数列表 - 空
        {"arglist", "E",
         ACT(
             GET(arglist);
             arglist.syn["count"] = "0";
             arglist.syn["regs"] = "";
             arglist.syn["types"] = "";
         )
        },

        // 参数列表 - 非空
        {"arglist", "exprlist",
         ACT(
             GET(arglist);
             GET(exprlist);
             arglist.syn["count"] = exprlist.syn["count"];
             arglist.syn["regs"] = exprlist.syn["regs"];
             arglist.syn["types"] = exprlist.syn["types"];
         )
        },

        // 表达式列表 - 单个表达式
        {"exprlist", "expr",
         ACT(
             GET(exprlist);
             GET(expr);
             exprlist.syn["count"] = "1";
             exprlist.syn["regs"] = expr.syn["reg"];
             exprlist.syn["types"] = expr.syn["type"];
         )
        },

        // 表达式列表 - 多个表达式
        {"exprlist", "expr", ",", "exprlist",
         ACT(
             GET(exprlist);
             GET(expr);
             GETI(exprlist, 1);
             int count = std::stoi(exprlist_1.syn["count"]) + 1;
             exprlist.syn["count"] = std::to_string(count);
             exprlist.syn["regs"] = expr.syn["reg"] + "," + exprlist_1.syn["regs"];
             exprlist.syn["types"] = expr.syn["type"] + "," + exprlist_1.syn["types"];
         )
        }
    };

    return prods;
};