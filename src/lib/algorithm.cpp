#include <algorithm.h>
#include <io.h>
#include <fault.h>

/**********************************************************************
 *  @details Constructor for TaskTree class
 *********************************************************************/
AvlTree::AvlTree()
{
    nNodes = 0;
    root = nullptr;
}

/**********************************************************************
 *  @details Inserts task t into the task tree structure based on the task's handle
 *  @param t - Pointer to task to add
 *********************************************************************/
void AvlTree::Insert(void *data, UINT64 *key)
{
    nNodes++;

    FaultLogAssertSerial(data, "NULL data pointer passed\n");
    FaultLogAssertSerial(key, "NULL key pointer passed\n");
    FaultLogAssertSerial(root->parent == nullptr, "Root has a parent\n");

    /* Allocate node object */
    auto node = new TREE_NODE;
    FaultLogAssertSerial(node, "Newly allocated node is NULL\n");

    //*node = { nullptr };
    /* Set node member attributes */
    node->data = data;
    node->key = key;
    node->height = 1;
    node->left = nullptr;
    node->right = nullptr;
    node->parent = nullptr;

    /* If the tree is empty, set the new node to the head */
    if (root == nullptr)
    {
        FaultLogAssertSerial(nNodes == 1, "NULL root with non-zero member tree\n");
        root = node;
        return;
    }

    /* Drill down tree structure, insert the node*/
    auto finderNode = root;
    while (true)
    {
        /* Move right */
        if (*node->key > *finderNode->key)
        {
            /* Leave the loop when we're ready to insert the node */
            if (finderNode->right == nullptr)
            {
                finderNode->right = node;
                node->parent = finderNode;
                UpdateHeight(node->parent);
                break;
            }

            finderNode = finderNode->right;
        }
        /* Move left */
        else
        {
            /* Leave the loop when we're ready to insert the node */
            if (finderNode->left == nullptr)
            {
                finderNode->left = node;
                node->parent = finderNode;
                UpdateHeight(node->parent);
                break;
            }

            finderNode = finderNode->left;
        }
    }

    /* Check and balance the newly added node */
    BalanceNode(node);
}

/**********************************************************************
 *  @details Removes a task with the specified handle from the task tree structure
 *  @param handle - Handle of task to remove
 *********************************************************************/
void AvlTree::Remove(KE_HANDLE handle)
{
    nNodes--;

    /* Get the task tree node from the handle */
    TREE_NODE *node = SearchNode(handle);
    FaultLogAssertSerial(node != nullptr, "Node %u to remove not found in tree\n", handle);
    FaultLogAssertSerial(*node->key == handle, "Found node ID:%16x does not match passed handle\n", (UINT64)node);

    TREE_NODE *parent = node->parent;
    /* Get successor node, or predecessor if successor doesn't exist */
    TREE_NODE *successor = Successor(node);
    TREE_NODE *predecessor = Predecessor(node);
    TREE_NODE *replacementNode = successor? successor : (predecessor? predecessor : nullptr);

    if (node == root)
    {
        FaultLogAssertSerial(replacementNode || (nNodes == 0), "NULL root with nonempty tree\n");

        root = replacementNode;
    }

    /* Move predecessor/successor */
    if (replacementNode)
    {
        /* Remove it from its parent */
        if (replacementNode->parent)
        {
            if (replacementNode == replacementNode->parent->right) replacementNode->parent->right = nullptr;
            else                                                   replacementNode->parent->left = nullptr;
        }

        /* Climb up the tree and update every node's height */
        for (TREE_NODE *climber = replacementNode->parent; climber != nullptr; climber = climber->parent)
        {
            UpdateHeight(climber);
        }

        /* Take over old node's attributes */
        replacementNode->parent = parent;
        replacementNode->left = node->left;
        replacementNode->right = node->right;

        UpdateHeight(replacementNode);
    }

    /* Set node parent's attributes */
    if (parent)
    {
        if (node == parent->left) parent->left = replacementNode;
        else                      parent->right = replacementNode;

        /* Update parent's height */
        /* TODO: Make UpdateHeight climb and update ALL nodes upwards */
        UpdateHeight(parent);
    }

    /* Set node children attributes */
    if (node->left)
    {
        node->left->parent = replacementNode;
    }
    if (node->right)
    {
        node->right->parent = replacementNode;
    }

    delete node;

    FaultLogAssertSerial(root->parent == nullptr, "Root has a parent\n");

    /* Travel up the tree and balance nodes that require it. More than once balance may be required */
    for (TREE_NODE *climber = parent; climber != nullptr; climber = climber->parent)
    {
        int leftHeight = climber->left? climber->left->height : 0;
        int rightHeight = climber->right? climber->right->height : 0;

        /* If the current node is not unbalanced, skip */
        if (math::abs(leftHeight - rightHeight) < 2)
        {
            continue;
        }

        /* If climber is an unbalanced node, let y be the larger height child of climber,
         * and let x be the larger height child of y */

        /* Get larger height child of unbalanced node */
        TREE_NODE *y = (leftHeight > rightHeight) ? climber->left : climber->right;

        /* Get larger height child of y */
        leftHeight = y->left? y->left->height : 0;
        rightHeight = y->right? y->right->height : 0;
        TREE_NODE *x = (leftHeight > rightHeight) ? y->left : y->right;

        FaultLogAssertSerial((y != nullptr) && (x != nullptr), "NULL x or y pointer for balancing step\n");

        /* Re-balance the nodes based off the imbalance pattern */
        /* Get the imbalance pattern, one of 4 cases:
         * left-left, left-right, right-right, right-left */
        auto imbalance = (AVL_TREE_IMBALANCE)((UINT64)(y == climber->left)*2 +
                                              (UINT64)(x == y->right));

            switch(imbalance)
        {
            case RIGHT_LEFT:
                RotateRight(y);
                RotateLeft(climber);
                break;
            case LEFT:
                RotateLeft(climber);
                break;
            case RIGHT:
                RotateRight(climber);
                break;
            case LEFT_RIGHT:
                RotateLeft(y);
                RotateRight(climber);
                break;
        }
    }
}

