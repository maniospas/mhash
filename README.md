# MHash

This is an implementation of a perfect hash function that uniquely maps a given string to a
`uint16_t` identifier. It works by concatenating several hash functions from a parameterized family 
until they combine into a unique hash. The main idea is that, with a large enough table holding 
entry ids, collisions are rare so you will want only a few hashes.

## 🔥 Features

- drop-in header installation
- easy to integrate: no exceptions, no aborts, no rtti
- sub-linear time string indexing
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
operations are internally performed. There can be up to 255 internal hashes, and computational cost is proportional 
to those. The number of stored elements is tracked through `MHash.count`.

⚠️ *Do NOT modify field values.*

#### mhash_init

It does not allocate any memory, but requires a manually allocated table pointer and associated capacity for its internal use.
It guarantees that it will use only the indicated memory region (so you can retrieve pointers from larger buffers). Do note that 
MHash does *not* track keys and values, so you need to manage those independently. The only requirement is that keys are known 
at the point where you call `mhash_init`. 

The last argument is a function pointer to computing the hash function given a `const void*` pointer to an instance of your
data type and a `uint8_t` identifier of the hash function within its family. See *mhash_str.h* for string implementations;
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

- Experiments only cover strings, which was my target use case.
- Ran on uniformly random string characters and `mhash_str_all` or `mhash_str_prefix` hashing. 
Natural language is not distributed this way, but if you know about easy distinction based on early characters
you can choose the second of the two hashing strategies, which combines the advantages of both hashing and
early exiting from comparisons.
- Tested strings only had `16` characters. This is not indicative of real world conditions.
- Times are measured across 1E6 random lookups and divided by total time for those to run. There are 100 experiment repetitions, but I 
do not report standard deviation to make things easier to look at.
- Internal hash table size is `2*n+n*n/2`, where `n` the number of entries. This was robust in producing only a few hashes but still
succeeding on the first try. Find a schema yourself (ideally with a retry to grow tables when it fails). One option that I recommend
is setting the maximum memory to, say, (stack-allocated)
1024 or 4096 table entries and just increase that as needed or fail your program if there are too many keys and a unique function
is not identified - this could be alright in certain applications. If you are alright with up to 255 hashes without failing, 
set up `#define MHASH_NO_WORST_CASE` instead.
- THERE ARE NO EXPERIMENTS FOR A LARGE NUMBER OF KEYS. Some testing reveals that the hashes of *mhash_str.h* are not well-suited
to, thousands of keys in that they fail to find unique combinations.

#### mhash_entry (mhash_str_all)

| keys | mhash (std) | linear (std) | speedup | avg hashes | memory    |
|------|-------------|--------------|---------|------------|-----------|
|    2 |  29ns (8ns) |   20ns (1ns) |    0.7x |        1.2 |      1 kb |
|    3 |  31ns (10ns) |   25ns (0ns) |    0.8x |        1.4 |      1 kb |
|    4 |  34ns (13ns) |   27ns (0ns) |    0.8x |        1.5 |      1 kb |
|    5 |  37ns (16ns) |   29ns (0ns) |    0.8x |        1.7 |      1 kb |
|    6 |  37ns (16ns) |   29ns (0ns) |    0.8x |        1.7 |      1 kb |
|    7 |  38ns (16ns) |   30ns (0ns) |    0.8x |        1.8 |      1 kb |
|    8 |  37ns (18ns) |   31ns (0ns) |    0.8x |        1.8 |      1 kb |
|    9 |  42ns (21ns) |   34ns (0ns) |    0.8x |        2.1 |      1 kb |
|   10 |  37ns (21ns) |   35ns (0ns) |    1.0x |        1.8 |      1 kb |
|   20 |  45ns (22ns) |   52ns (1ns) |    1.2x |        2.3 |      1 kb |
|   30 |  47ns (26ns) |   50ns (1ns) |    1.1x |        2.5 |      1 kb |
|   40 |  45ns (25ns) |   85ns (1ns) |    1.9x |        2.3 |      2 kb |
|   50 |  47ns (27ns) |   79ns (0ns) |    1.7x |        2.4 |      3 kb |
|   60 |  47ns (28ns) |   96ns (0ns) |    2.0x |        2.4 |      4 kb |
|   70 |  48ns (27ns) |  123ns (1ns) |    2.5x |        2.5 |      6 kb |
|   80 |  50ns (30ns) |  121ns (1ns) |    2.4x |        2.6 |      7 kb |
|   90 |  46ns (25ns) |  125ns (0ns) |    2.7x |        2.4 |      9 kb |
|  100 |  49ns (28ns) |  161ns (1ns) |    3.3x |        2.6 |     11 kb |

#### mhash_entry (mhash_str_prefix)

| keys | mhash (std) | linear (std) | speedup | avg hashes | memory    |
|------|-------------|--------------|---------|------------|-----------|
|    2 |  16ns (1ns) |   20ns (0ns) |    1.2x |        1.1 |      1 kb |
|    3 |  17ns (4ns) |   24ns (0ns) |    1.4x |        1.6 |      1 kb |
|    4 |  17ns (4ns) |   27ns (0ns) |    1.6x |        1.6 |      1 kb |
|    5 |  18ns (4ns) |   28ns (0ns) |    1.6x |        1.6 |      1 kb |
|    6 |  17ns (3ns) |   29ns (0ns) |    1.7x |        1.6 |      1 kb |
|    7 |  18ns (6ns) |   29ns (0ns) |    1.6x |        1.8 |      1 kb |
|    8 |  20ns (7ns) |   31ns (0ns) |    1.6x |        2.1 |      1 kb |
|    9 |  19ns (7ns) |   34ns (0ns) |    1.8x |        1.9 |      1 kb |
|   10 |  20ns (11ns) |   35ns (0ns) |    1.8x |        2.0 |      1 kb |
|   20 |  20ns (9ns) |   52ns (1ns) |    2.6x |        2.1 |      1 kb |
|   30 |  23ns (13ns) |   50ns (1ns) |    2.1x |        2.6 |      1 kb |
|   40 |  21ns (10ns) |   85ns (0ns) |    3.9x |        2.4 |      2 kb |
|   50 |  23ns (11ns) |   79ns (0ns) |    3.4x |        2.7 |      3 kb |
|   60 |  26ns (14ns) |   96ns (0ns) |    3.7x |        3.1 |      4 kb |
|   70 |  26ns (17ns) |  123ns (1ns) |    4.8x |        3.0 |      6 kb |
|   80 |  23ns (12ns) |  122ns (0ns) |    5.2x |        2.7 |      7 kb |
|   90 |  25ns (17ns) |  125ns (0ns) |    5.0x |        3.0 |      9 kb |
|  100 |  26ns (12ns) |  161ns (1ns) |    6.2x |        3.3 |     11 kb |