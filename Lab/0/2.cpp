#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
using namespace std;

typedef vector<vector<double>> Matrix;

// 生成随机矩阵
Matrix generate_matrix(int rows, int cols) {
    Matrix mat(rows, vector<double>(cols));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            mat[i][j] = static_cast<double>(rand()) / RAND_MAX;
    return mat;
}

// 传统三重循环实现矩阵乘法
Matrix matrix_multiply(const Matrix &A, const Matrix &B) {
    int m = A.size();
    int n = A[0].size();
    int p = B[0].size();
    Matrix C(m, vector<double>(p, 0.0));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
    return C;
}

int main(int argc, char* argv[]){
    if(argc != 4){
        cout << "Usage: " << argv[0] << " <M> <N> <K>" << endl;
        return 1;
    }
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);

    Matrix A = generate_matrix(M, N);
    Matrix B = generate_matrix(N, K);
    auto start = std::chrono::high_resolution_clock::now();
    Matrix C = matrix_multiply(A, B);
    auto end = std::chrono::high_resolution_clock::now();

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
    double time_taken = std::chrono::duration<double>(end - start).count();
    cout << "Time taken: " << time_taken << " seconds" << endl;
    return 0;
}
