#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
using namespace std;

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

