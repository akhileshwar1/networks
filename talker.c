/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

static inline uint64_t rdtsc_start() {
  unsigned cycles_high, cycles_low;
  asm volatile ("CPUID\n\t"
    "RDTSC\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t"
    : "=r" (cycles_high), "=r" (cycles_low)
    :: "%rax", "%rbx", "%rcx", "%rdx");
  return ((uint64_t)cycles_high << 32) | cycles_low;
}

static inline uint64_t rdtsc_end() {
  unsigned cycles_high, cycles_low;
  asm volatile("RDTSCP\n\t"        // read TSC + serialize
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t"
    "CPUID\n\t"
    : "=r" (cycles_high), "=r" (cycles_low)
    :: "%rax", "%rbx", "%rcx", "%rdx");
  return ((uint64_t)cycles_high << 32) | cycles_low;
}

struct timespec start, end;
#define SERVERPORT "4950"    // the port users will be connecting to
#define MAXBUFLEN 100
#define BUCKET_SIZE_NS 1000
#define MAX_LATENCY_NS 200000
#define NUM_BUCKETS (MAX_LATENCY_NS / BUCKET_SIZE_NS)

int histogram[NUM_BUCKETS] = {0};

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;
  char buf[MAXBUFLEN];
  char s[INET6_ADDRSTRLEN];

  if (argc != 3) {
    fprintf(stderr,"usage: talker hostname message\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  struct sockaddr_storage their_addr;

  if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      perror("talker: socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }
  
  socklen_t addr_len = sizeof their_addr;
  uint64_t min = UINT64_MAX;
  uint64_t max = 0;
  double avg, sum = 0.0;
  int count = 0;
  while (count < 10000) {
    // no need to connect, direct send but with additional parameters of the remote details
    // since there is no connection to look up that information from.
    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
      perror("talker: sendto");
      exit(1);
    }
    /* clock_gettime(CLOCK_MONOTONIC_RAW, &start); */

    uint64_t start_cycles = rdtsc_start();
    if ((numbytes = recvfrom(sockfd, buf,  MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr,
                             &addr_len)) == -1) {
      perror("recvfrom");
      exit(1);
    }
    uint64_t end_cycles = rdtsc_end();
    uint64_t latency_cycles = end_cycles - start_cycles;
    printf("latency: %" PRIu64 " cycles\n", latency_cycles);

    double cpu_ghz = 3.5; // adjust to your actual CPU frequency
    double latency_ns = latency_cycles / (cpu_ghz * 1e3); // cycles to ns

    /* clock_gettime(CLOCK_MONOTONIC_RAW, &end); */
    // Calculate delta in nanoseconds
    /* uint64_t latency_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + */
    /*   (end.tv_nsec - start.tv_nsec); */
    /* printf("%lu ns\n", latency_ns); */
    max = (latency_ns > max) ? latency_ns : max;
    min = (latency_ns < min) ? latency_ns : min;
    sum = sum + latency_ns;
    count++;

    int bucket = latency_ns / BUCKET_SIZE_NS;
    if (bucket >= NUM_BUCKETS) {
      bucket = NUM_BUCKETS - 1; // OUTLIERS TO THE LAST BUCKET.
    }
    histogram[bucket]++;

    /* printf("listener: got packet from %s\n", */
    /*        inet_ntop(their_addr.ss_family, */
    /*                  get_in_addr((struct sockaddr *)&their_addr), */
    /*                  s, sizeof s)); */

    /* printf("listener: packet is %d bytes long\n", numbytes); */
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);
  }
  avg = sum/(double) count;

  printf("-----------------After 10,000 packets ---------------------\n");
  printf("max latency :%" PRIu64 "ns\n", max);
  printf("min latency :%" PRIu64 "ns\n", min);
  printf("avg latency : %.2fns\n", avg);

  for (int i = 0; i < NUM_BUCKETS ; ++i) {
    printf("%3d - %3d us : %5d packets", i, i+1, histogram[i]);
  }

  freeaddrinfo(servinfo);
  printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
  close(sockfd);

  return 0;
}
