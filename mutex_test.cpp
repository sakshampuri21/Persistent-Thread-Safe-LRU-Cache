#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include "lru_cache.h"

void worker(LRUCache& cache, int id) {
    for (int i = 0; i < 10000; i++) {

        // Each thread uses different keys
        std::string key =
            "thread" + std::to_string(id) +
            "_key" + std::to_string(i);

        // Write
        cache.set(key, "value");

        // Read
        std::string value;
        cache.get(key, value);

        // Delete half of the keys
        if (i % 2 == 0) {
            cache.del(key);
        }
    }
}

int main() {

    LRUCache cache(100000);

    const int NUM_THREADS = 10;

    std::vector<std::thread> threads;

    // Create 10 threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker, std::ref(cache), i);
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All threads completed successfully.\n";
    std::cout << "Final cache size: "
              << cache.size() << "\n";

    return 0;
}