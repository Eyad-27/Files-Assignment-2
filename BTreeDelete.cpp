#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
using namespace std;

// ============================================================================
// Yassin & Pedro: Delete Record
// ============================================================================

// Sibling checks
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