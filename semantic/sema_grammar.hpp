#include "../grammar/grammar.hpp"
#include "sema_production.hpp"
#include "sema_tree.hpp"
#include <memory>
#include <type_traits>
#include <vector>

namespace semantic {
inline std::vector<grammar::production::production> to_productions(const std::vector<sema_production>& sema_prods) {
    std::vector<grammar::production::production> prods;
    prods.reserve(sema_prods.size());
    for (const auto& sema_prod : sema_prods) {
        prods.emplace_back(static_cast<grammar::production::production>(sema_prod));
    }
    return prods;
}

template <typename T>
class sema : public T {
public:
    static_assert(std::is_base_of<grammar::grammar_base, T>::value, "T must be a subclass of grammar::grammar_base");
    explicit sema(const std::vector<sema_production>& productions) : T(to_productions(productions)) {
        this->tree_ = std::make_shared<sema_tree>(productions);
    }
};
} // namespace semantic