//void AvlTree::TreePrintoutAsciiSerial()

/**********************************************************************
 *  @details Searches for a task with the specified handle
 *  @param handle - Handle of task being searched for
 *  @param startNode - Optional, node to begin searching from
 *  @returns - Pointer to located node
 *********************************************************************/
void *AvlTree::Search(KE_HANDLE handle, TREE_NODE *startNode)
{
    if (startNode == nullptr)
    {
        startNode = root;
        FaultLogAssertSerial(startNode, "(%u) NULL root node\n", handle);
    }

    TREE_NODE *nodeFinder = startNode;

    /* While the node has not been found */
    while ((nodeFinder != nullptr) && (*nodeFinder->key != handle))
    {
        /* Travel right if the key is bigger than the current node's */
        if (handle > *nodeFinder->key)
        {
            nodeFinder = nodeFinder->right;
        }
            /* Otherwise, travel left */
        else
        {
            nodeFinder = nodeFinder->left;
        }
    }

    if (nodeFinder == nullptr) return nullptr;
    return nodeFinder->data;
}

/**********************************************************************
 *  @details Searches for a task tree node with the specified handle
 *  @param handle - Handle of task being searched for
 *  @param startNode - Optional, node to begin searching from
 *  @returns - Pointer to located task tree node
 *********************************************************************/
TREE_NODE *AvlTree::SearchNode(KE_HANDLE handle, TREE_NODE *startNode)
{
    if (startNode == nullptr)
    {
        startNode = root;
        FaultLogAssertSerial(startNode, "%u NULL root node\n", handle);
    }

    TREE_NODE *nodeFinder = startNode;

    /* While the node has not been found */
    while (nodeFinder != nullptr && *nodeFinder->key != handle)
    {
        /* Travel right if the key is bigger than the current node's */
        if (handle > *nodeFinder->key)
        {
            nodeFinder = nodeFinder->right;
        }
        /* Otherwise, travel left */
        else
        {
            nodeFinder = nodeFinder->left;
        }
    }

    return nodeFinder;
}

/**********************************************************************
 *  @details Gets the predecessor of a node
 *  @param node - Node to get predecessor of
 *  @returns - Predecessor node or nullptr if no such node exists
 *********************************************************************/
TREE_NODE *AvlTree::Predecessor(TREE_NODE *node)
{
    /* Start at node to the left */
    TREE_NODE *nodeFinder = node->left;

    /* If no left node exists, then no predecessor exists */
    if (nodeFinder == nullptr)
    {
        return nullptr;
    }

    /* Find the leftmost node in the right subtree */
    while(true)
    {
        if (nodeFinder->right == nullptr)
        {
            if (nodeFinder->left == nullptr)
                break;

            nodeFinder = nodeFinder->left;
        }

        nodeFinder = nodeFinder->right;
    }

    return nodeFinder;
}

/**********************************************************************
 *  @details Gets the successor of a node
 *  @param node - Node to get successor of
 *  @returns - Successor node or nullptr if no such node exists
 *********************************************************************/
TREE_NODE *AvlTree::Successor(TREE_NODE *node)
{
    /* Start at node to the right */
    TREE_NODE *nodeFinder = node->right;

    /* If no right node exists, then no successor exists */
    if (nodeFinder == nullptr)
    {
        return nullptr;
    }

    /* Find the leftmost node in the right subtree */
    while(true)
    {
        if (nodeFinder->left == nullptr)
        {
            if (nodeFinder->right == nullptr)
                break;

            nodeFinder = nodeFinder->right;
        }

        nodeFinder = nodeFinder->left;
    }

    return nodeFinder;
}

