#include "mhash.h"
#include <stdio.h>

int main(void) {
    int values[] = {1, 2, 3, 4, 5, 6};
    // create map data
    size_t table_size = 36; // ideally let this grow quadratically to the number of entries
    size_t num_entries = 6;
    const char *keys[] = {"Apple", "Banana", "Cherry", "Date", "Doodoo", "D"};
    uint16_t table[table_size];

    // initialize the map (ALWAYS check for failure status, because init fails on excessive loads)
    MHash map;
    if(mhash_init(&map, table, table_size, keys, num_entries, hash_prefix)) {
        printf("Failed to create map\n");
        return 1;
    }
    printf("%d hashes\n", map.num_hashes);

    // query the map (retrieves void*)
    const char *query = "Cherry";
    printf("%s -> %d\n", query, values[mhash_entry(&map, query)]);
    return 0;
}
