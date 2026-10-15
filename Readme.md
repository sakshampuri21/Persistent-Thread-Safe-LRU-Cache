# Persistent Thread-Safe LRU Cache

An in-memory key-value store written in C++ with:

- **LRU eviction** in O(1) average time (`unordered_map` + doubly linked list)
- **TTL (time to live)** with lazy expiry: an expired key is removed when it is read
- **Write-Ahead Log (WAL)** with `fsync`, so data survives a restart or crash
- **Thread safety** using `std::mutex` and `std::lock_guard`

## Project structure

| File | Purpose |
|---|---|
| `lru_cache.h` / `lru_cache.cpp` | The LRU cache (map + doubly linked list, TTL, mutex) |
| `wal.h` | Write-ahead log: logs every change, replays it on startup |
| `main.cpp` | Command-line interface (`SET`, `GET`, `DEL`, ...) |
| `mutex_test.cpp` | Multi-threaded test (10 threads, 10,000 operations each) |

## Requirements

- A C++17 compiler (`g++` from MinGW-w64 / MSYS2 on Windows, or `g++` on Linux/macOS)
- Check it is installed:

```bash
g++ --version
```

## How to build and run

Open a terminal and go into the project folder:

```bash
cd Persistent-Thread-Safe-LRU-Cache
```

If you are in the parent folder, use the full path, for example:

```bash
cd ~/OneDrive/Desktop/lru/Persistent-Thread-Safe-LRU-Cache
```

### 1. Build the key-value store

```bash
g++ -std=c++17 -pthread lru_cache.cpp main.cpp -o kv
```

### 2. Run it

```bash
./kv
```

It asks for the cache capacity (a positive number). Type `5`, for example, and press Enter.

### 3. Build and run the thread-safety test

```bash
g++ -std=c++17 -pthread lru_cache.cpp mutex_test.cpp -o test
./test
```

Expected output:

```
All threads completed successfully.
Final cache size: 50000
```

On Windows, the compiler creates `kv.exe` and `test.exe`. `./kv` and `./test` still work in Git Bash.

## Commands

| Command | Description |
|---|---|
| `SET key value` | Store a value (no expiry) |
| `SETEX key seconds value` | Store a value that expires after N seconds |
| `GET key` | Read a value, or `(nil)` if missing or expired |
| `DEL key` | Delete a key (prints `1` if deleted, `0` if not found) |
| `SIZE` | Number of keys currently stored |
| `HELP` | Show the command list |
| `EXIT` | Quit |

Keys and values are single words (no spaces).

### Example session

```
Enter cache capacity: 3
> SET name saksham
OK
> SETEX temp 5 hello
OK
> GET temp
hello
> GET temp          (after 6 seconds)
(nil)
> SET a 1
> SET b 2
> SET c 3           (cache is full, so the least recently used key is evicted)
> SIZE
3
> EXIT
```

## How to test it properly

1. **LRU eviction:** use capacity `3`. Run `SET a 1`, `SET b 2`, `SET c 3`, `GET a`, `SET d 4`. Now `GET b` returns `(nil)`, because `b` was the least recently used.
2. **TTL while running:** run `SETEX temp 5 hello`, wait 6 seconds, then `GET temp` returns `(nil)`.
3. **Persistence:** run `SET city varanasi`, then `EXIT`. Start `./kv` again with the same capacity. `GET city` returns `varanasi`.
4. **Expiry across a restart:** run `SETEX x 5 hello`, `EXIT`, wait 10 seconds, start `./kv` again. `GET x` returns `(nil)`.
5. **Delete is replayed:** run `SET k v`, `DEL k`, `EXIT`, restart. `GET k` returns `(nil)`.
6. **Thread safety:** run `./test`.

To start from a clean state, delete the log file:

```bash
rm kvstore.wal
```

## How it works

### LRU cache
- A hash map finds a key in O(1).
- A doubly linked list keeps keys ordered by use: most recently used at the front, least recently used at the back.
- `get` and `set` move the key to the front. When the cache is full, the node at the back is evicted.

### TTL (lazy expiry)
- Each node stores `expiryMs`, the absolute time when it expires (`0` means never).
- There is no timer. `get()` checks the time, and if the key has expired it removes it and returns "not found".
- Drawback: an expired key that is never read stays in memory until it is evicted.

### Write-Ahead Log
- Every `SET` and `DEL` is appended to `kvstore.wal` **before** it is applied to memory.
- After each write, `fflush` and `fsync` force the data to disk, so it survives a power cut.
- On startup, the log is read from top to bottom and every operation is applied again. Keys whose expiry time has already passed are skipped.

Log format:

```
SET key value expiryMs      (expiryMs = 0 means no TTL)
DEL key
```

The log records history, not the current state. The cache capacity is not stored in the log, so replay uses the capacity you enter at startup. If you start with a smaller capacity, older keys are evicted during replay.

### Thread safety
- One `std::mutex` protects the map and the list.
- `std::lock_guard` locks at the start of each function and unlocks automatically when it ends.
- `get` also needs the lock, because it moves a node to the front, which changes the list.

## Known limitations

- Values are single words (the log is space-separated).
- The log is never compacted, so it grows over time.
- A half-written last log line (crash in the middle of a write) is not detected.
- `fsync` on every write is safe but slow. Real systems batch writes.
- Expired keys that are never read are removed only by LRU eviction.
- One global lock limits parallelism. Sharding the cache would improve it.
- Logging and applying a change are not one atomic step across threads.