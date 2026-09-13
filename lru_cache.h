#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <string>
#include <unordered_map>
#include <chrono>

struct Node {
    std::string key;
    std::string value;
    std::chrono::steady_clock::time_point expiry;
    bool has_ttl;
    Node* prev;
    Node* next;
    Node(const std::string& k, const std::string& v)
        : key(k), value(v), has_ttl(false), prev(nullptr), next(nullptr) {}
};

class LRUCache {
public:
    explicit LRUCache(size_t capacity);
    ~LRUCache();
    bool get(const std::string& key, std::string& value);
    void set(const std::string& key, const std::string& value, long ttl_seconds = 0);
    bool del(const std::string& key);
    long ttl(const std::string& key);
    size_t size() const { return map_.size(); }

private:
    size_t capacity_;
    std::unordered_map<std::string, Node*> map_;
    Node* head_;
    Node* tail_;
    void addToFront(Node* node);
    void removeNode(Node* node);
    void moveToFront(Node* node);
    void evictIfNeeded();
    bool isExpired(Node* node) const;
};

#endif