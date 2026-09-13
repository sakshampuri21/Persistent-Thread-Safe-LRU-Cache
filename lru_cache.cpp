#include "lru_cache.h"

LRUCache::LRUCache(size_t capacity) : capacity_(capacity) {
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

bool LRUCache::isExpired(Node* node) const {
    if (!node->has_ttl) return false;
    return std::chrono::steady_clock::now() >= node->expiry;
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
    if (map_.size() <= capacity_) return;
    Node* lru = tail_->prev;
    removeNode(lru);
    map_.erase(lru->key);
    delete lru;
}

bool LRUCache::get(const std::string& key, std::string& value) {
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    Node* node = it->second;
    if (isExpired(node)) {
        removeNode(node);
        map_.erase(it);
        delete node;
        return false;
    }
    moveToFront(node);
    value = node->value;
    return true;
}

void LRUCache::set(const std::string& key, const std::string& value, long ttl_seconds) {
    auto it = map_.find(key);
    if (it != map_.end()) {
        Node* node = it->second;
        node->value = value;
        if (ttl_seconds > 0) {
            node->has_ttl = true;
            node->expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
        } else {
            node->has_ttl = false;
        }
        moveToFront(node);
        return;
    }
    Node* node = new Node(key, value);
    if (ttl_seconds > 0) {
        node->has_ttl = true;
        node->expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
    }
    map_[key] = node;
    addToFront(node);
    evictIfNeeded();
}

bool LRUCache::del(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    Node* node = it->second;
    removeNode(node);
    map_.erase(it);
    delete node;
    return true;
}

long LRUCache::ttl(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return -1;
    Node* node = it->second;
    if (!node->has_ttl) return -1;
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        node->expiry - std::chrono::steady_clock::now()
    ).count();
    return remaining > 0 ? remaining : 0;
}