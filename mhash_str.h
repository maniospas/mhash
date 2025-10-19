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

#ifndef MHASH_STR_H
#define MHASH_STR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#ifndef MHASH_UINT
//#define MHASH_UINT uint16_t
#define MHASH_UINT uint64_t
#endif

static inline MHASH_UINT mhash_str_all(const void *_s, MHASH_UINT id) {
    MHASH_UINT h = 0x9E3779B97F4A7C15ULL * id;
    const unsigned char *s = (const unsigned char *)_s;
    for (;; ++s) {
        char c = *s;
        if (c == 0) break;
        h ^= (uint64_t)(c + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2));
    }
    return h;
}

static inline MHASH_UINT mhash_str_prefix(const void *_s, MHASH_UINT id) {
    MHASH_UINT h = 0x9E3779B97F4A7C15ULL * id;
    const unsigned char *s = (const unsigned char *)_s;
    const unsigned char *end = s + id;
    for (;s!=end; ++s) {
        char c = *s;
        if (c == 0) break;
        h ^= (uint64_t)(c + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2));
    }
    return h;
}

static inline int mhash_strcmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

#ifdef __cplusplus
}
#endif

#endif // MHASH_STR_H
