#include "semantic/sema.hpp"

namespace semantic {

std::vector<grammar::production::production> to_productions(const std::vector<sema_production>& sema_prods) {
    std::vector<grammar::production::production> prods;
    prods.reserve(sema_prods.size());
    for (const auto& sema_prod : sema_prods) {
        prods.emplace_back(static_cast<grammar::production::production>(sema_prod));
    }
    return prods;
}

} // namespace semantic