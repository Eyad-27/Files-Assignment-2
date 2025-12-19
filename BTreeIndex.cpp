#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// ============================================================================
// GLOBAL HISTORY STACK (Member 1 - Helper)
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
// MEMBER 1: FUNCTION 1 - Create Index File
// ============================================================================

void CreateIndexFileFile(char* filename, int numberOfRecords, int m) {
    ofstream file(filename, ios::binary | ios::trunc);

    if (!file.is_open()) {
        cerr << "Error: Could not create file " << filename << endl;
        return;
    }

    // Node 0 is the Free List Head
    // It points to the first empty node (node 1)
    Node freeListHead;
    freeListHead.flag = -1;  // Special flag for free list head
    freeListHead.nextFree = 1;  // Points to first empty node
    file.write((char*)&freeListHead, sizeof(Node));

    // Create all empty nodes in a linked list
    for (int i = 1; i < numberOfRecords; i++) {
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
         << " empty nodes (m=" << m << ")" << endl;
}


// ============================================================================
// MEMBER 1: FUNCTION 2 - Display Index File Content
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
// MEMBER 1: FUNCTION 3 - Search for a Record
// ============================================================================
int SearchARecord(char* filename, int RecordID) {
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return -1;
    }

    clearHistory();

    int currentIndex = 1;
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
        while (i < currentNode.numKeys && RecordID > currentNode.recordIDs[i]) i++;

        if (i < currentNode.numKeys && RecordID == currentNode.recordIDs[i]) {
            if (currentNode.flag == 0) {
                cout << "Record " << RecordID << " found at Node " << currentIndex
                     << ", Reference: " << currentNode.references[i] << endl;
                file.close();
                return currentNode.references[i];
            } else {
                currentIndex = currentNode.children[i + 1];
            }
        } else {
            if (currentNode.flag == 0) {
                cout << "Record " << RecordID << " not found in tree." << endl;
                file.close();
                return -1;
            } else {
                currentIndex = currentNode.children[i];
            }
        }
    }

    file.close();
    cout << "Record " << RecordID << " not found in tree." << endl;
    return -1;
}

// ============================================================================
// MEMBER 3: Split Node and Recursive Promotion
// ============================================================================
int SplitNodeAndInsert(fstream &file, int nodeIndex, int RecordID, int Reference, int m, int &newChildIndex) {
    Node fullNode;
    file.seekg(nodeIndex * sizeof(Node), ios::beg);
    file.read((char*)&fullNode, sizeof(Node));

    // Create temporary arrays for keys and refs
    int tempKeys[MAX_M + 1];
    int tempRefs[MAX_M + 1];
    int i = 0, j = 0;
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
    int medianIndex = totalKeys / 2;
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
    newNode.numKeys = totalKeys - medianIndex - 1;

    for (int k = 0; k < newNode.numKeys; k++) {
        newNode.recordIDs[k] = tempKeys[medianIndex + 1 + k];
        newNode.references[k] = tempRefs[medianIndex + 1 + k];
    }

    fullNode.numKeys = medianIndex;
    for (int k = 0; k < medianIndex; k++) {
        fullNode.recordIDs[k] = tempKeys[k];
        fullNode.references[k] = tempRefs[k];
    }

    // Write back nodes
    file.seekp(nodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&fullNode, sizeof(Node));
    file.seekp(newNodeIndex * sizeof(Node), ios::beg);
    file.write((char*)&newNode, sizeof(Node));

    cout << "Split node " << nodeIndex << ", promoted key " << medianKey
         << " to parent. New node index: " << newNodeIndex << endl;

    newChildIndex = newNodeIndex;
    return medianKey;
}

