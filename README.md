# MHash

This is an implementation of a hash function that uniquely maps a given string to a
`uint16_t` identifier. It works by concatenating several hash functions from a parameterized family 
until they combine into a unique hash. The main idea is that, with a large enough table holding 
entry ids, collisions are rare so you will want only a few hashes.

**The main goal is to substitute linear search for few (e.g., &lt;255) alternatives.**
In fact, in example benchmark we get +20% speedup compared to even simple lookup across two strings
at the cost of at most 28 bytes of memory!!!

There is a performant C++ wrapper for string hashing in *mhash_cpp.h* but this is not documented yet.

## 🔥 Features

- drop-in header installation
- hash items are determined at RUNTIME
- sub-linear time string indexing
- external value processing in contiguous memory
- compatible: no exceptions, no aborts, no rtti
- easily extensible: implement a hash and -optionally- a comparator

## 🚀 Quickstart

Place the file *mhash.h* into your project and include it like below.
You can optionally include *mhash_str.h* too, as it grants access to string
hashes and comparisons.

```C
#include "mhash.h"
#include "mhash_str.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    int values[] = {1, 2, 3, 4, 5, 6};
    // create map data
    size_t table_size = 37; // ideally let this grow quadratically to the number of entries, and be a prime
    size_t num_entries = 6;
    const char *keys[] = {"Apple", "Banana", "Cherry", "Date", "Doodoo", "D"};
    MHASH_INDEX_UINT table[table_size];

    // initialize the map (ALWAYS check for failure status, because init fails on excessive loads)
    MHash map;
    if(mhash_init(&map, table, table_size, keys, num_entries, mhash_str_prefix)) {
        printf("Failed to create map\n");
        return 1;
    }
    printf("%d hashes\n", map.num_hashes);

    // query the map (retrieves entry id)
    const char *query = "Cherry";
    printf("%s -> %d\n", query, values[mhash_entry(&map, query)]);

    // query the map for specific value (retrieves value pointer or NULL)
    int *val_ptr = (int *)mhash_check_at(&map, "Date", (const void**)keys, values, sizeof(int), mhash_strcmp);
    if(val_ptr) printf("Found Date -> %d\n", *val_ptr);
    else printf("Date not found\n");

    // safely query the map for a missing key (DO NOT USE mhash_entry for this)
    val_ptr = (int *)mhash_check_at(&map, "Unknown", (const void**)keys, values, sizeof(int), mhash_strcmp);
    if(val_ptr) printf("Found Unknown -> %d\n", *val_ptr);
    else printf("Unknown not found\n");
    return 0;
}
```

## 🔌 API

#### MHash

The hash map data structure. Once created, you can access `MHash.num_hashes` to get a sense of how many hashing 
operations are internally performed. There can be up to 255 internal hashes, and the 
computational cost for search key access proportional to those. The number of stored elements is tracked through `MHash.count`.

⚠️ *Do NOT modify field values.*

#### mhash_init

It does not allocate any memory, but requires a manually allocated table pointer and associated capacity for its internal use.
It guarantees that it will use only the indicated memory region (so you can retrieve pointers from larger buffers). Do note that 
MHash does *not* track keys and values, so you need to manage those independently. The only requirement is that keys are known 
at the point where you call `mhash_init`. 

The last argument is a pointer to computing the hash function given a `const void*` pointer to an instance of your
data type and a `MHASH_UINT` identifier of the hash function within its family. See *mhash_str.h* for string implementations;
that files contains also the family of hash functions *mhash_str_prefix* that is based on an assumption that uniqueness information
is heavily encoded in string prefixes.

⚠️ *ALWAYS check for success given table size - you may need to increase table size or change the base hash function.*

#### mhash_entry

Retrieves the value associated with a given string in the range `0`..`MHash.count-1`. If you want a mapping to ids, there is no 
need to store any kind of value elsewhere.

⚠️ *Calling mhash_entry for a non-registed key is UB.* Do not attempt to error check the result afterwards by looking at the
implementation - the interface is not equipped to present such info. This compromise is made for the sake of speed. Use the next 
function to safely get the value of an entry that could be missing.

