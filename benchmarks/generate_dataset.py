#!/usr/bin/env python3
"""Generate a deterministic text dataset for local indexing benchmarks."""

from __future__ import annotations

import argparse
import random
from pathlib import Path


VOCABULARY = (
    "algorithm atomic benchmark cache concurrency condition directory engine "
    "filesystem frequency index keyword latency lock memory mutex parallel path "
    "posting query queue ranking resource search synchronization task thread token "
    "worker"
).split()


def positive_integer(value: str) -> int:
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("value must be greater than zero")
    return number


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--files", type=positive_integer, default=1000)
    parser.add_argument("--words-per-file", type=positive_integer, default=5000)
    parser.add_argument("--seed", type=int, default=42)
    arguments = parser.parse_args()

    arguments.output.mkdir(parents=True, exist_ok=True)
    randomizer = random.Random(arguments.seed)
    extensions = (".txt", ".md", ".log", ".csv")

    for file_index in range(arguments.files):
        subdirectory = arguments.output / f"batch-{file_index // 100:03d}"
        subdirectory.mkdir(exist_ok=True)
        path = subdirectory / f"document-{file_index:06d}{extensions[file_index % 4]}"
        words = randomizer.choices(VOCABULARY, k=arguments.words_per_file)
        lines = (" ".join(words[start : start + 20]) for start in range(0, len(words), 20))
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(
        f"generated {arguments.files} files and "
        f"{arguments.files * arguments.words_per_file} tokens in {arguments.output}"
    )


if __name__ == "__main__":
    main()
