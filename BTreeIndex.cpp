#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// ============================================================================
// GLOBAL HISTORY STACK
// ============================================================================
int historyStack[MAX_HISTORY];
int historyTop = -1;

void pushHistory(int nodeIndex) {
    if (historyTop < MAX_HISTORY - 1) {
        historyStack[++historyTop] = nodeIndex;
    }
}

void clearHistory() {
    historyTop = -1;
}

// ============================================================================
// NODE STRUCTURE - Constructor Implementation
// ============================================================================
Node::Node() {
    flag = 0;
    numKeys = 0;
    nextFree = -1;
    memset(recordIDs, 0, sizeof(recordIDs));
    memset(references, 0, sizeof(references));
    memset(children, 0, sizeof(children));
}

// ============================================================================
// Eyad: FUNCTION 1 - Create Index File
// ============================================================================

void CreateIndexFileFile(char* filename, int numberOfRecords, int m) {
    ofstream file(filename, ios::binary | ios::trunc);

    if (!file.is_open()) {
        cerr << "Error: Could not create file " << filename << endl;
        return;
    }

    // Node 0 is the Free List Head + Metadata
    // Store m in the numKeys field for later retrieval
    // Store root index in recordIDs[0]
    // Free list starts from Node 2 (Node 1 is the root)
    Node freeListHead;
    freeListHead.flag = -1;  // Special flag for free list head
    freeListHead.numKeys = m;  // Store m here for retrieval by other functions
    freeListHead.recordIDs[0] = 1;  // Store root node index (initially Node 1)
    freeListHead.nextFree = 2;  // Points to first free node (skip Node 1 which is root)
    file.write((char*)&freeListHead, sizeof(Node));

    // Node 1 is the root - initialize as empty LEAF
    Node rootNode;
    rootNode.flag = 0;      // Leaf node
    rootNode.numKeys = 0;   // Empty
    rootNode.nextFree = -1; // Not in free list
    file.write((char*)&rootNode, sizeof(Node));

    // Create remaining empty nodes in a linked list (starting from Node 2)
    for (int i = 2; i < numberOfRecords; i++) {
        Node emptyNode;
        emptyNode.flag = -1;  // Mark as free

        // Link to next empty node (or -1 if last)
        if (i < numberOfRecords - 1) {
            emptyNode.nextFree = i + 1;
        } else {
            emptyNode.nextFree = -1;  // Last node in free list
        }

        file.write((char*)&emptyNode, sizeof(Node));
    }

    file.close();
    cout << "Index file '" << filename << "' created with " << numberOfRecords
         << " nodes (m=" << m << "), Node 1 initialized as root" << endl;
}


// ============================================================================
// Eyad: FUNCTION 2 - Display Index File Content
// ============================================================================
void DisplayIndexFileContent(char* filename) {
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return;
    }

    cout << "\n========================================" << endl;
    cout << "INDEX FILE CONTENT: " << filename << endl;
    cout << "========================================" << endl;

    Node node;
    int index = 0;
    while (file.read((char*)&node, sizeof(Node))) {
        cout << "\nNode Index: " << index << endl;
        if (index == 0) {
            cout << "  Type: FREE LIST HEAD" << endl;
            cout << "  Root Index: " << node.recordIDs[0] << endl;
            cout << "  B-Tree Order (m): " << node.numKeys << endl;
            cout << "  Next Free: " << node.nextFree << endl;
        } else if (node.flag == -1) {
            cout << "  Type: FREE NODE" << endl;
            cout << "  Next Free: " << node.nextFree << endl;
        } else if (node.flag == 0) {
            cout << "  Type: LEAF NODE" << endl;
            cout << "  Number of Keys: " << node.numKeys << endl;
            cout << "  Record IDs: ";
            for (int i = 0; i < node.numKeys; i++) cout << node.recordIDs[i] << " ";
            cout << "\n  References: ";
            for (int i = 0; i < node.numKeys; i++) cout << node.references[i] << " ";
            cout << endl;
        } else if (node.flag == 1) {
            cout << "  Type: NON-LEAF NODE" << endl;
            cout << "  Number of Keys: " << node.numKeys << endl;
            cout << "  Record IDs: ";
            for (int i = 0; i < node.numKeys; i++) cout << node.recordIDs[i] << " ";
            cout << "\n  Children Indices: ";
            for (int i = 0; i <= node.numKeys; i++) cout << node.children[i] << " ";
            cout << endl;
        }
        index++;
    }

    cout << "\n========================================" << endl;
    cout << "Total Nodes: " << index << endl;
    cout << "========================================\n" << endl;

    file.close();
}

