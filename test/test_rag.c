#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "../rag_config.h"

#define NVME_CMD_RAG_SEARCH 0xCA

#ifndef CONCURRENCY
#define CONCURRENCY 32          // 你可以改 8/16/32 去掃飽裝置平行度
#endif

static inline uint64_t nsec_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t*)a;
    uint64_t y = *(const uint64_t*)b;
    return (x > y) - (x < y);
}

typedef struct {
    int tid;
    int count;
    uint64_t *lat_ns;   // per-request latency for this thread (ns)
} worker_arg_t;

static void *worker(void *argp) {
    worker_arg_t *arg = (worker_arg_t*)argp;

    int fd = open("/dev/nvme1n1", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[T%d] open failed: %s\n", arg->tid, strerror(errno));
        return NULL;
    }

    void *buf = NULL;
    if (posix_memalign(&buf, 4096, RAG_RESULT_SIZE)) {
        fprintf(stderr, "[T%d] posix_memalign failed\n", arg->tid);
        close(fd);
        return NULL;
    }
    memset(buf, 0, RAG_RESULT_SIZE);

    struct nvme_passthru_cmd cmd = {
        .opcode = NVME_CMD_RAG_SEARCH,
        .nsid = 1,
        .addr = (unsigned long)buf,
        .data_len = RAG_RESULT_SIZE,
        .cdw10 = 0,
        .cdw11 = 0,
        .cdw12 = (RAG_RESULT_SIZE / 512) - 1,
        .timeout_ms = 600000,
    };

    for (int i = 0; i < arg->count; i++) {
        uint64_t t0 = nsec_now();
        int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
        uint64_t t1 = nsec_now();

        arg->lat_ns[i] = t1 - t0;

        if (ret < 0) {
            fprintf(stderr, "[T%d] ioctl failed at i=%d: %s\n",
                    arg->tid, i, strerror(errno));
            // 不中斷：繼續跑，讓你看統計時也能知道是否有錯誤
        }
    }

    free(buf);
    close(fd);
    return NULL;
}

int main() {
    // const int total = TEST_QUERY_COUNT;
    const int total = 100;
    const int conc  = CONCURRENCY;

    printf("TOTAL=%d, CONCURRENCY=%d, RAG_RESULT_SIZE=%d bytes\n",
           total, conc, (int)RAG_RESULT_SIZE);

    pthread_t th[CONCURRENCY];
    worker_arg_t args[CONCURRENCY];

    // 分配每個 thread 要跑的數量
    int base = total / conc;
    int rem  = total % conc;

    // 每個 thread 各自的 latency array
    for (int t = 0; t < conc; t++) {
        args[t].tid = t;
        args[t].count = base + (t < rem ? 1 : 0);
        args[t].lat_ns = (uint64_t*)calloc(args[t].count, sizeof(uint64_t));
        if (!args[t].lat_ns) {
            fprintf(stderr, "calloc failed\n");
            return 1;
        }
    }

    uint64_t start = nsec_now();
    for (int t = 0; t < conc; t++) {
        pthread_create(&th[t], NULL, worker, &args[t]);
    }
    for (int t = 0; t < conc; t++) {
        pthread_join(th[t], NULL);
    }
    uint64_t end = nsec_now();

    // 收集全部 latency
    uint64_t *all = (uint64_t*)malloc((size_t)total * sizeof(uint64_t));
    if (!all) return 1;

    int k = 0;
    for (int t = 0; t < conc; t++) {
        for (int i = 0; i < args[t].count; i++) {
            all[k++] = args[t].lat_ns[i];
        }
    }

    qsort(all, total, sizeof(uint64_t), cmp_u64);

    double wall_s = (double)(end - start) / 1e9;
    double iops = (double)total / wall_s;

    // 平均 latency
    long double sum = 0;
    for (int i = 0; i < total; i++) sum += (long double)all[i];
    double avg_ms = (double)(sum / (long double)total) / 1e6;

    // percentiles
    int p50 = (int)(0.50 * (total - 1));
    int p95 = (int)(0.95 * (total - 1));
    int p99 = (int)(0.99 * (total - 1));

    printf("Wall time: %.3f s\n", wall_s);
    printf("IOPS: %.2f\n", iops);
    printf("Latency avg: %.3f ms\n", avg_ms);
    printf("Latency p50: %.3f ms, p95: %.3f ms, p99: %.3f ms\n",
           (double)all[p50] / 1e6, (double)all[p95] / 1e6, (double)all[p99] / 1e6);

    for (int t = 0; t < conc; t++) free(args[t].lat_ns);
    free(all);
    return 0;
}
