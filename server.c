/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // port to connect to.
#define BACKLOG 10  // capacity of the connections queue.

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Background: */
/* 1. socket: socket is a file descriptor. It follows the unix philosophy that everything is a file, */
/*    what they mean by that is not the file on disk, but the raw stream of bytes that serves as a data structure */
/*    handled by the kernel. These raw bytes do end up somewhere on the disk for actual files, and for the sockets */
/*    they end up on the remote machine. */
/* 2. bind and accept: it will bind the socket to a port i.e it links the sockfd to a connection tuple in the kernel memory. */
/*                     A connection tuple is of the form ((host_ip, host_port), (remote_ip, remote_port)), therefore a  */
/*                     single host port can be linked to many other remote connections. That is how sends and recvs on the */
/*                     new_fd is handled by the kernel, it uses the remote ip and port in the tuple to encapsulate the  */
/*                     packet before sending it to NIC chip to actually send it out over the internet. */

/*    The entire network communication can be thought of as kernel to kernel communication, since it is the program  */
/*    that handles all the osi layer protocols for us. A kernel is like a fence and a gateway for all the processes outside */
/*    it wanting to talk to the hardware and to each other. */

int main(void) {
  int sockfd, new_fd; // listen on sockfd, new connection send/recv on new_fd.
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information.
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  char hostname[256];
  int rc = gethostname(hostname, sizeof(hostname));
  printf("hostname is %s\n", hostname);


  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my ip.

  if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  }

  // loop through all the results and bind to the first one we can.
  // beautiful piece of code, you see there are only three if blocks and they implicitly manage the side effects on 
  // the sockfd variable.
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server:socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure
  
  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) { // main accept() loop.
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, 
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) { // this is the child process
      close(sockfd); // child doesn't need the listener
      if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");
      close(new_fd);
      exit(0);
    }
    close(new_fd);  // parent doesn't need this

  }

  return 0;
}
