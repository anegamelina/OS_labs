#include <unistd.h>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

int main() {
    const char *shm_name = "/shm_example";
    const char *sem_name = "/sem_example";

    int shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = (char *)mmap(nullptr, sizeof(char) * 256, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(sem_name, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    char input[256];
    ssize_t read_bytes;

    while ((read_bytes = read(STDIN_FILENO, input, sizeof(input) - 1)) > 0) {
        input[read_bytes] = '\0';

        char* line = strtok(input, "\n");

        while (line) {
            std::istringstream stream(line);
            float number, sum = 0;

            while (stream >> number) {
                sum += number;
            }

            char output[256];
            int output_len = snprintf(output, sizeof(output), "Sum of elements: %.2f\n", sum);

            snprintf(shm_ptr, 256, "%s", output);

            sem_wait(sem);

            line = strtok(nullptr, "\n");
        }
    }

    sem_close(sem);
    munmap(shm_ptr, sizeof(char) * 256);
    close(shm_fd);

    return 0;
}