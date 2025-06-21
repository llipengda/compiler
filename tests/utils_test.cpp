#include "utils.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using namespace utils;

TEST(utils_test, trim_string) {
    EXPECT_EQ(trim(std::string("  hello  ")), "hello");
    EXPECT_EQ(trim(std::string("\tfoo\n")), "foo");
    EXPECT_EQ(trim(std::string("bar")), "bar");
    EXPECT_EQ(trim(std::string("   ")), "");
    EXPECT_EQ(trim(std::string("")), "");
}

#if __cplusplus >= 201703L
TEST(utils_test, trim_string_view) {
    EXPECT_EQ(trim(std::string_view("  hello  ")), "hello");
    EXPECT_EQ(trim(std::string_view("\tfoo\n")), "foo");
    EXPECT_EQ(trim(std::string_view("bar")), "bar");
    EXPECT_EQ(trim(std::string_view("   ")), "");
    EXPECT_EQ(trim(std::string_view("")), "");
}
#endif

TEST(utils_test, split_trim_string) {
    std::vector<std::string> result = split_trim(std::string(" a, b ,c "), std::string(","));
    std::vector<std::string> expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
    result = split_trim(std::string("one| two|three "), std::string("|"));
    expected = {"one", "two", "three"};
    EXPECT_EQ(result, expected);
}

#if __cplusplus >= 201703L
TEST(utils_test, split_trim_string_view) {
    std::vector<std::string_view> result = split_trim(std::string_view(" a, b ,c "), std::string_view(","));
    std::vector<std::string_view> expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
    result = split_trim(std::string_view("one| two|three "), std::string_view("|"));
    expected = {"one", "two", "three"};
    EXPECT_EQ(result, expected);
}
#endif

TEST(utils_test, join_string) {
    std::vector<std::string> v = {"a", "b", "c"};
    EXPECT_EQ(join(v, std::string(",")), "a,b,c");
    v = {"one", "two"};
    EXPECT_EQ(join(v, std::string("|")), "one|two");
    v = {};
    EXPECT_EQ(join(v, std::string(",")), "");
}

#if __cplusplus >= 201703L
TEST(utils_test, join_string_view) {
    std::vector<std::string_view> v = {"a", "b", "c"};
    EXPECT_EQ(join(v, std::string_view(",")), "a,b,c");
    v = {"one", "two"};
    EXPECT_EQ(join(v, std::string_view("|")), "one|two");
    v = {};
    EXPECT_EQ(join(v, std::string_view(",")), "");
}
#endif

TEST(utils_test, starts_with) {
    EXPECT_TRUE(starts_with(std::string("hello"), std::string("he")));
    EXPECT_FALSE(starts_with(std::string("hello"), std::string("lo")));
    EXPECT_TRUE(starts_with(std::string("abc"), std::string("")));
    EXPECT_FALSE(starts_with(std::string(""), std::string("a")));
}

TEST(utils_test, print_vector) {
    std::vector<int> v = {1, 2, 3};
    std::ostringstream oss;
    print(oss, v);
    EXPECT_EQ(oss.str(), "1 2 3 ");
}

TEST(utils_test, print_unordered_map) {
    std::unordered_map<std::string, int> m = {{"a", 1}, {"b", 2}};
    std::ostringstream oss;
    print(oss, m);
    std::string s = oss.str();
    // Accept either order
    EXPECT_TRUE(s == "{a=1,b=2}" || s == "{b=2,a=1}");
}

TEST(utils_test, print_stack) {
    std::stack<int> st;
    st.push(1);
    st.push(2);
    st.push(3);
    std::ostringstream oss;
    print(oss, st);
    EXPECT_EQ(oss.str(), "1 2 3 ");
}

TEST(utils_test, println_vector) {
    std::vector<int> v = {1, 2};
    std::ostringstream oss;
    println(oss, v);
    EXPECT_EQ(oss.str(), "1 2 \n");
}

TEST(utils_test, println_single) {
    std::ostringstream oss;
    println(oss, 42);
    EXPECT_EQ(oss.str(), "42\n");
}
