#ifndef FUSIONTREE_H
#define FUSIONTREE_H

/*
 * FusionTree.h
 *
 * A Fusion Tree is a data structure invented by Fredman & Willard (1990).
 * It achieves O(log_w n) search time, where w = word size (64 bits on modern CPUs).
 * This beats a standard BST's O(log n) because log_64(n) grows much slower.
 *
 * KEY IDEA (simplified for learning):
 * ------------------------------------
 * Instead of each node holding 1 key (like BST), each "Fusion Node" holds
 * up to B keys (we use B = 16 here, tunable). This is like a B-tree.
 *
 * Why is this faster in practice?
 *   - Fewer nodes to traverse (tree is shorter / "fatter")
 *   - CPU cache lines are ~64 bytes; a node with many keys fits in cache
 *   - Fewer cache misses = dramatically faster on large datasets
 *
 * Node structure:
 *   - keys[]   : sorted array of long long keys stored in this node
 *   - values[] : corresponding values
 *   - children[]: pointers to child nodes (NULL if leaf)
 *   - count    : how many keys are currently in this node
 *   - isLeaf   : true if this node has no children
 *
 * Branching factor B:
 *   - Each node holds at most B-1 keys and B children
 *   - B=16 means 15 keys and 16 children per node
 *   - Tree height = log_B(n), which for n=1,000,000 and B=16 is about 5
 *   - Compare to BST: height = log_2(1,000,000) ≈ 20
 */

// B is the branching factor: each node holds up to B-1 keys and B children
// Increase B for even more cache-friendly behavior
#define FUSION_B 16

struct FusionNode
{
    long long keys[FUSION_B - 1];       // sorted keys stored in this node
    long long values[FUSION_B - 1];     // corresponding values
    FusionNode* children[FUSION_B];     // child pointers (NULL for leaves)
    int  count;                          // current number of keys in this node
    bool isLeaf;                         // true if no children

    FusionNode();
};

class FusionTree
{
public:
    FusionTree();
    ~FusionTree();

    void  insert(long long key, long long value);
    bool  search(long long key, long long &outValue);
    bool  remove(long long key);
    void  printInOrder();
    int   getCount();

private:
    FusionNode* root;
    int totalCount;

    // Internal helpers
    void  destroyNode(FusionNode* node);
    bool  searchNode(FusionNode* node, long long key, long long &outValue);
    void  insertNonFull(FusionNode* node, long long key, long long value);
    void  splitChild(FusionNode* parent, int childIndex, FusionNode* child);
    void  printNode(FusionNode* node, int depth);

    // For deletion
    bool  removeFromNode(FusionNode* node, long long key);
    void  removeFromLeaf(FusionNode* node, int keyIndex);
    void  removeFromInternal(FusionNode* node, int keyIndex);
    long long getPredecessor(FusionNode* node, int keyIndex);
    long long getPredecessorValue(FusionNode* node, int keyIndex);
    long long getSuccessor(FusionNode* node, int keyIndex);
    long long getSuccessorValue(FusionNode* node, int keyIndex);
    void  fill(FusionNode* node, int childIndex);
    void  borrowFromPrev(FusionNode* node, int childIndex);
    void  borrowFromNext(FusionNode* node, int childIndex);
    void  merge(FusionNode* node, int childIndex);
};

#endif
