#include "FusionTree.h"
#include <iostream>

/*
 * FusionTree.cpp
 *
 * This implements a B-Tree where every node is a "Fusion Node" -
 * meaning each node stores multiple keys and uses wide branching.
 *
 * The three core operations:
 *
 * SEARCH: O(log_B n)
 *   - Start at root, scan the sorted keys in the node
 *   - Follow the correct child pointer
 *   - Repeat until found or reached a leaf
 *
 * INSERT: O(log_B n)
 *   - If the node we land on is full (has B-1 keys), SPLIT it first
 *   - Then insert into the now non-full node
 *   - Splitting pushes the middle key up to the parent
 *
 * DELETE: O(log_B n)
 *   - Several cases: key in leaf, key in internal node, etc.
 *   - We ensure each node we visit has at least t keys (t = min degree)
 *     so we can always delete without leaving a node underflowing
 */

// ============================================================
// FusionNode constructor
// ============================================================
FusionNode::FusionNode()
{
    count  = 0;
    isLeaf = true;

    for (int i = 0; i < FUSION_B - 1; i++)
    {
        keys[i]   = 0;
        values[i] = 0;
    }
    for (int i = 0; i < FUSION_B; i++)
        children[i] = NULL;
}


// ============================================================
// FusionTree constructor / destructor
// ============================================================
FusionTree::FusionTree()
{
    root       = new FusionNode();
    totalCount = 0;
}

FusionTree::~FusionTree()
{
    destroyNode(root);
}

void FusionTree::destroyNode(FusionNode* node)
{
    if (node == NULL)
        return;

    if (!node->isLeaf)
    {
        for (int i = 0; i <= node->count; i++)
            destroyNode(node->children[i]);
    }
    delete node;
}

int FusionTree::getCount()
{
    return totalCount;
}


// ============================================================
// SEARCH
// ============================================================

bool FusionTree::search(long long key, long long &outValue)
{
    return searchNode(root, key, outValue);
}

bool FusionTree::searchNode(FusionNode* node, long long key, long long &outValue)
{
    // Find the first key in this node that is >= the search key
    int i = 0;
    while (i < node->count && key > node->keys[i])
        i++;

    // Did we land exactly on the key?
    if (i < node->count && key == node->keys[i])
    {
        outValue = node->values[i];
        return true;
    }

    // Key not found here and we're at a leaf - truly not in the tree
    if (node->isLeaf)
        return false;

    // Recurse into the appropriate child
    return searchNode(node->children[i], key, outValue);
}


// ============================================================
// INSERT
// ============================================================

void FusionTree::insert(long long key, long long value)
{
    // Check if key already exists - if so just update the value
    long long existing = 0;
    if (searchNode(root, key, existing))
    {
        // Walk tree to update in place
        // For simplicity: remove old and reinsert (value update path)
        // We'll handle by just re-inserting after removal
        remove(key);
        totalCount--; // remove already decremented, we want net 0 for update
    }

    // If root is full, we need to split it first
    if (root->count == FUSION_B - 1)
    {
        FusionNode* newRoot = new FusionNode();
        newRoot->isLeaf     = false;
        newRoot->children[0] = root;

        splitChild(newRoot, 0, root);

        // Now newRoot has two children. Decide which to insert into.
        int idx = 0;
        if (newRoot->keys[0] < key)
            idx = 1;

        insertNonFull(newRoot->children[idx], key, value);

        root = newRoot;
    }
    else
    {
        insertNonFull(root, key, value);
    }

    totalCount++;
}

/*
 * insertNonFull:
 * We GUARANTEE the node passed here is NOT full (count < B-1).
 * So we can always insert without worrying about overflow at this node.
 *
 * If it's a leaf, we slide keys right and drop the new key in the correct spot.
 * If it's internal, we find the right child and recurse,
 * but first split the child if it's full (proactive splitting).
 */
