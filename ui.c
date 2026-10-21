
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MQ_NAME "/week4_commands"
#define PORT 5000
#define SIZE 512

int send_line(int fd, const char *msg)
{
    char data[SIZE];
    int len = snprintf(data, sizeof(data), "%s\n", msg);

    if (len < 0 || len >= SIZE)
        return -1;

    int sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0)
            return -1;
        sent += (int)n;
    }
    return 0;
}

int receive_line(int fd, char *buf, size_t size)
{
    size_t i = 0;
    char ch;

    while (i < size - 1) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0)
            return -1;
        if (ch == '\n')
            break;
        buf[i++] = ch;
    }

    buf[i] = '\0';
    return 0;
}

int main(void)
{
    mqd_t mq;
    int sock;
    struct sockaddr_in addr;
    char command[SIZE], response[SIZE];
    int choice, value;

    mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open: start Core first");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        mq_close(mq);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr,
                sizeof(addr)) < 0) {
        perror("connect to Core");
        close(sock);
        mq_close(mq);
        return 1;
    }

    printf("UI connected to Core.\n");

    while (1) {
        printf("\n=== WEEK 4 SIMULATOR ===\n");
        printf("1. CPU operation\n");
        printf("2. Store memory value\n");
        printf("3. Stack push\n");
        printf("4. Stack pop\n");
        printf("5. Queue enqueue\n");
        printf("6. Queue dequeue\n");
        printf("7. Display status\n");
        printf("0. Exit\n");
        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Invalid input.");
            continue;
        }

        switch (choice) {
        case 1:
            strcpy(command, "CPU");
            break;
        case 2:
            printf("Memory value: ");
            if (scanf("%d", &value) != 1) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                puts("Invalid value.");
                continue;
            }
            snprintf(command, SIZE, "MEMORY %d", value);
            break;
        case 3:
            printf("Stack value: ");
            if (scanf("%d", &value) != 1) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                puts("Invalid value.");
                continue;
            }
            snprintf(command, SIZE, "STACK_PUSH %d", value);
            break;
        case 4:
            strcpy(command, "STACK_POP");
            break;
        case 5:
            printf("Queue value: ");
            if (scanf("%d", &value) != 1) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                puts("Invalid value.");
                continue;
            }
            snprintf(command, SIZE, "QUEUE_ENQ %d", value);
            break;
        case 6:
            strcpy(command, "QUEUE_DEQ");
            break;
        case 7:
            strcpy(command, "STATUS");
            break;
        case 0:
            strcpy(command, "EXIT");
            break;
        default:
            puts("Invalid choice.");
            continue;
        }

        if (mq_send(mq, command, strlen(command) + 1, 0) < 0) {
            perror("mq_send");
            break;
        }

        if (receive_line(sock, response, sizeof(response)) < 0) {
            puts("Core disconnected.");
            break;
        }

        printf("Core response: %s\n", response);

        if (choice == 0)
            break;
    }

    close(sock);
    mq_close(mq);
    return 0;
}

