#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

// 矩阵下标宏定义（行优先存储）
#define IDX(i, j, cols) ((i) * (cols) + (j))

// 全局变量
int thread_count, m, n, k;
double *A, *B, *C;

typedef struct {
    int start_row;
    int end_row; // 不包括 end_row
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* targ = (ThreadArg*) arg;
    for (int i = targ->start_row; i < targ->end_row; i++) {
        for (int j = 0; j < k; j++) {
            double sum = 0.0;
            for (int l = 0; l < n; l++) {
                sum += A[IDX(i, l, n)] * B[IDX(l, j, k)];
            }
            C[IDX(i, j, k)] = sum;
        }
    }
    return NULL;
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

    // 计算每个线程负责的行数（考虑不整除情况）
    int rows_per_thread = m / thread_count;
    int remainder = m % thread_count;
    int current_row = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 创建线程
    for (int i = 0; i < thread_count; i++) {
        args[i].start_row = current_row;
        args[i].end_row = current_row + rows_per_thread + (i < remainder ? 1 : 0);
        current_row = args[i].end_row;
        pthread_create(&threads[i], NULL, thread_func, (void*)&args[i]);
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
