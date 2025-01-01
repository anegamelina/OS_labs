#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <semaphore.h>
#include <sys/stat.h>

void Create_pipe(int *fd);
void handle_error(const char *msg);

int main() {
    char file[256];
    ssize_t read_bytes;
    int pipe1[2];

    write(STDOUT_FILENO, "Enter the file name: ", 21);
    read_bytes = read(STDIN_FILENO, file, sizeof(file) - 1);

    if (read_bytes > 0) {
        file[read_bytes - 1] = '\0';
    } else {
        perror("Problems with the name of file");
        exit(EXIT_FAILURE);
    }

    Create_pipe(pipe1);

    const char *shm_name = "/shm_example";
    const char *sem_name = "/sem_example";

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) handle_error("shm_open");

    if (ftruncate(shm_fd, sizeof(char) * 256) == -1) handle_error("ftruncate");

    char *shm_ptr = (char *)mmap(nullptr, sizeof(char) * 256, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) handle_error("mmap");

    sem_t *sem = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem == SEM_FAILED) handle_error("sem_open");

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error with fork\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        close(pipe1[0]);

        int file_fd = open(file, O_RDONLY);
        if (file_fd == -1) {
            perror("Problems with opening file\n");
            exit(EXIT_FAILURE);
        }

        dup2(file_fd, STDIN_FILENO);
        dup2(pipe1[1], STDOUT_FILENO);

        close(file_fd);
        close(pipe1[1]);

        sem_wait(sem);

        write(STDOUT_FILENO, shm_ptr, strlen(shm_ptr));

        sem_close(sem);
        munmap(shm_ptr, sizeof(char) * 256);
        shm_unlink(shm_name);
        exit(EXIT_SUCCESS);
    }
    else {
        close(pipe1[1]);

        char buffer[256];
        while ((read_bytes = read(pipe1[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[read_bytes] = '\0';
            write(STDOUT_FILENO, buffer, read_bytes);
        }

        close(pipe1[0]);

        int status;
        waitpid(pid, &status, 0);

        sem_post(sem);

        sem_close(sem);
        sem_unlink(sem_name);
    }

    return 0;
}

void Create_pipe(int* fd) {
    if (pipe(fd) == -1) {
        perror("Error with pipe\n");
        exit(EXIT_FAILURE);
    }
}

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}