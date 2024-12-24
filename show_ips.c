/*
** showip.c -- show IP addresses for a host given on the command line
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
  struct addrinfo hints, *res, *p;
  int status;
  char ipstr[INET6_ADDRSTRLEN];

  if (argc != 2) {
    fprintf(stderr, "usage: showip hostname\n");
    return 1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; // this is a connection oriented socket, meaning the state of the connection is maintained on both ends. Connection is state.

  if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  printf("IP addresses for %s:\n\n", argv[1]);
  
  for(p = res; p != NULL; p = p->ai_next) {
    void *addr;
    char *ipver;

    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      ipver = "IPv4";
    } else { // IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPv6";
    }

    // convert the ip to string and print it:
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("  %s: %s\n", ipver, ipstr);
  }

  freeaddrinfo(res); // free the linked list.

  return 0;
}

/* struct addrinfo { */
/*     int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc. */
/*     int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC */
/*     int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM */
/*     int              ai_protocol;  // use 0 for "any" */
/*     size_t           ai_addrlen;   // size of ai_addr in bytes */
/*     struct sockaddr *ai_addr;      // struct sockaddr_in or _in6 */
/*     char            *ai_canonname; // full canonical hostname */

/*     struct addrinfo *ai_next;      // linked list, next node */
/* }; */

// (IPv4 only--see struct sockaddr_in6 for IPv6)

/* struct sockaddr_in { */
/*     short int          sin_family;  // Address family, AF_INET */
/*     unsigned short int sin_port;    // Port number */
/*     struct in_addr     sin_addr;    // Internet address */
/*     unsigned char      sin_zero[8]; // Same size as struct sockaddr */
/* }; */

// (IPv6 only--see struct sockaddr_in and struct in_addr for IPv4)

/* struct sockaddr_in6 { */
/*     u_int16_t       sin6_family;   // address family, AF_INET6 */
/*     u_int16_t       sin6_port;     // port number, Network Byte Order */
/*     u_int32_t       sin6_flowinfo; // IPv6 flow information */
/*     struct in6_addr sin6_addr;     // IPv6 address */
/*     u_int32_t       sin6_scope_id; // Scope ID */
/* }; */

/* struct in6_addr { */
/*     unsigned char   s6_addr[16];   // IPv6 address */
/* }; */

/* NOTE: Understand that sockets api is an application level layer api. To make the api uniform for ipv4 and ipv6,  */
/*       it takes struct *sockaddr as the input, the implementation function looks at the fist few bytes  sin_family inside it and then */
/*       casts the pointer to the sockaddr_in or sockaddr_in6 depending on the value of it. Kind of similar to how */
/*       clojure dispatches on value using defmethod. Data directed programming. */

/* NOTE: get_addrinfo() is a system call which loads struct addrinfo which is used to make calls to the sockets api. */
