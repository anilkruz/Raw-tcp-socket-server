#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

// kitne clients ek saath
#define NUM_CLIENTS 10000
// har client kitni requests karega
#define REQUESTS_PER_CLIENT 100

// shared counter — total requests done
int total_requests = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_thread(void* arg) {
    int id = *(int*)arg;

    // socket banao
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) return nullptr;

    // server ka address
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // connect karo
    if(connect(sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Client %d connect nahi hua\n", id);
        close(sock);
        return nullptr;
    }

    // REQUESTS_PER_CLIENT baar bhejo
    char msg[64]{};
    char buf[1024]{};
    snprintf(msg, sizeof(msg), "hello from client %d", id);

    for(int i = 0; i < REQUESTS_PER_CLIENT; i++) {
        // bhejo
        write(sock, msg, strlen(msg));

        // jawab lo
        int bytes = read(sock, buf, sizeof(buf)-1);
        if(bytes <= 0) break;

        // counter badao
        pthread_mutex_lock(&mutex);
        total_requests++;
        pthread_mutex_unlock(&mutex);
    }

    close(sock);
    return nullptr;
}

int main() {
    printf("Stress test shuru — %d clients, %d req each\n",
           NUM_CLIENTS, REQUESTS_PER_CLIENT);

    // time start
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // NUM_CLIENTS threads banao
    pthread_t threads[NUM_CLIENTS];
    int ids[NUM_CLIENTS];

    for(int i = 0; i < NUM_CLIENTS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], nullptr,
                      client_thread, &ids[i]);
    }

    // sab khatam hone ka wait
    for(int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], nullptr);
    }

    // time end
    clock_gettime(CLOCK_MONOTONIC, &end);

    double seconds = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Total requests:  %d\n", total_requests);
    printf("Total time:      %.2f seconds\n", seconds);
    printf("Requests/sec:    %.0f\n", total_requests / seconds);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    return 0;
}
