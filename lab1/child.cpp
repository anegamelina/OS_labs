#include <unistd.h>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>

int main() {

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
            write(STDOUT_FILENO, output, output_len);

            line = strtok(nullptr, "\n");
        }
    }
    return 0;
}