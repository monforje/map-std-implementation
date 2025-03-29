#ifndef map_HPP
#define map_HPP

#include "red-black-tree.hpp"
#include <functional>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include <type_traits>

/**
 * EBO (Empty Base Optimization) – приём, позволяющий хранить объекты пустых типов,
 * не занимая дополнительного места в памяти, путём наследования от них.
 */
template <typename T, bool UseEBO = (std::is_empty<T>::value && !std::is_final<T>::value)>
struct EBO;

/**
 * Специализация для пустого типа T
 */
template <typename T>
struct EBO<T, true> : private T 
{
    EBO(const T& t = T()) : T(t) {}
    T& get() { return *this; }
    const T& get() const { return *this; }
};

/**
 * Специализация для случаев, когда T не пустой
 * В этом случае просто храним значение в поле.
 */
template <typename T>
struct EBO<T, false> 
{
    T value;
    EBO(const T& t = T()) : value(t) {}
    T& get() { return value; }
    const T& get() const { return value; }
};

namespace mystl {
    template <typename Key, typename T,
              typename Compare = std::less<Key>,
              typename Allocator = std::allocator<std::pair<const Key, T>>>
    class map : private EBO<Compare>,
                private EBO<Allocator>
    {
    private:
        // Вспомогательные функции для доступа к компаратору и аллокатору,
        // учитывая, что они хранятся в EBO<>.
        const Compare& get_compare() const { return static_cast<const EBO<Compare>&>(*this).get(); }
        Compare& get_compare() { return static_cast<EBO<Compare>&>(*this).get(); }

        const Allocator& get_allocator() const { return static_cast<const EBO<Allocator>&>(*this).get(); }
        Allocator& get_allocator() { return static_cast<EBO<Allocator>&>(*this).get(); }

        RedBlackTree<Key, T, Compare, Allocator> tree;

        RedBlackTree<Key, T, Compare, Allocator> init_tree() 
        {
            return RedBlackTree<Key, T, Compare, Allocator>(get_compare(), get_allocator());
        }

    public:
        using value_type      = std::pair<const Key, T>;
        using key_type        = Key;
        using mapped_type     = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using key_compare     = Compare;
        using allocator_type  = Allocator;

        using pointer         = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer   = typename std::allocator_traits<Allocator>::const_pointer;

        using node_type       = typename RedBlackTree<Key, T, Compare, Allocator>::Node;

        using reference       = value_type&;
        using const_reference = const value_type&;

        class iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type        = std::pair<const Key, T>;
            using difference_type   = std::ptrdiff_t;
            using pointer           = value_type*;
            using reference         = value_type&;

            node_type* node;

            explicit iterator(node_type* n = nullptr) : node(n) {}

            reference operator*() const { return node->data; }
            pointer operator->() const { return &(node->data); }

            iterator& operator++() 
            {
                node = RedBlackTree<Key, T, Compare, Allocator>::successor(node);
                return *this;
            }

            iterator operator++(int) 
            {
                iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            iterator& operator--() 
            {
                node = RedBlackTree<Key, T, Compare, Allocator>::predecessor(node);
                return *this;
            }

            iterator operator--(int) 
            {
                iterator tmp(*this);
                --(*this);
                return tmp;
            }

            bool operator==(const iterator& other) const { return node == other.node; }
            bool operator!=(const iterator& other) const { return node != other.node; }
        };

        /**
         * Константный итератор, аналогичен iterator, но не позволяет изменять данные.
         */
        class const_iterator 
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type        = const std::pair<const Key, T>;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const value_type*;
            using reference         = const value_type&;

            node_type* node;

            explicit const_iterator(node_type* n = nullptr) : node(n) {}

            const_iterator(const iterator& it) : node(it.node) {}

            reference operator*() const { return node->data; }
            pointer operator->() const { return &(node->data); }

            const_iterator& operator++() 
            {
                node = RedBlackTree<Key, T, Compare, Allocator>::successor(node);
                return *this;
            }
            const_iterator operator++(int) 
            {
                const_iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            const_iterator& operator--() 
            {
                node = RedBlackTree<Key, T, Compare, Allocator>::predecessor(node);
                return *this;
            }
            const_iterator operator--(int) 
            {
                const_iterator tmp(*this);
                --(*this);
                return tmp;
            }

            bool operator==(const const_iterator& other) const { return node == other.node; }
            bool operator!=(const const_iterator& other) const { return node != other.node; }
        };

        using reverse_iterator         = std::reverse_iterator<iterator>;
        using const_reverse_iterator   = std::reverse_iterator<const_iterator>;

        map()
            : EBO<Compare>(), EBO<Allocator>(), tree(init_tree()) {}

        explicit map(const Allocator& alloc)
            : EBO<Compare>(), EBO<Allocator>(alloc), tree(init_tree()) {}

        explicit map(const Compare& comp)
            : EBO<Compare>(comp), EBO<Allocator>(), tree(init_tree()) {}

        map(const Compare& comp, const Allocator& alloc)
            : EBO<Compare>(comp), EBO<Allocator>(alloc), tree(init_tree()) {}

        map(std::initializer_list<value_type> init,
            const Compare& comp = Compare(),
            const Allocator& alloc = Allocator())
            : EBO<Compare>(comp), EBO<Allocator>(alloc), tree(init_tree())
        {
            for (const auto& elem : init)
                insert(elem);
        }

        ~map() = default;

        map(const map& other)
            : EBO<Compare>(other.get_compare()),
              EBO<Allocator>(other.get_allocator()),
              tree(other.tree)
        {}

        map& operator=(const map& other) 
        {
            if (this != &other) 
            {
                clear();
                for (auto it = other.begin(); it != other.end(); ++it)
                    insert(*it);
            }

            return *this;
        }

        map(map&& other) noexcept
            : EBO<Compare>(std::move(static_cast<EBO<Compare>&>(other).get())),
              EBO<Allocator>(std::move(static_cast<EBO<Allocator>&>(other).get())),
              tree(std::move(other.tree)) {}

        map& operator=(map&& other) noexcept 
        {
            if (this != &other) 
            {
                clear();
                tree = std::move(other.tree);
            }

            return *this;
        }

        // -- ИТЕРАТОРЫ --

        iterator begin() { return iterator(tree.minNode()); }
        iterator end()   { return iterator(nullptr); }

        const_iterator begin() const { return const_iterator(tree.minNode()); }
        const_iterator end() const   { return const_iterator(nullptr); }

        const_iterator cbegin() const { return const_iterator(tree.minNode()); }
        const_iterator cend() const   { return const_iterator(nullptr); }

        reverse_iterator rbegin() { return reverse_iterator(end()); }
        reverse_iterator rend()   { return reverse_iterator(begin()); }

        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const   { return const_reverse_iterator(begin()); }

        const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const   { return const_reverse_iterator(cbegin()); }

        // -- ЕМКОСТЬ --

        bool empty() const { return size() == 0; }

        size_type size() const { return tree.TreeSize(); }

        size_type max_size() const { return std::numeric_limits<size_type>::max(); }

        // -- ОПЕРАЦИИ ДОСТУПА --

        mapped_type& operator[](const key_type& key) 
        {
            auto it = find(key);
            if (it == end()) 
            {
                insert(std::make_pair(key, mapped_type()));
                it = find(key);
            }

            return it.node->data.second;
        }

        mapped_type& at(const key_type& key) 
        {
            auto it = find(key);
            if (it == end())
                throw std::out_of_range("Key not found");
            return it.node->data.second;
        }

        const mapped_type& at(const key_type& key) const {
            auto it = find(key);
            if (it == end())
                throw std::out_of_range("Key not found");
            return it.node->data.second;
        }

        void insert(const value_type& value) { tree.insertNode(value); }

        void emplace(const key_type& key, const mapped_type& value) { insert(std::make_pair(key, value)); }

        template <typename InputIt>
        void insert_range(InputIt first, InputIt last) 
        {
            for (auto it = first; it != last; ++it)
                insert(*it);
        }

        void erase(const key_type& key) { tree.removeNode(key); }

        iterator erase(iterator pos) {
            if (pos == end()) return pos;
            iterator next = pos;
            ++next;
            erase(pos->first); // erase по ключу
            return next;
        }

        template <typename Predicate>
        size_type erase_if(Predicate pred)
        {
            size_type count = 0;
            for (auto it = begin(); it != end(); )
            {
                if (pred(*it))
                {
                    auto to_erase = it++;
                    erase(to_erase->first);
                    ++count;
                }
                else
                {
                    ++it;
                }
            }
            return count;
        }


        void clear() { tree.clear(); }

        void insert_or_assign(const key_type& key, const mapped_type& value) 
        {
            auto it = find(key);

            if (it != end())
                it.node->data.second = value;
            else
                insert(std::make_pair(key, value));
        }

        iterator emplace_hint(iterator /*hint*/, const value_type& value) 
        {
            insert(value);
            return find(value.first);
        }

        template <typename... Args>
        std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) 
        {
            auto it = find(key);
            if (it != end())
                return {it, false};

            mapped_type value(std::forward<Args>(args)...);
            insert(std::make_pair(key, value));
            return {find(key), true};
        }

