#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>

int main() {
    const int n = 1000000;
    std::vector<int> vec;
    std::unordered_map<int, int> hashmap;
    int arr[n];
 
    // Measure time for std::vector::push_back
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
        vec.push_back(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "std::vector::push_back: " << diff.count() << " s\n";

    // Measure time for array insertion
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
        arr[i] = i;
    }
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Array insertion: " << diff.count() << " s\n";

    // Measure time for std::unordered_map insertion
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
        hashmap[i] = i;
    }
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "std::unordered_map insertion: " << diff.count() << " s\n";

    // Measure time for std::vector::clear
    start = std::chrono::high_resolution_clock::now();
    vec.clear();
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "std::vector::clear: " << diff.count() << " s\n";

    return 0;
}
