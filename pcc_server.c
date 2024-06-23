/* Template-Credit: recitation 10 code */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h> /* for int32_t, int16_t types. */
#include <signal.h> /* for handling signals. */

/* ------------------------------------------- prototypes & global-variables -------------------------------------------*/

#define Megabyte (1024*1024)
int connfd = -1;
uint32_t pcc_total[95] = { 0 }; /* init a d-s that will count how many times each printable character was observed in all client connections. The counts are 32-bits unsigned integers. */
uint32_t pcc_client[95] = { 0 };
int sigint = 0; /* indicator for sigint. */

/* ------------------------------------------- helpers -------------------------------------------*/

void servers_mannual_handler(int signal) {
    int i;
    if (connfd < 0) { /* if the server is not processing a client when SIGINT is delivered: */
        for (i = 32; i <= 126; i++) { printf("char '%c' : %u times\n", i, pcc_total[i-32]); } /* (a) For every printable character, print the number of times it was observed. */
        exit(0); /* (b) exit with exit code 0. */ 
    }
    else { sigint = 1; } /* if the server is processing a client when SIGINT is delivered finish handling this client. */
}

void initialize_sigint(void) {
    struct sigaction act; /* initialize a sigaction structure (<signal.h> is included). */
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART; 
    act.sa_handler = &servers_mannual_handler; /* a pointer to a function 'servers_mannual_handler' - the function that is going to be called whenever we receive a signal. */
    if (sigaction(SIGINT, &act, NULL) == -1) { fprintf(stderr, "An error occurred in sig: %s \n", strerror(errno)); exit(1); } /* specify the signal by the sigaction function. */
}

void initialize_pcc_total(void) {
    if(memset(pcc_total, 0, sizeof(pcc_total)) < 0) { fprintf(stderr, "An error occurred in memset: %s \n", strerror(errno)); exit(1); };
}

int listen_to_incoming_TCP_connections(uint16_t servers_port) {
    int listenfd = -1;
    struct sockaddr_in s_address;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    if (memset(&s_address, 0, addrsize)<0) { fprintf(stderr, "An error occurred in memset: %s \n", strerror(errno)); exit(1); } /* copies the character 0 to the first addrsize characters of the string pointed to (zero the whole address) */
    s_address.sin_family = AF_INET; /* AF_INET - IPv4 Internet protocols. */
    s_address.sin_addr.s_addr = htonl(INADDR_ANY); /* ip - host to network long (32 bits); To an IP address that is used when we don't want to bind a socket to any specific IP. */
    s_address.sin_port = htons(servers_port); /*port - host to network short (16 bits) */
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { fprintf(stderr, "An error occurred: sokcet() failed. %s \n", strerror(errno)); exit(1); } 
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) { fprintf(stderr, "An error occurred: setsockopt failed. %s \n", strerror(errno)); exit(1); } /* set socket options: • SOL_SOCKET: with the socket level set to SOL_SOCKET for all sockets. • SO_REUSEADDR: a socket may bind, except when there is an active listening socket bound to the address. CREDIT: • https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr • https://man7.org/linux/man-pages/man7/socket.7.html. Mentioned in part 'Relevant system calls and functions' of the assignmentץ */
    if (bind(listenfd, (struct sockaddr*)&s_address, addrsize ) < 0) { fprintf(stderr, "An error occurred: bind failed. %s \n", strerror(errno)); exit(1); } /* INADDR_ANY: to accept connections on all network interface (CREDIT: ip(7) manual page), e SO_REUSEADDR socket option to enable reusing the port quickly after the server terminates (CREDIT: e socket(7) manual page). */
    if (listen(listenfd, 10) != 0) { fprintf(stderr, "An error occurred: listen failed. %s \n", strerror(errno)); exit(1); } /* Use a listen() queue of size 10. */

    return listenfd;
}

int accepts_a_TCP_connection(int listenfd) {
    int connfd = -1;
    struct sockaddr_in c_address;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    if (memset(&c_address, 0, addrsize)<0) { fprintf(stderr, "An error occurred in memset: %s \n", strerror(errno)); exit(1); } /* copies the character 0 to the first addrsize characters of the string pointed to (zero the whole address) */
    connfd = accept(listenfd, (struct sockaddr*)&c_address, &addrsize);
    if(connfd < 0) { fprintf(stderr, "An error occurred: accept failed. %s \n", strerror(errno)); exit(1); }
    return connfd;
}