// ============================================================================
// Eyad: FUNCTION 3 - Search for a Record
// ============================================================================
int SearchARecord(char* filename, int RecordID) {
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return -1;
    }

    clearHistory();

    // Read root index from Node 0
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));
    int currentIndex = freeListHead.recordIDs[0];  // Root node index
    Node currentNode;

    while (currentIndex != -1 && currentIndex != 0) {
        pushHistory(currentIndex);

        file.seekg(currentIndex * sizeof(Node), ios::beg);
        file.read((char*)&currentNode, sizeof(Node));

        if (currentNode.flag == -1) {
            cout << "Search: Node " << currentIndex << " is free. Tree may be empty." << endl;
            file.close();
            return -1;
        }

        int i = 0;
        // Find the first key that is Greater than or Equal to RecordID
        while (i < currentNode.numKeys && RecordID > currentNode.recordIDs[i]) i++;

        if (currentNode.flag == 0) {
            // LEAF NODE: Explicit match check
            if (i < currentNode.numKeys && RecordID == currentNode.recordIDs[i]) {
                cout << "Record " << RecordID << " found at Node " << currentIndex
                     << ", Reference: " << currentNode.references[i] << endl;
                file.close();
                return currentNode.references[i];
            } else {
                cout << "Record " << RecordID << " not found in tree." << endl;
                file.close();
                return -1;
            }
        } else {

            currentIndex = currentNode.children[i];
        }
    }

    file.close();
    cout << "Record " << RecordID << " not found in tree." << endl;
    return -1;
}

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

// Helper function to get the maximum key in a subtree
int getMaxKeyInSubtree(ifstream &file, int nodeIndex) {
    if (nodeIndex == -1 || nodeIndex == 0) return -1;

    Node node;
    file.seekg(nodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&node, sizeof(Node));

    // If leaf node, return the rightmost key
    if (node.flag == 0) {
        if (node.numKeys > 0) {
            return node.recordIDs[node.numKeys - 1];
        }
        return -1;
    }

    // If internal node, recursively get max from rightmost child
    return getMaxKeyInSubtree(file, node.children[node.numKeys]);
}

