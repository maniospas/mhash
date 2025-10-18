/*
 * Copyright 2025 Emmanouil Krasanakis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MHASH_H
#define MHASH_H

#include <stdint.h>
#include <stddef.h>

// maybe help the compiler identify the rotl assembly instruction
#define ROTL16(x, r) ((uint16_t)(((x) << (r)) | ((x) >> (16 - (r)))))
#define MHASH_EMPTY_SLOT 0xFFFF
#define MHASH_FAILED 1
#define MHASH_OK 0
#ifndef MHASH_MAX_HASHES
#define MHASH_MAX_HASHES (UINT8_MAX-1)
#endif

typedef uint16_t (*mhash_func)(const char *s, uint8_t id);

typedef struct {
    uint16_t *table;     // externally allocated table
    size_t table_size;   // number of entries in the table
    uint8_t num_hashes;  // number of hash functions used
    size_t count;        // number of elements
    mhash_func hashf;    // active hash function
} MHash;

static inline uint16_t hash_all(const char *s, uint8_t id) {
    uint64_t h = 0x9E3779B97F4A7C15ULL * (id + 1);
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
        h = (h ^ (*p + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2)));
    return (uint16_t)(h & 0xFFFF);
}

static inline uint16_t hash_prefix(const char *s, uint8_t id) {
    uint64_t h = 0x9E3779B97F4A7C15ULL * (id + 1);
    const unsigned char *p = (const unsigned char *)s;
    for (uint8_t i = 0; i <= id+1 && *p; ++i, ++p)
        h ^= (uint64_t)(*p + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2));
    return (uint16_t)(h & 0xFFFF);
}

static inline uint16_t mhash__concat(const MHash *ph, const char *s) {
    uint32_t combined = 0;
    for (uint8_t i = 0; i < ph->num_hashes; ++i) {
        uint16_t part = ph->hashf(s, i);
        combined ^= ROTL16(part, (i * 2) & 15);
    }
    return (uint16_t)(combined & 0xFFFF);
}


static inline int mhash_init(MHash *ph,
                        uint16_t *table,
                        size_t table_size,
                        const char **strings,
                        size_t count,
                        mhash_func hashf) {
    if (!ph || !table || !strings || table_size == 0)
        return MHASH_FAILED;
    ph->table = table;
    ph->table_size = table_size;
    ph->count = count;
    ph->hashf = hashf;
    ph->num_hashes = 0;

    size_t worst_case = MHASH_MAX_HASHES;
    if(worst_case>count) worst_case = count;

    for (;;) {
        // clear table even on failure
        for (size_t i = 0; i < table_size; ++i)
            table[i] = MHASH_EMPTY_SLOT;
        if(ph->num_hashes >= worst_case)
            return MHASH_FAILED;

        ph->num_hashes++;
        uint8_t ok = 1;
        for(uint16_t i=0; i<count; ++i) {
            uint16_t idx = mhash__concat(ph, strings[i]) % table_size;
            if(table[idx] != MHASH_EMPTY_SLOT) {
                ok = 0;
                break;
            }
            table[idx] = i;
        }
        if(ok)
            return MHASH_OK;
    }
}

static inline uint16_t mhash_entry(const MHash *ph, const char *s) {
    uint16_t idx = mhash__concat(ph, s) % ph->table_size;
    return ph->table[idx];
}

static inline void *mhash_check_at(const MHash *ph,
                             const char *s,
                             const char **keys,
                             void *values,
                             size_t sizeof_value){
    uint16_t idx = mhash__concat(ph, s) % ph->table_size;
    uint16_t entry = ph->table[idx];
    if (entry == MHASH_EMPTY_SLOT || entry >= ph->count)
        return NULL;
    if (strcmp(keys[entry], s) != 0)
        return NULL;
    return (uint8_t *)values + ((size_t)entry * sizeof_value);
}

#endif // MHASH_H
