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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MHASH_FAILED 1
#define MHASH_OK 0
#ifndef MHASH_MAX_HASHES
#define MHASH_MAX_HASHES 16
#endif

#ifndef MHASH_UINT
//#define MHASH_UINT uint16_t
#define MHASH_UINT uint64_t
#endif

#ifndef MHASH_INDEX_UINT
//#define MHASH_INDEX_UINT uint16_t
#define MHASH_INDEX_UINT uint64_t
#endif

//#define ROTL16(x, r) (((x) << (r)) | ((x) >> (16 - (r))))
#define ROTL_CONST (sizeof(MHASH_UINT)*8)
#define ROTL(x, r) (((x) << (r)) | ((x) >> (ROTL_CONST - (r))))
#define MHASH_EMPTY_SLOT ((MHASH_INDEX_UINT)(-1))
typedef MHASH_UINT (*mhash_func)(const void *s, MHASH_UINT id);

typedef struct MHash {
    MHASH_INDEX_UINT *table;
    size_t table_size;
    MHASH_UINT num_hashes;
    size_t count;
    mhash_func hash_func;
} MHash;

static inline MHASH_UINT mhash__concat(mhash_func hash_func, MHASH_UINT num_hashes, const void *s) {
    MHASH_UINT combined = 0;
    for (MHASH_UINT i = 1; i <= num_hashes; ++i) {
        MHASH_UINT part = hash_func(s, i);
        combined ^= part;//ROTL(part, i*2);
    }
    return combined;
}

static inline int mhash_init(MHash *ph,
                        MHASH_INDEX_UINT *table,
                        size_t table_size,
                        const void **strings,
                        size_t count,
                        mhash_func hash_func) {
    if (!ph || !table || !strings || table_size == 0)
        return MHASH_FAILED;
    ph->table      = table;
    ph->table_size = table_size;
    ph->count      = count;
    ph->hash_func  = hash_func;
    ph->num_hashes = 0;

    size_t worst_case = MHASH_MAX_HASHES;

#ifndef MHASH_NO_WORST_CASE
    if (worst_case > count) worst_case = count;
#endif

    for (;;) {
        for (size_t i = 0; i < table_size; ++i)
            table[i] = MHASH_EMPTY_SLOT;
        if (ph->num_hashes >= worst_case)
            return MHASH_FAILED;
        ph->num_hashes++;
        int ok = 1;
        MHASH_UINT num_hashes = ph->num_hashes;
        mhash_func hash_func = ph->hash_func;
        for (MHASH_UINT i = 0; i < count; ++i) {
            MHASH_UINT idx = mhash__concat(hash_func, num_hashes, strings[i]) % (MHASH_UINT)table_size;
            if (table[idx] != MHASH_EMPTY_SLOT) {
                ok = 0;
                break;
            }
            table[idx] = i;
        }
        if (ok)
            return MHASH_OK;
    }
}


static inline MHASH_UINT mhash_entry_pos(const MHash *ph, const void *s) {
    return mhash__concat(ph->hash_func, ph->num_hashes, s) % (MHASH_UINT)ph->table_size;
}

static inline MHASH_INDEX_UINT mhash_entry(const MHash *ph, const void *s) {
    MHASH_UINT idx = mhash__concat(ph->hash_func, ph->num_hashes, s) % (MHASH_UINT)ph->table_size;
    return ph->table[idx];
}

static inline void *mhash_check_at(const MHash *ph,
                          const void *s,
                          const void **keys,
                          void *values,
                          size_t sizeof_value,
                          int (*cmp_func)(const void *, const void *)) {
    MHASH_UINT idx = mhash__concat(ph->hash_func, ph->num_hashes, s) % (MHASH_UINT)ph->table_size;
    MHASH_INDEX_UINT entry = ph->table[idx];
    if (entry == MHASH_EMPTY_SLOT)
        return NULL;
    if (cmp_func(keys[entry], s))
        return NULL;
    return (char *)values + ((size_t)entry * sizeof_value);
}

#ifdef __cplusplus
}
#endif

#endif // MHASH_H