/**********************************************************************
 *  @details Performs a left rotation on a given node
 *  @param node - Pointer to task tree node
 *********************************************************************/
void AvlTree::RotateLeft(TREE_NODE *node)
{
    TREE_NODE *parent = node->parent;
    TREE_NODE *right = node->right;

    FaultLogAssertSerial(node, "Passed argument is NULL\n");
    FaultLogAssertSerial(!(parent && (root == node)), "Root of tree has a parent\n");
    FaultLogAssertSerial(right, "Cannot perform left rotate on a node with no right child\n");

    /* If the root node is being rotated left, its right child is new root */
    if (node == root)
    {
        root = right;
    }

    /* Modify node's parent pointers */
    if (parent && (node == parent->left))
    {
        parent->left = right;
    }
    else if (parent)
    {
        parent->right = right;
    }

    /* Modify the node's pointers */
    node->right = right->left;
    node->parent = right;

    if (right->left)
    {
        right->left->parent = node;
    }

    /* Modify the right node's pointers */
    right->parent = parent;
    right->left = node;

    /* Recalculate the node's height */
    UpdateHeight(node);
    UpdateHeight(right);
}

/**********************************************************************
 *  @details Performs a right rotation on a given node
 *  @param node - Pointer to task tree node
 *********************************************************************/
void AvlTree::RotateRight(TREE_NODE *node)
{
    TREE_NODE *parent = node->parent;
    TREE_NODE *left = node->left;

    FaultLogAssertSerial(node, "Passed argument is NULL\n");
    FaultLogAssertSerial(root->parent == nullptr, "Root of tree has a parent\n");
    FaultLogAssertSerial(left, "Cannot perform right rotate on a node with no left child\n");

    /* If the root node is being rotated right, its left child is new root */
    if (node == root)
    {
        root = left;
    }

    /* Modify node's parent pointers */
    if ((parent != nullptr) && (node == parent->left))
    {
        parent->left = node->left;
    }
    else if (parent != nullptr)
    {
        parent->right = node->left;
    }

    /* Modify the node's pointers */
    node->left = left->right;
    node->parent = left;

    if (left->right)
    {
        left->right->parent = node;
    }

    /* Modify the left node's pointers */
    left->parent = parent;
    left->right = node;

    /* Recalculate the node's height */
    UpdateHeight(node);
    UpdateHeight(left);
}

/**********************************************************************
 *  @details Updates a node's height to the max of its left and right height plus 1
 *  @param node - Pointer to task tree node
 *********************************************************************/
void AvlTree::UpdateHeight(TREE_NODE *node)
{
    UINT64 leftHeight = node->left? node->left->height : 0;
    UINT64 rightHeight = node->right? node->right->height : 0;
    node->height = math::max(leftHeight + 1, rightHeight + 1);
}

/**********************************************************************
 *  @details Performs a right rotation on a given node
 *  @param node - Pointer to task tree node
 *********************************************************************/
void AvlTree::BalanceNode(TREE_NODE *node)
{
    /* Travel up the tree */
    TREE_NODE *lastNode = node;
    UINT64 i = 1;

    FaultLogAssertSerial(node != nullptr, "node is nullptr\n");

    for (TREE_NODE *finderNode = node->parent; finderNode != nullptr; finderNode = finderNode->parent)
    {
        if (++i > finderNode->height)
        {
            finderNode->height = i;
        }
            UINT64 leftHeight = finderNode->left? finderNode->left->height : 0;
        UINT64 rightHeight = finderNode->right? finderNode->right->height : 0;

            /* Continue traveling if no imbalances */
        if (math::abs(((int)leftHeight - (int)rightHeight)) < 2)
        {
            lastNode = finderNode;
            continue;
        }

        /* Get the imbalance pattern, one of 4 cases:
         * left-left, left-right, right-right, right-left */
        auto imbalance = (AVL_TREE_IMBALANCE)((UINT64)(lastNode == finderNode->left)*2 +
                                              (UINT64)(Search(*node->key, lastNode->right) != nullptr));

            switch(imbalance)
        {
            case RIGHT_LEFT:
                RotateRight(lastNode);
                RotateLeft(finderNode);
                return;
            case LEFT:
                RotateLeft(finderNode);
                return;
            case RIGHT:
                RotateRight(finderNode);
                return;
            case LEFT_RIGHT:
                RotateLeft(lastNode);
                RotateRight(finderNode);
                return;
        }
    }
}

void treePrintout(TREE_NODE *begin)
{
    TREE_NODE *traverse = begin;

    if (traverse->left)
    {
        treePrintout(traverse->left);
    }

    if (traverse->right)
    {
        treePrintout(traverse->right);
    }
}
