#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/my_shm"
#define SEM_EMPTY_NAME "/empty_sem"
#define SEM_FULL_NAME "/full_sem"

void Create_semaphores(sem_t **sem_empty, sem_t **sem_full);
void Create_shared_memory(char **shm_ptr, size_t shm_size);

int main() {
    char file[256];
    ssize_t read_bytes;

    write(STDOUT_FILENO, "Enter the file name: ", 21);
    read_bytes = read(STDIN_FILENO, file, sizeof(file) - 1);

    if (read_bytes > 0) {
        file[read_bytes - 1] = '\0';
    } else {
        perror("Problems with the name of file");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_empty, *sem_full;
    Create_semaphores(&sem_empty, &sem_full);

    char *shm_ptr;
    size_t shm_size = 1024 * 1024;
    Create_shared_memory(&shm_ptr, shm_size);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error with fork\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        sem_wait(sem_full);

        char *input = shm_ptr;
        if (strlen(input) == 0) {
            write(STDERR_FILENO, "No data in shared memory.\n", 26);
            exit(EXIT_FAILURE);
        }

        char *line = strtok(input, "\n");
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
                char output[256];
                int output_len = snprintf(output, sizeof(output), "Sum of elements: %.2f\n", sum);
                write(STDOUT_FILENO, output, output_len);
            } else {
                const char* error_msg = "Invalid input. Please enter numbers only.\n";
                write(STDOUT_FILENO, error_msg, strlen(error_msg));
            }

            line = strtok(nullptr, "\n");
        }

        munmap(shm_ptr, shm_size);
        sem_close(sem_empty);
        sem_close(sem_full);
        exit(EXIT_SUCCESS);
    }
    else {
        int file_fd = open(file, O_RDONLY);
        if (file_fd == -1) {
            perror("Problems with opening file\n");
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_read = read(file_fd, shm_ptr, shm_size - 1);
        close(file_fd);
        if (bytes_read < 0) {
            perror("Problems with reading file\n");
            exit(EXIT_FAILURE);
        }
        shm_ptr[bytes_read] = '\0';

        sem_post(sem_full);

        sem_wait(sem_empty);

        write(STDOUT_FILENO, shm_ptr, strlen(shm_ptr));

        waitpid(pid, NULL, 0);

        munmap(shm_ptr, shm_size);
        sem_close(sem_empty);
        sem_close(sem_full);

        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        shm_unlink(SHM_NAME);
    }

    return 0;
}

void Create_semaphores(sem_t **sem_empty, sem_t **sem_full) {
    *sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT | O_EXCL, 0644, 0);
    if (*sem_empty == SEM_FAILED) {
        perror("Error creating empty semaphore");
        exit(EXIT_FAILURE);
    }

    *sem_full = sem_open(SEM_FULL_NAME, O_CREAT | O_EXCL, 0644, 0);
    if (*sem_full == SEM_FAILED) {
        perror("Error creating full semaphore");
        exit(EXIT_FAILURE);
    }
}

void Create_shared_memory(char **shm_ptr, size_t shm_size) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("Error creating shared memory");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Error setting size of shared memory");
        exit(EXIT_FAILURE);
    }

    *shm_ptr = (char *)mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*shm_ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    close(shm_fd);
}