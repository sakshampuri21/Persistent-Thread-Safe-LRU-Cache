#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "lru_cache.h"
#include "wal.h"

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

void printHelp() {
    std::cout <<
        "Commands:\n"
        "  SET key value [ttl_seconds]\n"
        "  GET key\n"
        "  DEL key\n"
        "  TTL key\n"
        "  SIZE\n"
        "  HELP\n"
        "  EXIT\n";
}

int main() {
    size_t capacity;
    std::cout << "Enter cache capacity: ";
    std::cin >> capacity;
    std::cin.ignore();

    LRUCache cache(capacity);

    // --- WAL wiring, step 1: replay history BEFORE accepting new commands ---
    WriteAheadLog wal("kvstore.wal");
    wal.replay(cache);

    std::cout << "KV store ready. Capacity = " << capacity
              << ". Recovered " << cache.size() << " key(s) from log. Type HELP for commands.\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        auto tokens = tokenize(line);
        std::string cmd = tokens[0];
        for (auto& c : cmd) c = toupper(c);

        if (cmd == "EXIT") {
            break;
        } else if (cmd == "HELP") {
            printHelp();
        } else if (cmd == "SET") {
            if (tokens.size() < 3) {
                std::cout << "Usage: SET key value [ttl_seconds]\n";
                continue;
            }
            long ttl = tokens.size() >= 4 ? std::stol(tokens[3]) : 0;

            // --- WAL wiring, step 2: log BEFORE applying the write ---
            wal.logSet(tokens[1], tokens[2], ttl);
            cache.set(tokens[1], tokens[2], ttl);

            std::cout << "OK\n";
        } else if (cmd == "GET") {
            if (tokens.size() < 2) {
                std::cout << "Usage: GET key\n";
                continue;
            }
            std::string value;
            if (cache.get(tokens[1], value)) {
                std::cout << value << "\n";
            } else {
                std::cout << "(nil)\n";
            }
        } else if (cmd == "DEL") {
            if (tokens.size() < 2) {
                std::cout << "Usage: DEL key\n";
                continue;
            }
            // --- WAL wiring, step 3: log BEFORE applying the delete ---
            wal.logDel(tokens[1]);
            std::cout << (cache.del(tokens[1]) ? "1\n" : "0\n");
        } else if (cmd == "TTL") {
            if (tokens.size() < 2) {
                std::cout << "Usage: TTL key\n";
                continue;
            }
            std::cout << cache.ttl(tokens[1]) << "\n";
        } else if (cmd == "SIZE") {
            std::cout << cache.size() << "\n";
        } else {
            std::cout << "Unknown command. Type HELP.\n";
        }
    }
    return 0;
}