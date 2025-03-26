#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <math.h>

#define IDX(i, j, cols) ((i) * (cols) + (j))

// 打印矩阵函数
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
    int rank, size;
    double *A = NULL, *B = NULL, *C = NULL;
    double start, finish;
    int m, n, k;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 检查进程数是否为完全平方数
    int sqrt_p = (int)sqrt(size);
    if (sqrt_p * sqrt_p != size) {
        if (rank == 0) printf("错误：进程数必须为完全平方数（如1、4、9、16）！\n");
        MPI_Finalize();
        return 1;
    }

    // 进程0获取输入并广播矩阵维度
    if (rank == 0) {
        printf("请输入矩阵A的行数 m、A的列数 n、B的列数 k（取值范围128~2048）：\n");
        fflush(stdout);
        scanf("%d %d %d", &m, &n, &k);
    }
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 计算每个进程负责的块大小（假设m、n、k能被sqrt_p整除）
    int block_m = m / sqrt_p;  // 每个块的行数
    int block_n = n / sqrt_p;  // A的块列数，B的块行数
    int block_k = k / sqrt_p;  // 每个块的列数

    // 计算进程在二维网格中的坐标
    int row = rank / sqrt_p;  // 行索引
    int col = rank % sqrt_p;  // 列索引

    // 分配局部矩阵
    double *local_A = (double*)malloc(block_m * block_n * sizeof(double));
    double *local_B = (double*)malloc(block_n * block_k * sizeof(double));
    double *local_C = (double*)malloc(block_m * block_k * sizeof(double));

    // 进程0初始化完整矩阵A和B
    if (rank == 0) {
        A = (double*)malloc(m * n * sizeof(double));
        B = (double*)malloc(n * k * sizeof(double));
        srand(time(NULL));
        for (int i = 0; i < m * n; i++) A[i] = (double)(rand() % 10);
        for (int i = 0; i < n * k; i++) B[i] = (double)(rand() % 10);
    }

    // 分发矩阵A和B的块
    if (rank == 0) {
        for (int i = 0; i < sqrt_p; i++) {
            for (int j = 0; j < sqrt_p; j++) {
                int proc = i * sqrt_p + j;
                // 填充local_A和local_B
                for (int ii = 0; ii < block_m; ii++) {
                    for (int jj = 0; jj < block_n; jj++) {
                        local_A[ii * block_n + jj] = A[(i * block_m + ii) * n + (j * block_n + jj)];
                    }
                }
                for (int ii = 0; ii < block_n; ii++) {
                    for (int jj = 0; jj < block_k; jj++) {
                        local_B[ii * block_k + jj] = B[(i * block_n + ii) * k + (j * block_k + jj)];
                    }
                }
                // 如果不是进程0，发送数据
                if (proc != 0) {
                    MPI_Send(local_A, block_m * block_n, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD);
                    MPI_Send(local_B, block_n * block_k, MPI_DOUBLE, proc, 1, MPI_COMM_WORLD);
                }
            }
        }
    } else {
        // 其他进程接收数据
        MPI_Recv(local_A, block_m * block_n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_B, block_n * block_k, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // 同步并开始计时
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    // 本地矩阵乘法
    for (int i = 0; i < block_m; i++) {
        for (int j = 0; j < block_k; j++) {
            local_C[i * block_k + j] = 0.0;
            for (int l = 0; l < block_n; l++) {
                local_C[i * block_k + j] += local_A[i * block_n + l] * local_B[l * block_k + j];
            }
        }
    }

    finish = MPI_Wtime();
    double local_time = finish - start;
    double max_time;

    // 收集最大计算时间
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // 收集结果到进程0
    if (rank == 0) {
        C = (double*)malloc(m * k * sizeof(double));
    }
    if (rank != 0) {
        MPI_Send(local_C, block_m * block_k, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    } else {
        // 进程0先填入自己的块
        for (int i = 0; i < block_m; i++) {
            for (int j = 0; j < block_k; j++) {
                C[i * k + j] = local_C[i * block_k + j];
            }
        }
        // 接收其他进程的块
        for (int proc = 1; proc < size; proc++) {
            double *temp_C = (double*)malloc(block_m * block_k * sizeof(double));
            MPI_Recv(temp_C, block_m * block_k, MPI_DOUBLE, proc, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int r = proc / sqrt_p;
            int c = proc % sqrt_p;
            for (int i = 0; i < block_m; i++) {
                for (int j = 0; j < block_k; j++) {
                    C[(r * block_m + i) * k + (c * block_k + j)] = temp_C[i * block_k + j];
                }
            }
            free(temp_C);
        }
    }

    // 进程0输出结果
    if (rank == 0) {
        printf("\n矩阵乘法计算耗时：%f 秒\n", max_time);
        // 打印小矩阵结果
        if (m <= 10 && n <= 10 && k <= 10) {
            print_matrix(A, m, n, "矩阵A");
            print_matrix(B, n, k, "矩阵B");
            print_matrix(C, m, k, "结果矩阵C");
        }
    }

    // 释放内存
    free(local_A);
    free(local_B);
    free(local_C);
    if (rank == 0) {
        free(A);
        free(B);
        free(C);
    }

    MPI_Finalize();
    return 0;
}