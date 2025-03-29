#include <iostream>
#include "include/map.hpp"

void print_map(const mystl::map<int, std::string>& m, const std::string& label = "map") {
    std::cout << label << " contents:\n";
    for (const auto& [key, value] : m) {
        std::cout << "  [" << key << "] = " << value << '\n';
    }
}

int main() {
    mystl::map<int, std::string> m;

    // insert, operator[]
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";
    m.insert({4, "four"});
    m.insert(std::make_pair(5, "five"));

    print_map(m, "After insertions");

    // at
    try {
        std::cout << "m.at(3) = " << m.at(3) << '\n';
        std::cout << "m.at(100) = " << m.at(100) << '\n'; // должно выбросить исключение
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << '\n';
    }

    // find, count
    auto it = m.find(2);
    if (it != m.end())
        std::cout << "Found key 2 with value: " << it->second << '\n';

    std::cout << "Count of key 3: " << m.count(3) << '\n';
    std::cout << "Count of key 99: " << m.count(99) << '\n';

    // erase by key
    m.erase(2);
    print_map(m, "After erase(2)");

    // erase by iterator
    auto it2 = m.find(4);
    if (it2 != m.end()) m.erase(it2);
    print_map(m, "After erase(iterator to 4)");

    // clear
    m.clear();
    std::cout << "After clear: size = " << m.size() << ", empty = " << std::boolalpha << m.empty() << '\n';

    // initializer list
    m = {
        {10, "ten"},
        {20, "twenty"},
        {30, "thirty"}
    };
    print_map(m, "After initializer list");

    // lower_bound / upper_bound
    auto lb = m.lower_bound(15);
    if (lb != m.end())
        std::cout << "lower_bound(15): key = " << lb->first << ", value = " << lb->second << '\n';

    auto ub = m.upper_bound(20);
    if (ub != m.end())
        std::cout << "upper_bound(20): key = " << ub->first << ", value = " << ub->second << '\n';

    // equal_range
    auto range = m.equal_range(20);
    std::cout << "equal_range(20):\n";
    if (range.first != m.end())
        std::cout << "  first: " << range.first->first << " -> " << range.first->second << '\n';
    if (range.second != m.end())
        std::cout << "  second: " << range.second->first << " -> " << range.second->second << '\n';

    // copy constructor
    mystl::map<int, std::string> copy = m;
    print_map(copy, "Copied map");

    // move constructor
    mystl::map<int, std::string> moved = std::move(copy);
    print_map(moved, "Moved map");
    std::cout << "After move: original size = " << copy.size() << ", empty = " << copy.empty() << '\n';

    // comparison
    mystl::map<int, std::string> m1 = {{1, "a"}, {2, "b"}};
    mystl::map<int, std::string> m2 = {{1, "a"}, {2, "b"}};
    mystl::map<int, std::string> m3 = {{1, "a"}, {3, "c"}};

    std::cout << "m1 == m2: " << (m1 == m2) << '\n';
    std::cout << "m1 != m3: " << (m1 != m3) << '\n';
    std::cout << "m1 < m3 : " << (m1 < m3) << '\n';

    return 0;
}
