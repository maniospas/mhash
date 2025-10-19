# MHash

This is an implementation of a perfect hash function that uniquely maps a given string to a
`uint16_t` identifier. It works by adding hash functions from a parameterized family 
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

- Experiments only have one repetition and only cover strings, which is what I was looking for.
- Ran on uniformly random string characters and `mhash_str_all` or `mhash_str_prefix` hashing. 
Natural language is not distributed this way, but if you know about easy distinction based on early characters
you can choose the second of the two hashing strategies, which combines the advantages of both hashing and
early exiting from comparisons.
- Tested strings only had `16` characters. This is not indicative of real world conditions.
- Internal hash table size is `2*n+n*n/2`, where `n` the number of entries. This was robust in producing only a few hashes but still
succeeding on the first try. Find a schema yourself (ideally with a retry to grow tables when it fails). For example,
the `mhash_str_prefix` requires 6 hashes for the specific experiment on 8 keys, which can easily be addressed by growing
table sizes. The cost of this table is what grows the used memory (the latter is rounded up).

#### mhash_entry (mhash_str_all)

| #keys | mhash_entry | linear | speedup | hashes | memory |
| ----- | ----------- | ------ | ------- | ------ | ------ |
| 2 | 75 ns | 20 ns | 0.26x | 2 hash | 1kB |
| 3 | 28 ns | 24 ns | 0.86x | 1 hash | 1kB |
| 4 | 50 ns | 28 ns | 0.57x | 2 hash | 1kB |
| 5 | 27 ns | 29 ns | 1.09x | 1 hash | 1kB |
| 6 | 43 ns | 29 ns | 0.68x | 2 hash | 1kB |
| 7 | 27 ns | 30 ns | 1.11x | 1 hash | 1kB |
| 8 | 29 ns | 32 ns | 1.11x | 1 hash | 1kB |
| 9 | 28 ns | 33 ns | 1.18x | 1 hash | 1kB |
| 10 | 45 ns | 36 ns | 0.81x | 2 hash | 1kB |
| 20 | 44 ns | 52 ns | 1.17x | 2 hash | 1kB |
| 30 | 65 ns | 50 ns | 0.76x | 3 hash | 1kB |
| 40 | 72 ns | 86 ns | 1.19x | 4 hash | 2kB |
| 50 | 46 ns | 81 ns | 1.77x | 2 hash | 3kB |
| 60 | 28 ns | 96 ns | 3.45x | 1 hash | 4kB |
| 70 | 25 ns | 124 ns | 4.88x | 1 hash | 6kB |
| 80 | 42 ns | 122 ns | 2.87x | 2 hash | 7kB |
| 90 | 26 ns | 128 ns | 5.00x | 1 hash | 9kB |

#### mhash_entry (mhash_str_prefix)

| #keys | mhash_entry | linear | speedup | hashes | memory |
| ----- | ----------- | ------ | ------- | ------ | ------ |
| 2 | 22 ns | 32 ns | 1.47x | 1 hash | 1kB |
| 3 | 17 ns | 24 ns | 1.42x | 1 hash | 1kB |
| 4 | 22 ns | 30 ns | 1.39x | 1 hash | 1kB |
| 5 | 16 ns | 28 ns | 1.70x | 1 hash | 1kB |
| 6 | 24 ns | 28 ns | 1.16x | 3 hash | 1kB |
| 7 | 20 ns | 29 ns | 1.43x | 2 hash | 1kB |
| 8 | 47 ns | 30 ns | 0.64x | 6 hash | 1kB |
| 9 | 17 ns | 31 ns | 1.86x | 1 hash | 1kB |
| 10 | 17 ns | 35 ns | 2.04x | 1 hash | 1kB |
| 20 | 20 ns | 51 ns | 2.54x | 2 hash | 1kB |
| 30 | 20 ns | 50 ns | 2.49x | 2 hash | 1kB |
| 40 | 20 ns | 80 ns | 3.97x | 2 hash | 2kB |
| 50 | 31 ns | 83 ns | 2.68x | 4 hash | 3kB |
| 60 | 24 ns | 96 ns | 3.99x | 3 hash | 4kB |
| 70 | 20 ns | 131 ns | 6.59x | 2 hash | 6kB |
| 80 | 24 ns | 122 ns | 5.04x | 3 hash | 7kB |
| 90 | 31 ns | 130 ns | 4.25x | 4 hash | 9kB |