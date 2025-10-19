// COMPILE WITH: gcc examples/example.c -o examples/example -O3

#include "../mhash.h"
#include "../mhash_str.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    int values[] = {1, 2, 3, 4, 5, 6};
    // create map data
    size_t table_size = 17; // ideally let this grow quadratically to the number of entries, and be a prime
    size_t num_entries = 6;
    const char *keys[] = {"Apple", "Banana", "Cherry", "Date", "Doodoo", "D"};
    MHASH_INDEX_UINT table[table_size];

    // initialize the map (ALWAYS check for failure status, because init fails on excessive loads)
    MHash map;
    if(mhash_init(&map, table, table_size, (const void**)keys, num_entries, mhash_str_prefix)) {
        printf("Failed to create map\n");
        return 1;
    }
    printf("%ld hashes\n", map.num_hashes);

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
