#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>    // fcntl ke liye non-blocking
#include <errno.h>    // EAGAIN check ke liye

// fd ko non-blocking banao
// warna read() block ho jaayega
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); // purane flags lo
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // non-blocking add karo
}

int main() {

    // Step 1 — listening socket banao
    // AF_INET = ipv4, SOCK_STREAM = tcp, 0 = default protocol
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if(server == -1) {
        printf("socket nahi bana — kuch gadbad\n");
        return -1;
    }

    // Step 2 — TIME_WAIT ignore karo
    // warna "address already in use" aayega restart pe
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    // Step 3 — address form bharo
    // sin = socket internet, INADDR_ANY = koi bhi interface
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;   // ipv4
    addr.sin_port        = htons(9001); // port 9001, network format
    addr.sin_addr.s_addr = INADDR_ANY;  // wifi/eth/loopback sab suno

    // Step 4 — port pe board lagao
    // kernel ki table update hogi: port 9001 → ye process
    if(bind(server, (sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("bind nahi hua — port busy hoga shayad\n");
        return -1;
    }

    // Step 5 — shutter kholo
    // 1024 = waiting room ki size (backlog)
    // SYN queue + accept queue dono ban jayenge
    if(listen(server, 1024) == -1) {
        printf("listen nahi hua\n");
        return -1;
    }

    // Step 6 — server socket non-blocking karo
    // ET mode ke saath non-blocking zaroori hai
    set_nonblocking(server);
    printf("\n server listening on 9001 \n");

    // Step 7 — kernel epoll object banao
    // yeh ek resource hai isliye fd return hoga
    // 0 = koi extra flag nahi — default behavior
    int epfd = epoll_create1(0);
    if(epfd == -1) {
        printf("epoll nahi bana\n");
        return -1;
    }

    // Step 8 — server socket ko epoll mein add karo
    // EPOLLIN = data ready event
    // EPOLLET = edge triggered — sirf ek baar batayega
    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = server;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server, &ev);

    // Step 9 — event loop shuru
    // max 1024 events ek baar mein
    // server + client dono ke events yahan aayenge
    epoll_event events[1024]{};

    while(true) {

        // -1 = infinite wait — koi event aane tak so jao
        // CPU = 0% jab koi event nahi
        int n = epoll_wait(epfd, events, 1024, -1);
        if(n == -1) {
            printf("epoll_wait mein kuch gadbad\n");
            break;
        }

        // kitne events aaye — sab handle karo
        for(int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if(fd == server) {
                // naya client aaya — accept loop
                // ET mode mein loop zaroori hai
                // ek baar mein sab clients accept karo
                while(true) {
                    int client = accept(server,
                                       nullptr,  // client ip nahi chahiye
                                       nullptr); // size bhi nahi

                    if(client == -1 && errno == EAGAIN) {
                        // koi aur client nahi abhi
                        break;
                    }
                    if(client == -1) {
                        printf("accept mein gadbad\n");
                        break;
                    }

                    // naye client ko bhi non-blocking karo
                    set_nonblocking(client);

                    // naye client ko epoll mein add karo
                    // ab yeh bhi watch hoga
                    epoll_event cev{};
                    cev.events  = EPOLLIN | EPOLLET;
                    cev.data.fd = client;
                    epoll_ctl(epfd, EPOLL_CTL_ADD,
                             client, &cev);

                    printf("\n naya client aaya fd=%d\n", client);
                }

            } else {
                // purana client ne data bheja
                // ET mode mein read loop zaroori hai
                // ek baar mein poora data padho
                while(true) {
                    char buf[1024]{};
                    int bytes = read(fd, buf,
                                    sizeof(buf)-1);

                    if(bytes == -1 && errno == EAGAIN) {
                        // poora pad liya — buffer khali
                        break;
                    }
                    if(bytes <= 0) {
                        // client disconnect ho gaya
                        // epoll se bhi hatao warna leak hoga
                        printf("\n client disconnect fd=%d\n", fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL,
                                 fd, nullptr);
                        close(fd); // fd free karo
                        break;
                    }

                    // data aaya — wapas bhejo (echo)
                    printf("fd=%d ne bheja: %s\n", fd, buf);
                    write(fd, buf, bytes);
                }
            }
        }
    }

    // cleanup
    close(server);
    close(epfd);
    return 0;
}
