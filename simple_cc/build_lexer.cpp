#include "include/build_lexer.hpp"
#include "lexer/lexer.hpp"
#include <ranges>

const lexer::lexer::input_keywords_t<token_type> keywords = {
    {"int", token_type::INT, "int"},
    {"double", token_type::DOUBLE, "double"},
    {"long", token_type::LONG, "long"},
    {"if", token_type::IF, "if"},
    {"else", token_type::ELSE, "else"},
    {"for", token_type::FOR, "for"},
    {"while", token_type::WHILE, "while"},
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
    {"!=", token_type::NE, "!="},
    {"=", token_type::ASSIGN, " ="},
    {"&&", token_type::LOGAND, "&&"},
    {"\\|\\|", token_type::LOGOR, "||"},
    {"!", token_type::LOGNOT, "!"},
    {"~", token_type::BITNOT, "~"},
    {"&", token_type::BITAND, "&"},
    {"\\|", token_type::BITOR, "|"},
    {"^", token_type::BITXOR, "^"},
    {",", token_type::COMMA, ","},
    {R"("([^"]|(\\"))*")", token_type::STRING, "STRING"},
    {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
    {"[0-9]+", token_type::INTNUM, "INTNUM"},
    {"[0-9]+\\.[0-9]*", token_type::DOUBLENUM, "DOUBLENUM"},
    {"[ \t\n]+", token_type::WHITESPACE, "WHITESPACE"},
    {"(//[^\n]*)|(/\\*([^*]|\\*+[^*/])*\\*+/)", token_type::COMMENT, "COMMENT"}};

const std::unordered_set<std::string> terminals{
    "int", "double", "long", "if", "else", "for", "while", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "!=", "=", "ID", "INTNUM", "DOUBLENUM",
    "&", "|", "^", "&&", "||", ",", "STRING", "!", "~"};

std::map<std::string, std::string> strings;

std::string process_string_literal(const std::string& literal) {
    if (literal.length() < 2) return "";

    std::string content = literal.substr(1, literal.length() - 2);

    std::string processed;
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\\' && i + 1 < content.length()) {
            switch (content[i + 1]) {
            case 'n':
                processed += "\x0A";
                i++;
                break;
            case 't':
                processed += "\x09";
                i++;
                break;
            case 'r':
                processed += "\x0D";
                i++;
                break;
            case '\\':
                processed += "\\";
                i++;
                break;
            case '"':
                processed += R"(")";
                i++;
                break;
            default: processed += content[i]; break;
            }
        } else {
            processed += content[i];
        }
    }

    return processed;
}

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

std::vector<lexer::token> lex(const std::string& str) {
    std::vector<lexer::token> tokens;

    const lexer::lexer lex_(keywords, token_type::WHITESPACE);

    auto filtered = lex_.parse(str)
        | std::views::filter([](const lexer::token& t) { return t.type != static_cast<int>(token_type::WHITESPACE)
                                                             && t.type != static_cast<int>(token_type::COMMENT); });

    int string_counter = 0;
    for (auto& token : filtered) {
        if (token.type == static_cast<int>(token_type::STRING)) {
            if (std::string content = process_string_literal(token.value); !strings.contains(content)) {
                strings[content] = ".str." + std::to_string(string_counter++);
            }
        }
        tokens.emplace_back(token);
    }

    return tokens;
}
