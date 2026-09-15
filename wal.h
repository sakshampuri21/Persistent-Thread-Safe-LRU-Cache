#ifndef WRITE_AHEAD_LOG_H
#define WRITE_AHEAD_LOG_H

#include <string>
#include <fstream>
#include "lru_cache.h"

class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path)
        : path_(path) {
        out_.open(path_, std::ios::app);
    }

    void logSet(const std::string& key, const std::string& value) {
        out_ << "SET " << key << " " << value << "\n";
        out_.flush();
    }

    void logDel(const std::string& key) {
        out_ << "DEL " << key << "\n";
        out_.flush();
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