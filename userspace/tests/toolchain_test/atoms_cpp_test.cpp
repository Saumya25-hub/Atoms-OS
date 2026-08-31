/*
 * ATOMS OS — GN/Ninja Toolchain C++20 Test
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include "userspace/runtime/cpp/include/atomic"
#include "userspace/runtime/cpp/include/mutex"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main() {
    std::string s = "[TOOLCHAIN_CPP_TEST]";

    s += " C++20 GN+Ninja Build";

    std::vector<std::string> words;
    words.push_back(s);
    words.push_back("STATUS=PASS");

    std::atomic<int> counter(42);
    counter.fetch_add(8);

    std::mutex mtx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        printf("%s %s (Counter=%d)\n", words[0].c_str(), words[1].c_str(), counter.load());
    }

    return 0;
}
