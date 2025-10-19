// COMPILE WITH: gcc tests/bench.c -o tests/bench -O3 -lm

#include "../mhash.h"
#include "../mhash_str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_LOOKUPS 1000000  // number of lookups per test

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static char **make_keys(size_t n) {
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";
    size_t charset_size = sizeof(charset) - 1;
    char **keys = malloc(n * sizeof(char *));
    if (!keys) return NULL;
    for (size_t i = 0; i < n; ++i) {
        keys[i] = malloc(17);  // 16 chars + null terminator
        if (!keys[i]) continue;

        for (int j = 0; j < 16; ++j) {
            keys[i][j] = charset[rand() % charset_size];
        }
        keys[i][16] = '\0';
    }
    return keys;
}

static void free_keys(char **keys, size_t n) {
    for (size_t i = 0; i < n; ++i)
        free(keys[i]);
    free(keys);
}

int main(void) {
    srand(42);
    for (size_t n = 2; n <= 100; n+=(n<10?1:10)) {
        size_t table_size = 2*n+n*n/2;
        uint16_t *table = malloc(table_size * sizeof(uint16_t));

        char **keys = make_keys(n);
        int *values = malloc(n * sizeof(int));
        for (size_t i = 0; i < n; ++i) values[i] = (int)i + 1;

        // Initialize map
        MHash map;
        if (mhash_init(&map, table, table_size, (const char **)keys, n, mhash_str_prefix)) {
            printf("Failed to create map for %zu keys\n", n);
            free_keys(keys, n);
            free(values);
            free(table);
            continue;
        }

        volatile int sink = 0;
        double start, end;

        // --- mhash_entry benchmark ---
        start = now_sec();
        for (size_t i = 0; i < N_LOOKUPS; ++i) {
            size_t idx = rand() % n;
            int entry = mhash_entry(&map, keys[idx]);
            sink += values[entry];
        }
        end = now_sec();
        double mhash_time = (end - start) / N_LOOKUPS * 1e9;  // ns per lookup

        // --- linear search benchmark ---
        start = now_sec();
        for (size_t i = 0; i < N_LOOKUPS; ++i) {
            size_t idx = rand() % n;
            const char *key = keys[idx];
            for (size_t j = 0; j < n; ++j) {
                if (strcmp(keys[j], key) == 0) {
                    sink += values[j];
                    break;
                }
            }
        }
        end = now_sec();
        double linear_time = (end - start) / N_LOOKUPS * 1e9;

        printf("| %zu | %.0f ns | %.0f ns | %.2fx | %d hash | %ldkB |\n", 
            n, mhash_time, linear_time, linear_time / mhash_time, map.num_hashes, (map.table_size*sizeof(uint16_t)+1024)/1024);

        free_keys(keys, n);
        free(values);
        free(table);
    }

    return 0;
}
