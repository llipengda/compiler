#include "helper.hpp"
#include "semantic/sema_production.hpp"
#include <iomanip>
#include <sstream>
#include <string>

std::string convert_type(semantic::sema_env& env, const std::string& reg,
                         const std::string& from_type, const std::string& to_type) {
    if (from_type == to_type) return reg;

    std::string conv_reg = "%" + env.temp();
    if (from_type == "int" && to_type == "double") {
        env.emit("  " + conv_reg + " = sitofp i32 " + reg + " to double");
    } else if (from_type == "double" && to_type == "int") {
        env.emit("  " + conv_reg + " = fptosi double " + reg + " to i32");
    } else if (from_type == "int" && to_type == "long") {
        env.emit("  " + conv_reg + " = sext i32 " + reg + " to i64");
    } else if (from_type == "long" && to_type == "int") {
        env.emit("  " + conv_reg + " = trunc i64 " + reg + " to i32");
    } else if (from_type == "long" && to_type == "double") {
        env.emit("  " + conv_reg + " = sitofp i64 " + reg + " to double");
    } else if (from_type == "double" && to_type == "long") {
        env.emit("  " + conv_reg + " = fptosi double " + reg + " to i64");
    }
    return conv_reg;
}

std::string convert_operands(semantic::sema_env& env,
                             std::string& left_reg, std::string& right_reg,
                             const std::string& left_type, const std::string& right_type) {
    // 类型优先级：double > long > int
    if (left_type == "double" || right_type == "double") {
        if (left_type != "double") {
            left_reg = convert_type(env, left_reg, left_type, "double");
        }
        if (right_type != "double") {
            right_reg = convert_type(env, right_reg, right_type, "double");
        }
        return "double";
    } else if (left_type == "long" || right_type == "long") {
        if (left_type != "long") {
            left_reg = convert_type(env, left_reg, left_type, "long");
        }
        if (right_type != "long") {
            right_reg = convert_type(env, right_reg, right_type, "long");
        }
        return "long";
    }
    return "int";
}

std::string convert_to_i1(semantic::sema_env& env, const std::string& reg, const std::string& type) {
    std::string i1_reg = "%" + env.temp();
    if (type == "long") {
        env.emit("  " + i1_reg + " = icmp ne i64 " + reg + ", 0");
    } else if (type == "double") {
        env.emit("  " + i1_reg + " = fcmp one double " + reg + ", 0.0");
    } else {
        env.emit("  " + i1_reg + " = icmp ne i32 " + reg + ", 0");
    }
    return i1_reg;
}

void emit_alloca(const semantic::sema_env& env, const std::string& var_name, const std::string& var_type) {
    if (var_type == "int") {
        env.emit("  " + var_name + " = alloca i32, align 4");
    } else if (var_type == "long") {
        env.emit("  " + var_name + " = alloca i64, align 8");
    } else {
        env.emit("  " + var_name + " = alloca double, align 8");
    }
}

void emit_store(const semantic::sema_env& env, const std::string& value_reg, const std::string& var_name, const std::string& var_type) {
    if (var_type == "int") {
        env.emit("  store i32 " + value_reg + ", i32* " + var_name + ", align 4");
    } else if (var_type == "long") {
        env.emit("  store i64 " + value_reg + ", i64* " + var_name + ", align 8");
    } else {
        env.emit("  store double " + value_reg + ", double* " + var_name + ", align 8");
    }
}

void emit_load(const semantic::sema_env& env, const std::string& result_reg, const std::string& var_name, const std::string& var_type) {
    if (var_type == "int") {
        env.emit("  " + result_reg + " = load i32, i32* " + var_name + ", align 4");
    } else if (var_type == "long") {
        env.emit("  " + result_reg + " = load i64, i64* " + var_name + ", align 8");
    } else {
        env.emit("  " + result_reg + " = load double, double* " + var_name + ", align 8");
    }
}

void emit_add(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "double") {
        env.emit("  " + result_reg + " = fadd double " + left_reg + ", " + right_reg);
    } else if (result_type == "long") {
        env.emit("  " + result_reg + " = add nsw i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = add nsw i32 " + left_reg + ", " + right_reg);
    }
}

void emit_sub(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "double") {
        env.emit("  " + result_reg + " = fsub double " + left_reg + ", " + right_reg);
    } else if (result_type == "long") {
        env.emit("  " + result_reg + " = sub nsw i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = sub nsw i32 " + left_reg + ", " + right_reg);
    }
}

void emit_mul(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "double") {
        env.emit("  " + result_reg + " = fmul double " + left_reg + ", " + right_reg);
    } else if (result_type == "long") {
        env.emit("  " + result_reg + " = mul nsw i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = mul nsw i32 " + left_reg + ", " + right_reg);
    }
}

void emit_div(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "double") {
        env.emit("  " + result_reg + " = fdiv double " + left_reg + ", " + right_reg);
    } else if (result_type == "long") {
        env.emit("  " + result_reg + " = sdiv i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = sdiv i32 " + left_reg + ", " + right_reg);
    }
}

void emit_bitand(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "long") {
        env.emit("  " + result_reg + " = and i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = and i32 " + left_reg + ", " + right_reg);
    }
}

void emit_bitor(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "long") {
        env.emit("  " + result_reg + " = or i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = or i32 " + left_reg + ", " + right_reg);
    }
}

void emit_bitxor(const semantic::sema_env& env, const std::string& result_reg, const std::string& left_reg, const std::string& right_reg, const std::string& result_type) {
    if (result_type == "long") {
        env.emit("  " + result_reg + " = xor i64 " + left_reg + ", " + right_reg);
    } else {
        env.emit("  " + result_reg + " = xor i32 " + left_reg + ", " + right_reg);
    }
}

std::string to_hex(int value) {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0');
    ss << std::hex << value;
    return ss.str();
}

std::string to_llvmstr(const std::string& str) {
    std::string result = "\"";
    for (const char c : str) {
        if (c == '"') {
            result += "\\22"; // 转义双引号
        } else if (c == '\\') {
            result += "\\5C"; // 转义反斜杠
        } else if (c < 32 || c > 126) {
            result += "\\";
            result += to_hex(static_cast<int>(c));
        } else {
            result += c;
        }
    }
    result += "\\00\"";
    return result;
}
