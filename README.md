# 编译器项目 (Compiler Project)

华东师范大学(ECNU) 软件工程学院编译原理课程项目

## 写在开头

头歌平台代码：[educoder/](./educoder/)

---

一个基于 C++ 实现的编译器框架项目，包含通用编译器库和 `simple_cc` C语言子集编译器两大部分。

## 项目概述

本项目包含两个主要部分：

### 1. 通用编译器库 (Compiler Library)

提供可重用的编译器构建组件：

- **词法分析库 (Lexer Library)**: 基于正则表达式的通用词法分析器
- **语法分析库 (Parser Library)**: 支持 LL1、LR1 和 SLR 等多种语法分析算法
- **语义分析框架 (Semantic Framework)**: 属性文法和语义动作处理
- **正则表达式引擎**: 支持自定义正则表达式或标准库正则表达式
- **工具库**: 符号表管理、错误处理等通用工具

### 2. `simple_cc` C语言子集编译器

基于通用编译器库实现的具体编译器：

- **目标语言**: C语言子集（支持基本数据类型、控制结构、函数调用）
- **输出格式**: LLVM IR 中间表示
- **编译流程**: 词法分析 → 语法分析 → 语义分析 → 代码生成

## 项目结构

```text
compiler/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明文档
├── coverage.sh             # 代码覆盖率脚本
├── compile_commands.json   # 编译命令数据库
├── build/                  # 构建输出目录
├── cmake-build-debug/      # Debug 构建目录
├── educoder/               # 头歌平台代码
│
├── include/                # 通用编译器库头文件
│   ├── grammar/            # 语法分析相关头文件
│   │   ├── LL1.hpp         # LL1 分析器
│   │   ├── LR1.hpp         # LR1 分析器
│   │   ├── SLR.hpp         # SLR 分析器
│   │   ├── grammar_base.hpp # 语法分析基类
│   │   └── ...
│   ├── lexer/              # 词法分析相关头文件
│   │   └── lexer.hpp       # 通用词法分析器
│   ├── regex/              # 正则表达式引擎
│   │   ├── regex.hpp       # 正则表达式接口
│   │   ├── dfa.hpp         # 有限自动机
│   │   └── ...
│   ├── semantic/           # 语义分析框架
│   │   └── ...
│   └── utils.hpp           # 工具函数头文件
│
├── src/                    # 通用编译器库实现
│   ├── grammar/            # 语法分析实现
│   ├── lexer/              # 词法分析实现
│   ├── regex/              # 正则表达式实现
│   ├── semantic/           # 语义分析实现
│   └── utils.cpp           # 工具函数实现
│
├── simple_cc/              # C语言子集编译器
│   ├── main.cpp            # 编译器主程序
│   ├── build_grammar.cpp   # C语言语法规则定义
│   ├── build_lexer.cpp     # C语言词法规则定义
│   ├── helper.cpp          # 辅助函数（LLVM IR生成等）
│   ├── include/            # simple_cc 专用头文件
│   └── example/            # C语言示例程序
│       ├── calc.c          # 计算器示例
│       ├── for.c           # for 循环示例
│       └── while.c         # while 循环示例
│
└── tests/                  # 单元测试
    ├── grammar_test.cpp    # 语法分析测试
    ├── lexer_test.cpp      # 词法分析测试
    ├── regex_test.cpp      # 正则表达式测试
    ├── sema_test.cpp       # 语义分析测试
    └── utils_test.cpp      # 工具函数测试
```

## 功能特性

### 通用编译器库特性

- **多种语法分析算法**: LL1, LR1, SLR 等经典算法实现
- **灵活的词法分析**: 支持正则表达式驱动的词法规则定义
- **可扩展语义框架**: 基于属性文法的语义分析支持
- **错误处理机制**: 语法错误恢复和详细错误报告
- **符号表管理**: 支持作用域嵌套的符号表实现
- **模块化设计**: 各组件独立，便于复用和扩展

### simple_cc 支持的语言特性

