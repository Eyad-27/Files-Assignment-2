#include "BTreeIndex.h"
#include <iostream>
#include <fstream>
#include <queue>
using namespace std;

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
                // Add the max of the rightmost subtree
                int rightMaxKey = getMaxKeyInSubtree(file, node.children[node.numKeys]);
                if (rightMaxKey != -1) {
                    cout << ", " << rightMaxKey;
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
                            // Normal Case: The key at index 'j' is the separator for child 'j'
                            separator = node.recordIDs[j];
                        } else {
                            // Special Case: The Rightmost Child (j == numKeys)
                            // It doesn't have a key to its right.
                            // We usually label it with the key to its LEFT (the last key of the node)
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

