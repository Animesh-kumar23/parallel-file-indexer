# Benchmark Results

Measured on 2026-07-18. Values are the median of three consecutive warm-cache
runs. A separate first cold-cache invocation was excluded because it primarily
measured OneDrive/file-cache warm-up rather than steady-state worker scaling.

## Environment

- CPU: Intel Core i7-8550U @ 1.80 GHz, 8 logical processors
- Operating system: Microsoft Windows NT 10.0.26200.0
- Compiler: MSYS2 UCRT64 GCC 16.1.0
- CMake: 4.4.0, Ninja 1.13.2
- Build type: Release
- Dataset size: 20,120,417 bytes
- Eligible file count: 500
- Total tokens: 2,500,000 generated tokens
- Generator: `generate_dataset.py generated-data --files 500 --words-per-file 5000 --seed 42`
- Command: `file-indexer benchmark benchmarks/generated-data`

## Results

| Workers | Time | Speedup | Equivalent to baseline |
| ---: | ---: | ---: | :---: |
| 1 | 0.747 s | 1.00x | yes |
| 2 | 0.441 s | 1.69x | yes |
| 4 | 0.355 s | 2.10x | yes |
| 8 | 0.301 s | 2.48x | yes |

The executable checked every run's discovered/indexed/failed/skipped counts,
unique-word count, and total-token count against the one-worker baseline. All
runs were equivalent. These numbers are specific to this machine and dataset.
