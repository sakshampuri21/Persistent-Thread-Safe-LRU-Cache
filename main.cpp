#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lru_cache.h"
#include "wal.h"

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;

    while (iss >> tok)
        tokens.push_back(tok);

    return tokens;
}

void printHelp() {
    std::cout
        << "Commands:\n"
        << "  SET key value\n"
        << "  SETEX key seconds value\n"  
        << "  GET key\n"
        << "  DEL key\n"
        << "  SIZE\n"
        << "  HELP\n"
        << "  EXIT\n";
}

int main() {
    size_t capacity;

    std::cout << "Enter cache capacity: ";
    std::cin >> capacity;
    std::cin.ignore();

    LRUCache cache(capacity);
    WriteAheadLog wal("kvstore.wal");

    wal.replay(cache);

    std::cout
        << "KV store ready. Capacity = "
        << capacity
        << ". Recovered "
        << cache.size()
        << " key(s) from log.\n";

    std::cout << "Type HELP for commands.\n";

    std::string line;

    while (true) {
        std::cout << "> ";

        if (!std::getline(std::cin, line))
            break;

        if (line.empty())
            continue;

        std::vector<std::string> tokens = tokenize(line);

        if (tokens.empty())
            continue;

        std::string cmd = tokens[0];

        for (char& c : cmd)
            c = static_cast<char>(
                std::toupper(static_cast<unsigned char>(c))
            );

        if (cmd == "EXIT") {
            break;
        }
        else if (cmd == "HELP") {
            printHelp();
        }
        else if (cmd == "SET") {
            if (tokens.size() < 3) {
                std::cout << "Usage: SET key value\n";
                continue;
            }

            std::string key = tokens[1];
            std::string value = tokens[2];

            wal.logSet(key, value);
            cache.set(key, value);

            std::cout << "OK\n";
        }
        
        else if (cmd == "SETEX") {
            long long seconds = tokens.size() >= 4
                                    ? std::atoll(tokens[2].c_str()) : 0;

            if (seconds <= 0) {
                std::cout << "Usage: SETEX key seconds value\n";
                continue;
            }

            std::string key = tokens[1];
            std::string value = tokens[3];
            long long expiryMs = nowMs() + seconds * 1000;

            wal.logSet(key, value, expiryMs);
            cache.set(key, value, expiryMs);

            std::cout << "OK\n";
        }
        else if (cmd == "GET") {
            if (tokens.size() < 2) {
                std::cout << "Usage: GET key\n";
                continue;
            }

            std::string key = tokens[1];
            std::string value;

            if (cache.get(key, value))
                std::cout << value << "\n";
            else
                std::cout << "(nil)\n";
        }
        else if (cmd == "DEL") {
            if (tokens.size() < 2) {
                std::cout << "Usage: DEL key\n";
                continue;
            }

            std::string key = tokens[1];

            wal.logDel(key);
            bool deleted = cache.del(key);

            std::cout << (deleted ? "1\n" : "0\n");
        }
        else if (cmd == "SIZE") {
            std::cout << cache.size() << "\n";
        }
        else {
            std::cout << "Unknown command. Type HELP.\n";
        }
    }

    return 0;
}