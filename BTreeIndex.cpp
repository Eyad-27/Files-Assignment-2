#include "BTreeIndex.h"

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
        } else {
            if (node.flag == -1) {
                cout << "  Type: FREE NODE" << endl;
                cout << "  Next Free: " << node.nextFree << endl;
            } else if (node.flag == 0) {
                cout << "  Type: LEAF NODE" << endl;
                cout << "  Number of Keys: " << node.numKeys << endl;
                cout << "  Record IDs: ";
                for (int i = 0; i < node.numKeys; i++) {
                    cout << node.recordIDs[i] << " ";
                }
                cout << endl;
                cout << "  References: ";
                for (int i = 0; i < node.numKeys; i++) {
                    cout << node.references[i] << " ";
                }
                cout << endl;
            } else if (node.flag == 1) {
                cout << "  Type: NON-LEAF NODE" << endl;
                cout << "  Number of Keys: " << node.numKeys << endl;
                cout << "  Record IDs: ";
                for (int i = 0; i < node.numKeys; i++) {
                    cout << node.recordIDs[i] << " ";
                }
                cout << endl;
                cout << "  Children Indices: ";
                for (int i = 0; i <= node.numKeys; i++) {
                    cout << node.children[i] << " ";
                }
                cout << endl;
            }
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

    // Clear history stack before starting
    clearHistory();

    // Start from Root (Index 1)
    int currentIndex = 1;
    Node currentNode;

    while (currentIndex != -1 && currentIndex != 0) {
        // Add current node to history
        pushHistory(currentIndex);

        // Seek to the current node position
        file.seekg(currentIndex * sizeof(Node), ios::beg);
        file.read((char*)&currentNode, sizeof(Node));

        // If node is free, the tree is empty or corrupted
        if (currentNode.flag == -1) {
            cout << "Search: Node " << currentIndex << " is free. Tree may be empty." << endl;
            file.close();
            return -1;
        }

        // Search for the key in current node
        int i = 0;
        while (i < currentNode.numKeys && RecordID > currentNode.recordIDs[i]) {
            i++;
        }

        // Check if we found the key
        if (i < currentNode.numKeys && RecordID == currentNode.recordIDs[i]) {
            // Key found!
            if (currentNode.flag == 0) {
                // Leaf node - return the reference
                cout << "Record " << RecordID << " found at Node " << currentIndex
                     << ", Reference: " << currentNode.references[i] << endl;
                file.close();
                return currentNode.references[i];
            } else {
                // Non-leaf node - move to appropriate child
                currentIndex = currentNode.children[i + 1];
            }
        } else {
            // Key not found in this node
            if (currentNode.flag == 0) {
                // Leaf node - key doesn't exist
                cout << "Record " << RecordID << " not found in tree." << endl;
                file.close();
                return -1;
            } else {
                // Non-leaf node - move to appropriate child
                currentIndex = currentNode.children[i];
            }
        }
    }

    file.close();
    cout << "Record " << RecordID << " not found in tree." << endl;
    return -1;
}

// ============================================================================
// MEMBER 2 & 3: Insert New Record (STUB)
// ============================================================================
int InsertNewRecordAtIndex(char* filename, int RecordID, int Reference) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error opening index file.\n";
        return -1;
    }

    // prevent duplicate keys
    int searchResult = SearchARecord(filename, RecordID);
    if (searchResult != -1) {
        cout << "Insert failed: Record already exists.\n";
        file.close();
        return -1;
    }

    // leaf to insert into is the last node in history stack
    if (historyTop < 0) {
        cout << "Insert failed: Tree is empty or corrupted.\n";
        file.close();
        return -1;
    }

    int leafIndex = historyStack[historyTop];
    Node leaf;

    file.seekg(leafIndex * sizeof(Node), ios::beg);
    file.read((char*)&leaf, sizeof(Node));

    // safety check
    if (leaf.flag != 0) {
        cout << "Insert failed: Target node is not a leaf.\n";
        file.close();
        return -1;
    }

    // 3) If leaf has space, happy path
    if (leaf.numKeys < MAX_M) {

        int i = leaf.numKeys - 1;

        // shift keys to keep sorted order
        while (i >= 0 && RecordID < leaf.recordIDs[i]) {
            leaf.recordIDs[i + 1] = leaf.recordIDs[i];
            leaf.references[i + 1] = leaf.references[i];
            i--;
        }

        // insert new key
        leaf.recordIDs[i + 1] = RecordID;
        leaf.references[i + 1] = Reference;
        leaf.numKeys++;

        // write leaf back to file
        file.seekp(leafIndex * sizeof(Node), ios::beg);
        file.write((char*)&leaf, sizeof(Node));

        cout << "Inserted RecordID " << RecordID
             << " at leaf node " << leafIndex << endl;

        file.close();
        return leafIndex;
    }

    // Member 3 (Splitter)
    cout << "Leaf node " << leafIndex
         << " is full. Split required (Member 3).\n";

    file.close();
    return -1; // Member 3
}


// ============================================================================
// MEMBER 4 & 5: Delete Record (STUB)
// ============================================================================
void DeleteRecordFromIndex(char* filename, int RecordID) {
    // TODO: Member 4 - Handle finding record and leaf deletion
    // TODO: Member 5 - Handle Underflow (Merge/Borrow) and rebalancing
    cout << "DeleteRecordFromIndex: NOT IMPLEMENTED (Member 4 & 5)" << endl;
}

