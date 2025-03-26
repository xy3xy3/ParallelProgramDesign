#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define IDX(i, j, cols) ((i) * (cols) + (j))

// 定义聚合数据结构体，包含矩阵维度信息
typedef struct {
    int m;
    int n;
    int k;
} MatrixDims;

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
    MatrixDims dims;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 创建自定义MPI数据类型用于聚合通信
    MPI_Datatype mpi_matrix_dims;
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint displacements[3];

    // 计算结构体成员偏移量
    MPI_Aint base_address;
    MPI_Get_address(&dims, &base_address);
    MPI_Get_address(&dims.m, &displacements[0]);
    MPI_Get_address(&dims.n, &displacements[1]);
    MPI_Get_address(&dims.k, &displacements[2]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);

    // 创建并提交自定义数据类型
    MPI_Type_create_struct(3, blocklengths, displacements, types, &mpi_matrix_dims);
    MPI_Type_commit(&mpi_matrix_dims);

    // 进程0获取用户输入并使用自定义数据类型广播
    if (rank == 0) {
        printf("请输入矩阵A的行数 m、A的列数 n、B的列数 k（取值范围128~2048）：\n");
        fflush(stdout);
        scanf("%d %d %d", &dims.m, &dims.n, &dims.k);
    }
    MPI_Bcast(&dims, 1, mpi_matrix_dims, 0, MPI_COMM_WORLD);

    // 进程0分配B并初始化，然后使用广播发送给所有进程
    if (rank == 0) {
        B = (double*)malloc(dims.n * dims.k * sizeof(double));
        srand(time(NULL));
        for (int i = 0; i < dims.n * dims.k; i++) {
            B[i] = (double)(rand() % 10);
        }
    } else {
        B = (double*)malloc(dims.n * dims.k * sizeof(double));
    }
    MPI_Bcast(B, dims.n * dims.k, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 计算任务划分：按行块划分，考虑负载均衡
    int rows_per_proc = dims.m / size;
    int remainder = dims.m % size;
    int local_rows = (rank < remainder) ? rows_per_proc + 1 : rows_per_proc;
    int start_row = (rank < remainder) ? rank * (rows_per_proc + 1) : rank * rows_per_proc + remainder;

    // 分配局部矩阵
    double *local_A = (double*)malloc(local_rows * dims.n * sizeof(double));
    double *local_C = (double*)malloc(local_rows * dims.k * sizeof(double));

    // 进程0生成完整矩阵A，使用Scatterv分发
    if (rank == 0) {
        A = (double*)malloc(dims.m * dims.n * sizeof(double));
        for (int i = 0; i < dims.m * dims.n; i++) {
            A[i] = (double)(rand() % 10);
        }
    }

    // 准备Scatterv参数
    int send_counts[size], displs[size];
    for (int i = 0; i < size; i++) {
        send_counts[i] = ((i < remainder) ? (rows_per_proc + 1) : rows_per_proc) * dims.n;
        displs[i] = (i == 0) ? 0 : displs[i - 1] + send_counts[i - 1];
    }

    // 使用集合通信Scatterv分发数据
    MPI_Scatterv(A, send_counts, displs, MPI_DOUBLE,
                local_A, local_rows * dims.n, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    // 矩阵乘法计算 - 使用缓存优化
    MPI_Barrier(MPI_COMM_WORLD); // 同步所有进程
    start = MPI_Wtime();

    // 优化：改变循环顺序提高缓存命中率
    for (int i = 0; i < local_rows; i++) {
        for (int l = 0; l < dims.n; l++) {
            double temp = local_A[i * dims.n + l];
            for (int j = 0; j < dims.k; j++) {
                local_C[i * dims.k + j] += temp * B[l * dims.k + j];
            }
        }
    }

    finish = MPI_Wtime();
    double local_time = finish - start;
    double max_time;

    // 使用Reduce收集最大计算时间
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // 使用Gatherv收集结果
    if (rank == 0) {
        C = (double*)malloc(dims.m * dims.k * sizeof(double));
    }

    // 调整send_counts为输出矩阵的大小
    for (int i = 0; i < size; i++) {
        send_counts[i] = ((i < remainder) ? (rows_per_proc + 1) : rows_per_proc) * dims.k;
        displs[i] = (i == 0) ? 0 : displs[i - 1] + send_counts[i - 1];
    }

    MPI_Gatherv(local_C, local_rows * dims.k, MPI_DOUBLE,
               C, send_counts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    // 进程0输出结果
    if (rank == 0) {
        printf("\n矩阵乘法计算耗时：%f 秒\n", max_time);

        // 可选：打印小矩阵结果
        if (dims.m <= 10 && dims.n <= 10 && dims.k <= 10) {
            print_matrix(A, dims.m, dims.n, "矩阵A");
            print_matrix(B, dims.n, dims.k, "矩阵B");
            print_matrix(C, dims.m, dims.k, "结果矩阵C");
        }
    }

    // 释放资源
    free(B);
    free(local_A);
    free(local_C);
    if (rank == 0) {
        free(A);
        free(C);
    }

    MPI_Type_free(&mpi_matrix_dims);
    MPI_Finalize();
    return 0;
}
