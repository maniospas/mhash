# MHash

This is an implementation of a hash function that uniquely maps a given string to a
`uint16_t` identifier. It works by concatenating several hash functions from a parameterized family 
until they combine into a unique hash. The main idea is that, with a large enough table holding 
entry ids, collisions are rare so you will want only a few hashes.

**The main goal is to substitute linear search for few (e.g., &lt;255) alternatives.**
In fact, in example benchmark we get +15% speedup compared to even simple lookup across two strings
at the cost of at most 28 bytes of memory!!!

## 🔥 Features

- drop-in header installation
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
    uint16_t table[table_size];

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

The last argument is a function pointer to computing the hash function given a `const void*` pointer to an instance of your
data type and a `uint16_t` identifier of the hash function within its family. See *mhash_str.h* for string implementations;
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
|    2 |  17ns (2ns) |   20ns (2ns) |    1.2x |        1.1 |       7 x4B|
|    3 |  18ns (1ns) |   24ns (0ns) |    1.4x |        1.3 |      10 x4B|
|    4 |  18ns (2ns) |   28ns (0ns) |    1.6x |        1.4 |      14 x4B|
|    5 |  19ns (3ns) |   28ns (0ns) |    1.5x |        1.6 |      25 x4B|
|    6 |  19ns (3ns) |   31ns (2ns) |    1.6x |        1.7 |      33 x4B|
|    7 |  19ns (3ns) |   32ns (2ns) |    1.7x |        1.7 |      47 x4B|
|    8 |  22ns (6ns) |   33ns (1ns) |    1.5x |        2.2 |      35 x4B|
|    9 |  22ns (6ns) |   36ns (2ns) |    1.6x |        2.1 |      49 x4B|
|   10 |  21ns (5ns) |   37ns (1ns) |    1.8x |        2.0 |      55 x4B|
|   20 |  25ns (9ns) |   49ns (2ns) |    2.0x |        2.7 |     185 x4B|
|   30 |  25ns (8ns) |   58ns (1ns) |    2.3x |        2.8 |     478 x4B|
|   40 |  28ns (11ns) |   73ns (1ns) |    2.6x |        3.1 |     637 x4B|
|   50 |  30ns (12ns) |   87ns (1ns) |    2.9x |        3.3 |    1137 x4B|
|   60 |  32ns (11ns) |   96ns (2ns) |    3.0x |        3.7 |    1357 x4B|
|   70 |  35ns (14ns) |  115ns (2ns) |    3.3x |        3.9 |    1897 x4B|
|   80 |  37ns (15ns) |  126ns (1ns) |    3.4x |        4.1 |    2597 x4B|
|   90 |  37ns (16ns) |  137ns (2ns) |    3.7x |        4.1 |    3515 x4B|
|  100 |  38ns (14ns) |  149ns (3ns) |    3.9x |        4.3 |    3243 x4B|
|  200 |  41ns (17ns) |  264ns (2ns) |    6.5x |        4.8 |   23128 x4B|
|  300 |  43ns (19ns) |  395ns (3ns) |    9.2x |        5.0 |   41576 x4B|