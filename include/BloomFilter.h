#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

/*
 * BloomFilter.h
 *
 * A probabilistic data structure that tells us:
 *   - "DEFINITELY NOT in the set"  -> skip the expensive tree lookup entirely
 *   - "POSSIBLY in the set"        -> go check the Fusion Tree
 *
 * Hash functions used: FNV-1a (Fowler–Noll–Vo variant 1a)
 *   - Simple, fast, and widely taught in CS courses
 *   - We seed it differently each round to simulate multiple hash functions
 *
 * False positive rate is kept ~1% with good sizing (10 bits per element).
 */

class BloomFilter
{
public:
    BloomFilter(int capacity, int numHashFunctions);
    ~BloomFilter();

    void insert(long long key);
    bool mightContain(long long key);
    void clear();

    // for display / debugging
    int getSize();
    int getNumHashes();

private:
    // the bit array stored as plain ints (each int = 32 bits)
    int* bitArray;
    int  arraySize;    // number of int slots
    int  totalBits;    // total number of bits
    int  numHashes;    // how many hash functions to simulate

    void  setBit(int index);
    bool  getBit(int index);

    // FNV-1a hash: takes the key and a seed to produce different hash values
    // seed lets us simulate multiple independent hash functions cheaply
    int fnv1aHash(long long key, int seed);
};

#endif
