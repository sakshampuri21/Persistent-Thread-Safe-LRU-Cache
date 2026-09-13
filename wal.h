#ifndef WRITE_AHEAD_LOG_H
#define WRITE_AHEAD_LOG_H

#include <string>
#include <fstream>
#include "lru_cache.h"

// A minimal write-ahead log.
//
// The idea: memory is fast but volatile — if the process crashes, everything
// in the LRUCache is gone. The WAL fixes that by appending every write
// (SET/DEL) to a plain text file BEFORE it's applied in memory. On the next
// startup, we simply read that file line by line and replay each command
// into a fresh LRUCache, rebuilding exactly the state we had before the
// crash.
//
// This is intentionally simple: one line per write, flushed immediately.
// A production WAL would add things like checksums, binary encoding, and
// periodic compaction — we're demonstrating the core idea, not shipping
// Postgres.
class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path) : path_(path) {
        // Open in append mode: every write goes to the END of the existing
        // file, never overwriting history from before this run.
        out_.open(path_, std::ios::app);
    }

    // Call this BEFORE applying a SET to the cache.
    void logSet(const std::string& key, const std::string& value, long ttl_seconds) {
        out_ << "SET " << key << " " << value << " " << ttl_seconds << "\n";
        out_.flush(); // ensure it's actually on disk before we touch memory
    }

    // Call this BEFORE applying a DEL to the cache.
    void logDel(const std::string& key) {
        out_ << "DEL " << key << "\n";
        out_.flush();
    }

    // Call this ONCE at startup, before accepting new commands. Reads the
    // log file from the beginning and replays every write into `cache`,
    // reconstructing whatever state existed right before the last shutdown
    // or crash.
    void replay(LRUCache& cache) {
        std::ifstream in(path_);
        if (!in.is_open()) return; // no log yet — fresh start, nothing to replay

        std::string cmd;
        while (in >> cmd) {
            if (cmd == "SET") {
                std::string key, value;
                long ttl;
                in >> key >> value >> ttl;
                cache.set(key, value, ttl);
            } else if (cmd == "DEL") {
                std::string key;
                in >> key;
                cache.del(key);
            }
        }
    }

private:
    std::string path_;
    std::ofstream out_;
};

#endif