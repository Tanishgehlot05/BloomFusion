#ifndef DATABASE_H
#define DATABASE_H

/*
 * Database.h
 *
 * This is the main database class that ties together:
 *   1. BloomFilter  - the probabilistic pre-filter (gates ALL lookups)
 *   2. FusionTree   - the actual ordered key-value store
 *
 * The lookup pipeline:
 * ┌──────────────┐     "definitely not"     ┌──────────────┐
 * │  BloomFilter │ ──────────────────────>  │  return NULL │
 * │  (O(k) hash) │                          └──────────────┘
 * └──────┬───────┘
 *        │ "possibly exists"
 *        ▼
 * ┌──────────────────┐
 * │   Fusion Tree    │  O(log_B n) search
 * │  (wide B-tree)   │
 * └──────────────────┘
 *
 * Result:
 *   - Keys NOT in the DB are rejected in O(k) constant time by the Bloom Filter
 *   - Keys that ARE in the DB are found in O(log_B n) time
 *   - On very large datasets this is dramatically faster than std::map (O(log n))
 *     and std::unordered_map (O(1) average but poor cache behavior at scale)
 */

#include "BloomFilter.h"
#include "FusionTree.h"

class Database
{
public:
    // capacity = expected number of elements (used to size the Bloom Filter)
    Database(int capacity);
    ~Database();

    // Insert a key-value pair (updates value if key already exists)
    void insert(long long key, long long value);

    // Look up a key. Returns true and sets outValue if found.
    // Returns false immediately if Bloom Filter says "not here".
    bool get(long long key, long long &outValue);

    // Remove a key from both the tree (Bloom Filter is NOT updated on delete
    // because Bloom Filters don't support deletion - this is by design)
    bool remove(long long key);

    // Print all records in sorted key order
    void printAll();

    // Stats
    int  getCount();
    void printStats();

    // Silent mode: disables console output in get() (used by benchmark)
    void setSilent(bool s);

private:
    BloomFilter* filter;
    FusionTree*  tree;
    int          capacity;
    bool         silent;   // when true, get() prints nothing
};

#endif