// ============================================================================
// Level-Order Traversal with Parent Node ID and Separator Key
// ============================================================================
void DisplayTreeLevelOrder(char* filename) {
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return;
    }

    // Read root index from Node 0
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));
    int rootIndex = freeListHead.recordIDs[0];

    cout << "\n========================================" << endl;
    cout << "   LEVEL-ORDER TREE VISUALIZATION" << endl;
    cout << "========================================\n" << endl;

    if (rootIndex == -1 || rootIndex == 0) {
        cout << "Tree is empty.\n";
        file.close();
        return;
    }

    // BFS using queue: tuple of (nodeIndex, parentNodeIndex, separatorKey)
    // separatorKey = -1 means ROOT
    queue<tuple<int, int, int>> q;
    q.push(make_tuple(rootIndex, -1, -1));  // root has no parent

    int currentLevel = 0;
    int nodesInCurrentLevel = 1;
    int nodesInNextLevel = 0;

    while (!q.empty()) {
        // Print level header
        cout << "--- Level " << currentLevel << " ---" << endl;

        int nodesToProcess = nodesInCurrentLevel;
        nodesInNextLevel = 0;

        // Process all nodes in current level
        for (int i = 0; i < nodesToProcess; i++) {
            tuple<int, int, int> current = q.front();
            q.pop();
            int nodeIndex = get<0>(current);
            int parentNodeIndex = get<1>(current);
            int separatorKey = get<2>(current);

            if (nodeIndex == -1) continue;

            // Read the node
            Node node;
            file.seekg(nodeIndex * sizeof(Node), ios::beg);
            file.read((char*)&node, sizeof(Node));

            // Print node information - only numbers
            cout << "  Node " << nodeIndex << ": [ ";

            if (node.flag == 1) {  // non-leaf: show separator keys + right subtree max
                for (int j = 0; j < node.numKeys; j++) {
                    cout << node.recordIDs[j];
                    if (j < node.numKeys - 1) cout << ", ";
                }
                // Add the max of the rightmost subtree for visual context inside the bracket
                int rightMaxKey = getMaxKeyInSubtree(file, node.children[node.numKeys]);
                if (rightMaxKey != -1) {
                    if (node.numKeys > 0) cout << ", ";
                    cout << rightMaxKey;
                }
            } else {  // leaf: show all keys normally
                for (int j = 0; j < node.numKeys; j++) {
                    cout << node.recordIDs[j];
                    if (j < node.numKeys - 1) cout << ", ";
                }
            }

            cout << " ]";

            // Print parent reference with separator key
            if (parentNodeIndex != -1) {
                cout << "  (Parent -> Node ID: " << parentNodeIndex
                     << " | Key: " << separatorKey << ")";
            } else {
                cout << "  (ROOT)";
            }

            cout << endl;

            // If non-leaf node, add children to queue with proper separator keys
            if (node.flag == 1) {  // non-leaf
                for (int j = 0; j <= node.numKeys; j++) {
                    if (node.children[j] != 0 && node.children[j] != -1) {
                        int separator;

                        if (j < node.numKeys) {
                            // Normal Case: The key at index 'j' separates child 'j' and 'j+1'
                            separator = node.recordIDs[j];
                        } else {
                            // RIGHTMOST CHILD
                            // Use the key to its LEFT (the last key in the parent)
                            separator = node.recordIDs[j - 1];
                        }

                        q.push(make_tuple(node.children[j], nodeIndex, separator));
                        nodesInNextLevel++;
                    }
                }
            }
        }

        cout << endl;
        currentLevel++;
        nodesInCurrentLevel = nodesInNextLevel;
    }

    cout << "========================================\n" << endl;
    file.close();
}

// ============================================================================

// ============================================================================
// Yassin & Pedro: Delete Record
// ============================================================================

bool hasLeftSibling(fstream &file, int parentIndex, int nodeIndex, int &leftSiblingIndex) {
    return leftSiblingIndex != -1;
}
bool hasRightSibling(fstream &file, int parentIndex, int nodeIndex, int &rightSiblingIndex) {
    return rightSiblingIndex != -1;
}

int borrowFromLeftSibling(fstream &file, int parentIndex,int nodeIndex, int leftSiblingIndex, int m) {
    Node node;
    file.seekg(nodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&node, sizeof(Node));

    for (int i = node.numKeys; i >= 1 ; i--) {
        node.recordIDs[i] = node.recordIDs[i - 1];
        node.references[i] = node.references[i - 1];
    }
    Node leftSibling;
    file.seekg(leftSiblingIndex * sizeof(Node), ios::beg);
    file.read((char*)&leftSibling, sizeof(Node));
    node.recordIDs[0] = leftSibling.recordIDs[leftSibling.numKeys - 1];
    node.references[0] = leftSibling.references[leftSibling.numKeys - 1];
    node.numKeys++;
    leftSibling.numKeys--;
    file.seekp(nodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&node, sizeof(Node));
    file.seekp(leftSiblingIndex * sizeof(Node), ios::beg);
    file.write((char*)&leftSibling, sizeof(Node));
    return 0;
}

