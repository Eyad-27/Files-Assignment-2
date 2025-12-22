#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
using namespace std;

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

