#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>

struct Node {
    std::string key;
    std::string value;
    Node* prev;
    Node* next;

    Node(const std::string& k, const std::string& v)
        : key(k), value(v), prev(nullptr), next(nullptr) {}
};

class LRUCache {
public:
    explicit LRUCache(size_t capacity);
    ~LRUCache();

    bool get(const std::string& key, std::string& value);
    void set(const std::string& key, const std::string& value);
    bool del(const std::string& key);
    size_t size() const;

private:
    size_t capacity_;
    std::unordered_map<std::string, Node*> map_;
    Node* head_;
    Node* tail_;

    mutable std::mutex mutex_;

    void addToFront(Node* node);
    void removeNode(Node* node);
    void moveToFront(Node* node);
    void evictIfNeeded();
};

#endif