uint32_t read_N(int connfd) {
    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c */
    uint32_t ret;
    char *data = (char*)&ret;
    int left = sizeof(ret);
    int rc;
    do { 
        rc = read(connfd, data, left); /* the default is byte-stream mode, as in the assignment description */
        if (rc <= 0) { fprintf(stderr, "An error occurred: read failed. %s \n", strerror(errno)); exit(1); }
        else { data += rc; left -= rc; }
    }
    while (left > 0);
    return ntohl(ret);
}

uint32_t read_stream_of_bytes_and_count_pritable(int connfd, uint32_t N) {
    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c. */
    char buffer[Megabyte];
    uint32_t count_printable = 0;
    int rc, i;
    do { 
        rc = read(connfd, buffer, Megabyte); 
        if (rc < 0) { fprintf(stderr, "An error occurred: read failed. %s \n", strerror(errno)); exit(1); }
        else if (rc  == 0) { break; }
        else {
            for(i = 0; i < rc; i++) { if (32 <= buffer[i] && buffer[i] <= 126) { pcc_client[buffer[i] - 32]++; count_printable++; } }
            N -= rc; 
        }
    } while (N > 0);

    if (N != 0) { count_printable = -1; }
    return count_printable;
}

void write_count_printable_to_client(uint32_t count_printable, int connfd) {
    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c */
    count_printable = htonl(count_printable); /* determine the size of the file in bytes and htonl it. */
    char *data = (char*)&count_printable;
    int left = sizeof(count_printable);
    int rc;
    do { 
        rc = write(connfd, data, left); 
        if (rc <= 0) { fprintf(stderr, "An error occurred: write failed. %s \n", strerror(errno)); exit(1); }
        else { data += rc; left -= rc; }
    } while (left > 0);
}

/* ------------------------------------------- main: pcc-server -------------------------------------------*/

int main(int argc, char *argv[])
{    
    /* ------------------------------------------- initialization -------------------------------------------*/
    
    uint32_t count_printable, N; /* 0 <= count_printable <= N, while N is a 32-bit unsigned integer - the steams size. */
    int listenfd = -1;
    
    /* Initialize SIGINT. */ 
    initialize_sigint(); 

    /* Command-Line arguments. */
    if(!(argc == 2)) { fprintf(stderr, "An error occurred: invalid number of command line arguments. %s \n", strerror(errno)); exit(1); } /* validate that the correct number of command line arguments is passed. */

    /* ------------------------------------------- Flow -------------------------------------------*/
    
    /* 1. Initialize a data structure pcc_total. */
    initialize_pcc_total();

    /* 2. Listen to incoming TCP connections on the specified server port. */
    listenfd = listen_to_incoming_TCP_connections((uint16_t)atoi(argv[1]));
    
    /* 3. Enter a loop, in which each iteration: do (a) - (c). */
    while (1) {
        /* (a) Accepts a TCP connection. */
        if(sigint == 1) { servers_mannual_handler(SIGINT); } 
        else { connfd = accepts_a_TCP_connection(listenfd); }
        
        /* (b) When a connection is accepted: do (b),1 - (b),3 + (c). */
        N = read_N(connfd);
        count_printable = read_stream_of_bytes_and_count_pritable(connfd, N); /* (b), 1: reads a stream of bytes from the client. (b), 2: computes its printable character count. */
        
        if (count_printable != -1) {
            write_count_printable_to_client(count_printable, connfd); /* (b), 3: writes the result to the client over the TCP connection. */
            for(int i = 0; i < 95; i++){ pcc_total[i] += pcc_client[i]; } /* (c) Updates the pcc_total global data structure - during stage b (in compute_printable_char_count). */
        }

        if(memset(pcc_client, 0, sizeof(pcc_client)) < 0) { fprintf(stderr, "An error occurred in memset: %s \n", strerror(errno)); exit(1); }; /* init pcc_client. */
        
        close(connfd);
        connfd = -1;
    }
}

