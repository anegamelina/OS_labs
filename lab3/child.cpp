#include <unistd.h>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

const int SHM_SIZE = 4096;
const char* SHM_NAME = "/shared_memory";
const char* DATA_SEM_NAME = "/data_semaphore";
const char* PROCESSING_SEM_NAME = "/processing_semaphore";

int main() {
    int shm_fd;
    char* shm_ptr;
    ssize_t read_bytes;

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Problems with shm_open");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Problems with mmap");
        exit(EXIT_FAILURE);
    }

    sem_t* data_sem = sem_open(DATA_SEM_NAME, 0);
    if (data_sem == SEM_FAILED) {
        perror("Problems with sem_open");
        exit(EXIT_FAILURE);
    }

    sem_t* processing_sem = sem_open(PROCESSING_SEM_NAME, 0);
    if (processing_sem == SEM_FAILED) {
        perror("Problems with sem_open");
        exit(EXIT_FAILURE);
    }

    while (true) {
        sem_wait(data_sem);
        if (shm_ptr[0] == '\0') {
            break;
        }
        char* line = strtok(shm_ptr, "\n");

        while (line) {
            std::istringstream stream(line);
            float number, sum = 0;
            while (stream >> number) {
                sum += number;
            }
            char output[256];
            int output_len = snprintf(output, sizeof(output), "Sum of elements: %.2f\n", sum);
            write(STDOUT_FILENO, output, output_len);

            line = strtok(nullptr, "\n");
        }
        sem_post(processing_sem);
    }
    munmap(shm_ptr, SHM_SIZE);
    sem_close(data_sem);
    sem_close(processing_sem);
    return 0;
}