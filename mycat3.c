#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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

long io_blocksize() {
    return page_size;
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

    long buffer_size = io_blocksize();
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