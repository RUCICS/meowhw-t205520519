#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

// 根据任务5的实验结果，将缓冲区大小固定为最优值 256KB
#define OPTIMAL_BUFFER_SIZE (256 * 1024)

long page_size;

void init_page_size() {
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf");
        page_size = 4096; // Fallback
    }
}

void* align_alloc(size_t size) {
    void *original_ptr;
    void *aligned_ptr;
    size_t total_size = size + page_size - 1 + sizeof(void*);
    
    original_ptr = malloc(total_size);
    if (original_ptr == NULL) {
        return NULL;
    }

    uintptr_t aligned_addr = ((uintptr_t)original_ptr + sizeof(void*) + page_size - 1) & ~(page_size - 1);
    aligned_ptr = (void*)aligned_addr;
    
    *((void**)(aligned_ptr - sizeof(void*))) = original_ptr;
    
    return aligned_ptr;
}

void align_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    void* original_ptr = *((void**)(ptr - sizeof(void*)));
    free(original_ptr);
}

int main(int argc, char *argv[]) {
    init_page_size();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 直接使用实验得出的最优缓冲区大小
    long buffer_size = OPTIMAL_BUFFER_SIZE;
    char *buffer = (char *)align_alloc(buffer_size);
    if (buffer == NULL) {
        perror("align_alloc");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        ssize_t bytes_written = write(STDOUT_FILENO, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write");
            align_free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("read");
        align_free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }

    align_free(buffer);
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return 0;
} 