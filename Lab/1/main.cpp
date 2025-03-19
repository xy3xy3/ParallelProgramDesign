#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define IDX(i, j, cols) ((i) * (cols) + (j))

// 打印矩阵
void print_matrix(double *matrix, int rows, int cols, const char *name) {
    printf("%s:\n", name);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%6.2f ", matrix[IDX(i, j, cols)]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int rank, size, m, n, k;
    double *A = NULL, *B = NULL, *C = NULL;
    double start, finish;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 进程0获取用户输入并广播
    if (rank == 0) {
        printf("请输入矩阵A的行数 m、A的列数 n、B的列数 k（取值范围128~2048）：\n");
        fflush(stdout);
        scanf("%d %d %d", &m, &n, &k);
    }
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 进程0分配B并初始化，然后广播给所有进程
    if (rank == 0) {
        B = (double*)malloc(n * k * sizeof(double));
        srand(time(NULL));
        for (int i = 0; i < n * k; i++) {
            B[i] = (double)(rand() % 10);
        }
    } else {
        B = (double*)malloc(n * k * sizeof(double));
    }
    MPI_Bcast(B, n * k, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 计算每个进程负责的行数
    int rows_per_proc = m / size;
    int remainder = m % size;
    int local_rows = (rank < remainder) ? rows_per_proc + 1 : rows_per_proc;
    int start_row = (rank < remainder) ? rank * (rows_per_proc + 1) : rank * rows_per_proc + remainder;

    // 分配局部矩阵
    double *local_A = (double*)malloc(local_rows * n * sizeof(double));
    double *local_C = (double*)malloc(local_rows * k * sizeof(double));

    // 进程0 生成 A，并使用 MPI_Scatterv 分发
    if (rank == 0) {
        A = (double*)malloc(m * n * sizeof(double));
        for (int i = 0; i < m * n; i++) {
            A[i] = (double)(rand() % 10);
        }
    }
    int send_counts[size], displs[size];
    for (int i = 0; i < size; i++) {
        send_counts[i] = ((i < remainder) ? (rows_per_proc + 1) : rows_per_proc) * n;
        displs[i] = (i == 0) ? 0 : displs[i - 1] + send_counts[i - 1];
    }
    MPI_Scatterv(A, send_counts, displs, MPI_DOUBLE, local_A, local_rows * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 计算局部矩阵乘法
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < k; j++) {
            local_C[i * k + j] = 0.0;
            for (int l = 0; l < n; l++) {
                local_C[i * k + j] += local_A[i * n + l] * B[l * k + j];
            }
        }
    }
    finish = MPI_Wtime();
    double local_time = finish - start;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // 进程0 收集所有计算结果
    if (rank == 0) {
        C = (double*)malloc(m * k * sizeof(double));
    }
    MPI_Gatherv(local_C, local_rows * k, MPI_DOUBLE, C, send_counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 进程0 输出结果
    if (rank == 0) {
        printf("\n矩阵乘法计算耗时：%f 秒\n", max_time);
    }

    // 释放内存
    free(B);
    free(local_A);
    free(local_C);
    if (rank == 0) {
        free(A);
        free(C);
    }

    MPI_Finalize();
    return 0;
}
