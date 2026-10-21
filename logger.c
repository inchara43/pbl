
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_PATH "/tmp/week4_logger_fifo"
#define SIZE 512

int main(void)
{
    int fifo_fd;
    FILE *log_file;
    char buffer[SIZE];

    log_file = fopen("simulator.log", "a");
    if (log_file == NULL) {
        perror("fopen");
        return 1;
    }

    printf("Logger waiting for Core...\n");

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("open FIFO");
        fclose(log_file);
        return 1;
    }

    printf("Logger connected to Core through FIFO.\n");

    while (1) {
        size_t i = 0;
        char ch;
        ssize_t n;

        while (i < SIZE - 1) {
            n = read(fifo_fd, &ch, 1);

            if (n == 0)
                goto cleanup;

            if (n < 0) {
                perror("read FIFO");
                goto cleanup;
            }

            if (ch == '\n')
                break;

            buffer[i++] = ch;
        }

        buffer[i] = '\0';

        fprintf(log_file, "%s\n", buffer);
        fflush(log_file);

        printf("[LOG] %s\n", buffer);

        if (strcmp(buffer, "SHUTDOWN") == 0)
            break;
    }

cleanup:
    close(fifo_fd);
    fclose(log_file);

    printf("Logger stopped.\n");
    return 0;
}

