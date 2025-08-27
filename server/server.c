#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

volatile __sig_atomic_t server_shutdown = 0;

typedef struct{
    int client_sock;
    struct sockaddr_in client_addr;
} client_socket_t;

typedef struct {
    pthread_t *threads;
    int thread_count;
    client_socket_t **client_queue;
    int queue_size, queue_front, queue_rear, queue_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty, queue_not_full;
    // Client tracking fields
    int clients_processed;          // Total clients processed
    int clients_active;             // Currently being processed
    pthread_mutex_t stats_mutex;    // Protect counters
} thread_pool_t;

void thread_pool_destroy(thread_pool_t *pool);
void thread_pool_partial_destroy(thread_pool_t *pool, int threads_created);
void cleanup_server(thread_pool_t *pool, int server_sock);
void* worker_thread(void* arg);
int thread_pool_add_client(thread_pool_t* pool, client_socket_t* socket);
thread_pool_t* thread_pool_create(int thread_count);
void* process_client(client_socket_t *client_socket);
int count_last_char(const char* message);
int validate_alphabet_only(const char* message);

void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) return;
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_cancel(pool->threads[i]);
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->stats_mutex);
    pthread_cond_destroy(&pool->queue_not_full);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_mutex_destroy(&pool->queue_mutex);
    if (pool->client_queue) {
        for (int i = 0; i < pool->queue_size; i++) {
            if (pool->client_queue[i]) {
                close(pool->client_queue[i]->client_sock);
                free(pool->client_queue[i]);
            }
        }
        free(pool->client_queue);
    }
    free(pool->threads);
    free(pool);
}

void thread_pool_partial_destroy(thread_pool_t *pool, int threads_created) {
    if (!pool) return;
    for (int i = 0; i < threads_created; i++) {
        pthread_cancel(pool->threads[i]);
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->stats_mutex);
    pthread_cond_destroy(&pool->queue_not_full);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_mutex_destroy(&pool->queue_mutex);
    if (pool->client_queue) {
        for (int i = 0; i < pool->queue_size; i++) {
            if (pool->client_queue[i]) {
                close(pool->client_queue[i]->client_sock);
                free(pool->client_queue[i]);
            }
        }
        free(pool->client_queue);
    }
    free(pool->threads);
    free(pool);
}

void cleanup_server(thread_pool_t *pool, int server_sock){
    if(pool) thread_pool_destroy(pool);
    if (server_sock >= 0) close(server_sock);
}

thread_pool_t* thread_pool_create(int thread_count){
    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;
    pool->thread_count = thread_count;
    pool->threads = malloc(sizeof(pthread_t) * thread_count);
    if (!pool->threads) { free(pool); return NULL; }
    pool->queue_size = thread_count * 20;  // Increased queue size for high load
    pool->client_queue = malloc(sizeof(client_socket_t*) * pool->queue_size);
    if (!pool->client_queue) { free(pool->threads); free(pool); return NULL; }
    for (int i = 0; i < pool->queue_size; i++) pool->client_queue[i] = NULL;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->queue_count = 0;
    // Initialize client tracking fields
    pool->clients_processed = 0;
    pool->clients_active = 0;
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) { free(pool->client_queue); free(pool->threads); free(pool); return NULL; }
    if (pthread_cond_init(&pool->queue_not_empty, NULL) != 0) { pthread_mutex_destroy(&pool->queue_mutex); free(pool->client_queue); free(pool->threads); free(pool); return NULL; }
    if (pthread_cond_init(&pool->queue_not_full, NULL) != 0) { pthread_cond_destroy(&pool->queue_not_empty); pthread_mutex_destroy(&pool->queue_mutex); free(pool->client_queue); free(pool->threads); free(pool); return NULL; }
    if (pthread_mutex_init(&pool->stats_mutex, NULL) != 0) { 
        pthread_cond_destroy(&pool->queue_not_full); 
        pthread_cond_destroy(&pool->queue_not_empty); 
        pthread_mutex_destroy(&pool->queue_mutex); 
        free(pool->client_queue); 
        free(pool->threads); 
        free(pool); 
        return NULL; 
    }
    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            thread_pool_partial_destroy(pool, i);
            return NULL;
        }
    }
    return pool;
}

int thread_pool_add_client(thread_pool_t* pool, client_socket_t* socket){
    if(!pool || !socket) return -1;
    pthread_mutex_lock(&pool->queue_mutex);
    while(pool->queue_count == pool->queue_size && !server_shutdown){
        pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
    }
    if (server_shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }
    pool->client_queue[pool->queue_rear] = socket;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_size;
    pool->queue_count++;
    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);
    return 0;
}