        value_type extract(const key_type& key) 
        {
            auto it = find(key);
            if (it == end())
                throw std::out_of_range("Key not found");

            value_type val = *it;
            erase(key);
            return val;
        }

        std::pair<iterator, iterator> equal_range(const key_type& key) {
            return {lower_bound(key), upper_bound(key)};
        }
        
        std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
            return {lower_bound(key), upper_bound(key)};
        }

        void merge(map& source) 
        {
            for (auto it = source.begin(); it != source.end(); ) 
            {
                auto current = it++;

                if (!contains(current->first)) 
                {
                    insert(*current);
                    source.erase(current->first);
                }
            }
        }

        iterator find(const key_type& key) { return iterator(tree.find(key)); }
        const_iterator find(const key_type& key) const { return const_iterator(tree.find(key)); }

        size_type count(const key_type& key) const { return tree.find(key) ? 1 : 0; }

        bool contains(const key_type& key) const { return tree.find(key) != nullptr; }

        iterator lower_bound(const key_type& key) 
        {
            auto current = tree.getRoot();
            node_type* candidate = nullptr;

            while (current) 
            {
                if (!get_compare()(current->data.first, key)) 
                {
                    candidate = current;
                    current = current->left.get();
                } 
                else 
                {
                    current = current->right.get();
                }
            }

            return iterator(candidate);
        }
        const_iterator lower_bound(const key_type& key) const 
        {
            auto current = tree.getRoot();
            node_type* candidate = nullptr;

            while (current) 
            {
                if (!get_compare()(current->data.first, key)) 
                {
                    candidate = current;
                    current = current->left.get();
                } 
                else 
                {
                    current = current->right.get();
                }
            }

            return const_iterator(candidate);
        }

        iterator upper_bound(const key_type& key) 
        {
            auto current = tree.getRoot();
            node_type* candidate = nullptr;

            while (current) 
            {
                if (get_compare()(key, current->data.first)) 
                {
                    candidate = current;
                    current = current->left.get();
                } 
                else 
                {
                    current = current->right.get();
                }
            }

            return iterator(candidate);
        }
        const_iterator upper_bound(const key_type& key) const 
        {
            auto current = tree.getRoot();
            node_type* candidate = nullptr;

            while (current) 
            {
                if (get_compare()(key, current->data.first)) 
                {
                    candidate = current;
                    current = current->left.get();
                } 
                else 
                {
                    current = current->right.get();
                }
            }

            return const_iterator(candidate);
        }

        key_compare key_comp() const { return get_compare(); }

        struct value_compare 
        {
            value_compare(Compare c) : comp(c) {}

            bool operator()(const value_type& lhs, const value_type& rhs) const 
            {
                return comp(lhs.first, rhs.first);
            }

        private:
            Compare comp;
        };

        value_compare value_comp() const { return value_compare(get_compare()); }

        friend bool operator==(const map& lhs, const map& rhs) 
        {
            if (lhs.size() != rhs.size())
                return false;

            auto it1 = lhs.begin();
            auto it2 = rhs.begin();

            while (it1 != lhs.end()) 
            {
                if (*it1 != *it2)
                    return false;
                ++it1; ++it2;
            }

            return true;
        }

        friend bool operator!=(const map& lhs, const map& rhs) { return !(lhs == rhs); }

        friend bool operator<(const map& lhs, const map& rhs) 
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator<=(const map& lhs, const map& rhs) { return !(rhs < lhs); }

        friend bool operator>(const map& lhs, const map& rhs) { return rhs < lhs; }

        friend bool operator>=(const map& lhs, const map& rhs) { return !(lhs < rhs); }

        void swap(map& other) { std::swap(tree, other.tree); }
    };

} // namespace mystl

#endif // map_HPP