int borrowFromRightSibling(fstream &file, int parentIndex, int nodeIndex, int rightSiblingIndex, int m) {
    Node node;
    file.seekg(nodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&node, sizeof(Node));

    Node rightSibling;
    file.seekg(rightSiblingIndex * sizeof(Node), ios::beg);
    file.read((char*)&rightSibling, sizeof(Node));

    node.recordIDs[node.numKeys] = rightSibling.recordIDs[0];
    node.references[node.numKeys] = rightSibling.references[0];
    node.numKeys++;

    for (int i = 0; i < rightSibling.numKeys - 1; i++) {
        rightSibling.recordIDs[i] = rightSibling.recordIDs[i + 1];
        rightSibling.references[i] = rightSibling.references[i + 1];
    }
    rightSibling.numKeys--;

    file.seekp(nodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&node, sizeof(Node));
    file.seekp(rightSiblingIndex * sizeof(Node), ios::beg);
    file.write((char*)&rightSibling, sizeof(Node));
    return 0;
}

void mergeWithLeft(fstream &file, int parentIndex,int nodeIndex, int siblingIndex, bool isLeftSibling, int m) {
    if (isLeftSibling) {
        Node sibling;
        file.seekg(siblingIndex * sizeof(Node), ios::beg);
        file.read((char*)&sibling, sizeof(Node));

        Node node;
        file.seekg(nodeIndex * sizeof(Node), ios::beg);
        file.read((char*)&node, sizeof(Node));

        for (int i = 0; i < node.numKeys; i++) {
            sibling.recordIDs[sibling.numKeys + i] = node.recordIDs[i];
            sibling.references[sibling.numKeys + i] = node.references[i];
        }
        sibling.numKeys += node.numKeys;

        file.seekp(siblingIndex * sizeof(Node), ios::beg);
        file.write((char*)&sibling, sizeof(Node));

        // Mark node as free
        Node freeListHead;
        file.seekg(0, ios::beg);
        file.read((char*)&freeListHead, sizeof(Node));

        node.flag = -1;
        node.nextFree = freeListHead.nextFree;
        freeListHead.nextFree = nodeIndex;

        file.seekp(0, ios::beg);
        file.write((char*)&freeListHead, sizeof(Node));
        file.seekp(nodeIndex * sizeof(Node), ios::beg);
        file.write((char*)&node, sizeof(Node));
    }
}

void mergeWithRight(fstream &file, int parentIndex,int nodeIndex, int siblingIndex, bool isRightSibling, int m) {
    if (isRightSibling) {
        Node node;
        file.seekg(nodeIndex * sizeof(Node), ios::beg);
        file.read((char *) &node, sizeof(Node));

        Node sibling;
        file.seekg(siblingIndex * sizeof(Node), ios::beg);
        file.read((char *) &sibling, sizeof(Node));

        for (int i = 0; i < sibling.numKeys; i++) {
            node.recordIDs[node.numKeys + i] = sibling.recordIDs[i];
            node.references[node.numKeys + i] = sibling.references[i];
        }
        node.numKeys += sibling.numKeys;

        file.seekp(nodeIndex * sizeof(Node), ios::beg);
        file.write((char *) &node, sizeof(Node));

        // Mark sibling as free
        Node freeListHead;
        file.seekg(0, ios::beg);
        file.read((char *) &freeListHead, sizeof(Node));

        sibling.flag = -1;
        sibling.nextFree = freeListHead.nextFree;
        freeListHead.nextFree = siblingIndex;

        file.seekp(0, ios::beg);
        file.write((char *) &freeListHead, sizeof(Node));
        file.seekp(siblingIndex * sizeof(Node), ios::beg);
        file.write((char *) &sibling, sizeof(Node));
    }
}