- **数据类型**: `int`, `double`, `long`, 字符串字面量
- **变量声明**: 支持初始化和非初始化声明
- **表达式**: 算术、逻辑、关系、位运算表达式
- **控制结构**: `if/else`, `while`, `for` 循环
- **函数调用**: `printf`, `scanf` 内置函数
- **运算符**:
  - 算术运算符: `+`, `-`, `*`, `/`
  - 逻辑运算符: `&&`, `||`, `!`
  - 关系运算符: `<`, `>`, `<=`, `>=`, `==`, `!=`
  - 位运算符: `&`, `|`, `^`, `~`
  - 一元运算符: `-`, `&` (取地址)

### 输出和代码生成

- **LLVM IR 生成**: simple_cc 编译器输出标准 LLVM IR
- **类型检查**: 静态类型检查和自动类型转换
- **代码优化**: 支持基本的代码优化传递
- **目标代码生成**: 通过 LLVM 后端支持多种目标架构

## 构建要求

- **C++ 编译器**: 支持 C++20 标准 (GCC/Clang)
- **CMake**: 版本 3.10 或更高
- **操作系统**: Windows, Linux, macOS

## 构建说明

### 基本构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译项目
cmake --build .
```

### 构建选项

项目支持以下 CMake 选项：

- `CODE_COVERAGE=ON`: 启用代码覆盖率分析
- `SEMA_PROD_USE_INITIALIZER_LIST=ON`: 使用初始化列表
- `USE_STD_REGEX=ON`: 使用标准库正则表达式
- `DEBUG=ON`: 启用调试模式

示例：

```bash
cmake -DCODE_COVERAGE=ON -DDEBUG=ON ..
```

### Visual Studio (Windows)

```bash
# 生成 Visual Studio 项目
cmake -G "Visual Studio 16 2019" ..

# 或使用 MSBuild
cmake --build . --config Release
```

## 使用说明

### 使用通用编译器库

通用编译器库可以用于构建各种编程语言的编译器：

```cpp
#include "grammar/SLR.hpp"
#include "lexer/lexer.hpp"
#include "semantic/sema.hpp"

// 使用示例：构建自定义语言编译器
auto lexer = build_lexer();  // 定义词法规则
auto grammar = build_grammar();  // 定义语法规则
auto parser = semantic::sema<grammar::SLR>(grammar);
parser.parse(tokens);
```

### 使用 simple_cc 编译器

simple_cc 是一个完整的 C语言子集编译器：

```bash
# 基本用法
./simple_cc input.c

# 指定输出文件
./simple_cc input.c -o output.exe

# 指定优化级别
./simple_cc input.c -O2

# 传递参数给 clang
./simple_cc input.c -- -Wall -Wextra
```

### 示例程序

simple_cc 项目包含几个 C语言示例程序，展示编译器的功能：

#### 计算器示例 (`example/calc.c`)

```c
int main() {
    int a, b;
    scanf("%d", &a);
    scanf("%d", &b);
    
    printf("a + b = %d\n", a + b);
    printf("a - b = %d\n", a - b);
    printf("a * b = %d\n", a * b);
    printf("a / b = %d\n", a / b);
}
```

#### 循环示例 (`example/for.c`, `example/while.c`)

展示各种控制结构的使用。

## 运行测试

```bash
# 运行所有测试
cmake --build . --target test

# 或使用 ctest
ctest

# 运行特定测试
./compiler_tests --gtest_filter="*lexer*"
```

## 代码覆盖率

```bash
# 生成覆盖率报告 (需要安装 gcov/lcov)
./coverage.sh
```

## 开发指南

### 使用通用编译器库开发新编译器

1. **定义词法规则**: 使用正则表达式定义词法单元
2. **定义语法规则**: 使用产生式定义语法结构
3. **实现语义动作**: 定义属性计算和代码生成逻辑
4. **集成和测试**: 使用测试框架验证编译器功能

### 扩展 `simple_cc` 编译器

1. **词法分析**: 在 `simple_cc/build_lexer.cpp` 中添加新的词法规则
2. **语法分析**: 在 `simple_cc/build_grammar.cpp` 中添加语法产生式
3. **语义分析**: 实现相应的语义动作和代码生成逻辑
4. **测试**: 添加相应的单元测试

