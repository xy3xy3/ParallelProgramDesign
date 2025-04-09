#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

// 全局变量，用于在线程之间共享信息
long long total_points = 0;     // 采样总点数
int thread_count = 1;          // 线程数
long long circle_count_global = 0; // 所有线程圆内点计数之和
FILE* fp_out = NULL;           // 输出文件指针，用于写点坐标和是否在圆内

pthread_mutex_t mutex;         // 用于更新全局圆内计数和文件输出的互斥锁

// 线程函数参数
typedef struct {
    int tid;        // 线程id
} ThreadData;

// 在圆内返回1，否则返回0
int is_in_circle(double x, double y) {
    return (x*x + y*y <= 1.0) ? 1 : 0;
}

// 线程函数：生成随机点并统计落入圆内的数量，将数据写入文件
void* montecarlo_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int tid = data->tid;

    // 为了避免不同线程使用同样的随机种子，这里结合线程id等做一个简单处理
    unsigned int seed = (unsigned int)time(NULL) ^ (tid * 131542391);

    // 计算本线程需要采样的点数
    long long points_per_thread = total_points / thread_count;
    // 若不能整除，处理一下余数(仅让0号线程多分几次)
    if (tid == 0) {
        points_per_thread += (total_points % thread_count);
    }

    // 局部计数器
    long long local_in_circle = 0;

    // 创建本地缓冲区，减少互斥锁的使用次数
    const int BUFFER_SIZE = 1000; // 每1000个点批量写入一次文件
    double x_buffer[BUFFER_SIZE];
    double y_buffer[BUFFER_SIZE];
    int in_circle_buffer[BUFFER_SIZE];
    int buffer_count = 0;

    // 逐点生成
    for(long long i = 0; i < points_per_thread; i++){
        // 生成 [0,1) 区间的随机数
        double x = (double)rand_r(&seed) / (double)RAND_MAX;
        double y = (double)rand_r(&seed) / (double)RAND_MAX;
        int in_c = is_in_circle(x, y);

        // 添加到本地缓冲区
        x_buffer[buffer_count] = x;
        y_buffer[buffer_count] = y;
        in_circle_buffer[buffer_count] = in_c;
        buffer_count++;

        // 统计圆内点
        local_in_circle += in_c;

        // 当缓冲区满时，批量写入文件
        if (buffer_count == BUFFER_SIZE || i == points_per_thread - 1) {
            pthread_mutex_lock(&mutex);
            for (int j = 0; j < buffer_count; j++) {
                // 输出格式: x y in_circle
                fprintf(fp_out, "%.6f %.6f %d\n", x_buffer[j], y_buffer[j], in_circle_buffer[j]);
            }
            pthread_mutex_unlock(&mutex);
            buffer_count = 0; // 重置缓冲区计数
        }
    }

    // 累加到全局计数
    pthread_mutex_lock(&mutex);
    circle_count_global += local_in_circle;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("用法：%s <采样总点数> <线程数>\n", argv[0]);
        return -1;
    }

    total_points = atoll(argv[1]);
    thread_count = atoi(argv[2]);

    if (total_points < 1024 || total_points > 65536) {
        printf("采样点数应在 [1024, 65536] 范围内\n");
        return -1;
    }
    if (thread_count <= 0) {
        printf("线程数应为正整数\n");
        return -1;
    }

    // 打开输出文件
    fp_out = fopen("montecarlo_points.txt", "w");
    if (!fp_out) {
        perror("无法创建输出文件montecarlo_points.txt");
        return -1;
    }

    // 初始化互斥锁
    pthread_mutex_init(&mutex, NULL);

    // 计时开始
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 创建线程
    pthread_t* threads = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    ThreadData* thread_data = (ThreadData*)malloc(thread_count * sizeof(ThreadData));

    for(int i=0; i<thread_count; i++){
        thread_data[i].tid = i;
        if (pthread_create(&threads[i], NULL, montecarlo_thread, &thread_data[i]) != 0) {
            perror("pthread_create失败");
            return -1;
        }
    }

    // 等待所有线程完成
    for(int i=0; i<thread_count; i++){
        pthread_join(threads[i], NULL);
    }

    // 关闭文件
    fclose(fp_out);

    // 计时结束
    gettimeofday(&end, NULL);
    double time_consumed = (end.tv_sec - start.tv_sec) +
                           (end.tv_usec - start.tv_usec) / 1e6;

    // 计算近似 pi
    double pi_est = 4.0 * (double)circle_count_global / (double)total_points;

    // 输出结果
    printf("总采样点数: %lld\n", total_points);
    printf("线程数: %d\n", thread_count);
    printf("落在圆内的点数: %lld\n", circle_count_global);
    printf("估算的pi值: %.6f\n", pi_est);
    printf("求解耗时: %.6f 秒\n", time_consumed);

    // 释放资源
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);

    return 0;
}
