#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace lvxtest {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failure_count() {
    static int failures = 0;
    return failures;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

}  // namespace lvxtest

#define TEST(name)                                                   \
    static void name();                                              \
    static ::lvxtest::Registrar lvx_reg_##name(#name, name);         \
    static void name()

#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            ::lvxtest::failure_count()++;                                                  \
            std::cerr << "    CHECK failed: " #cond " (" << __FILE__ << ":" << __LINE__    \
                      << ")\n";                                                            \
        }                                                                                  \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                              \
    do {                                                                                   \
        double lvx_a = (a);                                                                \
        double lvx_b = (b);                                                                \
        if (std::fabs(lvx_a - lvx_b) > (eps)) {                                            \
            ::lvxtest::failure_count()++;                                                  \
            std::cerr << "    CHECK_NEAR failed: " #a "=" << lvx_a << " vs " #b "=" << lvx_b \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n";                     \
        }                                                                                  \
    } while (0)
