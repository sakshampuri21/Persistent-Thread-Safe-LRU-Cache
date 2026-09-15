#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

#include "lru_cache.h"
#include "wal.h"

// Split a command into whitespace-separated tokens.
//
// Example:
// SET name Saksham
//
// becomes:
// ["SET", "name", "Saksham"]
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

    // Remove the leftover newline.
    std::cin.ignore();

    // Create the in-memory LRU cache.
    LRUCache cache(capacity);

    // Create WAL.
    WriteAheadLog wal("kvstore.wal");

    // Recover previous state before accepting commands.
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

        // Convert command to uppercase.
        std::string cmd = tokens[0];

        for (char& c : cmd)
            c = static_cast<char>(
                std::toupper(static_cast<unsigned char>(c))
            );

        // ---------------- EXIT ----------------

        if (cmd == "EXIT") {

            break;
        }

        // ---------------- HELP ----------------

        else if (cmd == "HELP") {

            printHelp();
        }

        // ---------------- SET ----------------

        else if (cmd == "SET") {

            if (tokens.size() < 3) {

                std::cout
                    << "Usage: SET key value\n";

                continue;
            }

            std::string key = tokens[1];
            std::string value = tokens[2];

            // WAL FIRST
            wal.logSet(key, value);

            // Cache SECOND
            cache.set(key, value);

            std::cout << "OK\n";
        }

        // ---------------- GET ----------------

        else if (cmd == "GET") {

            if (tokens.size() < 2) {

                std::cout
                    << "Usage: GET key\n";

                continue;
            }

            std::string key = tokens[1];

            std::string value;

            if (cache.get(key, value)) {

                std::cout << value << "\n";

            } else {

                std::cout << "(nil)\n";
            }
        }

        // ---------------- DELETE ----------------

        else if (cmd == "DEL") {

            if (tokens.size() < 2) {

                std::cout
                    << "Usage: DEL key\n";

                continue;
            }

            std::string key = tokens[1];

            // WAL FIRST
            wal.logDel(key);

            // Cache SECOND
            bool deleted = cache.del(key);

            std::cout
                << (deleted ? "1\n" : "0\n");
        }

        // ---------------- SIZE ----------------

        else if (cmd == "SIZE") {

            std::cout << cache.size() << "\n";
        }

        // ---------------- UNKNOWN ----------------

        else {

            std::cout
                << "Unknown command. "
                << "Type HELP.\n";
        }
    }

    return 0;
}