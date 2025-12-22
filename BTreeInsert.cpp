#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
using namespace std;

// ============================================================================
// Jana: Split Node with UNIVERSAL Copy-Up Strategy
// For ALL node types (leaf and internal), the split key is COPIED to parent
// and KEPT in the left child as its maximum value.
// ============================================================================
int SplitNodeAndInsert(fstream &file, int nodeIndex, int RecordID, int Reference, int m, int &newChildIndex, bool isLeaf) {
    Node fullNode;
    file.seekg(nodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&fullNode, sizeof(Node));

    // Create temporary arrays for keys and refs
    int tempKeys[MAX_M + 1];
    int tempRefs[MAX_M + 1];
    int tempChildren[MAX_M + 2];

    memset(tempChildren, 0, sizeof(tempChildren));

    int i = 0, j = 0;

    // For internal nodes, we need to preserve children
    if (!isLeaf) {
        for (int c = 0; c <= fullNode.numKeys; c++) {
            tempChildren[c] = fullNode.children[c];
        }
    }

    // Insert new key in sorted position
    while (i < fullNode.numKeys && fullNode.recordIDs[i] < RecordID) {
        tempKeys[j] = fullNode.recordIDs[i];
        tempRefs[j] = fullNode.references[i];
        i++; j++;
    }
    tempKeys[j] = RecordID;
    tempRefs[j] = Reference;
    j++;
    while (i < fullNode.numKeys) {
        tempKeys[j] = fullNode.recordIDs[i];
        tempRefs[j] = fullNode.references[i];
        i++; j++;
    }

    int totalKeys = fullNode.numKeys + 1;

    // Split index: pos = (m + 1) / 2 (1-based), so median is at index (pos - 1)
    int splitPos = (m + 1) / 2;  // 1-based position
    int medianIndex = splitPos - 1;  // Convert to 0-based index
    int medianKey = tempKeys[medianIndex];

    // Allocate new node from free list
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));

    int newNodeIndex = freeListHead.nextFree;
    if (newNodeIndex == -1) {
        cerr << "Error: No free nodes available for split.\n";
        newChildIndex = -1;
        return -1;
    }

    Node tempFreeNode;
    file.seekg(newNodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&tempFreeNode, sizeof(Node));

    freeListHead.nextFree = tempFreeNode.nextFree;
    file.seekp(0, ios::beg);
    file.write((char*)&freeListHead, sizeof(Node));

    Node newNode;
    newNode.flag = fullNode.flag; // leaf or non-leaf

    // ===== UNIVERSAL COPY-UP STRATEGY =====
    // Left child: keys [0..medianIndex] (includes the median)
    // Right child: keys [medianIndex+1..end]
    // Median is COPIED to parent (applies to BOTH leaf and internal nodes)

    fullNode.numKeys = medianIndex + 1;  // Left keeps median as its max
    if (isLeaf) {
        for (int k = 0; k < medianIndex + 1; k++) {
            fullNode.recordIDs[k] = tempKeys[k];
            fullNode.references[k] = tempRefs[k];
        }
    } else {
        for (int k = 0; k < medianIndex + 1; k++) {
            fullNode.recordIDs[k] = tempKeys[k];
            fullNode.references[k] = tempRefs[k];
            fullNode.children[k] = tempChildren[k];
        }
        fullNode.children[medianIndex + 1] = tempChildren[medianIndex + 1];
    }

    newNode.numKeys = totalKeys - medianIndex - 1;  // Right gets rest
    if (isLeaf) {
        for (int k = 0; k < newNode.numKeys; k++) {
            newNode.recordIDs[k] = tempKeys[medianIndex + 1 + k];
            newNode.references[k] = tempRefs[medianIndex + 1 + k];
        }
    } else {
        for (int k = 0; k < newNode.numKeys; k++) {
            newNode.recordIDs[k] = tempKeys[medianIndex + 1 + k];
            newNode.references[k] = tempRefs[medianIndex + 1 + k];
            newNode.children[k] = tempChildren[medianIndex + 1 + k];
        }
        newNode.children[newNode.numKeys] = tempChildren[medianIndex + 1 + newNode.numKeys];
    }

    string nodeType = isLeaf ? "LEAF" : "INTERNAL";
    cout << nodeType << " SPLIT: Node " << nodeIndex << " [";
    for (int k = 0; k < fullNode.numKeys; k++) {
        cout << fullNode.recordIDs[k];
        if (k < fullNode.numKeys - 1) cout << ",";
    }
    cout << "] | KEY[" << medianKey << "] COPIED UP | [";
    for (int k = 0; k < newNode.numKeys; k++) {
        cout << newNode.recordIDs[k];
        if (k < newNode.numKeys - 1) cout << ",";
    }
    cout << "] -> New node: " << newNodeIndex << endl;

    // Write back nodes
    file.seekp(nodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&fullNode, sizeof(Node));
    file.seekp(newNodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&newNode, sizeof(Node));

    newChildIndex = newNodeIndex;
    return medianKey;
}

// ============================================================================
// Moaid & Jana: Insert New Record with Proper Recursive Overflow Checking
// ============================================================================

