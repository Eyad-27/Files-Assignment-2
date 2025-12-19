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
// NODE STRUCTURE (Fixed-length binary record)
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
// GLOBAL HISTORY STACK (Member 1 - Helper)
// ============================================================================
extern int historyStack[MAX_HISTORY];
extern int historyTop;

// Helper functions for history stack
void pushHistory(int nodeIndex);
void clearHistory();

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Member 1: Create Index File
void CreateIndexFileFile(char* filename, int numberOfRecords, int m);

// Member 1: Display Index File Content
void DisplayIndexFileContent(char* filename);

// Member 1: Search for a Record
int SearchARecord(char* filename, int RecordID);

// Member 2 & 3: Insert New Record
int InsertNewRecordAtIndex(char* filename, int RecordID, int Reference);

// Member 4 & 5: Delete Record
void DeleteRecordFromIndex(char* filename, int RecordID, int m);

#endif // BTREEINDEX_H

