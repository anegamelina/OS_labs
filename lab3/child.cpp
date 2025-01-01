#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_NAME "/my_shm"
#define SEM_EMPTY_NAME "/empty_sem"
#define SEM_FULL_NAME "/full_sem"

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("Error opening shared memory");
        exit(EXIT_FAILURE);
    }

    size_t shm_size = 1024 * 1024;
    char *shm_ptr = (char *)mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_empty = sem_open(SEM_EMPTY_NAME, 0);
    if (sem_empty == SEM_FAILED) {
        perror("Error opening empty semaphore");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_full = sem_open(SEM_FULL_NAME, 0);
    if (sem_full == SEM_FAILED) {
        perror("Error opening full semaphore");
        exit(EXIT_FAILURE);
    }

    while (true) {
        sem_wait(sem_full);

        ssize_t read_bytes = strlen(shm_ptr);
        if (read_bytes == 0) {
            break;
        }

        char *line = strtok(shm_ptr, "\n");
        while (line) {
            char *token = strtok(line, " ");
            float sum = 0;
            bool valid_input = true;

            while (token) {
                char *endptr;
                float number = strtof(token, &endptr);
                if (endptr == token || *endptr != '\0') {
                    valid_input = false;
                    break;
                }
                sum += number;
                token = strtok(nullptr, " ");
            }

            if (valid_input) {
                snprintf(shm_ptr, shm_size, "Sum of elements: %.2f\n", sum);
            } else {
                snprintf(shm_ptr, shm_size, "Invalid input. Please enter numbers only.\n");
            }

            sem_post(sem_empty);

            line = strtok(nullptr, "\n");
        }
    }

    munmap(shm_ptr, shm_size);
    close(shm_fd);
    sem_close(sem_empty);
    sem_close(sem_full);

    return 0;
}