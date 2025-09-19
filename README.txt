
CPSC 457 – Fall 2025 — Assignment 1 (Part I)
=============================================

Files included:
- a1p1.c — C source for Part I (treasure hunt with `fork`).
- A1_Part1_Report.pdf — brief reflection and required first‑page info placeholders.

Build on cslinux servers:
    gcc -O2 -Wall a1p1.c -o a1p1

Run (redirecting an input matrix file):
    ./a1p1 < test1.txt

Expected behavior:
- Spawns 100 children; each prints: `Child i (PID XXXX): Searching row i`
- Parent prints final line when treasure is found, e.g.:
    Parent: The treasure was found by child with PID 2411 at row 73 and column 652

Notes:
- We avoid shared memory entirely (per Part I requirements).
- Children communicate success via exit code 1; failure via exit code 0.
- Parent maps the winning PID to its row and then scans that row to get the exact column.

Security & Safety:
- All children exit via `_exit(...)`; parent waits for all children (no zombies).

Testing locally (with the provided inputs):
    ./a1p1 < "Assignment 1 - Inputs/test1.txt"
    ./a1p1 < "Assignment 1 - Inputs/test2.txt"
    …

Git URL placeholder:
    https://example.com/your-repo.git   (replace before submission)

Last updated: 2025-09-19T18:12:06
