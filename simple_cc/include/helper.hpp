#pragma once

#include <string>

// 前向声明
namespace semantic {
class sema_env;
}

// LLVM IR 类型转换函数
std::string convert_type(semantic::sema_env& env, const std::string& reg,
                         const std::string& from_type, const std::string& to_type);

// 操作数类型转换函数
std::string convert_operands(semantic::sema_env& env,
                             std::string& left_reg, std::string& right_reg,
                             const std::string& left_type, const std::string& right_type);

// 转换为 i1 类型函数
std::string convert_to_i1(semantic::sema_env& env, const std::string& reg, const std::string& type = "int");

// LLVM IR 代码生成辅助函数
void emit_alloca(const semantic::sema_env& env, const std::string& var_name, const std::string& var_type);
void emit_store(const semantic::sema_env& env, const std::string& value_reg, const std::string& var_name, const std::string& var_type);
void emit_load(const semantic::sema_env& env, const std::string& result_reg, const std::string& var_name, const std::string& var_type);

// 算术运算函数
void emit_add(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type);
void emit_sub(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type);
void emit_mul(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type);
void emit_div(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type);

// 位运算函数
void emit_bitand(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type = "int");
void emit_bitor(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type = "int");
void emit_bitxor(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type = "int");

// 字符串处理函数
std::string to_hex(int value);
std::string to_llvmstr(const std::string& str);
