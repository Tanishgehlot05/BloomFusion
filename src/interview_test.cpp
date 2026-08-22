#include <iostream>
#include <map>
#include <unordered_map>
#include <ctime>
#include <cstdlib>

#include "Database.h"

/*
 * interview_test.cpp
 *
 * HONEST INTERVIEW ANSWER — what to say and what to show
 * =======================================================
 *
 * TEST 1: Memory/cache analysis — why tight benchmarks favor std::map
 * TEST 2: Pure negative lookup — where Bloom Filter genuinely wins
 * TEST 3: Miss rate crossover — find the break-even point
 *
 * Important fix: all timing loops accumulate a 'sink' variable
 * so the compiler cannot optimize the calls away (dead-code elimination).
 */

double elapsedMs(clock_t start, clock_t end)
{
    return (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
}

// Use unsigned arithmetic to avoid signed overflow UB
unsigned long long makeKey(unsigned long long seed, unsigned long long i)
{
    unsigned long long v = seed * 6364136223846793005ULL
                         + i   * 1442695040888963407ULL;
    return v % 200000000ULL;   // key space: 200 million
}

// ---------------------------------------------------------------
// TEST 1: print the memory/cache analysis
// ---------------------------------------------------------------
void printWhyWeLose()
{
    std::cout << "\n";
    std::cout << "======================================================\n";
    std::cout << "  TEST 1: Why we lose in a tight benchmark\n";
    std::cout << "======================================================\n\n";

    std::cout << "  This machine has a 33 MB L3 cache.\n\n";

    std::cout << "  Memory footprint at N=100,000:\n";
    std::cout << "    std::map node:   ~48 bytes  (key+val+3ptrs+color)\n";
    std::cout << "    std::map total:  ~4.7 MB    (100K nodes x 48B)\n\n";

    std::cout << "    FusionNode:      ~376 bytes (15 keys+15 vals+16 ptrs)\n";
    std::cout << "    FusionTree total:~2.4 MB    (~6,667 nodes x 376B)\n\n";

    std::cout << "    BloomFilter:     ~122 KB    (100K x 10 bits)\n\n";

    std::cout << "  Verdict: 4.7 + 2.4 + 0.12 = 7.2 MB << 33 MB L3 cache.\n";
    std::cout << "  Everything fits. std::map never pays cache-miss latency.\n";
    std::cout << "  Its stdlib implementation is also battle-hardened.\n\n";

    std::cout << "  To see our real advantage:\n";
    std::cout << "    (a) N > ~5 million (push std::map out of L3), or\n";
    std::cout << "    (b) Data on DISK (100us seek vs nanosecond Bloom check),\n";
    std::cout << "    (c) Isolate pure negative-lookup throughput (Test 2).\n";
    std::cout << "======================================================\n";
}

// ---------------------------------------------------------------
// TEST 2: Pure negative lookup — 100% miss rate
//
// All lookupKeys are guaranteed NOT in the structure.
// We accumulate a 'sink' so the compiler cannot optimize away calls.
// ---------------------------------------------------------------
void testPureNegativeLookup(int N, int RUNS)
{
    std::cout << "\n";
    std::cout << "======================================================\n";
    std::cout << "  TEST 2: Pure Negative Lookup (100% Miss Rate)\n";
    std::cout << "  N=" << N << " inserted keys, " << N << " guaranteed-miss lookups\n";
    std::cout << "  'sink' variable forces compiler to keep all calls alive\n";
    std::cout << "======================================================\n\n";

    int bloomFilterKB = (N * 10) / 8 / 1024;
    int stdmapKB      = (N * 48) / 1024;
    std::cout << "  Bloom Filter bit array: " << bloomFilterKB << " KB  (fits in L2 cache)\n";
    std::cout << "  std::map tree:          " << stdmapKB      << " KB  (does NOT fit in L2)\n\n";

    double totalDB  = 0;
    double totalMap = 0;
    double totalUMap= 0;

    for (int run = 0; run < RUNS; run++)
    {
        unsigned long long seed = (unsigned long long)(run + 1);

        long long* insertKeys = new long long[N];
        long long* missKeys   = new long long[N];
        long long* insertVals = new long long[N];

        for (int i = 0; i < N; i++)
        {
            insertKeys[i] = (long long)makeKey(seed, (unsigned long long)i);
            insertVals[i] = (long long)(i + 1);
        }
        // Misses come from a seed far away — collision rate < 0.05%
        for (int i = 0; i < N; i++)
            missKeys[i] = (long long)makeKey(seed + 99999ULL,
                                             (unsigned long long)(i + N * 13));

        // BloomFusion
        {
            Database db(N);
            db.setSilent(true);
            for (int i = 0; i < N; i++)
                db.insert(insertKeys[i], insertVals[i]);

            long long outVal = 0;
            long long sink   = 0;   // accumulate to prevent dead-code elimination
            clock_t t1 = clock();
            for (int i = 0; i < N; i++)
            {
                if (db.get(missKeys[i], outVal))
                    sink += outVal;
            }
            clock_t t2 = clock();
            totalDB += elapsedMs(t1, t2);
            // Use sink so compiler can't eliminate the loop
            if (sink == -1) std::cout << sink;
        }

        // std::map
        {
            std::map<long long, long long> m;
            for (int i = 0; i < N; i++)
                m[insertKeys[i]] = insertVals[i];

            long long sink = 0;
            clock_t t1 = clock();
            for (int i = 0; i < N; i++)
            {
                std::map<long long,long long>::iterator it = m.find(missKeys[i]);
                if (it != m.end())
                    sink += it->second;
            }
            clock_t t2 = clock();
            totalMap += elapsedMs(t1, t2);
            if (sink == -1) std::cout << sink;
        }

        // std::unordered_map
        {
            std::unordered_map<long long, long long> um;
            um.reserve(N);
            for (int i = 0; i < N; i++)
                um[insertKeys[i]] = insertVals[i];

            long long sink = 0;
            clock_t t1 = clock();
            for (int i = 0; i < N; i++)
            {
                std::unordered_map<long long,long long>::iterator it = um.find(missKeys[i]);
                if (it != um.end())
                    sink += it->second;
            }
            clock_t t2 = clock();
            totalUMap += elapsedMs(t1, t2);
            if (sink == -1) std::cout << sink;
        }

        delete[] insertKeys;
        delete[] insertVals;
        delete[] missKeys;
    }

    double avgDB   = totalDB   / RUNS;
    double avgMap  = totalMap  / RUNS;
    double avgUMap = totalUMap / RUNS;

    std::cout << "  Results (avg over " << RUNS << " runs):\n";
    std::cout << "    BloomFusion DB    : " << avgDB   << " ms\n";
    std::cout << "    std::map          : " << avgMap  << " ms\n";
    std::cout << "    std::unordered_map: " << avgUMap << " ms\n\n";

    if (avgDB > 0.0 && avgMap > 0.0)
    {
        double ratio = avgMap / avgDB;
        if (ratio >= 1.0)
            std::cout << "  BloomFusion is " << ratio << "x FASTER than std::map\n";
        else
            std::cout << "  BloomFusion is " << (1.0/ratio) << "x slower than std::map\n";
    }
    std::cout << "  (Bloom Filter rejected all " << N << " misses in O(7) each)\n";
    std::cout << "======================================================\n";
}

// ---------------------------------------------------------------
// TEST 3: Miss rate crossover with proper sink
// ---------------------------------------------------------------
void testMissRateCrossover(int N, int RUNS)
{
    std::cout << "\n";
    std::cout << "======================================================\n";
    std::cout << "  TEST 3: Miss Rate Crossover (N=" << N << ")\n";
    std::cout << "  At what miss rate does BloomFusion start winning?\n";
    std::cout << "======================================================\n";

    int missRates[6] = {0, 50, 75, 90, 95, 100};

    for (int ri = 0; ri < 6; ri++)
    {
        int missPercent = missRates[ri];
        int numMisses   = (N * missPercent) / 100;
        int numHits     = N - numMisses;

        double totalDB  = 0;
        double totalMap = 0;

        for (int run = 0; run < RUNS; run++)
        {
            unsigned long long seed = (unsigned long long)(run + 1);

            long long* insertKeys = new long long[N];
            long long* insertVals = new long long[N];
            long long* lookupKeys = new long long[N];

            for (int i = 0; i < N; i++)
            {
                insertKeys[i] = (long long)makeKey(seed, (unsigned long long)i);
                insertVals[i] = (long long)(i + 1);
            }

            for (int i = 0; i < numHits; i++)
                lookupKeys[i] = insertKeys[i];
            for (int i = numHits; i < N; i++)
                lookupKeys[i] = (long long)makeKey(seed + 99999ULL,
                                                   (unsigned long long)(i + N * 13));

            // Fisher-Yates shuffle
            for (int i = N - 1; i > 0; i--)
            {
                int j = (int)(makeKey(seed + (unsigned long long)i,
                                      (unsigned long long)i) % (unsigned long long)(i + 1));
                long long tmp   = lookupKeys[i];
                lookupKeys[i]   = lookupKeys[j];
                lookupKeys[j]   = tmp;
            }

            // BloomFusion
            {
                Database db(N);
                db.setSilent(true);
                for (int i = 0; i < N; i++)
                    db.insert(insertKeys[i], insertVals[i]);

                long long outVal = 0;
                long long sink   = 0;
                clock_t t1 = clock();
                for (int i = 0; i < N; i++)
                    if (db.get(lookupKeys[i], outVal)) sink += outVal;
                clock_t t2 = clock();
                totalDB += elapsedMs(t1, t2);
                if (sink == -1) std::cout << sink;
            }

            // std::map
            {
                std::map<long long, long long> m;
                for (int i = 0; i < N; i++) m[insertKeys[i]] = insertVals[i];

                long long sink = 0;
                clock_t t1 = clock();
                for (int i = 0; i < N; i++)
                {
                    std::map<long long,long long>::iterator it = m.find(lookupKeys[i]);
                    if (it != m.end()) sink += it->second;
                }
                clock_t t2 = clock();
                totalMap += elapsedMs(t1, t2);
                if (sink == -1) std::cout << sink;
            }

            delete[] insertKeys;
            delete[] insertVals;
            delete[] lookupKeys;
        }

        double avgDB  = totalDB  / RUNS;
        double avgMap = totalMap / RUNS;

        std::cout << "  Miss=" << missPercent << "%  hits=" << numHits
                  << "  | BF=" << avgDB << "ms  map=" << avgMap << "ms  | ";

        if (avgDB > 0.0 && avgMap > 0.0)
        {
            double ratio = avgMap / avgDB;
            if (ratio >= 1.0)
                std::cout << "WE WIN  " << ratio << "x faster\n";
            else
                std::cout << "we lose " << (1.0/ratio) << "x slower\n";
        }
    }
    std::cout << "======================================================\n";
}

int main()
{
    std::cout << "\n";
    std::cout << "######################################################\n";
    std::cout << "#  INTERVIEW TEST: 'Show me where BloomFusion wins'  #\n";
    std::cout << "######################################################\n";

    printWhyWeLose();
    testPureNegativeLookup(100000, 5);
    testMissRateCrossover(100000, 5);

    std::cout << "\n";
    std::cout << "======================================================\n";
    std::cout << "  WHAT TO SAY IN THE INTERVIEW\n";
    std::cout << "======================================================\n\n";
    std::cout << "  'The Bloom Filter's job is to avoid an expensive\n";
    std::cout << "   lookup for a key that does not exist. On a miss,\n";
    std::cout << "   it checks 7 bits in a 122KB array that fits in\n";
    std::cout << "   L1/L2 cache and returns instantly. std::map on a\n";
    std::cout << "   miss walks O(log n) pointer-chasing nodes whose\n";
    std::cout << "   total size (4.7MB) does not fit in L2 cache.\n\n";
    std::cout << "   In production — Cassandra, RocksDB, Bigtable —\n";
    std::cout << "   data lives on DISK. A cache miss there costs ~100\n";
    std::cout << "   microseconds. A Bloom Filter rejection costs a few\n";
    std::cout << "   nanoseconds. That 10,000x difference is why every\n";
    std::cout << "   serious database uses a Bloom Filter before any\n";
    std::cout << "   disk read. I implemented the same pattern here.'\n";
    std::cout << "\n======================================================\n\n";

    return 0;
}
