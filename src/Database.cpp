#include "Database.h"
#include <iostream>

/*
 * Database.cpp
 *
 * The Database is the user-facing wrapper.
 * It orchestrates the Bloom Filter and Fusion Tree together.
 *
 * NOTE on Bloom Filter + Delete:
 * --------------------------------
 * Standard Bloom Filters do NOT support deletion.
 * Reason: multiple keys may share the same bit positions.
 * If we clear bits for a deleted key, we might corrupt the filter
 * for other keys that happened to hash to the same bits.
 *
 * Solution used here (standard approach for learning projects):
 *   - Bloom Filter bits are NEVER cleared on delete
 *   - A deleted key's bits stay set -> might cause a false positive
 *     (filter says "might exist", tree search confirms "nope, not there")
 *   - This is the CLASSIC trade-off of Bloom Filters with deletion
 *   - For production, one would use a "Counting Bloom Filter" (uses counters
 *     instead of bits, allows decrement on delete)
 */

Database::Database(int cap)
{
    capacity = cap;
    silent   = false;

    // 7 hash functions is the optimal number for a 10-bits-per-element Bloom Filter
    filter = new BloomFilter(cap, 7);
    tree   = new FusionTree();
}

Database::~Database()
{
    delete filter;
    delete tree;
}

void Database::setSilent(bool s)
{
    silent = s;
}

void Database::insert(long long key, long long value)
{
    filter->insert(key);
    tree->insert(key, value);
}

bool Database::get(long long key, long long &outValue)
{
    if (!filter->mightContain(key))
    {
        if (!silent)
            std::cout << "  [Bloom Filter] Key " << key << " is DEFINITELY not in the database.\n";
        return false;
    }

    if (!silent)
        std::cout << "  [Bloom Filter] Key " << key << " might exist. Checking Fusion Tree...\n";

    bool found = tree->search(key, outValue);

    if (!silent)
    {
        if (!found)
            std::cout << "  [Fusion Tree] Key " << key << " confirmed NOT found (Bloom false positive).\n";
        else
            std::cout << "  [Fusion Tree] Key " << key << " FOUND. Value = " << outValue << "\n";
    }

    return found;
}

bool Database::remove(long long key)
{
    // Bloom Filter is not updated (see note in Database.cpp header)
    bool removed = tree->remove(key);
    if (removed)
        std::cout << "  Key " << key << " removed from Fusion Tree.\n";
    else
        std::cout << "  Key " << key << " not found, nothing to remove.\n";
    return removed;
}

void Database::printAll()
{
    if (tree->getCount() == 0)
    {
        std::cout << "  Database is empty.\n";
        return;
    }
    std::cout << "  All records (sorted by key):\n";
    tree->printInOrder();
}

int Database::getCount()
{
    return tree->getCount();
}

void Database::printStats()
{
    std::cout << "\n  ===== Database Stats =====\n";
    std::cout << "  Records in Fusion Tree   : " << tree->getCount()        << "\n";
    std::cout << "  Bloom Filter size (bits) : " << filter->getSize()       << "\n";
    std::cout << "  Bloom Filter hash count  : " << filter->getNumHashes()  << "\n";
    std::cout << "  Branching factor (B)     : " << FUSION_B                << "\n";
    std::cout << "  ==========================\n\n";
}