#### mhash_check_at

This is a safer -albeit a tad slower- version of `mhash_entry`. It returns a pointer at the memory address of a corresponding 
value, or NULL if the element is not found. The main additional cost is an internal comparison mechanism that must be provided.

```C
int* value = mhash_check_at(map, query, keys, values, sizeof(*value), mhash_strcmp);
```

Pass the keys themselves as values to the last function to check that a value exists:

```C
if(mhash_check_at(map, "Doodoo", keys, keys, sizeof(char*), mhash_strcmp))
    printf("Key exists");
```

## ⏱️ Benchmarks

Benchmarks are lies. But they are useful lies. So here's a comparison
with plain-old linear search. Disclaimer that this is by no means scientifically
rigorous, because it follows the next setup. I may improve it in the future.

- Ran on uniformly random letters and numbers and `mhash_str_prefix` hashing and `mhash_check_at` safe item retrieval.
Because this is my target usage. 
Natural language is not necessarily distributed in a way that helps distinguish prefixes, 
but if you know this to be the case, you can get the advantages of both hashing and
early exiting from comparisons. If it ends up requiring few enough hashes (it normally does), 
this implementation also turns out to be very cache-friendly.
- Tested strings only had `16` characters. This is not indicative of real world conditions.
- Times are measured across 1E6 random lookups and divided by total time for those to run. There are 100 experiment repetitions.
- Internal hash table size starts from 3* the number of keys, increase by one until 16 entries, 
and then keeps being multiplied by `1.2` afterwards.
- THERE ARE NO EXPERIMENTS FOR A LARGE NUMBER OF KEYS. Some testing reveals that this may be tricky with the chosen hash functions.

#### mhash_check_at (mhash_str_prefix)

| keys | mhash (std) | linear (std) | speedup | avg hashes | max memory |
|------|-------------|--------------|---------|------------|------------|
|    2 |  16ns (1ns) |   20ns (0ns) |    1.3x |        1.1 |       7 x4B|
|    3 |  16ns (1ns) |   24ns (0ns) |    1.5x |        1.3 |     431 x4B|
|    4 |  17ns (1ns) |   27ns (1ns) |    1.6x |        1.5 |      14 x4B|
|    5 |  17ns (1ns) |   28ns (0ns) |    1.6x |        1.8 |      25 x4B|
|    6 |  17ns (1ns) |   31ns (1ns) |    1.8x |        1.8 |      27 x4B|
|    7 |  17ns (1ns) |   33ns (0ns) |    1.9x |        2.0 |      47 x4B|
|    8 |  18ns (2ns) |   36ns (1ns) |    2.0x |        2.3 |      35 x4B|
|    9 |  18ns (2ns) |   38ns (2ns) |    2.1x |        2.4 |      59 x4B|
|   10 |  18ns (2ns) |   39ns (1ns) |    2.1x |        2.4 |      55 x4B|
|   20 |  21ns (4ns) |   52ns (1ns) |    2.5x |        3.2 |     223 x4B|
|   30 |  21ns (4ns) |   67ns (2ns) |    3.1x |        3.5 |     478 x4B|
|   40 |  25ns (7ns) |   74ns (1ns) |    3.0x |        4.1 |     765 x4B|
|   50 |  24ns (6ns) |   93ns (2ns) |    3.9x |        3.9 |    1137 x4B|
|   60 |  23ns (6ns) |   96ns (1ns) |    4.1x |        3.9 |    1357 x4B|
|   70 |  27ns (8ns) |  100ns (2ns) |    3.7x |        4.5 |    2277 x4B|
|   80 |  27ns (10ns) |  124ns (1ns) |    4.5x |        4.4 |    3117 x4B|
|   90 |  28ns (9ns) |  151ns (1ns) |    5.4x |        4.5 |    2929 x4B|
|  100 |  29ns (9ns) |  156ns (2ns) |    5.3x |        4.6 |    3892 x4B|
|  200 |  32ns (12ns) |  265ns (2ns) |    8.2x |        5.2 |   16060 x4B|
|  300 |  35ns (14ns) |  377ns (1ns) |   10.9x |        5.6 |   34646 x4B|