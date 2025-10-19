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
#include <string.h>
#include <stdint.h>


static inline uint16_t mhash_str_all(const void *_s, uint16_t id) {
    const char *s = (const char *)_s;
    uint64_t h = 0x9E3779B97F4A7C15ULL * (id + 1);
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
        h = (h ^ (*p + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2)));
    return (uint16_t)(h & 0xFFFF);
}

static inline uint16_t mhash_str_prefix(const void *_s, uint16_t id) {
    const char *s = (const char *)_s;
    uint64_t h = 0x9E3779B97F4A7C15ULL * (id + 1);
    const unsigned char *p = (const unsigned char *)s;
    for (uint16_t i = 0; i <= id+1 && *p; ++i, ++p)
        h ^= (uint64_t)(*p + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2));
    return (uint16_t)(h & 0xFFFF);
}

static int mhash_strcmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

#endif // MHASH_STR_H