// Recompute parent's separator keys from its children using max-key semantics
static void UpdateParentSeparators(fstream &file, int parentIndex) {
    if (parentIndex <= 0) return;
    Node parent;
    file.seekg(parentIndex * sizeof(Node), ios::beg);
    file.read((char*)&parent, sizeof(Node));
    if (parent.flag != 1) return; // only internal nodes

    // Count valid children (non-zero and not -1)
    int childCount = 0;
    for (int i = 0; i <= MAX_M; i++) {
        int cid = parent.children[i];
        if (cid != 0 && cid != -1) childCount++; else break;
    }
    if (childCount <= 1) {
        parent.numKeys = 0;
        file.seekp(parentIndex * sizeof(Node), ios::beg);
        file.write((char*)&parent, sizeof(Node));
        return;
    }

    // Build keys as max of each child i (for i from 0..childCount-2)
    int newNumKeys = childCount - 1;
    for (int i = 0; i < newNumKeys; i++) {
        int childIdx = parent.children[i];
        if (childIdx == 0 || childIdx == -1) break;
        Node child;
        file.seekg(childIdx * sizeof(Node), ios::beg);
        file.read((char*)&child, sizeof(Node));
        // max key of child: leaf -> last key; internal -> max of rightmost subtree
        int maxKey;
        if (child.flag == 0) {
            maxKey = child.numKeys > 0 ? child.recordIDs[child.numKeys - 1] : 0;
        } else {
            // descend rightmost
            int cur = childIdx;
            while (true) {
                Node curNode;
                file.seekg(cur * sizeof(Node), ios::beg);
                file.read((char*)&curNode, sizeof(Node));
                if (curNode.flag == 0) {
                    maxKey = curNode.numKeys > 0 ? curNode.recordIDs[curNode.numKeys - 1] : 0;
                    break;
                }
                int nextChild = curNode.children[curNode.numKeys];
                if (nextChild == 0 || nextChild == -1) { maxKey = 0; break; }
                cur = nextChild;
            }
        }
        parent.recordIDs[i] = maxKey;
    }
    parent.numKeys = newNumKeys;
    // write back
    file.seekp(parentIndex * sizeof(Node), ios::beg);
    file.write((char*)&parent, sizeof(Node));
}

// Helper: Remove a child index from a parent node's children array
void RemoveChildFromParent(fstream &file, int parentIndex, int childIndexToRemove) {
    if (parentIndex == -1) return;

    Node parent;
    file.seekg(parentIndex * sizeof(Node), ios::beg);
    file.read((char*)&parent, sizeof(Node));

    int foundPos = -1;
    // Find position of the child to remove
    for (int i = 0; i <= parent.numKeys + 1; i++) { // search a bit past valid keys just in case
        if (parent.children[i] == childIndexToRemove) {
            foundPos = i;
            break;
        }
    }

    if (foundPos != -1) {
        // Shift children left to fill the gap
        for (int i = foundPos; i <= parent.numKeys; i++) {
            parent.children[i] = parent.children[i + 1];
        }
        parent.children[parent.numKeys + 1] = 0; // Clean up the end

        // We don't decrement numKeys here; UpdateParentSeparators does that based on remaining children
        file.seekp(parentIndex * sizeof(Node), ios::beg);
        file.write((char*)&parent, sizeof(Node));
    }
}


