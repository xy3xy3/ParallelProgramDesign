#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int main() {
    double a, b, c;  // 一元二次方程的系数

    printf("请输入一元二次方程 ax² + bx + c = 0 的系数 a, b, c：\n");
    if (scanf("%lf %lf %lf", &a, &b, &c) != 3) {
        printf("输入格式错误！\n");
        return 1;
    }

    // 检查a是否为0（如果a=0，则不是一元二次方程）
    if (a == 0) {
        printf("错误：a不能为0（不是一元二次方程）\n");
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 计算判别式
    double discriminant = b * b - 4 * a * c;

    // 计算根
    double x1, x2;

    if (discriminant >= 0) {
        x1 = (-b + sqrt(discriminant)) / (2 * a);
        x2 = (-b - sqrt(discriminant)) / (2 * a);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_consumed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 输出结果
    printf("\n方程 %.2fx² + %.2fx + %.2f = 0\n", a, b, c);

    if (discriminant > 0) {
        printf("有两个不同的实根：\n");
        printf("x1 = %.6f\n", x1);
        printf("x2 = %.6f\n", x2);
    } else if (discriminant == 0) {
        printf("有一个二重实根：\n");
        printf("x1 = x2 = %.6f\n", x1);
    } else {
        // 复数根的情况
        double real_part = -b / (2 * a);
        double imag_part = sqrt(-discriminant) / (2 * a);
        printf("有两个共轭复根：\n");
        printf("x1 = %.6f + %.6fi\n", real_part, imag_part);
        printf("x2 = %.6f - %.6fi\n", real_part, imag_part);
    }

    printf("\n求解耗时：%.9f 秒\n", time_consumed);

    return 0;
}
