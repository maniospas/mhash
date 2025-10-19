# MHash

This is an implementation of a perfect hash function that uniquely maps a given string to a
`uint16_t` identifier. It works by adding hash functions from a parameterized family 
until they combine into a unique hash. The main idea is that, with a large enough table holding 
entry position ids, collisions are rare so you will want only a few hashes.

## :rocket: Features

- drop-in header installation
- easy to integrate: no exceptions, no aborts, no rtti
- sub-linear time string indexing
- easily extensible: implement a hash and -optionally- a comparator

## :zap: Quickstart

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
    size_t table_size = 36; // ideally let this grow quadratically to the number of entries
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

## :book: API

#### MHash

The hash map data structure. Once created, you can access `MHash.num_hashes` to get a sense of how many hashing 
operations are internally performed. There can be up to 255 internal hashes, and computational cost is proportional 
to those. The number of stored elements is tracked through `MHash.count`.

⚠️⚠️⚠️ *Do NOT modify field values.*

#### mhash_init

It does not allocate any memory, but requires a manually allocated table pointer and associated capacity for its internal use.
It guarantess that it will use only the indicated memory region (so you can retrieve pointers from larger buffers). Do note that 
MHash does *not* track keys and values, so you need to manage those independently. The only requirement is that keys are known 
at the point where you call `mhash_init`.

#### mhash_entry

Retrieves the value associated with a given string in the range `0`..`MHash.count-1`. If you want a mapping to ids, there is no 
need to store any kind of value elsewhere.

⚠️⚠️⚠️ *Calling mhash_entry for a non-registed key is UB.* Do not attempt to error check the result afterwards by looking at the
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
