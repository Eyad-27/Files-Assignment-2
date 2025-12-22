#ifndef BTREEINDEX_H
#define BTREEINDEX_H

#include <bits/stdc++.h>

using namespace std;

// ============================================================================
// GLOBAL CONFIGURATION
// ============================================================================
const int MAX_M = 10;  // Maximum number of descendants (B-Tree order)
const int MAX_HISTORY = 1000;  // Maximum depth for history stack

// ============================================================================
// NODE STRUCTURE
// ============================================================================
struct Node {
    int flag;                    // 0 = Leaf, 1 = Non-Leaf, -1 = Free
    int numKeys;                 // Number of keys currently in node
    int recordIDs[MAX_M];        // Keys (Record IDs)
    int references[MAX_M];       // Reference offsets to data file (for leaves)
    int children[MAX_M + 1];     // Child node indices (for non-leaves)
    int nextFree;                // For free list: index of next free node (-1 if none)

    // Constructor to initialize the node
    Node();
};

// ============================================================================
// GLOBAL HISTORY STACK
// ============================================================================
extern int historyStack[MAX_HISTORY];
extern int historyTop;

// Helper functions for history stack
void pushHistory(int nodeIndex);
void clearHistory();

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Eyad: Create Index File
void CreateIndexFileFile(char* filename, int numberOfRecords, int m);

// Eyad: Display Index File Content
void DisplayIndexFileContent(char* filename);

// Eyad: Search for a Record
int SearchARecord(char* filename, int RecordID);

// Moaid & Jana: Insert New Record
int InsertNewRecordAtIndex(char* filename, int RecordID, int Reference);

// Yassin & Pedro: Delete Record
void DeleteRecordFromIndex(char* filename, int RecordID);

// Level-Order Visualization with Parent Links
void DisplayTreeLevelOrder(char* filename);

#endif // BTREEINDEX_H
