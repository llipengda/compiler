import os
import sys
import re

included_files = set()

# --- 正则定义 ---
include_pattern = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
pragma_once_pattern = re.compile(r'^\s*#\s*pragma\s+once\b')
inline_pattern = re.compile(r'\binline\b\s*')

# for (auto [a, b, c] : container)
for_tuple_pattern = re.compile(
    r'^\s*for\s*\(\s*'
    r'(?P<cv>(?:const|volatile)?\s*)auto(?P<ref>\&{1,2})?\s*'
    r'\[\s*(?P<vars>\w+\s*(?:,\s*\w+){1,2})\s*\]\s*:\s*'
    r'(?P<container>.+?)\)\s*({)?\s*$'
)

# if (init; cond)
if_init_pattern = re.compile(
    r'^\s*if\s*\(\s*(.+?)\s*;\s*(.+?)\s*\)\s*({?)\s*$'
)

# auto [a, b] = expr;
assignment_pattern = re.compile(
    r'^\s*(?P<cv>(?:const|volatile)?\s*)auto(?P<ref>\&{1,2})?\s*'
    r'\[\s*(?P<vars>\w+\s*(?:,\s*\w+){1,2})\s*\]\s*=\s*(?P<expr>.+?)\s*;\s*$'
)

def transform_line(line):
    # --- if (init; cond) ---
    m_if = if_init_pattern.match(line)
    if m_if:
        init_stmt = m_if.group(1)
        cond_expr = m_if.group(2)
        has_brace = m_if.group(3)
        indent = line[:line.find('if')]
        lines = [
            f"{indent}{init_stmt};\n",
            f"{indent}if ({cond_expr})" + (" {\n" if has_brace else "\n")
        ]
        return lines

    # --- for (auto [a, b, c] : container) ---
    m_for = for_tuple_pattern.match(line)
    if m_for:
        cv = m_for.group('cv') or ''
        ref = m_for.group('ref') or ''
        vars_str = m_for.group('vars')
        var_list = [v.strip() for v in vars_str.split(',')]
        if len(var_list) > 3:
            print(f"⚠️ 忽略超过3个变量的结构化绑定: {line.strip()}", file=sys.stderr)
            return [line]

        container = m_for.group('container').strip()
        has_brace = m_for.group(5)
        indent = line[:line.find('for')]
        it_name = "_it_" + var_list[0]

        new_lines = [f"{indent}for ({cv}auto{ref} {it_name} : {container})"]
        new_lines[-1] += " {\n" if has_brace else "\n"
        for i, var in enumerate(var_list):
            new_lines.append(f"{indent}    auto& {var} = std::get<{i}>({it_name});\n")
        return new_lines

    # --- auto [a, b, c] = expr; ---
    m_assign = assignment_pattern.match(line)
    if m_assign:
        cv = m_assign.group('cv') or ''
        ref = m_assign.group('ref') or ''
        vars_str = m_assign.group('vars')
        var_list = [v.strip() for v in vars_str.split(',')]
        if len(var_list) > 3:
            print(f"⚠️ 忽略超过3个变量的结构化绑定赋值: {line.strip()}", file=sys.stderr)
            return [line]

        expr = m_assign.group('expr').strip()
        tmp_name = "_tmp"
        indent = line[:line.find('auto')]
        lines = [f"{indent}{cv}auto{ref} {tmp_name} = {expr};\n"]
        for i, var in enumerate(var_list):
            lines.append(f"{indent}auto& {var} = std::get<{i}>({tmp_name});\n")
        return lines

    # --- 删除 inline ---
    line = inline_pattern.sub('', line)
    return [line]

def expand_includes(file_path, search_dirs):
    result_lines = []

    abs_path = os.path.abspath(file_path)
    if abs_path in included_files:
        return []
    included_files.add(abs_path)

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"❌ 找不到文件: {file_path}", file=sys.stderr)
        return []

    for line in lines:
        if pragma_once_pattern.match(line):
            continue

        m = include_pattern.match(line)
        if m:
            header_name = m.group(1)
            header_path = None
            for d in [os.path.dirname(file_path)] + search_dirs:
                candidate = os.path.join(d, header_name)
                if os.path.isfile(candidate):
                    header_path = candidate
                    break
            if header_path:
                result_lines.append(f"// Begin include \"{header_name}\"\n")
                included_content = expand_includes(header_path, search_dirs)
                for included_line in included_content:
                    result_lines.extend(transform_line(included_line))
                result_lines.append(f"// End include \"{header_name}\"\n")
            else:
                print(f"⚠️ 找不到 include 文件: \"{header_name}\"", file=sys.stderr)
                result_lines.append(line)
        else:
            result_lines.extend(transform_line(line))

    return result_lines

def main():
    if len(sys.argv) < 2:
        print("用法: python expand_and_transform.py <源文件> [搜索目录...] [--output 输出文件]", file=sys.stderr)
        sys.exit(1)

    args = sys.argv[1:]
    output_file = "output.cpp"
    if "--output" in args:
        idx = args.index("--output")
        if idx + 1 >= len(args):
            print("错误: 请指定输出文件名", file=sys.stderr)
            sys.exit(1)
        output_file = args[idx + 1]
        args = args[:idx]

    input_file = args[0]
    search_dirs = args[1:]

    result = expand_includes(input_file, search_dirs)

    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.writelines(result)
        print(f"\n✅ 转换完成，输出文件：{output_file}")
    except Exception as e:
        print(f"❌ 写入失败: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
