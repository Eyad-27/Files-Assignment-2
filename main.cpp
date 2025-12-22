#include "BTreeIndex.h"

using namespace std;

int main() {
    char filename[] = "btree_index.dat";
    int m = 5;  // Default B-Tree order
    int choice;
    bool fileCreated = false;

    cout << "\n==================================================" << endl;
    cout << "    B-TREE INDEX FILE MANAGEMENT SYSTEM" << endl;
    cout << "==================================================" << endl;

    while (true) {
        cout << "\n" << endl;
        cout << "==================================================" << endl;
        cout << "              MAIN MENU" << endl;
        cout << "==================================================" << endl;
        cout << "1. Create Index File" << endl;
        cout << "2. Insert Record" << endl;
        cout << "3. Search Record" << endl;
        cout << "4. Delete Record" << endl;
        cout << "5. Display File Content" << endl;
        cout << "6. Display Tree (Level-Order Visualization)" << endl;
        cout << "7. Exit" << endl;
        cout << "==================================================" << endl;
        cout << "Enter your choice (1-7): ";
        cin >> choice;

        switch (choice) {
            case 1: {
                // Create Index File
                int numberOfRecords;
                cout << "\nEnter number of nodes to allocate: ";
                cin >> numberOfRecords;
                cout << "Enter B-Tree order (m): ";
                cin >> m;
                CreateIndexFileFile(filename, numberOfRecords, m);
                fileCreated = true;
                break;
            }

            case 2: {
                // Insert Record
                if (!fileCreated) {
                    cout << "\nError: Please create index file first (Option 1)" << endl;
                    break;
                }
                int recordID, reference;
                cout << "\nEnter Record ID: ";
                cin >> recordID;
                cout << "Enter Reference: ";
                cin >> reference;
                InsertNewRecordAtIndex(filename, recordID, reference);
                break;
            }

            case 3: {
                // Search Record
                if (!fileCreated) {
                    cout << "\nError: Please create index file first (Option 1)" << endl;
                    break;
                }
                int recordID;
                cout << "\nEnter Record ID to search: ";
                cin >> recordID;
                int result = SearchARecord(filename, recordID);
                if (result != -1) {
                    cout << "\nReference found: " << result << endl;
                } else {
                    cout << "\nRecord not found." << endl;
                }
                cout << "\nHistory Stack (nodes visited): ";
                for (int i = 0; i <= historyTop; i++) {
                    cout << historyStack[i] << " ";
                }
                cout << endl;
                break;
            }

            case 4: {
                // Delete Record
                if (!fileCreated) {
                    cout << "\nError: Please create index file first (Option 1)" << endl;
                    break;
                }
                int recordID;
                cout << "\nEnter Record ID to delete: ";
                cin >> recordID;
                DeleteRecordFromIndex(filename, recordID);
                break;
            }

            case 5: {
                // Display File Content
                if (!fileCreated) {
                    cout << "\nError: Please create index file first (Option 1)" << endl;
                    break;
                }
                DisplayIndexFileContent(filename);
                break;
            }

            case 6: {
                // Display Tree (Level-Order Visualization)
                if (!fileCreated) {
                    cout << "\nError: Please create index file first (Option 1)" << endl;
                    break;
                }
                DisplayTreeLevelOrder(filename);
                break;
            }

            case 7: {
                // Exit
                cout << "\nExiting B-Tree Index Management System..." << endl;
                cout << "Goodbye!" << endl;
                return 0;
            }

            default: {
                cout << "\nInvalid choice! Please enter a number between 1 and 7." << endl;
            }
        }
    }

    return 0;
}