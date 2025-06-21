#include "regex/regex.hpp"
#include <gtest/gtest.h>

class regex_tests : public ::testing::Test {};

TEST_F(regex_tests, exact_concatenation_matches_full_sequence) {
    const auto re = regex::regex("abc");
    EXPECT_TRUE(re.match("abc"));
    EXPECT_FALSE(re.match("ab"));
}

TEST_F(regex_tests, kleene_star_allows_zero_or_more) {
    const auto re = regex::regex("a*b*");
    EXPECT_TRUE(re.match("aaabbb"));
    EXPECT_TRUE(re.match(""));
    EXPECT_TRUE(re.match("b"));
    EXPECT_FALSE(re.match("abc"));
}

TEST_F(regex_tests, plus_operator_requires_one_or_more) {
    const auto re = regex::regex("a+b+");
    EXPECT_TRUE(re.match("ab"));
    EXPECT_TRUE(re.match("aaaabbbb"));
    EXPECT_FALSE(re.match("a"));
    EXPECT_FALSE(re.match("b"));
    EXPECT_FALSE(re.match(""));
}

TEST_F(regex_tests, alternation_operator_matches_either_branch) {
    const auto re = regex::regex("a|b");
    EXPECT_TRUE(re.match("a"));
    EXPECT_TRUE(re.match("b"));
    EXPECT_FALSE(re.match("ab"));
}

TEST_F(regex_tests, character_class_accepts_any_listed_char) {
    const auto re = regex::regex("[abc]+");
    EXPECT_TRUE(re.match("a"));
    EXPECT_TRUE(re.match("bac"));
    EXPECT_TRUE(re.match("cabbbccc"));
    EXPECT_FALSE(re.match("def"));
    EXPECT_FALSE(re.match(""));
}

TEST_F(regex_tests, negated_character_class_excludes_specified_chars) {
    const auto re = regex::regex("[^abc]+");
    EXPECT_TRUE(re.match("xyz"));
    EXPECT_TRUE(re.match("defgh"));
    EXPECT_FALSE(re.match("a"));
    EXPECT_FALSE(re.match("bc"));
    EXPECT_FALSE(re.match("a1"));
}

TEST_F(regex_tests, combined_operators_support_complex_expressions) {
    const auto re = regex::regex("a+(b|c)*[de]+");
    EXPECT_TRUE(re.match("abbdde"));
    EXPECT_TRUE(re.match("acccdd"));
    EXPECT_TRUE(re.match("adde"));
    EXPECT_FALSE(re.match("a"));
    EXPECT_FALSE(re.match("abcdf"));
}

TEST_F(regex_tests, empty_pattern_matches_empty_string_only) {
    const auto re = regex::regex("");
    EXPECT_TRUE(re.match(""));
    EXPECT_FALSE(re.match("a"));
}

TEST_F(regex_tests, match_max_returns_zero_on_no_match) {
    const auto re = regex::regex("abc");
    EXPECT_EQ(re.match_max("xyz"), 0);
}

TEST_F(regex_tests, match_max_returns_full_length_on_full_match) {
    const auto re = regex::regex("abc");
    EXPECT_EQ(re.match_max("abc"), 3);
}

TEST_F(regex_tests, match_max_returns_partial_match_length) {
    const auto re = regex::regex("ab");
    EXPECT_EQ(re.match_max("abxyz"), 2);
}

TEST_F(regex_tests, match_max_ignores_excess_characters_after_match) {
    const auto re = regex::regex("abc");
    EXPECT_EQ(re.match_max("abcxyz"), 3);
}

TEST_F(regex_tests, match_max_allows_empty_match_with_star) {
    const auto re = regex::regex("a*");
    EXPECT_EQ(re.match_max(""), 0);
    EXPECT_EQ(re.match_max("aaa"), 3);
    EXPECT_EQ(re.match_max("aaabbb"), 3); // Only 'a*'
}

TEST_F(regex_tests, match_max_with_plus_operator) {
    const auto re = regex::regex("a+b+");
    EXPECT_EQ(re.match_max("aaabbbxyz"), 6);
    EXPECT_EQ(re.match_max("ab"), 2);
    EXPECT_EQ(re.match_max("a"), 0); // because need at least one b
}

TEST_F(regex_tests, match_max_with_char_class) {
    const auto re = regex::regex("[abc]+");
    EXPECT_EQ(re.match_max("abcxyz"), 3);
    EXPECT_EQ(re.match_max("aaabbbcccxyz"), 9);
    EXPECT_EQ(re.match_max("xyz"), 0);
}

TEST_F(regex_tests, match_max_with_negated_char_class) {
    const auto re = regex::regex("[^abc]+");
    EXPECT_EQ(re.match_max("xyzabc"), 3);  // xyz are not abc
    EXPECT_EQ(re.match_max("def"), 3);     // all not abc
    EXPECT_EQ(re.match_max("a123"), 0);    // starts with a
}

TEST_F(regex_tests, escaped_characters_in_pattern) {
    const auto re = regex::regex("a\\.b");
    EXPECT_TRUE(re.match("a.b"));
    EXPECT_FALSE(re.match("ab"));
    EXPECT_FALSE(re.match("a\\b"));
}

TEST_F(regex_tests, escaped_characters_in_char_set) {
    const auto re = regex::regex("[a.\tb\\]]");
    EXPECT_TRUE(re.match("a"));
    EXPECT_TRUE(re.match("."));
    EXPECT_TRUE(re.match("b"));
    EXPECT_TRUE(re.match("\t"));
    EXPECT_TRUE(re.match("]"));
    EXPECT_FALSE(re.match("c"));
    EXPECT_FALSE(re.match("ab"));
}