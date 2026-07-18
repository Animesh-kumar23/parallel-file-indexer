# Parallel File Indexer and Search Engine

A deliberately bounded, Linux-first C++20 command-line project that recursively
discovers text files, processes one file per task, builds an in-memory inverted
index, and ranks keyword searches. It is an educational portfolio project, not a
production search engine.

## Features

- Recursive `.txt`, `.md`, `.log`, and `.csv` discovery
- Case-insensitive deterministic tokenization
- Per-file frequency maps merged into an in-memory inverted index
- Fixed-size RAII thread pool using mutexes and condition variables
- Single-term and multi-term ranked search
- Indexing statistics and worker-count benchmarks
- CMake options for ASan, UBSan, and TSan

## Supported toolchain

The project requires C++20 and CMake 3.20 or newer. Current stable development
versions are recommended: GCC 16.1 and CMake 4.4. A supported recent Clang also
works.

The application uses only the C++ standard library. Python is used solely by the
optional benchmark dataset generator.

## Architecture

```text
directory
   |
   v
FileScanner ---> eligible paths ---> ThreadPool (one task per file)
                                      |
                                      v
                               FileProcessor
                                      |
                               local frequency map
                                      |
                                      v
                                InvertedIndex
                                      |
                         postings protected by one mutex
                                      |
                                      v
Query ---> Tokenizer ---> QueryProcessor ---> ranked SearchResult list
```

| Component | Responsibility |
| --- | --- |
| `FileScanner` | Validates and recursively scans a root, filters extensions, and skips inaccessible entries. |
| `Tokenizer` | Splits on non-alphanumeric bytes and lowercases tokens deterministically. |
| `FileProcessor` | Reads one file and creates a local word-frequency map. |
| `ThreadPool` | Owns workers, synchronizes its task queue, waits for idle, and joins on destruction. |
| `InvertedIndex` | Coarse-grained merge of completed file results and synchronized reads. |
| `QueryProcessor` | Normalizes unique query terms, combines postings, scores, and sorts. |
| `SearchEngine` | Coordinates scanning, indexing, searching, statistics, and benchmarks. |

## Concurrency model

Scanning happens on the caller thread. Each eligible path becomes one thread-pool
task. A worker reads and tokenizes its file without touching global state, then
acquires the index mutex once to merge the completed local map. Indexed and failed
file counters are atomic. Task exceptions are isolated so a failed file cannot
terminate a worker or prevent unrelated work from completing.

`waitUntilIdle()` waits until the queue is empty and no worker is active. The pool
destructor stops acceptance, drains already queued work, wakes all workers, and
joins every thread. Search is offered only after the complete indexing barrier, so
results never observe partially merged files.

## Build

Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

On Windows, the recommended native development environment is current MSYS2
UCRT64. The primary target remains Linux with GCC or Clang.

## Usage

The index intentionally lives only in memory. `index` therefore opens an
interactive session in the same process:

```bash
./build/file-indexer index ./sample-data
indexer> search "concurrency thread"
indexer> stats
indexer> clear
indexer> index ./another-directory
indexer> quit
```

Other entry points are:

```bash
./build/file-indexer benchmark <directory>
./build/file-indexer stats
./build/file-indexer clear
./build/file-indexer help
```

Invoking `file-indexer search` as a separate process reports an explanatory error
instead of pretending that an earlier in-memory index persisted. Search is fully
supported as `search "<query>"` inside the session opened by `index`.

The score is the sum of each distinct query term's frequency in a file. Results
sort by descending score, then lexicographically by normalized path. Duplicate
query terms are treated once rather than artificially boosting a score.

## Sanitizers and GDB

AddressSanitizer and UndefinedBehaviorSanitizer may share a build:

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=ON \
  -DENABLE_UBSAN=ON
