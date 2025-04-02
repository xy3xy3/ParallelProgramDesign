#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

// 矩阵下标宏定义（行优先存储）
#define IDX(i, j, cols) ((i) * (cols) + (j))

// 全局变量
int thread_count, m, n, k;
double *A, *B, *C;

typedef struct {
    int row_start;
    int row_end;   // 不包括 row_end
    int col_start;
    int col_end;   // 不包括 col_end
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* targ = (ThreadArg*) arg;
    for (int i = targ->row_start; i < targ->row_end; i++) {
        for (int j = targ->col_start; j < targ->col_end; j++) {
            double sum = 0.0;
            for (int l = 0; l < n; l++) {
                sum += A[IDX(i, l, n)] * B[IDX(l, j, k)];
            }
            C[IDX(i, j, k)] = sum;
        }
    }
    return NULL;
}

// 辅助函数：计算行块数和列块数，使得 row_blocks * col_blocks == thread_count
void compute_grid(int thread_count, int *row_blocks, int *col_blocks) {
    int r = (int)sqrt(thread_count);
    while (r > 0) {
        if (thread_count % r == 0) {
            *row_blocks = r;
            *col_blocks = thread_count / r;
            return;
        }
        r--;
    }
    *row_blocks = 1;
    *col_blocks = thread_count;
}

int main() {
    printf("请输入线程数 m n k（线程数范围1-16，矩阵维度范围128~2048）：\n");
    fflush(stdout);
    if (scanf("%d %d %d %d", &thread_count, &m, &n, &k) != 4) {
        printf("输入格式错误！\n");
        return 1;
    }

    // 分配内存
    A = (double*)malloc(m * n * sizeof(double));
    B = (double*)malloc(n * k * sizeof(double));
    C = (double*)malloc(m * k * sizeof(double));

    // 初始化矩阵A和B，取值范围为0-9
    srand((unsigned)time(NULL));
    for (int i = 0; i < m * n; i++) {
        A[i] = (double)(rand() % 10);
    }
    for (int i = 0; i < n * k; i++) {
        B[i] = (double)(rand() % 10);
    }

    pthread_t threads[thread_count];
    ThreadArg args[thread_count];

    // 计算行块数和列块数
    int row_blocks, col_blocks;
    compute_grid(thread_count, &row_blocks, &col_blocks);

    // 计算每个块的行数和列数（处理余数）
    int base_rows = m / row_blocks;
    int row_remainder = m % row_blocks;
    int base_cols = k / col_blocks;
    int col_remainder = k % col_blocks;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 为每个块创建一个线程
    int thread_id = 0;
    int current_row = 0;
    for (int i = 0; i < row_blocks; i++) {
        int extra_row = (i < row_remainder) ? 1 : 0;
        int block_rows = base_rows + extra_row;
        int current_col = 0;
        for (int j = 0; j < col_blocks; j++) {
            int extra_col = (j < col_remainder) ? 1 : 0;
            int block_cols = base_cols + extra_col;
            args[thread_id].row_start = current_row;
            args[thread_id].row_end = current_row + block_rows;
            args[thread_id].col_start = current_col;
            args[thread_id].col_end = current_col + block_cols;
            pthread_create(&threads[thread_id], NULL, thread_func, (void*)&args[thread_id]);
            thread_id++;
            current_col += block_cols;
        }
        current_row += block_rows;
    }

    // 等待所有线程完成
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double time_consumed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("\n矩阵乘法计算耗时：%f 秒\n", time_consumed);

    // 如果矩阵较小，则打印矩阵
    if (m <= 10 && n <= 10 && k <= 10) {
        printf("矩阵A:\n");
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                printf("%6.2f ", A[IDX(i, j, n)]);
            }
            printf("\n");
        }
        printf("\n");

        printf("矩阵B:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < k; j++) {
                printf("%6.2f ", B[IDX(i, j, k)]);
            }
            printf("\n");
        }
        printf("\n");

        printf("矩阵C:\n");
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < k; j++) {
                printf("%6.2f ", C[IDX(i, j, k)]);
            }
            printf("\n");
        }
        printf("\n");
    }

    free(A);
    free(B);
    free(C);
    return 0;
}
