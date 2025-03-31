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

// True - если если есть вложенные тип is_transparent
template <typename C>
struct is_transparent_helper<C, std::void_t<typename C::is_transparent>> : std::true_type {};

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class RedBlackTree 
{
public:
    struct Node;

    // Это функтор, который будет вызван при уничтожении std::unique_ptr<Node, NodeDeleter>
    struct NodeDeleter 
    {
        // Указатель на аллокатор (в виде, переопределённом для узлов)
        typename std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<Node>>::pointer allocPtr;
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>* alloc;

        NodeDeleter(typename std::allocator_traits<Allocator>::template rebind_alloc<Node>* a = nullptr)
            : alloc(a) {}

        void operator()(Node* p) const 
        {
            if (p) 
            {
                // вызова деструктора объекта Node
                std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<Node>>::destroy(*alloc, p);
                alloc->deallocate(p, 1);
            }
        }
    };

    struct Node 
    {
        std::pair<const Key, T> data;
        Color color;
        std::unique_ptr<Node, NodeDeleter> left;
        std::unique_ptr<Node, NodeDeleter> right;
        Node* parent;

        explicit Node(const std::pair<const Key, T>& val)
            : data(val), color(RED),
              left(nullptr, NodeDeleter(nullptr)),
              right(nullptr, NodeDeleter(nullptr)),
              parent(nullptr) {}
    };

private:
    std::unique_ptr<Node, NodeDeleter> root;
    Compare comp;

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    NodeAllocator node_alloc;

    NodeDeleter makeDeleter() { return NodeDeleter(&node_alloc); }

// Небольшая хитрость для различных компиляторов (MSVC или GCC/Clang), чтобы заставить функции агрессивно инлайниться
#if defined(_MSC_VER)
  #define FORCE_INLINE __forceinline
#else
  #define FORCE_INLINE inline __attribute__((always_inline))
#endif

    // Возвращает ссылку на тот unique_ptr, который у родительского узла (или у самого root) ссылается на x
    FORCE_INLINE std::unique_ptr<Node, NodeDeleter>* getLink(Node* x) 
    {
        if (!x->parent)
            return &root;
        if (x == x->parent->left.get())
            return &x->parent->left;
        else
            return &x->parent->right;
    }

    FORCE_INLINE void leftRotate(Node* x) 
    {
        if (!x || !x->right)
            return;
        auto xLink = getLink(x);
        std::unique_ptr<Node, NodeDeleter> y = std::move(x->right);

        if (y->left)
            y->left->parent = x;
        x->right = std::move(y->left);
        y->parent = x->parent;
        y->left = std::move(*xLink);
        y->left->parent = y.get();
        *xLink = std::move(y);
    }

    FORCE_INLINE void rightRotate(Node* y) 
    {
        if (!y || !y->left)
            return;
        auto yLink = getLink(y);
        std::unique_ptr<Node, NodeDeleter> x = std::move(y->left);

        if (x->right)
            x->right->parent = y;
        y->left = std::move(x->right);
        x->parent = y->parent;
        x->right = std::move(*yLink);
        x->right->parent = x.get();
        *yLink = std::move(x);
    }

    void fixInsert(Node* z) 
    {
        while (z != root.get() && z->parent->color == RED) 
        {
            if (z->parent == z->parent->parent->left.get()) 
            {
                Node* y = z->parent->parent->right.get();
                if (y && y->color == RED) 
                {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } 
                else 
                {
                    if (z == z->parent->right.get()) 
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
                Node* y = z->parent->parent->left.get();

                if (y && y->color == RED) 
                {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } 
                else 
                {
                    if (z == z->parent->left.get()) 
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
        assert(root);
        root->color = BLACK;
    }

    void transplant(Node* u, std::unique_ptr<Node, NodeDeleter>& v) 
    {
        auto uLink = getLink(u);
        if (v)
            v->parent = u->parent;
        *uLink = std::move(v);
    }

    void fixDelete(Node* x) 
    {
        while (x != root.get() && (!x || x->color == BLACK)) 
        {
            if (x == x->parent->left.get()) 
            {
                Node* w = x->parent->right.get();
                if (w && w->color == RED) 
                {
                    w->color = BLACK;
                    x->parent->color = RED;
                    leftRotate(x->parent);
                    w = x->parent->right.get();
                }
                if ((!w->left || w->left->color == BLACK) &&
                    (!w->right || w->right->color == BLACK)) 
                {
                    w->color = RED;
                    x = x->parent;
                } 
                else 
                {
                    if (!w->right || w->right->color == BLACK) 
                    {
                        if (w->left)
                            w->left->color = BLACK;

                        w->color = RED;
                        rightRotate(w);
                        w = x->parent->right.get();
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->right)
                        w->right->color = BLACK;

                    leftRotate(x->parent);
                    x = root.get();
                }
            } 
            else 
            {
                Node* w = x->parent->left.get();
                if (w && w->color == RED) 
                {
                    w->color = BLACK;
                    x->parent->color = RED;
                    rightRotate(x->parent);
                    w = x->parent->left.get();
                }
                if ((!w->right || w->right->color == BLACK) &&
                    (!w->left || w->left->color == BLACK)) 
                {
                    w->color = RED;
                    x = x->parent;
                } 
                else 
                {
                    if (!w->left || w->left->color == BLACK) 
                    {
                        if (w->right)
                            w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = x->parent->left.get();
                    }

                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->left)
                        w->left->color = BLACK;

                    rightRotate(x->parent);
                    x = root.get();
                }
            }
        }

        if (x)
            x->color = BLACK;
    }

    static Node* minimum(Node* node) 
    {
        while (node && node->left)
            node = node->left.get();

        return node;
    }

    static Node* maximum(Node* node) 
    {
        while (node && node->right)
            node = node->right.get();

        return node;
    }

    void clearHelper(std::unique_ptr<Node, NodeDeleter>& nodePtr) 
    {
        if (nodePtr) 
        {
            clearHelper(nodePtr->left);
            clearHelper(nodePtr->right);

            nodePtr.reset();
        }
    }

    Node* createNode(const std::pair<const Key, T>& val) 
    {
        Node* p = node_alloc.allocate(1);
        try {
            std::allocator_traits<NodeAllocator>::construct(node_alloc, p, val);
            p->left = std::unique_ptr<Node, NodeDeleter>(nullptr, makeDeleter());
            p->right = std::unique_ptr<Node, NodeDeleter>(nullptr, makeDeleter());
        } catch (...) {
            node_alloc.deallocate(p, 1);
            throw;
        }

        return p;
    }

public:
    std::size_t node_count = 0;

    RedBlackTree()
        : root(nullptr, makeDeleter()), comp(Compare()), node_alloc(NodeAllocator()) {}

    RedBlackTree(const Compare& comp, const Allocator& alloc)
        : root(nullptr, makeDeleter()), comp(comp), node_alloc(alloc) {}

    ~RedBlackTree() { clear(); }

    RedBlackTree(const RedBlackTree& other)
        : root(nullptr, makeDeleter()), comp(other.comp), node_alloc(other.node_alloc), node_count(0) 
    {
        std::function<void(Node*)> copyHelper = [&](Node* node) 
        {
            if (node) 
            {
                insertNode(node->data);
                if (node->left)
                    copyHelper(node->left.get());
                if (node->right)
                    copyHelper(node->right.get());
            }
        };

        copyHelper(other.root.get());
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

                    if (node->left)
                        copyHelper(node->left.get());
                    if (node->right)
                        copyHelper(node->right.get());
                }
            };

            copyHelper(other.root.get());
        }

        return *this;
    }

    void clear() { clearHelper(root); node_count = 0; }

    template <typename K>
    std::enable_if_t<std::is_convertible<K, Key>::value || is_transparent_helper<Compare>::value, Node*>
    find(const K& key) const
    {
        Node* current = root.get();

        while (current) 
        {
            if (comp(key, current->data.first))
                current = current->left.get();
            else if (comp(current->data.first, key))
                current = current->right.get();
            else
                return current;
        }

        return nullptr;
    }

    void insertNode(const std::pair<const Key, T>& val)
    {
        std::unique_ptr<Node, NodeDeleter> newNode(createNode(val), makeDeleter());
        Node* y = nullptr;
        Node* x = root.get();
        while (x) 
        {
            y = x;
            if (comp(newNode->data.first, x->data.first))
                x = x->left.get();
            else
                x = x->right.get();
        }
        newNode->parent = y;
        if (!y)
            root = std::move(newNode);
        else if (comp(newNode->data.first, y->data.first))
            y->left = std::move(newNode);
        else
            y->right = std::move(newNode);

        Node* inserted = find(val.first);
        fixInsert(inserted);

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

        this->deleteNode(z);
        node_count--;
    }

    std::size_t TreeSize() const { return node_count; }

    Node* minNode() const { return minimum(root.get()); }

    Node* maxNode() const
    {
        Node* current = root.get();
        if (!current)
            return nullptr;
        return maximum(current);
    }

    static Node* successor(Node* node)
    {
        if (!node) return nullptr;
        if (node->right)
            return minimum(node->right.get());
        Node* p = node->parent;

        while (p && node == p->right.get()) 
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
            return maximum(node->left.get());

        Node* p = node->parent;
        while (p && node == p->left.get()) 
        {
            node = p;
            p = p->parent;
        }

        return p;
    }

    Node* getRoot() const { return root.get(); }

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

                return validateHelper(node->left.get(), blackCount, pathBlackCount) &&
                       validateHelper(node->right.get(), blackCount, pathBlackCount);
            };

        int pathBlackCount = -1;
        return validateHelper(root.get(), 0, pathBlackCount);
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
            x = z->right.get();
            transplant(z, z->right);
        }
        else if (!z->right) 
        {
            x = z->left.get();
            transplant(z, z->left);
        }
        else 
        {
            y = minimum(z->right.get());
            y_original_color = y->color;
            x = y->right.get();
            if (y->parent == z) 
            {
                if (x)
                    x->parent = y;
            } 
            else 
            {
                if (x)
                    x->parent = y->parent;
                transplant(y, y->right);
                y->right = std::move(z->right);
                if (y->right)
                    y->right->parent = y;
            }

            transplant(z, *getLink(z));
            y->left = std::move(z->left);

            if (y->left)
                y->left->parent = y;
            y->color = z->color;
        }

        if (y_original_color == BLACK && x)
            fixDelete(x);
    }
};

#undef FORCE_INLINE

#endif // REDBLACKTREE_HPP
