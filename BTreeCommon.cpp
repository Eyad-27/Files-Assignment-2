#include "BTreeIndex.h"
#include <cstring>

// ============================================================================
// GLOBAL HISTORY STACK
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

