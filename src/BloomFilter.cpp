#include "BloomFilter.h"
#include <iostream>


BloomFilter::BloomFilter(int capacity, int numHashFunctions)
{
    // Rule of thumb: 10 bits per element gives ~1% false positive rate
    totalBits = capacity * 10;
    if (totalBits < 64)
        totalBits = 64;

    // We store bits inside int slots (each int has 32 bits)
    arraySize = (totalBits + 31) / 32;

    bitArray = new int[arraySize];
    for (int i = 0; i < arraySize; i++)
        bitArray[i] = 0;

    numHashes = numHashFunctions;
    if (numHashes < 1)  numHashes = 1;
    if (numHashes > 20) numHashes = 20;
}

BloomFilter::~BloomFilter()
{
    delete[] bitArray;
}

void BloomFilter::clear()
{
    for (int i = 0; i < arraySize; i++)
        bitArray[i] = 0;
}

int BloomFilter::getSize()
{
    return totalBits;
}

int BloomFilter::getNumHashes()
{
    return numHashes;
}

// setBit: turns on the bit at position 'index' in our bit array
void BloomFilter::setBit(int index)
{
    int slot   = index / 32;    // which int slot holds this bit
    int offset = index % 32;    // which bit inside that slot
    bitArray[slot] = bitArray[slot] | (1 << offset);
}

// getBit: checks if the bit at position 'index' is ON (1) or OFF (0)
bool BloomFilter::getBit(int index)
{
    int slot   = index / 32;
    int offset = index % 32;
    return (bitArray[slot] & (1 << offset)) != 0;
}

/*
 * FNV-1a hash implementation
 *
 * We break the 'key' into individual bytes and feed each byte
 * through the FNV-1a algorithm.
 *
 * The 'seed' is XORed into the key before hashing so that
 * calling this with seed=0, seed=1, seed=2, ... gives us
 * completely different hash values for the same key.
 */
int BloomFilter::fnv1aHash(long long key, int seed)
{
    // FNV-1a 32-bit constants (standard values, always used)
    unsigned int hash  = 2166136261u;
    unsigned int prime = 16777619u;

    // Mix the seed into the key to get a different starting point
    long long mixedKey = key ^ ((long long)seed * 2654435761LL);

    // Process each of the 8 bytes of the long long key
    unsigned char* bytes = (unsigned char*)&mixedKey;
    for (int i = 0; i < 8; i++)
    {
        hash ^= (unsigned int)bytes[i];   // step a: XOR with byte
        hash *= prime;                     // step b: multiply by prime
    }

    // Make sure the result is a valid positive index
    if (hash < 0) hash = -hash;
    return (int)(hash % (unsigned int)totalBits);
}

/*
 * insert: to add a key to the bloom filter
 * We compute k different hash values for the key,
 * and set those k bit positions to 1.
 */
void BloomFilter::insert(long long key)
{
    for (int i = 0; i < numHashes; i++)
    {
        int bitPosition = fnv1aHash(key, i);
        setBit(bitPosition);
    }
}

/*
 * mightContain: to check if a key could be in the set
 *
 * We compute the same k hash values.
 * If ALL k bits are set -> key MIGHT be present (possible false positive)
 * If ANY one bit is 0   -> key is DEFINITELY NOT present (100% certain)
 *
 * This is the core power of a Bloom Filter:
 *   false negatives are IMPOSSIBLE
 *   false positives are POSSIBLE but rare (~1% with good sizing)
 */
bool BloomFilter::mightContain(long long key)
{
    for (int i = 0; i < numHashes; i++)
    {
        int bitPosition = fnv1aHash(key, i);
        if (!getBit(bitPosition))
            return false;  // definitely not present
    }
    return true;  // possibly present -> go check the tree
}
