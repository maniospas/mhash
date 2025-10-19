// g++ tests/bench_cpp.cpp -o tests/bench_cpp -O3 -std=c++20

#include "../mhash_cpp.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <random>
#include <chrono>
#include <string>

using namespace std;
using Clock = chrono::high_resolution_clock;

static vector<string> make_random_strings(size_t n, size_t len) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    static thread_local mt19937_64 rng{12345};
    uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

    vector<string> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        string s;
        s.reserve(len);
        for (size_t j = 0; j < len; ++j)
            s.push_back(charset[dist(rng)]);
        out.push_back(std::move(s));
    }
    return out;
}

int main() {
    constexpr size_t N = 20;
    constexpr size_t REPEATS = 100000;
    auto keys = make_random_strings(N, 16);

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<size_t> dist(0, N - 1);

    {
        cout << "Benchmarking MHashMap...\n";
        MHashMap<int> mhash;
        for (size_t i = 0; i < N; ++i)
            mhash.insert(keys[i], int(i));
        mhash.build();

        auto start = Clock::now();
        // first access triggers table build
        size_t found = 0;
        for (size_t repeat = 0; repeat < REPEATS; ++repeat) {
            for (size_t i = 0; i < N; ++i) {
                auto* v = mhash.get(keys[dist(rng)]);
                found += *v;
            }
        }
        auto end = Clock::now();
        chrono::duration<double> elapsed = end - start;
        cout << "MHashMap get time: " << elapsed.count() << " s, checksum=" << found << "\n";
    }

    {
        cout << "\nBenchmarking std::unordered_map...\n";
        unordered_map<string, int> umap;

        for (size_t i = 0; i < N; ++i)
            umap.emplace(keys[i], int(i));

        auto start = Clock::now();
        size_t found = 0;
        for (size_t repeat = 0; repeat < REPEATS; ++repeat)
            for (size_t i = 0; i < N; ++i) {
                 auto it = umap.find(keys[dist(rng)]);
                 found += it->second;
            }

        auto end = Clock::now();
        chrono::duration<double> elapsed = end - start;
        cout << "unordered_map get time: " << elapsed.count() << " s, checksum=" << found << "\n";
    }

    return 0;
}
