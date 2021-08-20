#include <typedef.h>
#include <math/math.h>

enum AVL_TREE_IMBALANCE
{
    RIGHT_LEFT = 0,
    LEFT = 1,
    RIGHT = 2,
    LEFT_RIGHT = 3
};

/* AVL tree based on TASK IDs */
typedef struct TREE_NODE
{
    void *data;
    UINT64 *key;

    UINT64 height;
    TREE_NODE *left;
    TREE_NODE *right;
    TREE_NODE *parent;
} TREE_NODE;

class AvlTree
{
public:
    TREE_NODE *root;
    UINT64 nNodes;

    AvlTree();
    void Insert(void *data, UINT64 *key);
    void Remove(KE_HANDLE handle);
    void *Search(KE_HANDLE handle, TREE_NODE *startNode = nullptr);
    TREE_NODE *SearchNode(KE_HANDLE handle, TREE_NODE *startNode = nullptr);
    TREE_NODE *Predecessor(TREE_NODE *node);
    TREE_NODE *Successor(TREE_NODE *node);
    void RotateLeft(TREE_NODE *node);
    void RotateRight(TREE_NODE *node);
    void UpdateHeight(TREE_NODE *node);
    void BalanceNode(TREE_NODE *node);
};

void treePrintout(TREE_NODE *begin);