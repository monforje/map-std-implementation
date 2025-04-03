#ifndef REDBLACKTREE_HPP
#define REDBLACKTREE_HPP

#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <functional>
#include <utility>
#include <type_traits>

enum Color { RED, BLACK };

// Для того, чтобы компаратор мог работать с объектами не конвертируемыми в Key
template <typename C, typename = void>
struct is_transparent_helper : std::false_type {};

// True - если есть вложенный тип is_transparent
template <typename C>
struct is_transparent_helper<C, std::void_t<typename C::is_transparent>> : std::true_type {};

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class RedBlackTree 
{
public:
    struct Node;

    struct Node 
    {
        std::pair<const Key, T> data;
        Color color;
        Node* left;
        Node* right;
        Node* parent;

        explicit Node(const std::pair<const Key, T>& val)
            : data(val), color(RED), left(nullptr), right(nullptr), parent(nullptr) {}
    };

private:
    Node* root;
    Compare comp;

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    NodeAllocator node_alloc;

#if defined(_MSC_VER)
  #define FORCE_INLINE __forceinline
#else
  #define FORCE_INLINE inline __attribute__((always_inline))
#endif

    // Возвращает указатель на указатель, через который доступен узел (у родителя или root)
    FORCE_INLINE Node** getLink(Node* x) 
    {
        if (!x->parent)
            return &root;
        if (x == x->parent->left)
            return &(x->parent->left);
        else
            return &(x->parent->right);
    }

    FORCE_INLINE void leftRotate(Node* x) 
    {
        if (!x || !x->right)
            return;
        Node** xLink = getLink(x);
        Node* y = x->right;

        x->right = y->left;
        if (y->left)
            y->left->parent = x;
        y->parent = x->parent;
        y->left = x;
        x->parent = y;
        *xLink = y;
    }

    FORCE_INLINE void rightRotate(Node* y) 
    {
        if (!y || !y->left)
            return;
        Node** yLink = getLink(y);
        Node* x = y->left;

        y->left = x->right;
        if (x->right)
            x->right->parent = y;
        x->parent = y->parent;
        x->right = y;
        y->parent = x;
        *yLink = x;
    }

    void fixInsert(Node* z) 
    {
        while (z != root && z->parent->color == RED) 
        {
            if (z->parent == z->parent->parent->left) 
            {
                Node* y = z->parent->parent->right;
                if (y && y->color == RED) 
                {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } 
                else 
                {
                    if (z == z->parent->right) 
                    {
                        z = z->parent;
                        leftRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    rightRotate(z->parent->parent);
                }
            } 
            else 
            {
                Node* y = z->parent->parent->left;
                if (y && y->color == RED) 
                {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } 
                else 
                {
                    if (z == z->parent->left) 
                    {
                        z = z->parent;
                        rightRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    leftRotate(z->parent->parent);
                }
            }
        }
        if (root)
            root->color = BLACK;
    }

    void transplant(Node* u, Node* v) 
    {
        Node** uLink = getLink(u);
        if (v)
            v->parent = u->parent;
        *uLink = v;
    }

    void fixDelete(Node* x) 
    {
        while (x != root && x->color == BLACK) 
        {
            if (x == x->parent->left) 
            {
                Node* w = x->parent->right;
                if (w && w->color == RED) 
                {
                    w->color = BLACK;
                    x->parent->color = RED;
                    leftRotate(x->parent);
                    w = x->parent->right;
                }
                if ((!(w->left) || w->left->color == BLACK) &&
                    (!(w->right) || w->right->color == BLACK)) 
                {
                    w->color = RED;
                    x = x->parent;
                } 
                else 
                {
                    if (!(w->right) || w->right->color == BLACK) 
                    {
                        if (w->left)
                            w->left->color = BLACK;
                        w->color = RED;
                        rightRotate(w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->right)
                        w->right->color = BLACK;
                    leftRotate(x->parent);
                    x = root;
                }
            } 
            else 
            {
                Node* w = x->parent->left;
                if (w && w->color == RED) 
                {
                    w->color = BLACK;
                    x->parent->color = RED;
                    rightRotate(x->parent);
                    w = x->parent->left;
                }
                if ((!(w->right) || w->right->color == BLACK) &&
                    (!(w->left) || w->left->color == BLACK)) 
                {
                    w->color = RED;
                    x = x->parent;
                } 
                else 
                {
                    if (!(w->left) || w->left->color == BLACK) 
                    {
                        if (w->right)
                            w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = x->parent->left;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->left)
                        w->left->color = BLACK;
                    rightRotate(x->parent);
                    x = root;
                }
            }
        }
        if (x)
            x->color = BLACK;
    }

    static Node* minimum(Node* node) 
    {
        while (node && node->left)
            node = node->left;
        return node;
    }

    static Node* maximum(Node* node) 
    {
        while (node && node->right)
            node = node->right;
        return node;
    }

    void clearHelper(Node* node) 
    {
        if (node) 
        {
            clearHelper(node->left);
            clearHelper(node->right);
            std::allocator_traits<NodeAllocator>::destroy(node_alloc, node);
            node_alloc.deallocate(node, 1);
        }
    }

    Node* createNode(const std::pair<const Key, T>& val) 
    {
        Node* p = node_alloc.allocate(1);
        try {
            std::allocator_traits<NodeAllocator>::construct(node_alloc, p, val);
        } catch (...) {
            node_alloc.deallocate(p, 1);
            throw;
        }
        return p;
    }

public:
    std::size_t node_count = 0;

    RedBlackTree()
        : root(nullptr), comp(Compare()), node_alloc(NodeAllocator()) {}

    RedBlackTree(const Compare& comp, const Allocator& alloc)
        : root(nullptr), comp(comp), node_alloc(alloc) {}

    ~RedBlackTree() { clear(); }

    RedBlackTree(const RedBlackTree& other)
        : root(nullptr), comp(other.comp), node_alloc(other.node_alloc), node_count(0) 
    {
        std::function<void(Node*)> copyHelper = [&](Node* node) 
        {
            if (node) 
            {
                insertNode(node->data);
                copyHelper(node->left);
                copyHelper(node->right);
            }
        };

        copyHelper(other.root);
    }

    RedBlackTree& operator=(const RedBlackTree& other) 
    {
        if (this != &other) 
        {
            clear();
            comp = other.comp;
            node_alloc = other.node_alloc;
            std::function<void(Node*)> copyHelper = [&](Node* node) 
            {
                if (node) 
                {
                    insertNode(node->data);
                    copyHelper(node->left);
                    copyHelper(node->right);
                }
            };

            copyHelper(other.root);
        }

        return *this;
    }

    void clear() 
    { 
        clearHelper(root); 
        root = nullptr;
        node_count = 0;
    }

    template <typename K>
    std::enable_if_t<std::is_convertible<K, Key>::value || is_transparent_helper<Compare>::value, Node*>
    find(const K& key) const
    {
        Node* current = root;
        while (current) 
        {
            if (comp(key, current->data.first))
                current = current->left;
            else if (comp(current->data.first, key))
                current = current->right;
            else
                return current;
        }

        return nullptr;
    }

    void insertNode(const std::pair<const Key, T>& val)
    {
        Node* newNode = createNode(val);
        newNode->color = RED;
        Node* y = nullptr;
        Node* x = root;
        while (x) 
        {
            y = x;
            if (comp(newNode->data.first, x->data.first))
                x = x->left;
            else
                x = x->right;
        }
        newNode->parent = y;
        if (!y)
            root = newNode;
        else if (comp(newNode->data.first, y->data.first))
            y->left = newNode;
        else
            y->right = newNode;

        fixInsert(newNode);
        node_count++;
    }

    void removeNode(const Key& key)
    {
        Node* z = find(key);
        if (!z) 
        {
            std::cerr << "Node with key " << key << " not found in the tree." << std::endl;
            return;
        }

        deleteNode(z);
        node_count--;
    }

    std::size_t TreeSize() const { return node_count; }

    Node* minNode() const { return minimum(root); }

    Node* maxNode() const { return maximum(root); }

    static Node* successor(Node* node)
    {
        if (!node) return nullptr;
        if (node->right)
            return minimum(node->right);
        Node* p = node->parent;

        while (p && node == p->right) 
        {
            node = p;
            p = p->parent;
        }

        return p;
    }

    static Node* predecessor(Node* node)
    {
        if (!node) return nullptr;
        if (node->left)
            return maximum(node->left);

        Node* p = node->parent;
        while (p && node == p->left) 
        {
            node = p;
            p = p->parent;
        }

        return p;
    }

    Node* getRoot() const { return root; }

    bool validate()
    {
        std::function<bool(Node*, int, int&)> validateHelper =
            [&](Node* node, int blackCount, int& pathBlackCount) -> bool 
        {
            if (!node) 
            {
                if (pathBlackCount == -1)
                    pathBlackCount = blackCount;
                else if (blackCount != pathBlackCount)
                    return false;

                return true;
            }
            if (node->color == BLACK)
                blackCount++;
            else if (node->parent && node->parent->color == RED)
                return false;

            return validateHelper(node->left, blackCount, pathBlackCount) &&
                   validateHelper(node->right, blackCount, pathBlackCount);
        };

        int pathBlackCount = -1;
        return validateHelper(root, 0, pathBlackCount);
    }

private:
    void deleteNode(Node* z)
    {
        if (!z)
            return;
        Node* y = z;
        Node* x = nullptr;
        Color y_original_color = y->color;

        if (!z->left) 
        {
            x = z->right;
            transplant(z, z->right);
        }
        else if (!z->right) 
        {
            x = z->left;
            transplant(z, z->left);
        }
        else 
        {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;
            if (y->parent == z) 
            {
                if (x)
                    x->parent = y;
            } 
            else 
            {
                transplant(y, y->right);
                y->right = z->right;
                if (y->right)
                    y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            if (y->left)
                y->left->parent = y;
            y->color = z->color;
        }

        std::allocator_traits<NodeAllocator>::destroy(node_alloc, z);
        node_alloc.deallocate(z, 1);
        if (y_original_color == BLACK && x)
            fixDelete(x);
    }
};

#undef FORCE_INLINE

#endif // REDBLACKTREE_HPP