// ============================================================================
// MEMBER 2 & 3: Insert New Record (Recursive for Parent Split)
// ============================================================================
int InsertNewRecordAtIndex(char* filename, int RecordID, int Reference, int m) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error opening index file.\n";
        return -1;
    }

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

    if (leaf.numKeys < MAX_M) {
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

    // Node full → Split
    cout << "Leaf node " << leafIndex << " is full. Split required (Member 3).\n";

    int newChildIndex;
    int promotedKey = SplitNodeAndInsert(file, leafIndex, RecordID, Reference, m, newChildIndex);

    // Handle parent/root recursively
    int parentIndex = -1;
    int childIndex = leafIndex;

    for (int h = historyTop - 1; h >= 0; h--) {
        parentIndex = historyStack[h];
        Node parentNode;
        file.seekg(parentIndex * sizeof(Node), ios::beg);
        file.read((char*)&parentNode, sizeof(Node));

        if (parentNode.numKeys < MAX_M) {
            int i = parentNode.numKeys - 1;
            while (i >= 0 && promotedKey < parentNode.recordIDs[i]) {
                parentNode.recordIDs[i + 1] = parentNode.recordIDs[i];
                parentNode.children[i + 2] = parentNode.children[i + 1];
                i--;
            }
            parentNode.recordIDs[i + 1] = promotedKey;
            parentNode.children[i + 2] = newChildIndex;
            parentNode.numKeys++;
            parentNode.flag = 1; // non-leaf
            file.seekp(parentIndex * sizeof(Node), ios::beg);
            file.write((char*)&parentNode, sizeof(Node));
            file.close();
            return parentIndex;
        } else {
            // parent full → split
            int tempChildIndex;
            promotedKey = SplitNodeAndInsert(file, parentIndex, promotedKey, newChildIndex, m, tempChildIndex);
            childIndex = parentIndex;
            newChildIndex = tempChildIndex;
        }
    }

    // Root split → create new root
    Node newRoot;
    newRoot.flag = 1;
    newRoot.numKeys = 1;
    newRoot.recordIDs[0] = promotedKey;
    newRoot.children[0] = leafIndex;
    newRoot.children[1] = newChildIndex;

    Node freeListHead;
    file.seekg(0, ios::beg);
    file.read((char*)&freeListHead, sizeof(Node));

    int newRootIndex = freeListHead.nextFree;
    Node tempFreeNode;
    file.seekg(newRootIndex * sizeof(Node), ios::beg);
    file.read((char*)&tempFreeNode, sizeof(Node));

    freeListHead.nextFree = tempFreeNode.nextFree;
    file.seekp(0, ios::beg);
    file.write((char*)&freeListHead, sizeof(Node));

    file.seekp(newRootIndex * sizeof(Node), ios::beg);
    file.write((char*)&newRoot, sizeof(Node));

    cout << "Created new root node " << newRootIndex << " with promoted key " << promotedKey << endl;
    file.close();
    return newRootIndex;
}

// ============================================================================
// MEMBER 4 & 5: Delete Record (STUB)
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


void DeleteRecordFromIndex(char *filename, int RecordID) {
    // TODO: Member 4 - Handle finding record and leaf deletion
    // TODO: Member 5 - Handle Underflow (Merge/Borrow) and rebalancing
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error opening index file.\n";
        return;
    }

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
    int minKeys = ceil(5 / 2);
    if (leaf.numKeys > minKeys) {
        int i = 0;
        while (i < leaf.numKeys && leaf.recordIDs[i] != RecordID) i++;

        if (i == leaf.numKeys) {
            cout << "Delete failed: Record not found in leaf node.\n";
            file.close();
            return;
        }

        for (int j = i; j < leaf.numKeys - 1; j++) {
            leaf.recordIDs[j] = leaf.recordIDs[j + 1];
            leaf.references[j] = leaf.references[j + 1];
        }
        leaf.numKeys--;

        file.seekp(leafIndex * sizeof(Node), ios::beg);
        file.write((char *) &leaf, sizeof(Node));
        cout << "Deleted RecordID " << RecordID << " from leaf node " << leafIndex << endl;
        file.close();
        return;
    } else {
        if (hasLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex)) {
            borrowFromLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex, 5);
            cout << "Borrowed from left sibling during deletion of RecordID " << RecordID << endl;
            file.close();
            return;
        } else if (hasRightSibling(file, parentIndex, leafIndex, rightSiblingIndex)) {
            borrowFromRightSibling(file, parentIndex, leafIndex, rightSiblingIndex, 5);
            cout << "Borrowed from right sibling during deletion of RecordID " << RecordID << endl;
            file.close();
            return;
        } else {
            if(hasLeftSibling(file, parentIndex, leafIndex, leftSiblingIndex)) {
                mergeWithLeft(file, parentIndex, leafIndex, leftSiblingIndex, true, 5);
                cout << "Merged with left sibling during deletion of RecordID " << RecordID << endl;
            } else if(hasRightSibling(file, parentIndex, leafIndex, rightSiblingIndex)) {
                mergeWithRight(file, parentIndex, leafIndex, rightSiblingIndex, true, 5);
                cout << "Merged with right sibling during deletion of RecordID " << RecordID << endl;
            }
            file.close();
            return;
        }
    }

}

