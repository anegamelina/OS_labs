#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#define MAX_THREADS 10
#define MAX_SIZE 10

typedef struct {
    int id;
    int max_threads;
    double matrix[MAX_SIZE][MAX_SIZE];
    double b[MAX_SIZE];
    int n;
} ThreadData;

sem_t semaphore;

void *Gauss_thread(void *arg);
void Gauss_method(double matrix[MAX_SIZE][MAX_SIZE], double b[MAX_SIZE], int n, int max_threads);
void Print_matrix(double matrix[MAX_SIZE][MAX_SIZE], double b[MAX_SIZE], int n);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Incorrect input\n");
        return 1;
    }

    srand(time(NULL));

    int n = atoi(argv[1]);
    int max_threads = atoi(argv[2]);

    if (n <= 0 || n > MAX_SIZE  || max_threads <= 0 || max_threads > MAX_THREADS) {
        printf("Incorrect matrix size or number of threads.\n");
        return 1;
    }

    double matrix[MAX_SIZE][MAX_SIZE];
    double b[MAX_SIZE];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (double)(rand() % 100);
        }
        b[i] = (double)(rand() % 100);
    }

    Print_matrix(matrix, b, n);

    clock_t start_time = clock(); // засекаем время начала выполнения

    Gauss_method(matrix, b, n, max_threads);

    clock_t end_time = clock(); // засекаем время окончания выполнения

    printf("Answers are:\n");
    for (int i = 0; i < n; i++) {
        printf("x%d = %f\n", i, b[i]);
    }

    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n", time_spent);

    return 0;
}

void *Gauss_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int id = data->id;
    int n = data->n;
    double (*matrix)[MAX_SIZE] = data->matrix;
    double *b = data->b;

    for (int k = 0; k < n; k++) {
        if (id == k % data->max_threads) {
            if (matrix[k][k] == 0) {
                printf("Zero element on the diagonal at row %d\n", k);
                exit(1);
            }

            for (int i = k + 1; i < n; i++) {
                double factor = matrix[i][k] / matrix[k][k];
                for (int j = k; j < n; j++) {
                    matrix[i][j] -= factor * matrix[k][j];
                }
                b[i] -= factor * b[k];
            }
        }
        sem_post(&semaphore);
    }
    return NULL;
}

void Gauss_method(double matrix[MAX_SIZE][MAX_SIZE], double b[MAX_SIZE], int n, int max_threads) {
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    sem_init(&semaphore, 0, 0);

    for (int i = 0; i < max_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].max_threads = max_threads;
        thread_data[i].n = n;
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                thread_data[i].matrix[j][k] = matrix[j][k];
            }
            thread_data[i].b[j] = b[j];
        }
        pthread_create(&threads[i], NULL, Gauss_thread, &thread_data[i]);
    }

    for (int k = 0; k < n; k++) {
        sem_wait(&semaphore);
    }

    for (int i = n - 1; i >= 0; i--) {
        if (matrix[i][i] == 0) {
            printf("Zero element on the diagonal at row %d during back substitution\n", i);
            exit(1);
        }
        for (int j = i + 1; j < n; j++) {
            b[i] -= matrix[i][j] * b[j];
        }
        b[i] /= matrix[i][i];
    }

    sem_destroy(&semaphore);
}

void Print_matrix(double matrix[MAX_SIZE][MAX_SIZE], double b[MAX_SIZE], int n) {
    printf("Matrix:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%8.2f ", matrix[i][j]);
        }
        printf("| %8.2f\n", b[i]);
    }
    printf("\n");
}