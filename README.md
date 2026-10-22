# BloomFusion

A resume-level learning project implementing a custom key-value database
that combines two advanced data structures:

1. **Bloom Filter** — probabilistic pre-filter (FNV-1a hash)
2. **Fusion Tree** — wide B-tree with "fusion nodes" (O(log_B n) search)

No frontend, no backend, no external libraries. Pure C++ multi-file class system.

---

## Project Structure

```
BloomFusionDB/
├── include/
│   ├── BloomFilter.h      # Bloom Filter class interface
│   ├── FusionTree.h       # Fusion Tree / Fusion Node class interface
│   └── Database.h         # Database wrapper interface
├── src/
│   ├── BloomFilter.cpp    # FNV-1a hash implementation, bit array
│   ├── FusionTree.cpp     # B-tree with wide nodes: insert, search, delete
│   ├── Database.cpp       # Orchestrates BloomFilter + FusionTree
│   ├── main.cpp           # Interactive menu-based interface
│   └── benchmark.cpp      # Performance comparison vs std::map and unordered_map
├── Makefile
└── README.md
```

---

## Build

```bash
make          # builds ./db (interactive menu)
make bench    # builds ./benchmark
make all      # builds both
make clean    # removes built files
```

Requires: g++ with C++11 support (standard on any modern Linux/Mac/WSL)

---

## Run

### Interactive Database

```bash
./db
```

You'll be asked for the expected number of elements (used to size the Bloom Filter).
Then a menu appears:

```
1. Insert (key, value)
2. Get value by key       <- shows Bloom Filter + Fusion Tree steps
3. Delete key
4. Print all records (sorted by key)
5. Show database stats
6. Exit
```

Keys and values are **long long integers**.

### Benchmark

```bash
./benchmark
```

Runs INSERT and LOOKUP at N = 1,000 / 10,000 / 100,000 across 3 runs each.
Compares average time of our DB vs `std::map` vs `std::unordered_map`.

---

## How It Works

### Bloom Filter (BloomFilter.h / BloomFilter.cpp)

A Bloom Filter is a **probabilistic data structure** that answers the question:
> "Has this key ever been inserted?"

It can say two things:
- **"DEFINITELY NO"** — the key is 100% not in the database. Skip the tree entirely.
- **"POSSIBLY YES"** — the key might be in the database. Check the tree.

It NEVER produces false negatives. It CAN produce false positives (~1% rate).

**FNV-1a Hash (Fowler–Noll–Vo variant 1a):**
```
hash = 2166136261
for each byte of input:
    hash ^= byte          // XOR mixes the byte in
    hash *= 16777619      // multiply by FNV prime "avalanches" the bits
```
We simulate k=7 different hash functions by XORing the key with a different
seed value (0, 1, 2, ..., 6) before hashing. Each seed gives a completely
different output. This is cheap and standard.

**Why k=7?** The optimal number of hash functions for 10 bits-per-element is:
```
k = (m/n) * ln(2) = 10 * 0.693 ≈ 6.93 ≈ 7
```

### Fusion Tree (FusionTree.h / FusionTree.cpp)

A Fusion Tree achieves **O(log_w n)** lookup time where w = word size (64 bits).
Our implementation is a **B-tree** where every node is a "Fusion Node" holding
up to B-1 = 15 keys and B = 16 children (branching factor B=16).

Why is this faster in practice than a BST?
- A standard BST has height ~log₂(n). For n=1,000,000 that's ~20 levels.
- Our tree has height ~log₁₆(n). For n=1,000,000 that's ~5 levels.
- Fewer levels = fewer memory accesses = fewer CPU cache misses.
- A node with 15 keys fits in 1-2 CPU cache lines (64 bytes each).

Operations:
- **Search**: scan sorted keys in a node, follow child pointer. O(log_B n)
- **Insert**: proactive splitting ensures no node is ever full when we arrive
- **Delete**: borrow from sibling or merge nodes to maintain balance

### Database Lookup Pipeline

```
user calls db.get(key)
        |
        v
[Bloom Filter] -- "DEFINITELY NOT" --> return false immediately  (O(k) time)
        |
        | "POSSIBLY EXISTS"
        v
[Fusion Tree] -----------------------> search and return result  (O(log_B n))
```

On a 50% miss workload, the Bloom Filter saves half of all tree traversals.

---

## Key Learning Points

| Concept | Where to look |
|---|---|
| FNV-1a hash algorithm | `src/BloomFilter.cpp` lines 40-70 |
| Bloom Filter bit manipulation | `BloomFilter::setBit`, `getBit` |
| Why k=7 hash functions | `src/Database.cpp` constructor comment |
| B-tree insert with splitting | `FusionTree::insertNonFull`, `splitChild` |
| B-tree delete (all cases) | `FusionTree::removeFromNode` and helpers |
| False positive demonstration | Run `./db`, insert key 5, delete key 5, then GET key 5 |
| Cache-friendliness | `FUSION_B` constant in `FusionTree.h` — try changing it |

---

## Notes on Bloom Filter + Delete

Standard Bloom Filters **do not support deletion**.

When a key is deleted from the Fusion Tree, its bits in the Bloom Filter
stay set. This can cause a false positive: the filter says "might exist",
the tree confirms "not here." This is expected behavior.

For a production system that needs deletion + Bloom Filter, use a
**Counting Bloom Filter** (replaces each bit with a small counter that
can be decremented on delete).

---

## Honest Benchmark Note

On a VM or in a tight benchmark loop, `std::map` and `std::unordered_map`
benefit from warm CPU caches and heavily optimized stdlib internals.
Our custom structures show their advantage on:
- **Cold caches** (real applications between many other operations)
- **Very large datasets** where cache miss penalty dominates
- **High miss-rate workloads** where the Bloom Filter does the most work

The data structures and algorithms here are valid and production-tested
in databases like Cassandra, RocksDB, and BigTable (all use Bloom Filters
with similar tree structures).
