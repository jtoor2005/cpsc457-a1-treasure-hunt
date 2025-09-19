
/*
 * CPSC 457 – Fall 2025
 * Assignment 1 – Part I: Basic Parallel Program with fork()
 * Author: Jaiveer Toor
 * UCID: 30204466
 *
 * Build:   gcc -O2 -Wall a1p1.c -o a1p1
 * Run:     ./a1p1 < inputfilep1.txt
 *
 * Notes:
 * - Spawns 100 child processes; each scans one row (1000 cols) for treasure '1'.
 * - Children communicate success/failure via exit status (1 = found, 0 = not found).
 * - Parent maps PID -> row, then rescans that row to determine the treasure column.
 * - No shared memory is used in Part I (as required).
 * - All children are waited on; parent exits with code 0.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define ROWS 100
#define COLS 1000

static int matrix[ROWS][COLS];

int main(void) {
    // Read 100 x 1000 matrix of 0/1 from stdin
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (scanf("%d", &matrix[i][j]) != 1) {
                fprintf(stderr, "Error: failed to read matrix at row %d col %d\n", i, j);
                return 1;
            }
        }
    }

    pid_t pids[ROWS];
    for (int i = 0; i < ROWS; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // Attempt to clean up already-forked children by waiting
            for (int k = 0; k < i; ++k) {
                int status;
                waitpid(pids[k], &status, 0);
            }
            return 1;
        } else if (pid == 0) {
            // Child process: scan its row
            pid_t me = getpid();
            printf("Child %d (PID %d): Searching row %d\n", i, (int)me, i);
            fflush(stdout);
            int found = 0;
            for (int j = 0; j < COLS; ++j) {
                if (matrix[i][j] == 1) {
                    found = 1;
                    break;
                }
            }
            // Exit status: 1 if found, 0 otherwise (fits into 8-bit wait status)
            _exit(found ? 1 : 0);
        } else {
            // Parent
            pids[i] = pid;
        }
    }

    // Parent: wait for all, track the PID that reported success
    pid_t winner_pid = -1;
    int status;
    for (int c = 0; c < ROWS; ++c) {
        pid_t w = wait(&status);
        if (w == -1) {
            perror("wait");
            return 1;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            winner_pid = w;
        }
    }

    if (winner_pid == -1) {
        printf("Parent: No treasure found.\n");
        return 0;
    }

    // Map the winning PID back to its row index
    int winner_row = -1;
    for (int i = 0; i < ROWS; ++i) {
        if (pids[i] == winner_pid) {
            winner_row = i;
            break;
        }
    }
    if (winner_row == -1) {
        fprintf(stderr, "Internal error: winner PID not recognized.\n");
        return 1;
    }

    // Determine the exact column by scanning the stored matrix row
    int winner_col = -1;
    for (int j = 0; j < COLS; ++j) {
        if (matrix[winner_row][j] == 1) {
            winner_col = j;
            break;
        }
    }

    if (winner_col == -1) {
        // Should not happen if child reported found
        printf("Parent: Child with PID %d reported treasure in row %d, but column not found.\n",
               (int)winner_pid, winner_row);
    } else {
        printf("Parent: The treasure was found by child with PID %d at row %d and column %d\n",
               (int)winner_pid, winner_row, winner_col);
    }
    return 0;
}
