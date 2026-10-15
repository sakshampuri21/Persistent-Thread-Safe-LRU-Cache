#ifndef WRITE_AHEAD_LOG_H
#define WRITE_AHEAD_LOG_H

#include <cstdio>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <io.h>        
#else
#include <unistd.h>    
#endif

#include "lru_cache.h"

class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path)
        : path_(path) {
        file_ = std::fopen(path_.c_str(), "a");   
    }

    ~WriteAheadLog() {
        if (file_)
            std::fclose(file_);
    }

    void logSet(const std::string& key, const std::string& value,
                long long expiryMs = 0) {
        std::fprintf(file_, "SET %s %s %lld\n",
                     key.c_str(), value.c_str(), expiryMs);
        writeToDisk();
    }

    void logDel(const std::string& key) {
        std::fprintf(file_, "DEL %s\n", key.c_str());
        writeToDisk();
    }

    void replay(LRUCache& cache) {
        std::ifstream in(path_);

        if (!in.is_open())
            return;

        std::string cmd;

        while (in >> cmd) {
            if (cmd == "SET") {
                std::string key;
                std::string value;
                long long expiryMs = 0;

                in >> key >> value >> expiryMs;

                if (expiryMs != 0 && expiryMs <= nowMs())
                    cache.del(key);
                else
                    cache.set(key, value, expiryMs);

            } else if (cmd == "DEL") {
                std::string key;

                in >> key;
                cache.del(key);
            }
        }
    }

private:
    std::string path_;
    FILE* file_;

    // fflush moves data to the OS, fsync forces the OS to write to disk.
    void writeToDisk() {
        std::fflush(file_);
#ifdef _WIN32
        _commit(_fileno(file_));
#else
        fsync(fileno(file_));
#endif
    }
};

#endif