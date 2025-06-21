#include "grammar/exception.hpp"

namespace grammar::exception {

ambiguous_grammar_exception::ambiguous_grammar_exception(const std::vector<grammar::production::production>& prods) {
    for (const auto& prod : prods) {
        msg += prod.to_string() + "\n";
    }
}

const char* ambiguous_grammar_exception::what() const noexcept {
    return msg.c_str();
}

grammar_error::grammar_error(const std::string& message) : msg(message) {}

const char* grammar_error::what() const noexcept {
    return msg.c_str();
}

} // namespace grammar::exception