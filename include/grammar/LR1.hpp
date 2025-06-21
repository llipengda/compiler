#pragma once
#ifndef GRAMMAR_LR1_HPP
#define GRAMMAR_LR1_HPP

#include "SLR.hpp"
#include "production.hpp"
#include <vector>

namespace grammar {

class LR1 : public SLR<production::LR1_production> {
public:
    explicit LR1(const std::vector<production::production>& productions);
    explicit LR1(const std::string& str);

private:
    void init_first_item_set() override;
    std::unordered_set<production::symbol> expand_item_set(const std::unordered_set<production::symbol>& symbols, items_t& current_item_set, const std::unordered_set<production::symbol>& after_dot) override;
    void build_acc_and_reduce(const items_t& current_items, std::size_t idx) override;
};

} // namespace grammar

#endif