/*
 * CPSC 457 – Fall 2025
 * Assignment 1 – Part II: Parallel Prime Number Finder (processes + shared memory)
 * Author: Jaiveer Toor
 * UCID: 30204466
 * Repo URL: https://github.com/jtoor2005/cpsc457-a1-treasure-hunt
 *
 * Build:
 *   gcc -O2 -Wall a1p2.c -o a1p2 -lm
 *
 * Run:
 *   ./a1p2 <LOWER_BOUND> <UPPER_BOUND> <NPROCS>
 *
 * Spec highlights:
 * - Parent: shmget/shmat to allocate shared memory; fork() N_eff children; wait() for all;
 *   print primes; shmdt + shmctl(IPC_RMID) to free shared memory.
 * - Children: compute non-overlapping subranges; write results into per-child block in SHM.
 * - Layout: counts[N_eff] + primes[N_eff * MAX_PER_CHILD], where MAX_PER_CHILD = ceil(total_len / N_eff).
 * - No locking needed because each child writes only to its own block.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <math.h>

static int is_prime(int num) {
    if (num < 2) return 0;
    int limit = (int)(sqrt((double)num));
    for (int i = 2; i <= limit; ++i) {
        if (num % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s LOWER_BOUND UPPER_BOUND N\n", argv[0]);
        return 1;
    }

    char *endp = NULL;
    long lower = strtol(argv[1], &endp, 10);
    if (*endp != '\0') { fprintf(stderr, "Error: invalid LOWER_BOUND\n"); return 1; }
    long upper = strtol(argv[2], &endp, 10);
    if (*endp != '\0') { fprintf(stderr, "Error: invalid UPPER_BOUND\n"); return 1; }
    long N = strtol(argv[3], &endp, 10);
    if (*endp != '\0' || N <= 0) { fprintf(stderr, "Error: invalid N\n"); return 1; }
    if (lower > upper) { fprintf(stderr, "Error: LOWER_BOUND must be <= UPPER_BOUND\n"); return 1; }

    long total_len = upper - lower + 1;

    // Cap process count so we never spawn more children than values to check.
    long N_eff = N;
    if (total_len < N_eff) N_eff = total_len;  // ensures each child gets >= 1 value (when possible)
    if (N_eff <= 0) {
        printf("Parent: All children finished. Primes found:\n\n");
        return 0;
    }

    // Capacity per child: ceil(total_len / N_eff) -- enough to hold all primes in its subrange
    long max_per_child = (total_len + N_eff - 1) / N_eff;

    // Shared memory layout: [counts (N_eff ints)] [primes (N_eff * max_per_child ints)]
    size_t ints_needed = (size_t)N_eff + (size_t)N_eff * (size_t)max_per_child;
    size_t shm_size = ints_needed * sizeof(int);

    int shmid = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    int *shm = (int *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    int *counts = shm;                  // counts[0..N_eff-1]
    int *primes = shm + N_eff;          // primes[0..N_eff*max_per_child - 1]

    // Initialize counts
    for (long i = 0; i < N_eff; ++i) counts[i] = 0;

    // Balanced partitioning: base-sized chunks, first 'extra' get +1
    long base  = total_len / N_eff;
    long extra = total_len % N_eff;

    pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * (size_t)N_eff);
    if (!pids) {
        fprintf(stderr, "malloc failed\n");
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    for (long i = 0; i < N_eff; ++i) {
        long len_i = base + (i < extra ? 1 : 0);
        long offset = i * base + (i < extra ? i : extra);
        long start = lower + offset;
        long end = start + len_i - 1; // inclusive

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // Cleanup: wait for already-forked children
            for (long k = 0; k < i; ++k) {
                int st;
                waitpid(pids[k], &st, 0);
            }
            free(pids);
            shmdt(shm);
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        } else if (pid == 0) {
            // ----- Child -----
            pid_t me = getpid();
            printf("Child PID %d checking range [%ld, %ld]\n", (int)me, start, end);
            fflush(stdout);

            int count = 0;
            for (long n = start; n <= end; ++n) {
                if (is_prime((int)n)) {
                    if (count < max_per_child) {
                        long idx = i * max_per_child + count;
                        primes[idx] = (int)n;
                        count++;
                    } else {
                        // Defensive: capacity should be enough by construction
                        break;
                    }
                }
            }
            counts[i] = count;

            // Detach and exit
            if (shmdt(shm) == -1) perror("shmdt (child)");
            _exit(0);
        } else {
            // ----- Parent -----
            pids[i] = pid;
        }
    }

    // Parent waits for all children
    for (long i = 0; i < N_eff; ++i) {
        int st;
        if (waitpid(pids[i], &st, 0) == -1) {
            perror("waitpid");
        }
    }
    free(pids);

    // Print results
    printf("Parent: All children finished. Primes found:\n");
    int first = 1;
    for (long i = 0; i < N_eff; ++i) {
        int cnt = counts[i];
        for (int j = 0; j < cnt; ++j) {
            int val = primes[i * max_per_child + j];
            if (!first) printf(" ");
            printf("%d", val);
            first = 0;
        }
    }
    printf("\n");

    // Cleanup SHM
    if (shmdt(shm) == -1) perror("shmdt (parent)");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) perror("shmctl IPC_RMID");

    return 0;
}
