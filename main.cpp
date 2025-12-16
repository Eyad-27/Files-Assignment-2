#include "BTreeIndex.h"

using namespace std;

// ============================================================================
// MAIN APPLICATION ENTRY POINT
// Owner: Member 5 (Integration & Testing)
// ============================================================================
// CURRENT STATUS:
// - Validates Member 1's Structure (Create, Display, Search).
// - Insert & Delete are currently stubs.
//
// TODO (Member 5):
// 1. Once Member 2/3 finish Insertion, test the insert functionality.
// 2. Once Member 4/5 finish Deletion, test the delete functionality.
// ============================================================================


int main() {
    char filename[] = "btree_index.dat";
    int numberOfRecords = 10;  // Create file with 10 nodes
    int m = 5;                 // Order of B-Tree (5 descendants)

    cout << "==================================================" << endl;
    cout << "B-TREE INDEX FILE MANAGEMENT - MEMBER 1 TESTING" << endl;
    cout << "==================================================" << endl;

    // Test 1: Create Index File
    cout << "\n[TEST 1] Creating Index File..." << endl;
    CreateIndexFileFile(filename, numberOfRecords, m);

    // Test 2: Display Index File Content
    cout << "\n[TEST 2] Displaying Index File Content..." << endl;
    DisplayIndexFileContent(filename);

    // Test 3: Search for a record (will not be found in empty tree)
    cout << "\n[TEST 3] Searching for Record ID 100..." << endl;
    int result = SearchARecord(filename, 100);
    cout << "Search Result: " << result << endl;

    // Display history stack
    cout << "\nHistory Stack (nodes visited): ";
    for (int i = 0; i <= historyTop; i++) {
        cout << historyStack[i] << " ";
    }
    cout << endl;

    cout << "\n==================================================" << endl;
    cout << "MEMBER 1 TESTING COMPLETE" << endl;
    cout << "==================================================" << endl;
    cout << "\nNOTE: Insert and Delete functions are stubs for teammates." << endl;
    cout << "      - Members 2 & 3: Implement InsertNewRecordAtIndex" << endl;
    cout << "      - Members 4 & 5: Implement DeleteRecordFromIndex" << endl;

    return 0;
}