// Helper: Split current root if it overflows using Universal Copy-Up
static int SplitOverflowedRoot(fstream &file, int m) {
    // Read Node 0 to get current root index
    Node meta;
    file.seekg(0, ios::beg);
    file.read((char*)&meta, sizeof(Node));
    int oldRootIndex = meta.recordIDs[0];
    if (oldRootIndex <= 0) return oldRootIndex;

    // Load old root
    Node oldRoot;
    file.seekg(oldRootIndex * sizeof(Node), ios::beg);
    file.read((char*)&oldRoot, sizeof(Node));

    // Compute split pos/index and promoted key
    int splitPos = (m + 1) / 2; // 1-based
    int medianIndex = splitPos - 1; // 0-based
    int promotedKey = oldRoot.recordIDs[medianIndex];

    // Allocate new sibling and new root from free list
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));
    int newSiblingIndex = freeListHead.nextFree;
    if (newSiblingIndex == -1) {
        cerr << "Error: No free nodes for root split sibling.\n";
        return -1;
    }
    Node tmpFree;
    file.seekg(newSiblingIndex * sizeof(Node), ios::beg);
    file.read((char*)&tmpFree, sizeof(Node));
    int newRootIndex = tmpFree.nextFree; // provisional, will update after allocating new root

    // Detach sibling from free list
    freeListHead.nextFree = tmpFree.nextFree;
    file.seekp(0, ios::beg);
    file.write((char*)&freeListHead, sizeof(Node));

    // Create right sibling node
    Node right;
    right.flag = oldRoot.flag; // same type as root (internal expected)
    right.numKeys = oldRoot.numKeys - (medianIndex + 1);

    // Copy keys to right sibling (after median)
    for (int k = 0; k < right.numKeys; k++) {
        right.recordIDs[k] = oldRoot.recordIDs[medianIndex + 1 + k];
        right.references[k] = oldRoot.references[medianIndex + 1 + k];
    }

    // Copy children to right sibling if internal AND REMOVE FROM OLD ROOT
    if (oldRoot.flag == 1) {
        for (int k = 0; k <= right.numKeys; k++) {
            // Transfer ownership to right node
            right.children[k] = oldRoot.children[medianIndex + 1 + k];

            // Zero out the pointer in the old root so it doesn't duplicate the child
            oldRoot.children[medianIndex + 1 + k] = 0;
        }
    }

    // Left (old root) keeps median per Universal Copy-Up
    oldRoot.numKeys = medianIndex + 1;

    // Persist old root and right sibling
    file.seekp(oldRootIndex * sizeof(Node), ios::beg);
    file.write((char*)&oldRoot, sizeof(Node));
    file.seekp(newSiblingIndex * sizeof(Node), ios::beg);
    file.write((char*)&right, sizeof(Node));

    // Allocate new root from free list
    Node meta2;
    file.seekg(0, ios::beg);
    file.read((char*)&meta2, sizeof(Node));
    int allocRootIdx = meta2.nextFree;
    if (allocRootIdx == -1) {
        cerr << "Error: No free nodes for new root.\n";
        return -1;
    }
    Node tmpFree2;
    file.seekg(allocRootIdx * sizeof(Node), ios::beg);
    file.read((char*)&tmpFree2, sizeof(Node));
    meta2.nextFree = tmpFree2.nextFree;
    meta2.recordIDs[0] = allocRootIdx; // update root index
    file.seekp(0, ios::beg);
    file.write((char*)&meta2, sizeof(Node));

    // Build new root with promoted key copied
    Node newRoot;
    newRoot.flag = 1;
    newRoot.numKeys = 1;
    newRoot.recordIDs[0] = promotedKey;
    newRoot.children[0] = oldRootIndex;
    newRoot.children[1] = newSiblingIndex;

    file.seekp(allocRootIdx * sizeof(Node), ios::beg);
    file.write((char*)&newRoot, sizeof(Node));
    cout << "Split Root: new root " << allocRootIdx << " with key " << promotedKey
         << ", children [" << oldRootIndex << ", " << newSiblingIndex << "]\n";
    return allocRootIdx;
}