void* worker_thread(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    while (!server_shutdown) {
        pthread_mutex_lock(&pool->queue_mutex);
        while (pool->queue_count == 0 && !server_shutdown) {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }
        if (server_shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        client_socket_t* client_socket = pool->client_queue[pool->queue_front];
        pool->client_queue[pool->queue_front] = NULL;
        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
        pool->queue_count--;
        
        // Increment active clients counter
        pthread_mutex_lock(&pool->stats_mutex);
        pool->clients_active++;
        pthread_mutex_unlock(&pool->stats_mutex);
        
        pthread_cond_signal(&pool->queue_not_full);
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Process client
        process_client(client_socket);
        
        // Update counters after processing
        pthread_mutex_lock(&pool->stats_mutex);
        pool->clients_active--;
        pool->clients_processed++;
        printf("Cliente procesado. Total procesados: %d, Activos: %d\n", 
               pool->clients_processed, pool->clients_active);
        
        // Check if all work is completed
        if (pool->clients_active == 0 && pool->queue_count == 0) {
            printf("Servidor completó todas las tareas. Total clientes atendidos: %d\n", 
                   pool->clients_processed);
        }
        pthread_mutex_unlock(&pool->stats_mutex);
    }
    return NULL;
}

void* process_client(client_socket_t *client_socket) {
    if (!client_socket) return NULL;
    char buffer[1024];
    char message_complete[4096] = {0};
    int total_bytes = 0;
    int bytes_received;
    int message_completed = 0;
    while(!message_completed && (bytes_received = recv(client_socket->client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (total_bytes + bytes_received >= (int)sizeof(message_complete) - 1){
            char *error_msg = "Error: Mensaje muy largo\n";
            send(client_socket->client_sock, error_msg, strlen(error_msg), 0);
            break;
        }
        strncat(message_complete, buffer, bytes_received);
        total_bytes += bytes_received;
        if (strchr(message_complete, '\n') != NULL) {
            message_completed = 1;
            message_complete[strcspn(message_complete, "\n")] = '\0';
        }
    }
    if (message_completed && strlen(message_complete) > 0) {
        if (!validate_alphabet_only(message_complete)) {
            char *error_msg = "Error: El mensaje solo puede contener caracteres del alfabeto (a-z, A-Z)\n";
            send(client_socket->client_sock, error_msg, strlen(error_msg), 0);
        } else {
            int last_char_count = count_last_char(message_complete);
            char *response = NULL;
            asprintf(&response, "Último carácter '%c' encontrado %d veces\n", message_complete[strlen(message_complete)-1], last_char_count);
            if (response) {
                send(client_socket->client_sock, response, strlen(response), 0);
                free(response);
            }
        }
    }
    close(client_socket->client_sock);
    free(client_socket);
    return NULL;
}

int count_last_char(const char* message){
    int len = strlen(message);
    if (len == 0) return 0;
    char last = message[len-1];
    int count = 0;
    for (int i=0; i<len; i++) {
        if (message[i] == last) count++;
    }
    return count;
}

int validate_alphabet_only(const char* message) {
    if (!message || strlen(message) == 0) {
        return 0;
    }
    for (int i = 0; message[i] != '\0'; i++) {
        unsigned char c = (unsigned char)message[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]){
    char *server_ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 5050;
    int thread_count = argc > 3 ? atoi(argv[3]) : 200;  // Default 200 threads for high load
    
    // Validate thread count
    if (thread_count < 1) {
        printf("Error: Thread count must be at least 1\n");
        return 1;
    }
    if (thread_count > 1000) {
        printf("Warning: Very high thread count (%d), consider reducing\n", thread_count);
    }
    
    printf("Iniciando servidor con %d hilos de trabajo\n", thread_count);
    int sock;
    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 1;
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) { close(sock); return 1; }
    if(bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { close(sock); return 1; }
    if (listen(sock, 1024) < 0) { close(sock); return 1; }  // Increased backlog for high load
    printf("Servidor escuchando en %s:%d\n", server_ip, port);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) { close(sock); return 1; }
    int sfd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (sfd == -1) { close(sock); return 1; }
    thread_pool_t *pool = thread_pool_create(thread_count);
    if (!pool) { close(sfd); close(sock); return 1; }
    struct pollfd fds[2];
    fds[0].fd = sock;   fds[0].events = POLLIN;
    fds[1].fd = sfd;    fds[1].events = POLLIN;
    while (!server_shutdown) {
        int ret = poll(fds, 2, -1);
        if (ret == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (fds[1].revents & POLLIN) {
            struct signalfd_siginfo fdsi;
            ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
            if (s != sizeof(fdsi)) break;
            if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
                server_shutdown = 1;
                break;
            }
        }
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_sock == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                if (errno == EINTR) continue;
                continue;
            }
            client_socket_t *client_socket = malloc(sizeof(client_socket_t));
            if (!client_socket) { close(client_sock); continue; }
            client_socket->client_sock = client_sock;
            client_socket->client_addr = client_addr;
            if (thread_pool_add_client(pool, client_socket) != 0) {
                close(client_sock);
                free(client_socket);
                continue;
            }
        }
    }
    pthread_mutex_lock(&pool->queue_mutex);
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);
    close(sfd);
    cleanup_server(pool, sock);
    return 0;
}
