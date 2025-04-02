#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

int thread_count, n;
int *A;
long long global_sum = 0;
pthread_mutex_t mutex;

typedef struct {
    int start;
    int end; // 不包括 end
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* targ = (ThreadArg*) arg;
    long long partial_sum = 0;
    for (int i = targ->start; i < targ->end; i++) {
        partial_sum += A[i];
    }
    // 使用互斥锁更新全局和
    pthread_mutex_lock(&mutex);
    global_sum += partial_sum;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    printf("请输入线程数和数组长度 n（数组规模范围1M~128M）：\n");
    if (scanf("%d %d", &thread_count, &n) != 2) {
        printf("输入格式错误！\n");
        return 1;
    }

    A = (int*) malloc(n * sizeof(int));
    if (A == NULL) {
        printf("内存分配失败！\n");
        return 1;
    }

    // 随机初始化数组A（取值0~9）
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        A[i] = rand() % 10;
    }

    pthread_t threads[thread_count];
    ThreadArg args[thread_count];

    pthread_mutex_init(&mutex, NULL);

    int chunk = n / thread_count;
    int remainder = n % thread_count;
    int current = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < thread_count; i++) {
        args[i].start = current;
        args[i].end = current + chunk + (i < remainder ? 1 : 0);
        current = args[i].end;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double time_consumed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    printf("数组求和结果：%lld\n", global_sum);
    printf("计算耗时：%f 秒\n", time_consumed);

    pthread_mutex_destroy(&mutex);
    free(A);
    return 0;
}
