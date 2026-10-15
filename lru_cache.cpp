#include "lru_cache.h"

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity) {
    head_ = new Node("", "");
    tail_ = new Node("", "");

    head_->next = tail_;
    tail_->prev = head_;
}

LRUCache::~LRUCache() {
    Node* curr = head_;

    while (curr) {
        Node* next = curr->next;
        delete curr;
        curr = next;
    }
}

void LRUCache::addToFront(Node* node) {
    node->next = head_->next;
    node->prev = head_;

    head_->next->prev = node;
    head_->next = node;
}

void LRUCache::removeNode(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void LRUCache::moveToFront(Node* node) {
    removeNode(node);
    addToFront(node);
}

void LRUCache::evictIfNeeded() {
    if (map_.size() <= capacity_)
        return;

    Node* lru = tail_->prev;

    removeNode(lru);
    map_.erase(lru->key);
    delete lru;
}

bool LRUCache::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);

    if (it == map_.end())
        return false;

    Node* node = it->second;

    // NEW: lazy TTL check. If the key has expired, delete it and say "not found".
    if (node->expiryMs != 0 && nowMs() >= node->expiryMs) {
        removeNode(node);
        map_.erase(it);
        delete node;
        return false;
    }

    moveToFront(node);
    value = node->value;

    return true;
}

void LRUCache::set(const std::string& key, const std::string& value,
                   long long expiryMs) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);

    if (it != map_.end()) {
        Node* node = it->second;

        node->value = value;
        node->expiryMs = expiryMs;   // NEW: update expiry too
        moveToFront(node);

        return;
    }

    Node* node = new Node(key, value, expiryMs);   // NEW: pass expiry

    map_[key] = node;
    addToFront(node);
    evictIfNeeded();
}

bool LRUCache::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);

    if (it == map_.end())
        return false;

    Node* node = it->second;

    removeNode(node);
    map_.erase(it);
    delete node;

    return true;
}

size_t LRUCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return map_.size();
}