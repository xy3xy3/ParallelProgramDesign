#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// 全局变量用于共享中间计算结果及同步标志
double g_b2 = 0.0;       // 用于存放 b^2 的结果
double g_fourac = 0.0;   // 用于存放 4ac 的结果
double g_sqrtD = 0.0;    // 用于存放 sqrt(D) 的结果

int done_b2 = 0;       // 标志 b^2 计算是否完成
int done_fourac = 0;   // 标志 4ac 计算是否完成
int done_sqrt = 0;     // 标志 sqrt(D) 计算是否完成

// 同步互斥锁和条件变量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 线程函数：计算 b^2
void* compute_b2(void* arg) {
    double b = *(double*)arg;
    double temp = b * b;

    pthread_mutex_lock(&mutex);
    g_b2 = temp;
    done_b2 = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// 线程函数：计算 4ac
void* compute_fourac(void* arg) {
    double* coeffs = (double*)arg; // coeffs[0]=a, coeffs[1]=c
    double temp = 4 * coeffs[0] * coeffs[1];

    pthread_mutex_lock(&mutex);
    g_fourac = temp;
    done_fourac = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// 线程函数：计算 sqrt(D)（仅当 D>=0 时调用）
void* compute_sqrt(void* arg) {
    double D = *(double*)arg;
    double temp = sqrt(D);

    pthread_mutex_lock(&mutex);
    g_sqrtD = temp;
    done_sqrt = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    double a, b, c;
    printf("请输入一元二次方程 ax² + bx + c = 0 的系数 a, b, c：\n");
    if (scanf("%lf %lf %lf", &a, &b, &c) != 3) {
        printf("输入格式错误！\n");
        return EXIT_FAILURE;
    }
    if (a == 0) {
        printf("错误：a 不能为 0（这不是一元二次方程）\n");
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t thread_b2, thread_fourac, thread_sqrt;

    // 启动线程计算 b²，注意传入 b 的地址
    double b_copy = b; // 为防止直接传入全局变量产生竞争，这里复制一份
    if (pthread_create(&thread_b2, NULL, compute_b2, &b_copy) != 0) {
        perror("创建计算 b² 的线程失败");
        exit(EXIT_FAILURE);
    }

    // 启动线程计算 4ac，传入包含 a 和 c 的数组
    double ac_args[2] = {a, c};
    if (pthread_create(&thread_fourac, NULL, compute_fourac, ac_args) != 0) {
        perror("创建计算 4ac 的线程失败");
        exit(EXIT_FAILURE);
    }

    // 主线程等待 b² 和 4ac 计算完成
    pthread_mutex_lock(&mutex);
    while (!(done_b2 && done_fourac)) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // 计算判别式 D
    double D = g_b2 - g_fourac;

    // 若 D >= 0，启动线程计算 sqrt(D)
    if (D >= 0) {
        if (pthread_create(&thread_sqrt, NULL, compute_sqrt, &D) != 0) {
            perror("创建计算 sqrt(D) 的线程失败");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        while (!done_sqrt) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        pthread_join(thread_sqrt, NULL);
    }

    // 等待之前启动的线程结束
    pthread_join(thread_b2, NULL);
    pthread_join(thread_fourac, NULL);

    // 根据判别式判断根的情况并计算根值
    double x1, x2;
    if (D > 0) { // 两个不同的实根
        x1 = (-b + g_sqrtD) / (2 * a);
        x2 = (-b - g_sqrtD) / (2 * a);
    } else if (D == 0) { // 一个二重实根
        x1 = x2 = -b / (2 * a);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_consumed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 输出结果
    printf("\n方程 %.2fx² + %.2fx + %.2f = 0\n", a, b, c);
    if (D > 0) {
        printf("有两个不同的实根：\n");
        printf("x1 = %.6f\n", x1);
        printf("x2 = %.6f\n", x2);
    } else if (D == 0) {
        printf("有一个二重实根：\n");
        printf("x = %.6f\n", x1);
    } else { // D < 0，输出共轭复根
        double real_part = -b / (2 * a);
        double imag_part = sqrt(-D) / (2 * a);
        printf("有两个共轭复根：\n");
        printf("x1 = %.6f + %.6fi\n", real_part, imag_part);
        printf("x2 = %.6f - %.6fi\n", real_part, imag_part);
    }

    printf("\n求解耗时：%.9f 秒\n", time_consumed);

    return EXIT_SUCCESS;
}
