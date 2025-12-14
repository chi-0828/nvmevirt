/* test_rag.c */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// [手動同步區] 必須與 ssd.c 一致
#define K_TOP_RESULTS       5
#define CHUNK_SIZE_BYTES    4096 
// #define RAG_RESULT_SIZE     (K_TOP_RESULTS * CHUNK_SIZE_BYTES) // 20480
#define RAG_RESULT_SIZE     4096
int main() {
    int fd;
    void *buf;
    struct timespec start, end;
    long elapsed_ns;

    // 開啟虛擬 SSD (請確認是 /dev/nvme1n1)
    fd = open("/dev/nvme1n1", O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("Open SSD failed (Check device path!)");
        return 1;
    }

    // 記憶體對齊 (O_DIRECT 要求)
    // 雖然我們要讀 20480 bytes，但 buffer 最好對齊到 4KB (4096)
    if (posix_memalign(&buf, 4096, RAG_RESULT_SIZE)) {
        perror("Memory allocation failed");
        close(fd);
        return 1;
    }

    printf("Sending RAG Request (Size: %d bytes)...\n", RAG_RESULT_SIZE);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // 發送請求
    int ret = pread(fd, buf, RAG_RESULT_SIZE, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (ret < 0) {
        perror("Read failed");
    } else {
        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        printf("Success! Time: %.3f ms (%ld ns)\n", elapsed_ns / 1000000.0, elapsed_ns);
    }

    free(buf);
    close(fd);
    return 0;
}