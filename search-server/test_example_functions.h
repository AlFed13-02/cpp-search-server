#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>

template <typename First, typename Second>
std::ostream& operator<<(std::ostream& os, std::pair<First, Second> p) {
    using namespace std;
    os << p.first << ": " << p.second;
    return os;
}

template <typename Container>
void Print(std::ostream& os , const Container& container) {
    using namespace std;
    auto size = container.size();
    for (const auto& entry: container) {
        if (size > 1 ) {
            os << entry << ", "s;
            --size;
        } else {
            os << entry;
        }
    }
}

template <typename Element>
std::ostream& operator<<(std::ostream& os, const std::vector<Element>& v) {
    using namespace std;
    os << "["s;
    Print(os, v);
    os << "]"s;
    return os;
}

template <typename Element>
std::ostream& operator<< (std::ostream& os, const std::set<Element> s) {
    using namespace std;
    os << "{"s;
    Print(os, s);
    os << "}"s;
    return os;
}

template <typename Key, typename Value> 
std::ostream& operator<<(std::ostream& os, std::map<Key, Value> m) {
    using namespace std;
    os << "{"s;
    Print(os, m);
    os << "}"s;
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    using namespace std;
    
    if (t != u) {
        std::cout << std::boolalpha;
        std:: cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        std::abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint) {
    using namespace std;
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        std::abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(TestFunc func, const std::string& test_func_name) {
    using namespace std;
    func();
    std::cerr << test_func_name << " OK" << std::endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)