int InsertNewRecordAtIndex(char* filename, int RecordID, int Reference) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error opening index file.\n";
        return -1;
    }

    // Read m from Node 0 (Free List Head) - stored in numKeys field
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));
    int m = freeListHead.numKeys;  // m is stored here during CreateIndexFileFile

    int searchResult = SearchARecord(filename, RecordID);
    if (searchResult != -1) {
        cout << "Insert failed: Record already exists.\n";
        file.close();
        return -1;
    }

    if (historyTop < 0) {
        cout << "Insert failed: Tree is empty or corrupted.\n";
        file.close();
        return -1;
    }

    int leafIndex = historyStack[historyTop];
    Node leaf;
    file.seekg(leafIndex * sizeof(Node), ios::beg);
    file.read((char*)&leaf, sizeof(Node));

    if (leaf.numKeys < m) {
        int i = leaf.numKeys - 1;
        while (i >= 0 && RecordID < leaf.recordIDs[i]) {
            leaf.recordIDs[i + 1] = leaf.recordIDs[i];
            leaf.references[i + 1] = leaf.references[i];
            i--;
        }
        leaf.recordIDs[i + 1] = RecordID;
        leaf.references[i + 1] = Reference;
        leaf.numKeys++;

        file.seekp(leafIndex * sizeof(Node), ios::beg);
        file.write((char*)&leaf, sizeof(Node));
        cout << "Inserted RecordID " << RecordID
             << " at leaf node " << leafIndex << endl;
        file.close();
        return leafIndex;
    }

    // Leaf is full → Split (UNIVERSAL COPY-UP)
    cout << "Leaf node " << leafIndex << " is full (has " << leaf.numKeys << " keys, m=" << m << "). Split required.\n";

    int newChildIndex;
    int promotedKey = SplitNodeAndInsert(file, leafIndex, RecordID, Reference, m, newChildIndex, true);  // isLeaf=true

    // Now propagate the promoted key upwards with proper overflow checking
    int currentChildIndex = leafIndex;
    int currentNewSiblingIndex = newChildIndex;
    int currentPromotedKey = promotedKey;

    // Traverse upwards through the history stack
    for (int h = historyTop - 1; h >= 0; h--) {
        int parentIndex = historyStack[h];
        Node parentNode;
        file.seekg(parentIndex * sizeof(Node), ios::beg);
        file.read((char*)&parentNode, sizeof(Node));

        // Try to insert promoted key into parent
        // First, find the correct position
        int insertPos = 0;
        while (insertPos < parentNode.numKeys && currentPromotedKey > parentNode.recordIDs[insertPos]) {
            insertPos++;
        }

        // Check if parent has room BEFORE inserting
        if (parentNode.numKeys < m) {
            // Parent has room - insert and we're done
            for (int i = parentNode.numKeys; i > insertPos; i--) {
                parentNode.recordIDs[i] = parentNode.recordIDs[i - 1];
                parentNode.children[i + 1] = parentNode.children[i];
            }
            parentNode.recordIDs[insertPos] = currentPromotedKey;
            parentNode.children[insertPos + 1] = currentNewSiblingIndex;
            parentNode.numKeys++;
            parentNode.flag = 1; // non-leaf

            file.seekp(parentIndex * sizeof(Node), ios::beg);
            file.write((char*)&parentNode, sizeof(Node));
            cout << "Inserted promoted key " << currentPromotedKey << " into parent node " << parentIndex << "\n";

            // Check if the parent is the root and it overflows
            Node metaCheck;
            file.seekg(0, ios::beg);
            file.read((char*)&metaCheck, sizeof(Node));
            int rootIdx = metaCheck.recordIDs[0];
            if (parentIndex == rootIdx && parentNode.numKeys >= m) {
                cout << "Root node " << parentIndex << " overflowed (" << parentNode.numKeys << "/" << m << "). Performing root split...\n";
                int newRootIdx = SplitOverflowedRoot(file, m);
                file.close();
                return newRootIdx;
            }

            file.close();
            return parentIndex;

        } else {
            // Parent is full → split the parent (INTERNAL NODE SPLIT)
            cout << "Parent node " << parentIndex << " is full (has " << parentNode.numKeys
                 << " keys, m=" << m << "). Splitting parent...\n";

            int tempNewSiblingIndex;
            int tempPromotedKey = SplitNodeAndInsert(file, parentIndex, currentPromotedKey,
                                                     currentNewSiblingIndex, m, tempNewSiblingIndex, false);  // isLeaf=false

            // Update for next iteration
            currentChildIndex = parentIndex;
            currentNewSiblingIndex = tempNewSiblingIndex;
            currentPromotedKey = tempPromotedKey;
        }
    }

    // If we get here, we've propagated to the root and it split
    // Create a new root
    Node newRoot;
    newRoot.flag = 1;  // non-leaf
    newRoot.numKeys = 1;
    newRoot.recordIDs[0] = currentPromotedKey;
    newRoot.children[0] = currentChildIndex;  // Left child is the old root that was split
    newRoot.children[1] = currentNewSiblingIndex;  // Right child is the new sibling from split

    Node rootFreeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&rootFreeListHead, sizeof(Node));

    int newRootIndex = rootFreeListHead.nextFree;
    if (newRootIndex == -1) {
        cerr << "Error: No free nodes for new root.\n";
        file.close();
        return -1;
    }

    Node tempFreeNode;
    file.seekg(newRootIndex * sizeof(Node), ios::beg);
    file.read((char*)&tempFreeNode, sizeof(Node));

    rootFreeListHead.nextFree = tempFreeNode.nextFree;
    rootFreeListHead.recordIDs[0] = newRootIndex;  // Update root index in Node 0
    file.seekp(0, ios::beg);
    file.write((char*)&rootFreeListHead, sizeof(Node));

    file.seekp(newRootIndex * sizeof(Node), ios::beg);
    file.write((char*)&newRoot, sizeof(Node));

    cout << "Created new root node " << newRootIndex << " with key [" << currentPromotedKey
         << "], children: [" << currentChildIndex << ", " << currentNewSiblingIndex << "]\n";
    file.close();
    return newRootIndex;
}