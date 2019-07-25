/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32 && M == 32) {
        for (int i = 0; i < M; i += 8) {
            for (int j = 0; j < N; j += 8) {
                for (int k = i; k < i+8; k++) {
                    int tempv0 = A[k][j];
                    int tempv1 = A[k][j+1];
                    int tempv2 = A[k][j+2];
                    int tempv3 = A[k][j+3];
                    int tempv4 = A[k][j+4];
                    int tempv5 = A[k][j+5];
                    int tempv6 = A[k][j+6];
                    int tempv7 = A[k][j+7];

                    B[j][k] = tempv0;
                    B[j+1][k] = tempv1;
                    B[j+2][k] = tempv2;
                    B[j+3][k] = tempv3;
                    B[j+4][k] = tempv4;
                    B[j+5][k] = tempv5;
                    B[j+6][k] = tempv6;
                    B[j+7][k] = tempv7;
                }
            }
        }
    } else if (N == 64 && M == 64) {
        for (int i = 0; i < N; i += 8) {
            for (int j = 0; j < M; j += 8) {
                for (int k = i; k < i+4; k++) {
                    int tempv0 = A[k][j];
                    int tempv1 = A[k][j+1];
                    int tempv2 = A[k][j+2];
                    int tempv3 = A[k][j+3];
                    int tempv4 = A[k][j+4];
                    int tempv5 = A[k][j+5];
                    int tempv6 = A[k][j+6];
                    int tempv7 = A[k][j+7];
                    
                    B[j][k] = tempv0;
                    B[j+1][k] = tempv1;
                    B[j+2][k] = tempv2;
                    B[j+3][k] = tempv3;
                    B[j][k+4] = tempv4;
                    B[j+1][k+4] = tempv5;
                    B[j+2][k+4] = tempv6;
                    B[j+3][k+4] = tempv7;
                }

                for (int y = j; y < j+4; y++) {
                    int tempv0 = A[i+4][y];
                    int tempv1 = A[i+5][y];
                    int tempv2 = A[i+6][y];
                    int tempv3 = A[i+7][y];
                    int tempv4 = B[y][i+4];
                    int tempv5 = B[y][i+5];
                    int tempv6 = B[y][i+6];
                    int tempv7 = B[y][i+7];

                    B[y][i+4] = tempv0;
                    B[y][i+5] = tempv1;
                    B[y][i+6] = tempv2;
                    B[y][i+7] = tempv3;
                    B[y+4][i] = tempv4;
                    B[y+4][i+1] = tempv5;
                    B[y+4][i+2] = tempv6;
                    B[y+4][i+3] = tempv7;                           
                }

                for (int x = i+4; x < i+8; x++) {
                    int tempv0 = A[x][j+4];
                    int tempv1 = A[x][j+5];
                    int tempv2 = A[x][j+6];
                    int tempv3 = A[x][j+7];
                    B[j+4][x] = tempv0;
                    B[j+5][x] = tempv1;
                    B[j+6][x] = tempv2;
                    B[j+7][x] = tempv3;
                }
            }
        }
    } else {
        for (int i = 0; i < N; i += 16) {
            for (int j = 0; j < M; j += 16) {
                for (int k = i; k < i+16 && k < N; k++) {
                    // record the disago value
                    int temp_position = -1;
                    int temp_value = 0;
                    for (int l = j; l < j+16 && l < M; l++) {
                        if (k == l) {
                            temp_position = k;
                            temp_value = A[k][k];
                        } else {
                            B[l][k] = A[k][l];
                        }
                    }

                    if (temp_position != -1) {
                        B[temp_position][temp_position] = temp_value;
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

