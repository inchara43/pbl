
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MQ_NAME "/week4_commands"
#define FIFO_PATH "/tmp/week4_logger_fifo"
#define PORT 5000
#define SIZE 512
#define LIMIT 100

int stack[LIMIT], top = -1;
int queue[LIMIT], front = 0, rear = 0, count = 0;
int cpu_count = 0, memory_value = 0;

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

void log_event(int fd, const char *msg)
{
    char data[SIZE];
    int len = snprintf(data, sizeof(data), "%s\n", msg);

    if (len <= 0 || len >= SIZE)
        return;

    int sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, data + sent, len - sent);
        if (n <= 0)
            break;
        sent += (int)n;
    }
}

void process_command(const char *cmd, char *response)
{
    int value;

    if (strcmp(cmd, "CPU") == 0) {
        cpu_count++;
        snprintf(response, SIZE,
                 "CPU operation completed. Count=%d", cpu_count);

    } else if (sscanf(cmd, "MEMORY %d", &value) == 1) {
        memory_value = value;
        snprintf(response, SIZE,
                 "Memory value stored: %d", value);

    } else if (sscanf(cmd, "STACK_PUSH %d", &value) == 1) {
        if (top == LIMIT - 1)
            snprintf(response, SIZE, "Stack overflow.");
        else {
            stack[++top] = value;
            snprintf(response, SIZE, "Pushed %d onto stack.", value);
        }

    } else if (strcmp(cmd, "STACK_POP") == 0) {
        if (top == -1)
            snprintf(response, SIZE, "Stack is empty.");
        else
            snprintf(response, SIZE, "Popped %d.", stack[top--]);

    } else if (sscanf(cmd, "QUEUE_ENQ %d", &value) == 1) {
        if (count == LIMIT)
            snprintf(response, SIZE, "Queue is full.");
        else {
            queue[rear] = value;
            rear = (rear + 1) % LIMIT;
            count++;
            snprintf(response, SIZE, "Enqueued %d.", value);
        }

    } else if (strcmp(cmd, "QUEUE_DEQ") == 0) {
        if (count == 0)
            snprintf(response, SIZE, "Queue is empty.");
        else {
            value = queue[front];
            front = (front + 1) % LIMIT;
            count--;
            snprintf(response, SIZE, "Dequeued %d.", value);
        }

    } else if (strcmp(cmd, "STATUS") == 0) {
        snprintf(response, SIZE,
                 "CPU=%d Memory=%d Stack=%d Queue=%d",
                 cpu_count, memory_value, top + 1, count);

    } else {
        snprintf(response, SIZE, "Unknown command.");
    }
}

int main(void)
{
    mqd_t mq;
    struct mq_attr attr;
    int server_fd, client_fd, fifo_fd;
    struct sockaddr_in addr;
    char command[SIZE], response[SIZE], event[SIZE];

    /* Create the POSIX message queue for UI commands. */
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = SIZE;

    mq_unlink(MQ_NAME);
    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    /* Create the named pipe used by Logger. */
    if (mkfifo(FIFO_PATH, 0666) < 0 && access(FIFO_PATH, F_OK) < 0) {
        perror("mkfifo");
        mq_close(mq);
        mq_unlink(MQ_NAME);
        return 1;
    }

    /* Start TCP server for UI responses. */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(server_fd, 1) < 0) {
        perror("Core bind/listen");
        close(server_fd);
        return 1;
    }

    printf("Core ready: MQ + TCP port %d\n", PORT);
    printf("Start UI and Logger when ready.\n");

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    /* Wait for Logger to open its read end of the FIFO. */
    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) {
        perror("open logger FIFO");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    log_event(fifo_fd, "Core started.");

    while (1) {
        unsigned int priority;
        ssize_t n = mq_receive(mq, command, SIZE, &priority);

        if (n < 0) {
            perror("mq_receive");
            break;
        }

        command[n < SIZE ? n : SIZE - 1] = '\0';

        if (strcmp(command, "EXIT") == 0) {
            snprintf(response, SIZE, "Simulator shutting down.");
            send_line(client_fd, response);
            log_event(fifo_fd, "UI requested shutdown.");
            break;
        }

        process_command(command, response);

        if (send_line(client_fd, response) < 0) {
            log_event(fifo_fd, "UI socket disconnected.");
            break;
        }

        snprintf(event, SIZE, "Command: %s | Result: %s",
                 command, response);
        log_event(fifo_fd, event);
    }

    log_event(fifo_fd, "Core stopped.");
    log_event(fifo_fd, "SHUTDOWN");

    close(fifo_fd);
    close(client_fd);
    close(server_fd);
    mq_close(mq);
    mq_unlink(MQ_NAME);
    unlink(FIFO_PATH);

    return 0;
}

