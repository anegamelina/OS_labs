#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

void Create_pipe(int *fd);

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

        execl("./child", "./child", nullptr);

        perror("Error with execl\n");
        exit(EXIT_FAILURE);
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
    }

    return 0;
}

void Create_pipe(int* fd) {
    if (pipe(fd) == -1) {
        perror("Error with pipe\n");
        exit(EXIT_FAILURE);
    }
}