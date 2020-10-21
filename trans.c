/**
 * Name: Aichen Yao
 * Andrew ID: aicheny
 * @file trans.c
 * @brief Contains various implementations of matrix transpose
 *
 * Each transpose function must have a prototype of the form:
 *   void trans(size_t M, size_t N, double A[N][M], double B[M][N],
 *              double tmp[TMPCOUNT]);
 *
 * All transpose functions take the following arguments:
 *
 *   @param[in]     M    Width of A, height of B
 *   @param[in]     N    Height of A, width of B
 *   @param[in]     A    Source matrix
 *   @param[out]    B    Destination matrix
 *   @param[in,out] tmp  Array that can store temporary double values
 *
 * A transpose function is evaluated by counting the number of hits and misses,
 * using the cache parameters and score computations described in the writeup.
 *
 * Programming restrictions:
 *   - No out-of-bounds references are allowed
 *   - No alterations may be made to the source array A
 *   - Data in tmp can be read or written
 *   - This file cannot contain any local or global doubles or arrays of doubles
 *   - You may not use unions, casting, global variables, or
 *     other tricks to hide array data in other forms of local or global memory.
 *
 * TODO: fill in your name and Andrew ID below.
 * @author Your Name <andrewid@andrew.cmu.edu>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "cachelab.h"

/**
 * @brief Checks if B is the transpose of A.
 *
 * You can call this function inside of an assertion, if you'd like to verify
 * the correctness of a transpose function.
 *
 * @param[in]     M    Width of A, height of B
 * @param[in]     N    Height of A, width of B
 * @param[in]     A    Source matrix
 * @param[out]    B    Destination matrix
 *
 * @return True if B is the transpose of A, and false otherwise.
 */
static bool is_transpose(size_t M, size_t N, const double A[N][M],
                         double B[M][N]) {
    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                fprintf(stderr,
                        "Transpose incorrect.  Fails for B[%zd][%zd] = %.3f, "
                        "A[%zd][%zd] = %.3f\n",
                        j, i, B[j][i], i, j, A[i][j]);
                return false;
            }
        }
    }
    return true;
}

/*
 * You can define additional transpose functions here. We've defined
 * some simple ones below to help you get started, which you should
 * feel free to modify or delete.
 */

/**
 * @brief A simple baseline transpose function, not optimized for the cache.
 *
 * Note the use of asserts (defined in assert.h) that add checking code.
 * These asserts are disabled when measuring cycle counts (i.e. when running
 * the ./test-trans) to avoid affecting performance.
 */
static void trans_basic(size_t M, size_t N, const double A[N][M],
                        double B[M][N], double tmp[TMPCOUNT]) {
    assert(M > 0);
    assert(N > 0);

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < M; j++) {
            B[j][i] = A[i][j];
        }
    }
    assert(is_transpose(M, N, A, B));
}

/**
 * @brief A contrived example to illustrate the use of the temporary array.
 *
 * This function uses the first four elements of tmp as a 2x2 array with
 * row-major ordering.
 */
static void trans_tmp(size_t M, size_t N, const double A[N][M], double B[M][N],
                      double tmp[TMPCOUNT]) {
    assert(M > 0);
    assert(N > 0);

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < M; j++) {
            size_t di = i % 2;
            size_t dj = j % 2;
            tmp[2 * di + dj] = A[i][j];
            B[j][i] = tmp[2 * di + dj];
        }
    }
    assert(is_transpose(M, N, A, B));
}

size_t tmpIndex(size_t row, size_t col) {
    size_t absIndex = (row << 3) + col;
    size_t setIndex = absIndex >> 3;
    return setIndex;
}
static void trans_32(size_t M, size_t N, const double A[N][M], double B[M][N],
                     double tmp[TMPCOUNT]) {
    assert(M == 32);
    assert(N == 32);
    size_t b = 8;
    size_t tmpi, tmpj, i, j;
    // inspired by the code from recitation 5 matrix multiplication
    for (i = 0; i < M; i += b)
        for (j = 0; j < N; j += b)
            for (tmpi = i; tmpi < i + b; tmpi++)
                for (tmpj = j; tmpj < j + b; tmpj++) {
                    if (tmpi != tmpj) {
                        B[tmpi][tmpj] = A[tmpj][tmpi];
                    }
                }
    for (i = 0; i < M; i++) {
        // Entries on the diagonal would cause a double eviction every time
        // so we don't do that in the major loop, we handle it here.
        B[i][i] = A[i][i];
    }
    assert(is_transpose(M, N, A, B));
}

static void trans_1024(size_t M, size_t N, const double A[N][M], double B[M][N],
                       double tmp[TMPCOUNT]) {
    // deal with 32*32 matrix transpose only
    assert(M == 1024);
    assert(N == 1024);
    size_t b = 8;
    size_t tmpi, tmpj, i, j;
    // inspired by the code from recitation 5 matrix multiplication
    for (i = 0; i < M; i += b)
        for (j = 0; j < N; j += b)
            for (tmpi = i; tmpi < i + b; tmpi++)
                for (tmpj = j; tmpj < j + b; tmpj++) {
                    B[tmpi][tmpj] = A[tmpj][tmpi];
                }
    // Not too many thrasing would happen in this case since each set now
    // has 8 lines. So we don't need to handle the diagonals separately.
    assert(is_transpose(M, N, A, B));
}

static void trans_32Or1024(size_t M, size_t N, const double A[N][M],
                           double B[M][N], double tmp[TMPCOUNT]) {
    // works like a wrapper, call trans_32 or trans_1024 based on the inputs
    // M and N.
    if ((M == 32) && (N == 32)) {
        trans_32(M, N, A, B, tmp);
        return;
    }
    if ((M == 1024) && (N == 1024)) {
        trans_1024(M, N, A, B, tmp);
        return;
    }
    return;
}
/**
 * @brief The solution transpose function that will be graded.
 *
 * You can call other transpose functions from here as you please.
 * It's OK to choose different functions based on array size, but
 * this function must be correct for all values of M and N.
 */
static void transpose_submit(size_t M, size_t N, const double A[N][M],
                             double B[M][N], double tmp[TMPCOUNT]) {
    if (((M == 32) && (N == 32)) || ((M == 1024) && (N == 1024)))
        trans_32Or1024(M, N, A, B, tmp);
    else
        trans_tmp(M, N, A, B, tmp);
}

/**
 * @brief Registers all transpose functions with the driver.
 *
 * At runtime, the driver will evaluate each function registered here, and
 * and summarize the performance of each. This is a handy way to experiment
 * with different transpose strategies.
 */
void registerFunctions(void) {
    // Register the solution function. Do not modify this line!
    registerTransFunction(transpose_submit, SUBMIT_DESCRIPTION);
    registerTransFunction(trans_basic, "Basic transpose");
    registerTransFunction(trans_tmp, "Transpose using the temporary array");
}
