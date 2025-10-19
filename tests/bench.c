// COMPILE WITH: gcc tests/bench.c -o tests/bench -O3 -lm

#define MHASH_NO_WORST_CASE

#include "../mhash.h"
#include "../mhash_str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define N_LOOKUPS 1000000
#define N_REPS    100

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ------------------- Unique key generator -------------------
static char **make_keys(size_t n) {
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";
    size_t charset_size = sizeof(charset) - 1;
    char **keys = malloc(n * sizeof(char *));
    if (!keys) return NULL;

    for (size_t i = 0; i < n; ++i) {
        keys[i] = malloc(17);
        if (!keys[i]) continue;

        // random prefix (12 chars)
        for (int j = 0; j < 12; ++j)
            keys[i][j] = charset[rand() % charset_size];

        // unique suffix (base-62 encoding)
        size_t x = i;
        for (int j = 15; j >= 12; --j) {
            keys[i][j] = charset[x % charset_size];
            x /= charset_size;
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

// ------------------- Statistics helpers -------------------
static double mean(double *a, int n) {
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += a[i];
    return s / n;
}

static double stdev(double *a, int n, double avg) {
    double s = 0.0;
    for (int i = 0; i < n; ++i)
        s += (a[i] - avg) * (a[i] - avg);
    return sqrt(s / n);
}

// ------------------- Benchmark -------------------
int main(void) {
    srand(42);

    printf("| keys | mhash (std) | linear (std) | speedup | avg hashes | memory    |\n");
    printf("|------|-------------|--------------|---------|------------|-----------|\n");

    for (size_t n = 2; n <= 100; n += (n < 10 ? 1 : 10)) {
        size_t table_size = 2 * n + n * n / 2;

        double mhash_times[N_REPS];
        double linear_times[N_REPS];
        double hash_counts[N_REPS];
        int ok_runs = 0;

        for (int rep = 0; rep < N_REPS; ++rep) {
            uint16_t *table = malloc(table_size * sizeof(uint16_t));
            char **keys = make_keys(n);
            int *values = malloc(n * sizeof(int));
            for (size_t i = 0; i < n; ++i) values[i] = (int)i + 1;

            MHash map;
            if (mhash_init(&map, table, table_size, (const char **)keys, n, mhash_str_all)) {
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
            mhash_times[ok_runs] = (end - start) / N_LOOKUPS * 1e9;
            hash_counts[ok_runs] = (double)map.num_hashes;

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
            linear_times[ok_runs] = (end - start) / N_LOOKUPS * 1e9;

            ok_runs++;

            free_keys(keys, n);
            free(values);
            free(table);
        }

        if (ok_runs == 0) {
            printf("| %zu | FAILED |\n", n);
            continue;
        }

        double mean_mhash = mean(mhash_times, ok_runs);
        double mean_linear = mean(linear_times, ok_runs);
        double mean_hashes = mean(hash_counts, ok_runs);
        double sd_mhash = stdev(mhash_times, ok_runs, mean_mhash);
        double sd_linear = stdev(linear_times, ok_runs, mean_linear);

        size_t mem_kb = (table_size * sizeof(uint16_t) + 1024) / 1024;

        printf("| %4zu |%4.0fns (%.0fns) |%5.0fns (%.0fns) | %6.1fx | %10.1f | %6zu kb |\n",
               n, mean_mhash, sd_mhash, mean_linear, sd_linear,
               mean_linear / mean_mhash, mean_hashes, mem_kb);
    }

    return 0;
}