cmake --build build-asan
```

ThreadSanitizer must use its own build:

```bash
cmake -S . -B build-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_TSAN=ON
cmake --build build-tsan
```

Sanitizer availability depends on the compiler and operating system. No sanitizer
run should be considered successful unless its command actually completes.

On Windows, MSYS2 UCRT64 GCC does not provide the ASan/UBSan runtime libraries.
The recorded Windows sanitizer run therefore used MSYS2 Clang64 22.1.8 and
compiler-rt. Linux GCC remains the primary documented sanitizer path.

Basic GDB workflow:

```bash
gdb --args ./build/file-indexer index ./sample-data
(gdb) break file_indexer::SearchEngine::indexDirectory
(gdb) run
(gdb) thread apply all bt
```

## Benchmark methodology

`benchmark` indexes the same directory with 1, 2, 4, and
`std::thread::hardware_concurrency()` workers, removing duplicate worker counts.
Every run clears the prior index, uses `std::chrono::steady_clock`, and checks its
statistics against the one-worker baseline. Speedup is baseline time divided by
run time.

Generate a reproducible development dataset and run a release benchmark:

```bash
python3 benchmarks/generate_dataset.py benchmarks/generated-data \
  --files 1000 --words-per-file 5000 --seed 42
./build-release/file-indexer benchmark benchmarks/generated-data
```

Record real measurements and environment details in
`benchmarks/benchmark_results.md`. No measurements are fabricated.

The repository's recorded run used a 500-file, 2.5-million-token dataset and
reports the median of three warm-cache release runs. See the results file for the
machine, exact times, and cache methodology.

## Verified checks

On 2026-07-18, the following checks completed successfully:

- GCC 16.1 debug configure and warning-clean build
- CLI help, interactive index/search/stats, and benchmark smoke checks
- GCC 16.1 release configure and warning-clean build
- Clang 22.1.8 ASan+UBSan build
- Release benchmark with equivalent index statistics at every worker count

ThreadSanitizer was not run on this Windows host because compiler-rt does not
support TSan for the Windows target. The separate `ENABLE_TSAN` configuration is
provided for the primary Linux environment.

## Complexity

Let `F` be the number of files, `T` the total number of tokens, `Q` the number of
distinct query terms, and `R` the combined number of posting visits/matching files
used to build the ranked result.

- Average-case indexing: `O(T)` under average constant-time hash-map operations,
  plus filesystem traversal proportional to entries visited.
- Search and ranking: `O(Q + R log R)` under the simplified project assumptions.
- Memory: `O(T)` in the loose worst case, with practical cost proportional to
  unique terms, per-file local maps, and one posting per term/file pair.

Local maps keep tokenization lock-free and reduce global synchronization from one
lock per token to one coarse merge per file. The simple global index mutex favors
clarity and correctness; very small files can create merge contention. Additional
workers eventually stop helping because storage bandwidth, filesystem cache,
allocation, and synchronized merges become bottlenecks.

## Design decisions and trade-offs

- An in-memory index keeps ownership and concurrency easy to explain but does not
  survive process exit.
- Contiguous ASCII-style alphanumeric sequences are tokens. This handles CSV
  delimiters predictably but does not provide Unicode word segmentation.
- Query terms are OR-combined and ranked by raw frequency. There is no phrase,
  positional, semantic, fuzzy, or TF-IDF search.
- A standard mutex is used for the index because indexing and searching are
  phase-separated. A shared mutex would add little value to this workflow.
- Re-indexing clears the previous index, avoiding stale or duplicated postings.
- Paths are sorted after scanning and used as the final result tie-breaker.

## Error handling

Missing or non-directory roots are rejected. Permission errors during traversal
are skipped and counted. An unreadable file emits a warning, increments the failed
counter, and does not stop other tasks. Empty files index successfully with zero
tokens. Zero workers and empty interactive queries are rejected explicitly.

## Known limitations

- No persistent index or incremental updates
- Byte-oriented tokenization rather than full Unicode normalization
- Whole-file logical indexing with line-by-line input; no file-size admission limit
- Search occurs after indexing, not concurrently with active merges
- Raw-frequency ranking can favor longer files
- Benchmark results are sensitive to filesystem cache and background I/O

## Future improvements

Appropriate later extensions include positional postings, stop-word configuration,
bounded file-size policies, richer benchmark controls, and—in a separately scoped
optional phase—SQLite metadata for skipping unchanged files. None is part of the
initial implementation.

## Resume-ready description

> Built a C++20 parallel file indexer using `std::filesystem`, a custom RAII thread
> pool, synchronized inverted-index merges, deterministic ranked search, CMake,
> sanitizer configurations, and reproducible worker-scaling benchmarks.

## License

MIT; see [LICENSE](LICENSE).
