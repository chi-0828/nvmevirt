#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <string.h>
#include "../rag_config.h"

#define NVME_CMD_RAG_SEARCH 0xCA
// #define RAG_RESULT_SIZE     20480 // 20KB

int main() {
    int fd = open("/dev/nvme1n1", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    void *buf;
    if (posix_memalign(&buf, 4096, RAG_RESULT_SIZE)) return 1;
    memset(buf, 0, RAG_RESULT_SIZE);

    struct nvme_passthru_cmd cmd = {
        .opcode = NVME_CMD_RAG_SEARCH, // 0xCA
        .nsid = 1,
        .addr = (unsigned long)buf,
        .data_len = RAG_RESULT_SIZE,
        .cdw10 = 0,
        .cdw11 = 0,
        .cdw12 = (RAG_RESULT_SIZE / 512) - 1, // 告訴 Driver 資料長度
        .timeout_ms = 60000,
    };

    printf("Sending RAG IOCTL (Opcode 0xCA)...\n");
    if (ioctl(fd, NVME_IOCTL_IO_CMD, &cmd) < 0) {
        perror("ioctl failed");
    } else {
        printf("Success! RAG Command sent.\n");
    }

    close(fd);
    free(buf);
    return 0;
}