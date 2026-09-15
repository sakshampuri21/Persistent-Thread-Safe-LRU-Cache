#ifndef WRITE_AHEAD_LOG_H
#define WRITE_AHEAD_LOG_H

#include <string>
#include <fstream>

#include "lru_cache.h"

// Minimal Write-Ahead Log.
//
// Every modifying operation is written to the WAL before
// being applied to the in-memory cache.
//
// Example:
//     SET name Saksham
//     DEL name
//
// On program restart, the WAL is replayed to reconstruct
// the cache state.
class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path)
        : path_(path) {

        // Open the WAL in append mode.
        // Existing log entries are preserved.
        out_.open(path_, std::ios::app);
    }

    // Write a SET operation to the WAL.
    void logSet(
        const std::string& key,
        const std::string& value
    ) {
        out_ << "SET "
              << key << " "
              << value << "\n";

        // Flush the stream before updating memory.
        out_.flush();
    }

    // Write a DELETE operation to the WAL.
    void logDel(const std::string& key) {
        out_ << "DEL "
              << key << "\n";

        out_.flush();
    }

    // Replay all previous operations into the cache.
    void replay(LRUCache& cache) {

        std::ifstream in(path_);

        // No WAL file means there is nothing to recover.
        if (!in.is_open())
            return;

        std::string cmd;

        while (in >> cmd) {

            if (cmd == "SET") {

                std::string key;
                std::string value;

                in >> key >> value;

                cache.set(key, value);

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