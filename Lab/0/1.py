#!/usr/bin/env python3
import sys
import time
import random

def generate_matrix(rows, cols):
    # 生成 rows x cols 随机矩阵（元素取值0～1浮点数）
    return [[random.random() for _ in range(cols)] for _ in range(rows)]

def matrix_multiply(A, B):
    m = len(A)
    n = len(A[0])
    p = len(B[0])
    # 初始化结果矩阵
    C = [[0.0 for _ in range(p)] for _ in range(m)]
    for i in range(m):
        for j in range(p):
            sum_val = 0.0
            for k in range(n):
                sum_val += A[i][k] * B[k][j]
            C[i][j] = sum_val
    return C

def main():
    if len(sys.argv) != 4:
        print("Usage: python matrix_mult.py <M> <N> <K>")
        sys.exit(1)
    M = int(sys.argv[1])
    N = int(sys.argv[2])
    K = int(sys.argv[3])
    A = generate_matrix(M, N)
    B = generate_matrix(N, K)
    start = time.time()
    C = matrix_multiply(A, B)
    end = time.time()

    # print("Matrix A:")
    # for row in A:
    #     print(row)
    # print("Matrix B:")
    # for row in B:
    #     print(row)
    # print("Matrix C:")
    # for row in C:
    #     print(row)
    print("Time taken: {:.6f} seconds".format(end - start))

if __name__ == "__main__":
    main()
