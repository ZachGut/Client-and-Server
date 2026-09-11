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
int cl_sockfd = -1;

/* SIGINT handler for the client */
void SIGINT_handler( int sig ) {

   /* Issue an error */
   fprintf( stderr, "server: Server interrupted. Shutting down.\n" );

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
    if( cl_sockfd > -1 ) {
        close( cl_sockfd );
        cl_sockfd = -1; /* Mark it as unused */
    }
}

int main( int argc, char *argv[] ) {

    /* Local Variables */
    char fname[80];
    int file_cntr = 1;
    const char *port_in;
    unsigned short int port;
    int val = 1; /* Setting for one of the socket options */
    struct sockaddr_in cl_sa; /* Socket address of the client, once connected */
    int bytes_read; /* Keep track of *actual* vs *expected* bytes */
    int bytes_written;
    
    /* Compute, and store the size of the socket address structure
        so we can pass it INDIRECTLY as a pointer to 'accept()' */
    socklen_t cl_sa_size;

    /* Singal Handler */
    signal( SIGINT, SIGINT_handler);

    /* Create Buffer */
    buf = (void *) malloc( BUF_SIZE );
    if( buf == NULL ) {
        fprintf( stderr, "server: ERROR: Failed to allocate memory.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }

    /* Obtain IP and PORT */
    if (argc <2) {
        fprintf(stderr, "server: USAGE: %s <PORT>\n", argv[0]);
        cleanup();
        exit( EXIT_FAILURE );
    }
    
    /* IP */
    if( inet_aton( "127.0.0.1", &ia ) == 0 ) {
        fprintf( stderr, "server: ERROR: Setting the IP address.\n" );
    }
    sa.sin_family = AF_INET;              /* Use IPv4 addresses */
    sa.sin_addr   = ia;                   /* Attach IP address structure */
    
    /* PORT */
    port_in = argv[1];
    port = (unsigned short int)atoi(port_in);

    if (port <= 1023) {
        fprintf(stderr, "server: ERROR: Port number is privileged.\n");
        cleanup();
        exit( EXIT_FAILURE );
    }
    
    sa.sin_port = htons( port );/* Set port number in Network Byte Order */

    /* Create a new TCP socket */
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );

    /* Check that it succeeded */
    if( sockfd < 0 ) {
        fprintf( stderr, "server: ERROR: Failed to create socket.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }

    /* Socket Options */
    if( setsockopt( sockfd,
        SOL_SOCKET, SO_REUSEADDR, (const void *) &val, sizeof( int ) ) != 0 ) {
            fprintf( stderr, "server: ERROR: setsockopt() failed.\n" );
            cleanup();
            exit( EXIT_FAILURE );
    }

    /* Bind our listening socket to the socket address specified by 'sa'
        on the server side */
    if( bind( sockfd, (struct sockaddr *) &sa, sizeof( sa ) ) != 0 ) {
        fprintf( stderr, "server: ERROR: Failed to bind socket.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }

    /* Turn an already-bound socket into a 'listening' socket
        to start accepting connection requests from clients. */
    if ( listen( sockfd, 1024 ) != 0 ) {
        fprintf( stderr, "server: ERROR: listen() Failed.\n" );
        cleanup();
        exit( EXIT_FAILURE );
    }

    /* Clear all the bytes of this structure to zero */
    cl_sa_size = sizeof(cl_sa);
    memset( (void *) &cl_sa, 0, sizeof( cl_sa ) );

    /* Loop Accept() */
    while (1) {

        cl_sa_size = sizeof(cl_sa);
        cl_sockfd = accept(sockfd, (struct sockaddr *)&cl_sa, &cl_sa_size);

        if (cl_sockfd < 0) {
            fprintf(stderr, "server: ERROR: accept() failed.\n");
            cleanup();
            exit(EXIT_FAILURE);
        }

        printf("server: Connection accepted!\n");

        /* Create output file */
        sprintf(fname, "file-%02d.dat", file_cntr);

        fd_out = open(fname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd_out < 0) {
            fprintf(stderr, "server: ERROR: Unable to create: %s\n", fname);
            cleanup();
            exit(EXIT_FAILURE);
        }

        printf("server: Receiving file...\n");

        /* Loop file read/write */
        while (1) {

            bytes_read = recv(cl_sockfd, buf, BUF_SIZE, 0);

            if (bytes_read < 0) {
                fprintf(stderr, "server: ERROR: Reading from socket.\n");
                cleanup();
                exit(EXIT_FAILURE);
            }

            /* File is empty */
            if (bytes_read == 0) {
                break;
            }

            /* Try to write */
            printf("server: Saving file: \"%s\"...\n", fname);
            bytes_written = write(fd_out, buf, (size_t)bytes_read);
            if (bytes_written != bytes_read) {
                fprintf(stderr, "server: ERROR: Unable to write: %s\n", fname);
                cleanup();
                exit(EXIT_FAILURE);
            }
        }

        /* Close file and socket */
        close(fd_out);
        fd_out = -1;
        close(cl_sockfd);
        cl_sockfd = -1;

        file_cntr++;
        printf("server: Done.\n");
    }

} /* End main() */