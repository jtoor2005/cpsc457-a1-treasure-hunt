CPSC 457 – Fall 2025 — Assignment 1 (Parts I & II)
==================================================

Repo URL:
  https://github.com/jtoor2005/cpsc457-a1-treasure-hunt

Files
-----
- a1p1.c  : Part 1 — treasure hunt with fork, exit, wait (no shared memory).
- a1p2.c  : Part 2 — parallel prime finder using shared memory (shmget/shmat/shmdt/shmctl).
- A1_Report.pdf : Combined report with reflections for Part 1 and Part 2.

Build (on cslinux)
------------------
# Part 1
gcc -O2 -Wall a1p1.c -o a1p1

# Part 2
gcc -O2 -Wall a1p2.c -o a1p2 -lm

Run
---
# Part 1 (reads 100x1000 matrix from stdin)
./a1p1 < input.txt

# Part 2
./a1p2 <LOWER> <UPPER> <N>

Notes
-----
- Part 1 uses only exit and wait; prints per-child search lines and parent summary.
- Part 2 caps processes to N_eff = min(N, upper-lower+1); per-child shared-memory blocks avoid locking
- Verified on cslinux servers.
