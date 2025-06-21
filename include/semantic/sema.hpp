#pragma once
#ifndef SEMANTIC_SEMA_HPP
#define SEMANTIC_SEMA_HPP

#include "grammar/grammar.hpp"
#include "sema_production.hpp"
#include "sema_tree.hpp"

#include <memory>
#include <type_traits>
#include <vector>

namespace semantic {
std::vector<grammar::production::production> to_productions(const std::vector<sema_production>& sema_prods);

template <typename T>
class sema : public T {
public:
    static_assert(std::is_base_of<grammar::grammar_base, T>::value, "T must be a subclass of grammar::grammar_base");
    explicit sema(const std::vector<sema_production>& productions);
};
} // namespace semantic

#pragma region tpp

template <typename T>
semantic::sema<T>::sema(const std::vector<sema_production>& productions) : T(to_productions(productions)) {
    this->tree_ = std::make_shared<sema_tree>(productions);
}

#pragma endregion

#endif