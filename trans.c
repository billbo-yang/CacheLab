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
	//we want to do the block strategy discussed in lecture/lab
	//want to account for 32x32, 64x64, and another unknown size

	//keeping the variables unspecific so I can reuse them without getting overly confused
	//because of the 12 int limit
	int i, j, k, l;
	int temp; //temporary variables to be used for different things
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	//32x32 - 4 loops (2 for blocks, 2 for inside blocks)
	if(M == 32 && N == 32)
	{
		for (i = 0; i < N; i+=8)
		{
			for(j = 0; j < M; j+=8)
			{
				for(k = j; k < j+8; k++)
				{
					for(l = i; l < i+8; l++)
					{
						//can ignore diagonals...
						if(k != l)
						{
							B[l][k] = A[k][l];
						}
						else
						{
							temp = A[k][l];
							temp1 = k;
						}
					}

					//also ignore these diagonals...
					if(i == j)
					{
						B[temp1][temp1] = temp;
					}
				}
			}
		}
	}
	//64x64 - two levels of blocks now
	else if (N == 64)
	{
		//doing the same as above produces too many misses
		//we want large blocks as before, then blocks inside said blocks
		for(i = 0; i < N; i += 8)
		{
			for(j = 0; j < M; j += 8)
			{
				//first half of miniblocks
				for (k = i; k < i + 4; k++)
				{
					//storing into temps to be referenced in second half of miniblocks
					temp = A[k][j];
					temp1 = A[k][j+1];
					temp2 = A[k][j+2];
					temp3 = A[k][j+3];
					temp4 = A[k][j+4];
					temp5 = A[k][j+5];
					temp6 = A[k][j+6];
					temp7 = A[k][j+7];

					B[j][k] = temp;
					B[j+1][k] = temp1;
					B[j+2][k] = temp2;
					B[j+3][k] = temp3;
					B[j][k+4] = temp4;
					B[j+1][k+4] = temp5;
					B[j+2][k+4] = temp6;
					B[j+3][k+4] = temp7;
				}

				//second half of miniblocks
				for(k = j; k < j + 4; k++)
				{
					//store next four values in A into temp vars to be used later...
					temp4 = A[i+4][k];
					temp5 = A[i+5][k];
					temp6 = A[i+6][k];
					temp7 = A[i+7][k];

					//same with these values in B here...
					temp = B[k][i+4];
					temp1 = B[k][i+5];
					temp2 = B[k][i+6];
					temp3 = B[k][i+7];

					//set values in B to what they should be
					B[k][i+4] = temp4;
					B[k][i+5] = temp5;
					B[k][i+6] = temp6;
					B[k][i+7] = temp7;
					B[k+4][i] = temp;
					B[k+4][i+1] = temp1;
					B[k+4][i+2] = temp2;
					B[k+4][i+3] = temp3;

					//can get the next values in B straight from A (switch like normal)
					for(l = 0; l < 4; l++)
					{
						B[k+4][i+l+4] = A[i+l+4][k+4];
					}
				}
			}
		}
	}
	//other size - will use the 32x32 code b/c unsure of the size so its the safest option
	else
	{
		//same as the code for 32x32 except b/c the matrix isn't guaranteed to be square
		//we have to add another check while going through the blocks to make sure that
		//k & l don't go out of bounds
		for (i = 0; i < M; i += 16)
		{
			for (j = 0; j < N; j += 16)
			{
				for (k = j; (k < N) && (k < j + 16); k++)
				{
					for (l = i; (l < M) && (l < i + 16); l++)
					{
						//can ignore diagonals...
						if (k != l)
						{
							B[l][k] = A[k][l];
						}
						else
						{
							temp = A[k][l];
							temp1 = k;
						}
					}

					//also ignore these diagonals...
					if (i == j)
					{
						B[temp1][temp1] = temp;
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

