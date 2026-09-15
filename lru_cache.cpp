#include "lru_cache.h"

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity) {

    // Dummy/sentinel nodes
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

// Add a node immediately after head_.
// This makes it the Most Recently Used node.
void LRUCache::addToFront(Node* node) {
    node->next = head_->next;
    node->prev = head_;

    head_->next->prev = node;
    head_->next = node;
}

// Remove a node from the doubly linked list.
void LRUCache::removeNode(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// Move an existing node to the front.
// The front represents the Most Recently Used position.
void LRUCache::moveToFront(Node* node) {
    removeNode(node);
    addToFront(node);
}

// If the cache exceeds its capacity,
// remove the Least Recently Used node.
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

    // Key does not exist
    if (it == map_.end())
        return false;

    Node* node = it->second;

    // Accessing a key makes it Most Recently Used.
    moveToFront(node);

    value = node->value;

    return true;
}

void LRUCache::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);

    // Key already exists
    if (it != map_.end()) {
        Node* node = it->second;

        node->value = value;

        // Updating a key also makes it Most Recently Used.
        moveToFront(node);

        return;
    }

    // Create a new node
    Node* node = new Node(key, value);

    // Add to hash map
    map_[key] = node;

    // Add to front of LRU list
    addToFront(node);

    // Remove LRU item if capacity is exceeded
    evictIfNeeded();
}

bool LRUCache::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);

    // Key does not exist
    if (it == map_.end())
        return false;

    Node* node = it->second;

    // Remove from linked list
    removeNode(node);

    // Remove from hash map
    map_.erase(it);

    // Free memory
    delete node;

    return true;
}

size_t LRUCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return map_.size();
}