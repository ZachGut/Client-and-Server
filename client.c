/*
 * Auth:   Zach Gutowski
 * Date:   4-28-26 (Due: 4-29-26)
 * Course: CSCI-3550 (Sec: 001)
 * Desc:   PROJECT-01, TCP/IP Project.
 */

#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>

/* Prototypes */
void SIGINT_handler(int sig);
void cleanup(void);

/* Gloabl Variables */
#define BUF_SIZE (10*1024*1024) /* 10MiB */
struct in_addr ia;
struct sockaddr_in sa;
char *buf = NULL;
int fd_in = -1;
int fd_out = -1;
int sockfd = -1;

/* SIGINT handler for the client */
void SIGINT_handler( int sig ) {

   /* Issue an error */
   fprintf( stderr, "client: Client interrupted. Shutting down.\n" );

   /* Cleanup after yourself */
   cleanup();

   /* Exit for 'reals' */
   exit( EXIT_FAILURE );

} /* end SIGINT_handler() */

/* Cleanup function */
void cleanup( void ) {
    if ( buf != NULL ) {
        free( buf );
        buf = NULL;
    }
    if( fd_in >= 0 ) {
        close( fd_in );
        fd_in = -1;     /* Mark it as unused */
    }
    if( fd_out >= 0 ) {
        close( fd_out );
        fd_out = -1;    /* Mark it as unused */
    }
    /* Close socket */
    if( sockfd > -1 ) {
        close( sockfd );
        sockfd = -1;    /* Mark it as unused */
    }
}

int main( int argc, char *argv[] ) {
    
    /* Local Variables */
    const char *port_in;
    unsigned short int port;
    int val = 1; /* Setting for one of the socket options */
    int bytes_read; /* Keep track of HOW MANY bytes we've read */
    int total_sent; /* Keep track of HOW MANY bytes we've sent */
    int i;
    int sent;

    /* Singal Handler */
    signal( SIGINT, SIGINT_handler);

    /* Create Buffer */
    buf = (void *) malloc( BUF_SIZE );
    if( buf == NULL ) {
        fprintf( stderr, "client: ERROR: Failed to allocate memory.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }

    /* Obtain IP and PORT */
    if (argc < 4) {
        fprintf(stderr, "client: USAGE: %s <server_IP> <server_PORT> file1 file2 ...\n", argv[0]);
        cleanup();
        exit( EXIT_FAILURE );
    }
    
    /* IP */
    if( inet_aton( argv[1], &ia ) == 0 ) {
        fprintf( stderr, "client: ERROR: Setting the IP address.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }
    
    sa.sin_family = AF_INET;              /* Use IPv4 addresses */
    sa.sin_addr   = ia;                   /* Attach IP address structure */
    
    /* PORT */
    port_in = argv[2];
    port = (unsigned short int)atoi(port_in);

    if (port <= 1023) {
        fprintf(stderr, "client: ERROR: Port number is privileged.\n");
        cleanup();
        exit( EXIT_FAILURE );
    }
    
    sa.sin_port = htons( port );/* Set port number in Network Byte Order */

    /* Iterate over files and Send*/
    for (i = 3; i < argc; i++) {

        /* Create a new TCP socket */
        sockfd = socket( AF_INET, SOCK_STREAM, 0 );

        /* Check that it succeeded */
        if( sockfd < 0 ) {
            fprintf( stderr, "client: ERROR: Failed to create socket.\n" );
            cleanup();
            exit( EXIT_FAILURE );
        }

        /* Socket Options */
        if( setsockopt( sockfd,
            SOL_SOCKET, SO_REUSEADDR, (const void *) &val, sizeof( int ) ) != 0 ) {
                fprintf( stderr, "client: ERROR: setsockopt() failed.\n" );
                cleanup();
                exit( EXIT_FAILURE );
        }

        /* Attempt to Connect */
        printf("client: Connecting to %s:%s...\n", argv[1], argv[2]);
        if( connect( sockfd, (struct sockaddr *) &sa, sizeof( sa ) ) != 0 ) {
            fprintf( stderr, "client: ERROR: connecting to %s:%s\n", argv[1], argv[2] );
            cleanup();
            exit( EXIT_FAILURE );
        }
        printf("client: Success!\n");

        /* Try to open */
        printf("client: Sending: \"%s\"...\n", argv[i]);
        fd_in = open(argv[i], O_RDONLY);
        if (fd_in < 0) {
            fprintf(stderr, "client: ERROR: Failed to open file: %s\n", argv[i]);
            cleanup();
            exit(EXIT_FAILURE);
        }

        while (1) {

            /* Try to read */
            bytes_read = read(fd_in, buf, BUF_SIZE);
            if (bytes_read < 0) {
                fprintf(stderr, "client: ERROR: Unable to read: %s\n", argv[i]);
                cleanup();
                exit(EXIT_FAILURE);
            }

            /* End of File */
            if (bytes_read == 0) {
                break;
            }

            total_sent = 0;

            while (total_sent < bytes_read) {

                sent = send(sockfd, buf + total_sent, bytes_read - total_sent, 0);

                if (sent < 0) {
                    fprintf(stderr, "client: ERROR: While sending data.\n");
                    cleanup();
                    exit(EXIT_FAILURE);
                }

                total_sent += sent;
            }
        }

        /* Close file and socket */
        close(fd_in);
        fd_in = -1;
        close(sockfd);
        sockfd = -1;

        printf("client: Done.\n");
        printf("client: File transfer(s) complete.\n");
        printf("client: Goodbye!\n");
    }

    cleanup();
    return 0;

} /* End main() */