#ifndef MHASH_MAP_H
#define MHASH_MAP_H

#include "mhash.h"
#include "mhash_str.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <bit>
#include <cstring>
#include <cstdlib>

template<typename ValueType>
class MHashMap {
    struct Entry {
        std::string key;
        ValueType value;
    };
    MHash mhash_{};
    std::vector<MHASH_INDEX_UINT> table_;
    std::vector<Entry> entries_;
    // staging before build
    std::vector<std::string> staged_keys_;
    std::vector<ValueType> staged_values_;
public:
    MHashMap() = default;
    MHashMap(const MHashMap&) = delete;
    MHashMap& operator=(const MHashMap&) = delete;
    MHashMap(MHashMap&& o) noexcept { move_from(std::move(o)); }
    MHashMap& operator=(MHashMap&& o) noexcept {
        if (this != &o) { cleanup(); move_from(std::move(o)); }
        return *this;
    }
    ~MHashMap() { cleanup(); }

    inline void insert(const std::string& key, const ValueType& value) {
        staged_keys_.push_back(key);
        staged_values_.push_back(value);
    }

    inline ValueType* get(const std::string& key) {
        const MHASH_UINT pos = mhash_entry_pos(&mhash_, key.c_str());
        const MHASH_INDEX_UINT entry_idx = mhash_.table[pos];
        if (entry_idx == MHASH_EMPTY_SLOT) [[unlikely]]
            return nullptr;
        Entry& e = entries_[entry_idx];
        if(e.key != key) [[unlikely]]
            return nullptr;
        return &e.value;
    }

    inline ValueType* get_existing(const std::string& key) {
        const MHASH_UINT pos = mhash_entry_pos(&mhash_, key.c_str());
        const MHASH_INDEX_UINT entry_idx = mhash_.table[pos];
        //if (entry_idx == MHASH_EMPTY_SLOT) [[unlikely]]
        //    return nullptr;
        Entry& e = entries_[entry_idx];
        return &e.value;
    }

    inline const ValueType* get(const std::string& key) const {
        return const_cast<MHashMap*>(this)->get(key);
    }

    inline size_t size() const noexcept { return entries_.size(); }
    inline bool empty() const noexcept { return entries_.empty(); }

    void build() {
        if (staged_keys_.empty()) return;
        const size_t new_count = staged_keys_.size();
        const size_t old_count = entries_.size();
        const size_t total_count = old_count + new_count;
        entries_.reserve(total_count);
        for (size_t i = 0; i < new_count; ++i)
            entries_.push_back({std::move(staged_keys_[i]), staged_values_[i]});
        staged_keys_.clear();
        staged_values_.clear();
        rebuild();
    }

    void clear() {
        cleanup();
        entries_.clear();
        staged_keys_.clear();
        staged_values_.clear();
    }

private:
    void rebuild() {
        if (entries_.empty()) return;
        const size_t n = entries_.size();
        size_t table_size = n * 3;
        const size_t max_hashes = std::bit_width(n) + 2;
        const size_t max_table_size = 128 * n;
        table_.assign(table_size, MHASH_EMPTY_SLOT);
        std::vector<const void*> key_ptrs(n);
        for (size_t i = 0; i < n; ++i)
            key_ptrs[i] = entries_[i].key.c_str();
        for (;;) {
            const int success = (mhash_init(&mhash_, table_.data(), table_size, key_ptrs.data(), n, mhash_str_prefix) == MHASH_OK);
            if(success && mhash_.num_hashes < max_hashes) break;
            if(table_size < 16) ++table_size;
            else table_size = table_size + table_size / 5 + 1;
            if(table_size > 65536 || table_size > max_table_size) {
                if (!success) throw std::runtime_error("Failed to build map: either too many collisions, too many keys, or duplicate keys.");
                break;
            }
            table_.assign(table_size, MHASH_EMPTY_SLOT);
        }
    }

    static inline MHASH_UINT mhash_entry_pos(const MHash *ph, const void *s) {
        return mhash__concat(ph->hash_func, ph->num_hashes, s) % (MHASH_UINT)ph->table_size;
    }
    void move_from(MHashMap&& o) noexcept {
        mhash_ = o.mhash_;
        table_ = std::move(o.table_);
        entries_ = std::move(o.entries_);
        staged_keys_ = std::move(o.staged_keys_);
        staged_values_ = std::move(o.staged_values_);
        mhash_.table = table_.data();
    }
    void cleanup() noexcept {
        table_.clear();
        mhash_ = {};
    }
};

#endif // MHASH_MAP_H
