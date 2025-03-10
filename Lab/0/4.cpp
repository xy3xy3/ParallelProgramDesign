#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cblas.h>  // 开源 BLAS 库的头文件
#include <chrono>
using namespace std;

typedef vector<vector<double>> Matrix;

// 生成随机矩阵（元素范围 [0,1)）
Matrix generate_matrix(int rows, int cols) {
    Matrix mat(rows, vector<double>(cols));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            mat[i][j] = static_cast<double>(rand()) / RAND_MAX;
    return mat;
}

// 将二维矩阵转换为一维数组（行优先顺序）
vector<double> convert_to_array(const Matrix &mat) {
    int rows = mat.size();
    int cols = mat[0].size();
    vector<double> array(rows * cols);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            array[i * cols + j] = mat[i][j];
    return array;
}

// 将一维数组转换回二维矩阵
Matrix convert_to_matrix(const vector<double> &arr, int rows, int cols) {
    Matrix mat(rows, vector<double>(cols));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            mat[i][j] = arr[i * cols + j];
    return mat;
}

int main(int argc, char* argv[]){
    if(argc != 4){
        cout << "Usage: " << argv[0] << " <M> <N> <K>" << endl;
        return 1;
    }

    int M = atoi(argv[1]);  // A的行数
    int N = atoi(argv[2]);  // A的列数，同时也是B的行数
    int K = atoi(argv[3]);  // B的列数


    // 生成随机矩阵 A (MxN) 和 B (NxK)
    Matrix A = generate_matrix(M, N);
    Matrix B = generate_matrix(N, K);

    // 转换为一维数组格式
    vector<double> A_arr = convert_to_array(A);
    vector<double> B_arr = convert_to_array(B);
    vector<double> C_arr(M * K, 0.0);

    auto start = std::chrono::high_resolution_clock::now();

    // 使用 BLAS 的 cblas_dgemm 进行矩阵乘法
    // C = alpha * A * B + beta * C, 这里 alpha = 1.0, beta = 0.0
    // 参数说明：矩阵存储采用 row-major，A 为 MxN, B 为 NxK, C 为 MxK
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                M, K, N, 1.0, A_arr.data(), N, B_arr.data(), K, 0.0, C_arr.data(), K);

    auto end = std::chrono::high_resolution_clock::now();
    double time_taken = std::chrono::duration<double>(end - start).count();

    // 将计算结果转换回二维矩阵
    Matrix C = convert_to_matrix(C_arr, M, K);

    // 输出矩阵
    // cout << "Matrix A:" << endl;
    // for (auto &row : A) {
    //     for (auto &val : row)
    //         cout << val << " ";
    //     cout << endl;
    // }

    // cout << "Matrix B:" << endl;
    // for (auto &row : B) {
    //     for (auto &val : row)
    //         cout << val << " ";
    //     cout << endl;
    // }

    // cout << "Matrix C:" << endl;
    // for (auto &row : C) {
    //     for (auto &val : row)
    //         cout << val << " ";
    //     cout << endl;
    // }

    cout << "Time taken: " << time_taken << " seconds" << endl;
    return 0;
}