void FusionTree::insertNonFull(FusionNode* node, long long key, long long value)
{
    int i = node->count - 1;

    if (node->isLeaf)
    {
        // Shift keys and values right to make room for the new key
        while (i >= 0 && key < node->keys[i])
        {
            node->keys[i + 1]   = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        // Insert the new key at position i+1
        node->keys[i + 1]   = key;
        node->values[i + 1] = value;
        node->count++;
    }
    else
    {
        // Find which child we need to go into
        while (i >= 0 && key < node->keys[i])
            i--;
        i++;  // i is now the index of the child to recurse into

        // If that child is full, split it before recursing
        if (node->children[i]->count == FUSION_B - 1)
        {
            splitChild(node, i, node->children[i]);

            // After the split, the middle key came up to node->keys[i]
            // Decide if our new key goes into the left or right half
            if (key > node->keys[i])
                i++;
        }
        insertNonFull(node->children[i], key, value);
    }
}

/*
 * splitChild:
 * 'child' is the full child of 'parent' at position childIndex.
 * We split it into two nodes:
 *   left half stays as 'child'
 *   right half goes into a new node
 *   middle key (at position t-1) gets promoted up to 'parent'
 *
 * t = minimum degree = B/2
 * A full node has 2t-1 keys. After split: each half has t-1 keys.
 */
void FusionTree::splitChild(FusionNode* parent, int childIndex, FusionNode* child)
{
    int t = FUSION_B / 2;   // minimum degree

    FusionNode* newChild = new FusionNode();
    newChild->isLeaf     = child->isLeaf;
    newChild->count      = t - 1;

    // Copy the right half of child's keys/values into newChild
    for (int j = 0; j < t - 1; j++)
    {
        newChild->keys[j]   = child->keys[j + t];
        newChild->values[j] = child->values[j + t];
    }

    // Copy the right half of child's children into newChild (if not leaf)
    if (!child->isLeaf)
    {
        for (int j = 0; j < t; j++)
            newChild->children[j] = child->children[j + t];
    }

    // The middle key that gets promoted
    long long midKey   = child->keys[t - 1];
    long long midValue = child->values[t - 1];

    // Shrink the original child
    child->count = t - 1;

    // Make room in parent for the new child pointer
    for (int j = parent->count; j >= childIndex + 1; j--)
        parent->children[j + 1] = parent->children[j];

    parent->children[childIndex + 1] = newChild;

    // Make room in parent for the promoted key
    for (int j = parent->count - 1; j >= childIndex; j--)
    {
        parent->keys[j + 1]   = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }

    parent->keys[childIndex]   = midKey;
    parent->values[childIndex] = midValue;
    parent->count++;
}


// ============================================================
// DELETE
// ============================================================

bool FusionTree::remove(long long key)
{
    bool result = removeFromNode(root, key);

    // If the root has no more keys and is not a leaf,
    // make its only child the new root
    if (root->count == 0 && !root->isLeaf)
    {
        FusionNode* oldRoot = root;
        root = root->children[0];
        delete oldRoot;
    }

    if (result) totalCount--;
    return result;
}

bool FusionTree::removeFromNode(FusionNode* node, long long key)
{
    int t = FUSION_B / 2;

    // Find the position of the key in this node
    int i = 0;
    while (i < node->count && key > node->keys[i])
        i++;

    if (i < node->count && key == node->keys[i])
    {
        // Case 1: Key found in this node
        if (node->isLeaf)
        {
            removeFromLeaf(node, i);
            return true;
        }
        else
        {
            removeFromInternal(node, i);
            return true;
        }
    }
    else
    {
        // Key not in this node
        if (node->isLeaf)
            return false;   // Key doesn't exist

        // Determine which child to descend into
        bool isLastChild = (i == node->count);

        // Ensure the child has at least t keys before descending
        if (node->children[i]->count < t)
            fill(node, i);

        // After fill, the structure may have changed
        if (isLastChild && i > node->count)
            return removeFromNode(node->children[i - 1], key);
        else
            return removeFromNode(node->children[i], key);
    }
}

// Remove key at keyIndex from a leaf node (just shift keys left)
void FusionTree::removeFromLeaf(FusionNode* node, int keyIndex)
{
    for (int i = keyIndex + 1; i < node->count; i++)
    {
        node->keys[i - 1]   = node->keys[i];
        node->values[i - 1] = node->values[i];
    }
    node->count--;
}

// Remove key at keyIndex from an internal node
void FusionTree::removeFromInternal(FusionNode* node, int keyIndex)
{
    int t = FUSION_B / 2;
    long long key = node->keys[keyIndex];

    if (node->children[keyIndex]->count >= t)
    {
        // Replace with in-order predecessor (largest in left subtree)
        node->keys[keyIndex]   = getPredecessor(node, keyIndex);
        node->values[keyIndex] = getPredecessorValue(node, keyIndex);
        removeFromNode(node->children[keyIndex], node->keys[keyIndex]);
    }
    else if (node->children[keyIndex + 1]->count >= t)
    {
        // Replace with in-order successor (smallest in right subtree)
        node->keys[keyIndex]   = getSuccessor(node, keyIndex);
        node->values[keyIndex] = getSuccessorValue(node, keyIndex);
        removeFromNode(node->children[keyIndex + 1], node->keys[keyIndex]);
    }
    else
    {
        // Both children have t-1 keys: merge and delete from merged child
        merge(node, keyIndex);
        removeFromNode(node->children[keyIndex], key);
    }
}

long long FusionTree::getPredecessor(FusionNode* node, int keyIndex)
{
    FusionNode* curr = node->children[keyIndex];
    while (!curr->isLeaf)
        curr = curr->children[curr->count];
    return curr->keys[curr->count - 1];
}

long long FusionTree::getPredecessorValue(FusionNode* node, int keyIndex)
{
    FusionNode* curr = node->children[keyIndex];
    while (!curr->isLeaf)
        curr = curr->children[curr->count];
    return curr->values[curr->count - 1];
}

long long FusionTree::getSuccessor(FusionNode* node, int keyIndex)
{
    FusionNode* curr = node->children[keyIndex + 1];
    while (!curr->isLeaf)
        curr = curr->children[0];
    return curr->keys[0];
}

long long FusionTree::getSuccessorValue(FusionNode* node, int keyIndex)
{
    FusionNode* curr = node->children[keyIndex + 1];
    while (!curr->isLeaf)
        curr = curr->children[0];
    return curr->values[0];
}

/*
 * fill: ensure node->children[childIndex] has at least t keys
 * by borrowing from a sibling or merging with one
 */
void FusionTree::fill(FusionNode* node, int childIndex)
{
    int t = FUSION_B / 2;

    if (childIndex != 0 && node->children[childIndex - 1]->count >= t)
        borrowFromPrev(node, childIndex);
    else if (childIndex != node->count && node->children[childIndex + 1]->count >= t)
        borrowFromNext(node, childIndex);
    else
    {
        if (childIndex != node->count)
            merge(node, childIndex);
        else
            merge(node, childIndex - 1);
    }
}

void FusionTree::borrowFromPrev(FusionNode* node, int childIndex)
{
    FusionNode* child = node->children[childIndex];
    FusionNode* sib   = node->children[childIndex - 1];

    // Shift child's keys/values right
    for (int i = child->count - 1; i >= 0; i--)
    {
        child->keys[i + 1]   = child->keys[i];
        child->values[i + 1] = child->values[i];
    }
    if (!child->isLeaf)
    {
        for (int i = child->count; i >= 0; i--)
            child->children[i + 1] = child->children[i];
    }

    // Move separator key from parent down into child
    child->keys[0]   = node->keys[childIndex - 1];
    child->values[0] = node->values[childIndex - 1];
    if (!child->isLeaf)
        child->children[0] = sib->children[sib->count];

    // Move sibling's last key up to parent
    node->keys[childIndex - 1]   = sib->keys[sib->count - 1];
    node->values[childIndex - 1] = sib->values[sib->count - 1];

    child->count++;
    sib->count--;
}

void FusionTree::borrowFromNext(FusionNode* node, int childIndex)
{
    FusionNode* child = node->children[childIndex];
    FusionNode* sib   = node->children[childIndex + 1];

    // Move separator key from parent down into child
    child->keys[child->count]   = node->keys[childIndex];
    child->values[child->count] = node->values[childIndex];
    if (!child->isLeaf)
        child->children[child->count + 1] = sib->children[0];

    // Move sibling's first key up to parent
    node->keys[childIndex]   = sib->keys[0];
    node->values[childIndex] = sib->values[0];

    // Shift sibling's keys/values left
    for (int i = 1; i < sib->count; i++)
    {
        sib->keys[i - 1]   = sib->keys[i];
        sib->values[i - 1] = sib->values[i];
    }
    if (!sib->isLeaf)
    {
        for (int i = 1; i <= sib->count; i++)
            sib->children[i - 1] = sib->children[i];
    }

    child->count++;
    sib->count--;
}

void FusionTree::merge(FusionNode* node, int childIndex)
{
    int t = FUSION_B / 2;

    FusionNode* left  = node->children[childIndex];
    FusionNode* right = node->children[childIndex + 1];

    // Pull separator key from parent into the left child
    left->keys[t - 1]   = node->keys[childIndex];
    left->values[t - 1] = node->values[childIndex];

    // Copy right child's keys/values into left
    for (int i = 0; i < right->count; i++)
    {
        left->keys[i + t]   = right->keys[i];
        left->values[i + t] = right->values[i];
    }
    if (!left->isLeaf)
    {
        for (int i = 0; i <= right->count; i++)
            left->children[i + t] = right->children[i];
    }

    left->count = 2 * t - 1;

    // Remove separator key from parent and shift its children left
    for (int i = childIndex + 1; i < node->count; i++)
    {
        node->keys[i - 1]      = node->keys[i];
        node->values[i - 1]    = node->values[i];
        node->children[i]      = node->children[i + 1];
    }
    node->count--;

    delete right;
}


// ============================================================
// IN-ORDER PRINT (for debugging/display)
// ============================================================

void FusionTree::printInOrder()
{
    printNode(root, 0);
    std::cout << std::endl;
}

void FusionTree::printNode(FusionNode* node, int depth)
{
    if (node == NULL) return;

    for (int i = 0; i < node->count; i++)
    {
        if (!node->isLeaf)
            printNode(node->children[i], depth + 1);

        for (int d = 0; d < depth; d++) std::cout << "  ";
        std::cout << "[" << node->keys[i] << " -> " << node->values[i] << "]" << std::endl;
    }

    if (!node->isLeaf)
        printNode(node->children[node->count], depth + 1);
}