void DeleteRecordFromIndex(char *filename, int RecordID) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error opening index file.\n";
        return;
    }

    // Read m from Node 0
    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));
    int m = freeListHead.numKeys;
    int minKeys = m / 2; // integer division corresponds to floor(m/2)

    int searchResult = SearchARecord(filename, RecordID);
    if (searchResult == -1) {
        cout << "Delete failed: Record not found.\n";
        file.close();
        return;
    }

    if (historyTop < 0) {
        cout << "Delete failed: Tree is empty or corrupted.\n";
        file.close();
        return;
    }

    int leafIndex = historyStack[historyTop];
    Node leaf;
    int parentIndex = -1;
    int leftSiblingIndex = -1;
    int rightSiblingIndex = -1;

    // Determine parent and siblings
    if (historyTop - 1 >= 0) {
        parentIndex = historyStack[historyTop - 1];
        Node parentNode;
        file.seekg(parentIndex * sizeof(Node), ios::beg);
        file.read((char *) &parentNode, sizeof(Node));
        for (int i = 0; i <= parentNode.numKeys; i++) {
            if (parentNode.children[i] == leafIndex) {
                if (i - 1 >= 0) leftSiblingIndex = parentNode.children[i - 1];
                if (i + 1 <= parentNode.numKeys) rightSiblingIndex = parentNode.children[i + 1];
                break;
            }
        }
    }

    file.seekg(leafIndex * sizeof(Node), ios::beg);
    file.read((char *) &leaf, sizeof(Node));

    // Detect if we're deleting the maximum key (for propagation)
    int oldMax = -1;
    int newMax = -1;
    int deletePos = -1;
    bool isMaxKey = false;

    for (int i = 0; i < leaf.numKeys; i++) {
        if (leaf.recordIDs[i] == RecordID) {
            deletePos = i;
            break;
        }
    }

    if (deletePos == -1) {
        cout << "Delete failed: Record not found in leaf node." << endl;
        file.close();
        return;
    }

    if (deletePos == leaf.numKeys - 1) {
        isMaxKey = true;
        oldMax = RecordID;
    }

    // === PERFORM LOCAL DELETION ===
    // Shift keys to delete the record locally
    for (int j = deletePos; j < leaf.numKeys - 1; j++) {
        leaf.recordIDs[j] = leaf.recordIDs[j + 1];
        leaf.references[j] = leaf.references[j + 1];
    }
    leaf.numKeys--;

    // Save the node state temporarily
    file.seekp(leafIndex * sizeof(Node), ios::beg);
    file.write((char *) &leaf, sizeof(Node));
    cout << "Deleted RecordID " << RecordID << " from leaf node " << leafIndex << endl;

    // Calculate new max key after local deletion
    if (leaf.numKeys > 0) {
        newMax = leaf.recordIDs[leaf.numKeys - 1];
    }

    // --- CASE 1: ROOT IS LEAF ---
    if (parentIndex == -1) {
        // Nothing special to do for root, even if keys < minKeys
        file.close();
        return;
    }

    // --- CASE 2: NO UNDERFLOW (Node has >= minKeys) ---
    if (leaf.numKeys >= minKeys) {
        UpdateParentSeparators(file, parentIndex);

        // Propagate Max Key Change if needed
        if (isMaxKey && newMax != -1 && newMax != oldMax) {
            for (int h = historyTop - 1; h >= 0; h--) {
                int ancestorIndex = historyStack[h];
                Node ancestor;
                file.seekg(ancestorIndex * sizeof(Node), ios::beg);
                file.read((char*)&ancestor, sizeof(Node));
                bool found = false;
                for (int k = 0; k < ancestor.numKeys; k++) {
                    if (ancestor.recordIDs[k] == oldMax) {
                        ancestor.recordIDs[k] = newMax;
                        found = true;
                    }
                }
                if (found) {
                    file.seekp(ancestorIndex * sizeof(Node), ios::beg);
                    file.write((char*)&ancestor, sizeof(Node));
                }
            }
        }
        file.close();
        return;
    }

    // --- CASE 3: UNDERFLOW (Must Borrow or Merge) ---
    bool borrowed = false;

    // Try Borrowing from LEFT
    if (hasLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex)) {
        Node leftSib;
        file.seekg(leftSiblingIndex * sizeof(Node));
        file.read((char*)&leftSib, sizeof(Node));

        // CHECK CONDITION: Sibling must have MORE than minKeys
        if (leftSib.numKeys > minKeys) {
            int siblingOldMax = leftSib.recordIDs[leftSib.numKeys - 1];

            borrowFromLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex, m);
            cout << "Borrowed from left sibling (Node " << leftSiblingIndex << ")\n";
            borrowed = true;

            // Handle Max Key updates for the sibling (since it lost its max key)
            // ... (Logic remains similar to your original code for updates) ...
        }
    }

    // Try Borrowing from RIGHT (if left failed)
    if (!borrowed && hasRightSibling(file, parentIndex, leafIndex, rightSiblingIndex)) {
        Node rightSib;
        file.seekg(rightSiblingIndex * sizeof(Node));
        file.read((char*)&rightSib, sizeof(Node));

        // CHECK CONDITION: Sibling must have MORE than minKeys
        if (rightSib.numKeys > minKeys) {
            borrowFromRightSibling(file, parentIndex, leafIndex, rightSiblingIndex, m);
            cout << "Borrowed from right sibling (Node " << rightSiblingIndex << ")\n";
            borrowed = true;
        }
    }

    if (borrowed) {
        // If we borrowed successfully, just update separators and propagate changes
        UpdateParentSeparators(file, parentIndex);

        // Re-read leaf to get current max (borrow might have changed it)
        file.seekg(leafIndex * sizeof(Node));
        file.read((char*)&leaf, sizeof(Node));
        newMax = leaf.recordIDs[leaf.numKeys - 1];

        // Propagate change (oldMax -> newMax)
        if (isMaxKey && newMax != oldMax) {
             for (int h = historyTop - 1; h >= 0; h--) {
                int ancestorIndex = historyStack[h];
                Node ancestor;
                file.seekg(ancestorIndex * sizeof(Node), ios::beg);
                file.read((char*)&ancestor, sizeof(Node));
                bool found = false;
                for (int k = 0; k < ancestor.numKeys; k++) {
                    if (ancestor.recordIDs[k] == oldMax) {
                        ancestor.recordIDs[k] = newMax;
                        found = true;
                    }
                }
                if (found) {
                    file.seekp(ancestorIndex * sizeof(Node), ios::beg);
                    file.write((char*)&ancestor, sizeof(Node));
                }
            }
        }
        file.close();
        return;
    }

    // --- MERGE LOGIC (If borrow was not possible) ---
    // At this point, siblings are at minKeys, so we MUST merge.

    if (hasLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex)) {
        cout << "Merging with Left Sibling (Node " << leftSiblingIndex << ")...\n";
        mergeWithLeft(file, parentIndex, leafIndex, leftSiblingIndex, true, m);

        // CRITICAL: The current node (leafIndex) is now free.
        // We must remove it from the parent's children list.
        RemoveChildFromParent(file, parentIndex, leafIndex);

    } else if (hasRightSibling(file, parentIndex, leafIndex, rightSiblingIndex)) {
        cout << "Merging with Right Sibling (Node " << rightSiblingIndex << ")...\n";
        mergeWithRight(file, parentIndex, leafIndex, rightSiblingIndex, true, m);

        // CRITICAL: The right sibling (rightSiblingIndex) is now free.
        // We must remove it from the parent's children list.
        RemoveChildFromParent(file, parentIndex, rightSiblingIndex);
    }

    // Final updates after merge
    UpdateParentSeparators(file, parentIndex);

    // Propagate max key changes (since we merged, the deleted key might have been a separator)
    // For simplicity, just run the propagation loop if oldMax was involved
    if (isMaxKey) {
        // Determine the survivor node index to find the new max
        int survivorIndex = (leftSiblingIndex != -1) ? leftSiblingIndex : leafIndex;

        Node survivor;
        file.seekg(survivorIndex * sizeof(Node));
        file.read((char*)&survivor, sizeof(Node));

        if (survivor.numKeys > 0) {
            newMax = survivor.recordIDs[survivor.numKeys - 1];
            if (newMax != oldMax) {
                 for (int h = historyTop - 1; h >= 0; h--) {
                    int ancestorIndex = historyStack[h];
                    Node ancestor;
                    file.seekg(ancestorIndex * sizeof(Node), ios::beg);
                    file.read((char*)&ancestor, sizeof(Node));
                    bool found = false;
                    for (int k = 0; k < ancestor.numKeys; k++) {
                        if (ancestor.recordIDs[k] == oldMax) {
                            ancestor.recordIDs[k] = newMax;
                            found = true;
                        }
                    }
                    if (found) {
                        file.seekp(ancestorIndex * sizeof(Node), ios::beg);
                        file.write((char*)&ancestor, sizeof(Node));
                    }
                }
            }
        }
    }

    file.close();
}

