#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // inet_pton ke liye — string ip to binary
#include <unistd.h>     // read, write, close ke liye
#include <string.h>     // strlen ke liye

int main() {

    // Step 1 — apna socket banao
    // server ki tarah — dono ko apna apna phone chahiye
    // AF_INET = ipv4, SOCK_STREAM = tcp, 0 = default
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        printf("socket nahi bana — kuch gadbad\n");
        return -1;
    }
    printf("Socket bana — fd=%d\n", sock);

    // Step 2 — server ka address form bharo
    // {} se garbage nahi aayega — sab zero hoga
    sockaddr_in addr{};
    addr.sin_family = AF_INET;      // ipv4 use karo
    addr.sin_port   = htons(9001);  // port 9001 — network format mein
    
    // "127.0.0.1" string ko binary mein convert karo
    // network pe string nahi — 4 bytes jaate hain
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // Step 3 — server se connect karo
    // andar 3-way handshake hoga:
    // SYN → SYN-ACK → ACK
    // tab return hoga yeh function
    if(connect(sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("connect nahi hua — server chal raha hai?\n");
        close(sock);
        return -1;
    }
    printf("Server se connected!\n");

    // Step 4 — baar baar bhejo jab tak 'q' na dabaao
    // persistent connection — ek baar connect, baar baar bhejo
    char input[1024]{};
    while(true) {
        printf("Bhejo (q = quit): ");
        
        // keyboard se lo — stdin = fd=0
        // fgets \n bhi include karta hai — theek hai
        fgets(input, sizeof(input), stdin);

        // q dabaya = band karo
        if(input[0] == 'q') break;

        // data bhejo — strlen kyunki '\0' nahi bhejna
        // hardcode nahi — consistency ke liye
        write(sock, input, strlen(input));

        // server ka jawab suno
        // read() block karega jab tak jawab na aaye
        char buf[1024]{};
        int bytes = read(sock, buf, sizeof(buf)-1);
        // sizeof(buf)-1 — ek byte '\0' ke liye chhodi
        
        if(bytes <= 0) {
            printf("Server ne connection tod diya\n");
            break;
        }

        printf("Server ne bola: %s\n", buf);
    }

    // Step 5 — connection band karo
    // FIN bheja jaayega — 4-way handshake hoga
    // TIME_WAIT mein jaayega
    close(sock);
    printf("Bye!\n");
    return 0;
}
