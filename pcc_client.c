/* Template-Credit: recitation 10 code */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>  /* for O_RDONLY flag. */
#include <unistd.h> /* for STDOUT_FILENO (1). */
#include <stdint.h> /* for int32_t, int16_t types. */

/* ------------------------------------------- prototypes & global-variables -------------------------------------------*/

#define Megabyte (1024*1024)

/* ------------------------------------------- helpers -------------------------------------------*/

FILE *open_file_for_reading(char *file_path) {
    FILE *file = fopen(file_path, "rb"); /* open for reading in binary mode. */
    if(file == NULL) { fprintf(stderr, "An error occurred: open() failed. %s \n", strerror(errno)); exit(1); }
    return file;
}

void create_TCP_connection_to_server(char *servers_port, char *servers_ip, int *sockfd, int *connfd, FILE *file){
    struct sockaddr_in s_address; /* where we want to get to. */
    socklen_t addrsize = sizeof(struct sockaddr_in );

    *sockfd = socket(AF_INET, SOCK_STREAM, 0); /* socket create and verification. • AF_INET: Address from the Internet. • SOCK_STREAM: 0=TCP. Provides sequenced, two-way byte streams with a transmission mechanism for stream data. */
    if(*sockfd < 0) { fprintf(stderr, "An error occurred: could not create socket. %s \n", strerror(errno)); fclose(file); exit(1); }

    /* assign family, ip, port. */
    memset(&s_address, 0, addrsize); /* sets the whole bytes of serv_addr to the value 0. */
    s_address.sin_family = AF_INET; /* the address family for the transport address = AF_INET (Address from the Internet). */
    s_address.sin_port = htons(atoi(servers_port)); /* port - host to network short (16 bits). */
    if (inet_pton(AF_INET, servers_ip, &s_address.sin_addr) != 1) { fprintf(stderr, "An error occurred: inet_pton failed. %s \n", strerror(errno)); exit(1); } /* convert ip address from text to binary form. */

    *connfd = connect(*sockfd, (struct sockaddr*) &s_address, addrsize); /* socket connection create and verification - to the target servers address. */
    if(*connfd < 0) { fprintf(stderr, "An error occurred: connect Failed. %s \n", strerror(errno)); exit(1); }
}

void send_the_server_file_size(FILE *file, int sockfd) {
    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c. */
    uint32_t N;
    char *data;
    int left, rc;

    /* get the file size. */
    fseek(file, 0, SEEK_END); /* move the file pointer to the end of the file. */
    N = htonl(ftell(file)); /* determine the size of the file in bytes and htonl it. */
    rewind(file);  /* rewinds the file pointer to the beginning of the file. */

    data = (char *)&N;
    left = sizeof(N);

    /* write the file size to server. */
    do { 
        rc = write(sockfd, data, left);
        if (rc <= 0) { fprintf(stderr, "An error occurred: write failed. %s \n", strerror(errno)); exit(1); }
        else { data += rc; left -= rc; }
    } while (left > 0);
}

void send_the_server_files_content(int sockfd, FILE *file) {
    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c */
    char buffer[Megabyte];
    int count_read_bytes, count_sent_bytes, total_count_sent_bytes;

    while(!feof(file)) {
        count_read_bytes = fread(buffer, 1, Megabyte, file); /*  the number of full items successfully read. */
        if (count_read_bytes < 0) { fprintf(stderr, "An error occurred: fread() failed. %s \n", strerror(errno)); exit(1); }

        total_count_sent_bytes = 0;
        while (total_count_sent_bytes < count_read_bytes) {
            count_sent_bytes = send(sockfd, buffer + total_count_sent_bytes, count_read_bytes - total_count_sent_bytes, 0);
            if (count_sent_bytes < 0) { fprintf(stderr, "An error occurred: send() failed. %s \n", strerror(errno)); exit(1); }
            total_count_sent_bytes += count_sent_bytes;
        }
        memset(&buffer, 0, Megabyte); /* sets the whole bytes of buffer to the value 0. */
    }
    if (fclose(file) == EOF) { fprintf(stderr, "An error occurred: write failed. %s \n", strerror(errno)); exit(1); } /* close file. */
}

uint32_t read_number_of_pritable_characters_from_server(int sockfd) {  

    /* CREDIT: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c */
    uint32_t ret;
    char *data = (char*)&ret;
    int left = sizeof(ret);
    int rc;
    do { 
        rc = read(sockfd, data, left); /* the default is byte-stream mode, as in the assignment description */
        if (rc <= 0) { fprintf(stderr, "An error occurred: read failed. %s \n", strerror(errno)); exit(1); }
        else { data += rc; left -= rc; }
    }
    while (left > 0);
    
    return ntohl(ret);

    /* char buffer[sizeof(uint32_t)];
    int count_read_bytes;
    uint32_t number_of_pritable_characters;
	//printf("\t be 2 \n");
    count_read_bytes = read(sockfd, buffer, sizeof(number_of_pritable_characters));
    //printf("\t 3 2 \n");
    if (count_read_bytes < 0) { fprintf(stderr, "An error occurred: read() failed. %s \n", strerror(errno)); exit(1); }
    if (count_read_bytes != sizeof(number_of_pritable_characters)) { fprintf(stderr, "An error occurred: read() did not read the expected bytes number. %s \n", strerror(errno)); exit(1); }
    memcpy(&number_of_pritable_characters, buffer, sizeof(number_of_pritable_characters));
    
    return ntohl(number_of_pritable_characters); */
}

void print_the_number_of_printable_characters (uint32_t c) {
	printf("# of printable characters: %u\n", c);
}

/* ------------------------------------------- main: pcc-client ------------------------------------------- */

int main(int argc, char *argv[])
{    
    /* ------------------------------------------- initialization ------------------------------------------- */
    
    uint32_t c;
    FILE *file = NULL;
    int sockfd = -1;
    int connfd = -1;

    /* ------------------------------------------- Flow ------------------------------------------- */
    
    /* 0. validate that the correct number of command line arguments is passed. */
    if(!(argc == 4)) { fprintf(stderr, "An error occurred: invalid number of command line arguments. %s \n", strerror(errno)); exit(1); }

    /* 1. Open the specified file for reading. */
    file = open_file_for_reading(argv[3]);

    /* 2. Create a TCP connection to the specified server port on the specified server IP. */
    create_TCP_connection_to_server(argv[2], argv[1], &sockfd, &connfd, file);
    
    /* 3. Transfer the contents of the file to the server over the TCP connection and receive the count of printable characters computed by the server, using the following protocol: do (a) - (c). */

        /* (a) The client sends the server N, the number of bytes that will be transferred (i.e., the file size). */
        send_the_server_file_size(file, sockfd);

        /* (b) The client sends the server N bytes (the file’s content). */
        send_the_server_files_content(sockfd, file);

        /* (c) The server sends the client C, the number of printable characters. */
        c = read_number_of_pritable_characters_from_server(sockfd);
    
    /* 4. Print the number of printable characters obtained to stdout. */
    print_the_number_of_printable_characters(c);
    
    close(sockfd);

    /* 5. Exit with exit code 0. */
    exit(0);
}
