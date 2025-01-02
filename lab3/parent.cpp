#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <semaphore.h>
#include <sys/mman.h>

const int SHM_SIZE = 4096;
const char* SHM_NAME = "/shared_memory";
const char* DATA_SEM_NAME = "/data_semaphore";
const char* PROCESSING_SEM_NAME = "/processing_semaphore";

int main() {
    char file[256];
    ssize_t read_bytes;
    int shm_fd;
    char* shm_ptr;

    write(STDOUT_FILENO, "Enter the file name: ", 21);
    read_bytes = read(STDIN_FILENO, file, sizeof(file) - 1);

    if (read_bytes > 0) {
        file[read_bytes - 1] = '\0';
    } else {
        perror("Problems with the name of file");
        exit(EXIT_FAILURE);
    }
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1) {
        perror("Problems with shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("Problems with ftruncate");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm_ptr == MAP_FAILED) {
        perror("Problems with mmap");
        exit(EXIT_FAILURE);
    }

    sem_t* data_sem = sem_open(DATA_SEM_NAME, O_CREAT, 0666, 0);
    if (data_sem == SEM_FAILED) {
        perror("Problems with sem_open");
        exit(EXIT_FAILURE);
    }

    sem_t* processing_sem = sem_open(PROCESSING_SEM_NAME, O_CREAT, 0666, 0);
    if (processing_sem == SEM_FAILED) {
        perror("Problems with sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error with fork\n");
        exit(EXIT_FAILURE);

    }
    else if (pid == 0) {
        execl("./child", "./child", nullptr);
        perror("Error with execl\n");
        exit(EXIT_FAILURE);
    }
    else {
        int file_fd = open(file, O_RDONLY);
        if (file_fd == -1) {
            perror("Problems with opening file");
            exit(EXIT_FAILURE);
        }

        while ((read_bytes = read(file_fd, shm_ptr, SHM_SIZE - 1)) > 0) {
            shm_ptr[read_bytes] = '\0';
            sem_post(data_sem);
            sem_wait(processing_sem);
        }
        shm_ptr[0] = '\0';
        sem_post(data_sem);
        close(file_fd);
        waitpid(pid, nullptr, 0);
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(data_sem);
        sem_unlink(DATA_SEM_NAME);
        sem_close(processing_sem);
        sem_unlink(PROCESSING_SEM_NAME);
    }

    return 0;
}