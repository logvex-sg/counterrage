#include "test.hpp"

#include "logvex/log.hpp"

int main() {
    lvx::log::init("", lvx::log::Level::Error);
    int passed = 0;
    for (auto& test : lvxtest::registry()) {
        int before = lvxtest::failure_count();
        test.fn();
        if (lvxtest::failure_count() == before) {
            ++passed;
            std::cout << "[PASS] " << test.name << "\n";
        } else {
            std::cout << "[FAIL] " << test.name << "\n";
        }
    }
    std::cout << "\n" << passed << "/" << lvxtest::registry().size() << " test cases passed, "
              << lvxtest::failure_count() << " assertions failed\n";
    return lvxtest::failure_count() == 0 ? 0 : 1;
}
