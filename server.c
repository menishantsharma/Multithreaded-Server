#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>k
#include<unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BACKLOG 4096
#define BUFFER_SIZE 128

void error(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

typedef struct Task {
    int client_sock;
    struct Task* next;
} Task;

int taskCount = 0;
Task* head = NULL;
Task* tail = NULL;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
int done = 0;

void handle_client(int client_sock) {
    int cnt;

    char buffer[BUFFER_SIZE];
    cnt = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if(cnt < 0) error("Error to receive.");
    
    if(cnt > 0) {
        buffer[cnt] = '\0';
        printf("Received: %s\n", buffer);

        cnt = send(client_sock, "world", 5, 0);
        if(cnt < 0) error("Failed to send.");
    }

    close(client_sock);
}

void enqueueTask(Task* task) {
    pthread_mutex_lock(&mutexQueue);
    if(!tail) {
        head=task;
        tail=task;
    }

    else {
        tail->next = task;
        tail = task;
    }

    taskCount++;
    pthread_cond_signal(&condQueue);
    pthread_mutex_unlock(&mutexQueue);
}

Task* dequeueTask() {
    pthread_mutex_lock(&mutexQueue);

    while(taskCount == 0 && !done) pthread_cond_wait(&condQueue, &mutexQueue);

    if(taskCount == 0 && done) {
        pthread_mutex_unlock(&mutexQueue);
        return NULL;
    }

    Task* task = head;
    head = head->next;
    if(!head) tail = NULL;
    taskCount--;
    pthread_mutex_unlock(&mutexQueue);
    return task;
}

void* workerThread(void* arg) {
    while(1) {
        Task* task = dequeueTask();
        if(!task) break;
        handle_client(task->client_sock);
        free(task);
    }
}

void signal_handler(int signal) {
    if(signal == SIGINT) {
        pthread_mutex_lock(&mutexQueue);
        printf("Received SIGINT, shutting down...\n");
        done = 1;
        pthread_cond_broadcast(&condQueue);
        pthread_mutex_unlock(&mutexQueue);
    }
}

int main(int argc, char *argv[]) {
    if(argc!=3) {
        printf("USAGE: %s <port> <thread_pool_size>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int ret;

    ret = pthread_mutex_init(&mutexQueue, NULL);
    if(ret < 0) error("Failed to initialize mutex.");

    ret = pthread_cond_init(&condQueue, NULL);
    if(ret < 0) error("Failed to initialize condition variable.");

    signal(SIGINT, signal_handler);

    int port = atoi(argv[1]);
    int thread_pool_size = atoi(argv[2]);
    pthread_t threads[thread_pool_size];

    for(int i=0; i<thread_pool_size; i++) {
        ret = pthread_create(&threads[i], NULL, &workerThread, NULL);
        if(ret < 0) error("Failed to create thread.");
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) error("Failed to create socket.");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =  INADDR_ANY;
    serv_addr.sin_port = htons(port);

    ret = bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret < 0) error("Failed to bind.");

    ret = listen(server_sock, BACKLOG);
    if(ret < 0) error("Failed to listen.");

    printf("Server has started and waiting for request.\n");
    
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);

    while(1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if(client_sock < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                if(done) break;
                continue;
            }
            else error("Failed to accept.");
        }

        Task* newTask = malloc(sizeof(Task));
        if(newTask == NULL) error("Failed to allocate memory to new task.");

        newTask->client_sock = client_sock;
        newTask->next = NULL;
        enqueueTask(newTask);
    }

    for(int i=0; i<thread_pool_size; i++) {
        ret = pthread_join(threads[i], NULL);
        if(ret < 0) error("Error to join threads.");
    }

    close(server_sock);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    return 0;  
}